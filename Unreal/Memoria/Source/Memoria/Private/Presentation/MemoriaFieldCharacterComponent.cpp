#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "Presentation/MemoriaVerdanArt.h"
#include "Presentation/MemoriaFieldAnimInstance.h"
#include "Presentation/MemoriaCombatClips.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "Misc/Paths.h"
namespace
{
const TCHAR* Directions[4] = {TEXT("Down"), TEXT("Up"), TEXT("Left"), TEXT("Right")};
UPaperSprite* LoadAt(const FString& Package)
{ return LoadObject<UPaperSprite>(nullptr, *(Package + TEXT(".") + FPaths::GetBaseFilename(Package)), nullptr, LOAD_NoWarn | LOAD_Quiet); }
FString RiggedPath(const FString& Id, const FString& Prefix, const FString& Suffix = TEXT(""))
{
    const FString Name = Id.Left(1).ToUpper() + Id.Mid(1).ToLower();
    const FString Asset = Prefix + Name + Suffix;
    return TEXT("/Game/Memoria/Presentation/Field3D/") + Name + TEXT("/") + Asset + TEXT(".") + Asset;
}
template<class T> T* RiggedAsset(const FString& Id, const FString& Prefix, const FString& Suffix = TEXT(""))
{ return LoadObject<T>(nullptr, *RiggedPath(Id, Prefix, Suffix), nullptr, LOAD_NoWarn | LOAD_Quiet); }
bool HasRigged(const FString& Id)
{
    return RiggedAsset<USkeletalMesh>(Id, TEXT("SK_")) &&
        RiggedAsset<UAnimSequence>(Id, TEXT("A_"), TEXT("_Idle")) &&
        RiggedAsset<UAnimSequence>(Id, TEXT("A_"), TEXT("_Walk"));
}
FString HD(const FString& Id, const FString& Name) { return FString::Printf(TEXT("/Game/Memoria/Presentation/FieldHD/SPR_%s_%s"), *Id, *Name); }
}
UMemoriaFieldCharacterComponent::UMemoriaFieldCharacterComponent()
{
    PrimaryComponentTick.bCanEverTick = true; PrimaryComponentTick.TickGroup = TG_PostPhysics;
    Walk.SetNum(0);
}
FString UMemoriaFieldCharacterComponent::DescribeArt(const FString& Id)
{
    const FString Name = Id.Left(1).ToUpper() + Id.Mid(1).ToLower();
    if (HasRigged(Id)) return TEXT("rigged");
    if (LoadAt(HD(Name, TEXT("Down")))) return TEXT("hd");
    return MemoriaVerdanArt::LoadSprite(Name + TEXT("Down")) || MemoriaVerdanArt::LoadSprite(Name) ? TEXT("pixel") : TEXT("missing");
}
int32 UMemoriaFieldCharacterComponent::DirectionIndex(const FString& Direction)
{
    for (int32 I = 0; I < 4; ++I) if (Direction == Directions[I]) return I;
    return 0;
}
bool UMemoriaFieldCharacterComponent::InitializeCharacter(const FString& Id, float WorldHeight)
{
    const FString Name = Id.Left(1).ToUpper() + Id.Mid(1).ToLower();
    Height = WorldHeight; Walk.Reset(); WalkFrames = 0; bMirrorLeft = false;
    if (Skeletal) { Skeletal->DestroyComponent(); Skeletal = nullptr; }
    for (const auto& Prop : Props) if (Prop) Prop->DestroyComponent();
    Props.Reset(); Phase = Weight = Age = 0.f;
    if (InitializeRigged(Id)) { if (Card) Card->SetHiddenInGame(true); return true; }
    if (Card) Card->SetHiddenInGame(false);
    // FIELD_SPRITE_ART_SPEC.md: down/up/right (left mirrors right), two optional walk contacts per view.
    bHighResolution = LoadAt(HD(Name, TEXT("Down"))) != nullptr;
    if (bHighResolution)
    {
        Stand[0] = LoadAt(HD(Name, TEXT("Down"))); Stand[1] = LoadAt(HD(Name, TEXT("Up"))); Stand[3] = LoadAt(HD(Name, TEXT("Right")));
        Stand[2] = nullptr; bMirrorLeft = true;
        if (LoadAt(HD(Name, TEXT("WalkDown0"))))
        {
            WalkFrames = 2;
            for (int32 D = 0; D < 4; ++D) for (int32 F = 0; F < 2; ++F)
                Walk.Add(D == 2 ? nullptr : LoadAt(HD(Name, FString::Printf(TEXT("Walk%s%d"), Directions[D], F))));
        }
    }
    else
    {
        // Source pixel field sprites (assets/sprites/field/<id>): four views, and Arrel's four walk frames.
        for (int32 D = 0; D < 4; ++D) Stand[D] = MemoriaVerdanArt::LoadSprite(Name + Directions[D]);
        if (!Stand[0]) Stand[0] = MemoriaVerdanArt::LoadSprite(Name);
        for (int32 D = 1; D < 4; ++D) if (!Stand[D]) Stand[D] = Stand[0];
        if (MemoriaVerdanArt::LoadSprite(Name + TEXT("WalkDown0")))
        {
            WalkFrames = 4;
            for (int32 D = 0; D < 4; ++D) for (int32 F = 0; F < 4; ++F)
                Walk.Add(MemoriaVerdanArt::LoadSprite(FString::Printf(TEXT("%sWalk%s%d"), *Name, Directions[D], F)));
        }
    }
    if (!Stand[0]) return false;
    if (!Stand[1]) Stand[1] = Stand[0];
    if (!Stand[3]) Stand[3] = Stand[0];
    if (!Card)
    {
        Card = NewObject<UPaperSpriteComponent>(GetOwner(), NAME_None);
        Card->SetupAttachment(this); Card->RegisterComponent();
        Card->SetCollisionEnabled(ECollisionEnabled::NoCollision); Card->SetGenerateOverlapEvents(false);
        Card->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, TEXT("/Paper2D/MaskedLitSpriteMaterial.MaskedLitSpriteMaterial")));
        // Grazing lantern light made the tilted card shadow itself in streaks; the ground blob grounds it instead.
        Card->SetCastShadow(false);
        // Channel 1 receives the character fill light; channel 0 the lanterns and moon.
        Card->SetLightingChannels(true, true, false);
    }
    // Pixel art keeps its dots: point sampling, never bilinear smear.
    if (!bHighResolution)
    {
        TSet<UTexture2D*> Seen;
        auto Crisp = [&](UPaperSprite* Sprite)
        {
            UTexture2D* Texture = Sprite ? Sprite->GetBakedTexture() : nullptr;
            if (Texture && !Seen.Contains(Texture)) { Seen.Add(Texture); if (Texture->Filter != TF_Nearest) { Texture->Filter = TF_Nearest; Texture->UpdateResource(); } }
        };
        for (auto& S : Stand) Crisp(S); for (auto& S : Walk) Crisp(S);
    }
    // The card faces the 48-degree field camera, feet on the visual ground anchor.
    const float ArtHeight = FMath::Max(1.f, float(Stand[0]->GetRenderBounds().BoxExtent.Z * 2.0));
    Scale = Height / ArtHeight;
    Card->SetRelativeRotation(FRotator(0, 0, 42)); Card->SetRelativeLocation(FVector(0, 0, -8));
    ApplyFrame();
    return true;
}
void UMemoriaFieldCharacterComponent::Face(const FString& Direction)
{
    Facing = Directions[DirectionIndex(Direction)]; ApplyFrame();
}
void UMemoriaFieldCharacterComponent::AdvanceLocomotion(const FVector& Step, float DeltaSeconds)
{
    const double Distance = Step.Size2D();
    const bool bMoving = Distance > .01 && DeltaSeconds > 0;
    if (bMoving)
    {
        Facing = FMath::Abs(Step.X) >= FMath::Abs(Step.Y) ? (Step.X > 0 ? TEXT("Right") : TEXT("Left")) : (Step.Y > 0 ? TEXT("Up") : TEXT("Down"));
        Phase = FMath::Fmod(Phase + float(Distance) / StrideLength, 1.f);
    }
    // The gait blends in quickly and settles within a few frames of stopping.
    const float Target = bMoving ? 1.f : 0.f;
    Weight = Target + (Weight - Target) * FMath::Exp(-14.f * FMath::Max(DeltaSeconds, 0.f));
    if (!bMoving && Weight < .02f) Weight = 0.f;
    ApplyFrame();
}
FVector UMemoriaFieldCharacterComponent::FocusPosition() const { return GetComponentLocation() + FVector(0, 0, Height * .55f); }
FString UMemoriaFieldCharacterComponent::GetFrameName() const
{
    if (Skeletal) return FString::Printf(TEXT("%s_%s_%02d"), *Skeletal->GetSkeletalMeshAsset()->GetName(), *Facing, FMath::FloorToInt(Phase * 30));
    const UPaperSprite* Sprite = Card ? Card->GetSprite() : nullptr;
    return Sprite ? Sprite->GetName() + (Card->GetRelativeScale3D().X < 0 ? TEXT("#mirror") : TEXT("")) : FString();
}
void UMemoriaFieldCharacterComponent::ApplyFrame()
{
    if (Skeletal)
    {
        Skeletal->SetRelativeRotation(FRotator(0, GetYaw() + MeshYawOffset, 0));
        if (auto* Anim = Cast<UMemoriaFieldAnimInstance>(Skeletal->GetAnimInstance()))
        {
            Anim->Age = Age; Anim->Phase = Phase; Anim->Weight = Weight;
            Anim->Action = ActionClip; Anim->ActionTime = ActionTime; Anim->ActionWeight = ActionWeight;
        }
        return;
    }
    if (!Card) return;
    int32 D = DirectionIndex(Facing);
    const bool bMirror = D == 2 && (bMirrorLeft || !Stand[2]);
    const int32 Source = bMirror ? 3 : D;
    UPaperSprite* Frame = Stand[Source];
    if (Weight > .5f && WalkFrames > 0)
    {
        // Pixel walk cycles have four frames; the illustrated set has two contact poses.
        const int32 F = FMath::Clamp(FMath::FloorToInt(Phase * WalkFrames), 0, WalkFrames - 1);
        if (UPaperSprite* W = Walk.IsValidIndex(Source * WalkFrames + F) ? Walk[Source * WalkFrames + F].Get() : nullptr) Frame = W;
    }
    if (Card->GetSprite() != Frame) Card->SetSprite(Frame);
    // Two soft bounces per stride while walking; a slow breath while standing.
    const float Bounce = Weight * FMath::Abs(FMath::Sin(Phase * 2.f * PI)) * Height * (bHighResolution ? .022f : .014f);
    const float Breath = (1.f - Weight) * .009f * FMath::Sin(Age * 1.8f);
    Card->SetRelativeLocation(FVector(0, 0, -8 + Bounce));
    Card->SetRelativeScale3D(FVector(Scale * (bMirror ? -1.f : 1.f) * (1.f - Breath * .5f), Scale, Scale * (1.f + Breath)));
}
void UMemoriaFieldCharacterComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    Age += DeltaTime;
    if (ActionClip)
    {
        // Blend in over 0.08 s and out over the last 0.12 s; a held clip (death) rests on its final pose.
        const float Length = GetActionLength();
        ActionTime = FMath::Min(ActionTime + DeltaTime * ActionRate, Length);
        const float In = FMath::Clamp(ActionTime / .08f, 0.f, 1.f), Out = bActionHold ? 1.f : FMath::Clamp((Length - ActionTime) / .12f, 0.f, 1.f);
        ActionWeight = FMath::Min(In, Out);
        if (!bActionHold && ActionTime >= Length) StopAction();
    }
    ApplyFrame();
}


float UMemoriaFieldCharacterComponent::GetActionLength() const { return ActionClip ? ActionClip->GetPlayLength() : 0.f; }
float UMemoriaFieldCharacterComponent::GetYaw() const
{
    const float Yaws[4] = {-90.f, 90.f, 180.f, 0.f};
    return bAim ? AimYaw : Yaws[DirectionIndex(Facing)];
}
bool UMemoriaFieldCharacterComponent::PlayAction(const TCHAR* Clip, float Rate, bool bHold)
{
    UAnimSequence* Sequence = MemoriaCombatClips::Load(CharacterId, Clip);
    if (!Sequence || !Skeletal || Sequence->GetSkeleton() != Skeletal->GetSkeletalMeshAsset()->GetSkeleton()) return false;
    ActionClip = Sequence; ActionTime = 0.f; ActionRate = FMath::Max(.05f, Rate); ActionWeight = 0.f; bActionHold = bHold;
    ApplyFrame(); return true;
}
void UMemoriaFieldCharacterComponent::StopAction() { ActionClip = nullptr; ActionTime = ActionWeight = 0.f; bActionHold = false; ApplyFrame(); }
bool UMemoriaFieldCharacterComponent::InitializeMannequin(float WorldHeight, const FLinearColor& Tint)
{
    Height = WorldHeight; CharacterId = TEXT("Mannequin");
    if (Skeletal) { Skeletal->DestroyComponent(); Skeletal = nullptr; }
    auto Seq = [](const TCHAR* Path) { return LoadObject<UAnimSequence>(nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet); };
    auto* Mesh = LoadObject<USkeletalMesh>(nullptr, *(MemoriaCombatClips::MannequinMesh() + TEXT(".SKM_Manny_Simple")), nullptr, LOAD_NoWarn | LOAD_Quiet);
    // Epic's mannequin faces +Y in mesh space; the field figures face +X at yaw 0.
    MeshYawOffset = -90.f;
    if (!CreateSkeletal(Mesh, Seq(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle")),
        Seq(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Walk/MF_Unarmed_Walk_Fwd.MF_Unarmed_Walk_Fwd")))) return false;
    // A void-dark husk: every slot takes the tint instead of the mannequin grey.
    if (auto* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
        for (int32 I = 0; I < Skeletal->GetNumMaterials(); ++I)
        {
            auto* Material = UMaterialInstanceDynamic::Create(Base, Skeletal);
            Material->SetVectorParameterValue(TEXT("Color"), Tint); Skeletal->SetMaterial(I, Material);
        }
    if (Card) Card->SetHiddenInGame(true);
    bHighResolution = true; WalkFrames = 30; ApplyFrame(); return true;
}
bool UMemoriaFieldCharacterComponent::CreateSkeletal(USkeletalMesh* Mesh, UAnimSequence* Idle, UAnimSequence* WalkClip)
{
    if (!Mesh || !Idle || !WalkClip || Idle->GetSkeleton() != Mesh->GetSkeleton() || WalkClip->GetSkeleton() != Mesh->GetSkeleton()) return false;
    Skeletal = NewObject<USkeletalMeshComponent>(GetOwner(), NAME_None);
    Skeletal->SetupAttachment(this);
    Skeletal->SetCollisionEnabled(ECollisionEnabled::NoCollision); Skeletal->SetGenerateOverlapEvents(false);
    Skeletal->SetSkeletalMesh(Mesh);
    Skeletal->SetRelativeScale3D(FVector(Height / FMath::Max(1.f, float(Mesh->GetBounds().BoxExtent.Z * 2))));
    Skeletal->SetRelativeLocation(FVector(0, 0, -8));
    Skeletal->SetLightingChannels(true, true, false); Skeletal->SetCastShadow(true);
    Skeletal->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    Skeletal->PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
    Skeletal->SetAnimationMode(EAnimationMode::AnimationBlueprint);
    Skeletal->SetAnimInstanceClass(UMemoriaFieldAnimInstance::StaticClass());
    Skeletal->RegisterComponent(); Skeletal->AddTickPrerequisiteComponent(this);
    auto* Anim = Cast<UMemoriaFieldAnimInstance>(Skeletal->GetAnimInstance());
    if (!Anim) { Skeletal->DestroyComponent(); Skeletal = nullptr; return false; }
    Anim->Idle = Idle; Anim->Walk = WalkClip;
    return true;
}
// Animation assets and their skeleton must agree before replacing a working card.
bool UMemoriaFieldCharacterComponent::InitializeRigged(const FString& Id)
{
    auto* Mesh = RiggedAsset<USkeletalMesh>(Id, TEXT("SK_"));
    CharacterId = Id.Left(1).ToUpper() + Id.Mid(1).ToLower(); MeshYawOffset = 0.f;
    if (!CreateSkeletal(Mesh, RiggedAsset<UAnimSequence>(Id, TEXT("A_"), TEXT("_Idle")), RiggedAsset<UAnimSequence>(Id, TEXT("A_"), TEXT("_Walk")))) return false;
    // Bind rigid grips against the actual idle pose, including the FBX import basis.
    Skeletal->TickAnimation(0.f, false); Skeletal->RefreshBoneTransforms();
    bHighResolution = true; WalkFrames = 30;
    // Coordinates are in the imported reference-pose frame: X forward, Y right, Z up.
    if (Id == TEXT("arrel")) AttachProp(Id, TEXT("sword_sheathed"), TEXT("pelvis"), FVector(0, -21, 13));
    else if (Id == TEXT("elia")) AttachProp(Id, TEXT("staff"), TEXT("hand_l"), FVector(0, 0, -2));
    else if (Id == TEXT("malet"))
    {
        AttachProp(Id, TEXT("ledger"), TEXT("hand_r"), FVector(3, 0, -3));
        AttachProp(Id, TEXT("vials"), TEXT("pelvis"), FVector(10, 18, 0));
    }
    ApplyFrame(); return true;
}
void UMemoriaFieldCharacterComponent::AttachProp(const FString& Id, const FString& Prop, FName Bone, const FVector& Offset)
{
    auto* Mesh = RiggedAsset<UStaticMesh>(Id, TEXT("SM_"), TEXT("_") + Prop);
    if (!Mesh || !Skeletal) return;
    const auto& Ref = Skeletal->GetSkeletalMeshAsset()->GetRefSkeleton();
    const int32 Index = Ref.FindBoneIndex(Bone); if (Index == INDEX_NONE) return;
    const FTransform BoneTransform = Skeletal->GetSocketTransform(Bone, RTS_Component);
    auto* Part = NewObject<UStaticMeshComponent>(GetOwner(), NAME_None);
    Part->SetStaticMesh(Mesh); Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Part->SetGenerateOverlapEvents(false); Part->SetLightingChannels(true, true, false);
    Part->SetupAttachment(Skeletal, Bone);
    // The prop FBX origin is its grip. Cancel the idle grip rotation, then follow the animated bone.
    const FTransform Desired(FRotator(0, 90, 0).Quaternion(), BoneTransform.GetLocation() + Offset);
    Part->SetRelativeTransform(Desired.GetRelativeTransform(BoneTransform));
    if (Prop == TEXT("staff")) Part->SetRelativeScale3D(FVector(.85f));
    Part->RegisterComponent(); Props.Add(Part);
}
