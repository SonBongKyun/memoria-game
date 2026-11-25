#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "Presentation/MemoriaPresentationMetrics.h"
#include "Presentation/MemoriaVerdanArt.h"
#include "Presentation/MemoriaFieldAnimInstance.h"
#include "Presentation/MemoriaCombatClips.h"
#include "Presentation/MemoriaHeroFinish.h"
#include "Presentation/MemoriaHeroPropMount.h"
#include "Presentation/MemoriaCharacterPersonalityS362.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Settings/MemoriaSettingsSubsystem.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/StrongObjectPtr.h"
#if WITH_EDITOR
#include "Materials/Material.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "MaterialShared.h"
#include "ShaderCompiler.h"
#include "UObject/SavePackage.h"
#include "HAL/FileManager.h"
#include <initializer_list>
#endif
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "SkeletalRenderPublic.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
namespace
{
const TCHAR* Directions[4] = {TEXT("Down"), TEXT("Up"), TEXT("Left"), TEXT("Right")};
UPaperSprite* LoadAt(const FString& Package)
{ return LoadObject<UPaperSprite>(nullptr, *(Package + TEXT(".") + FPaths::GetBaseFilename(Package)), nullptr, LOAD_NoWarn | LOAD_Quiet); }
FString RiggedPath(const FString& Id, const FString& Prefix, const FString& Suffix = TEXT(""))
{
    const FString Name = Id.Left(1).ToUpper() + Id.Mid(1).ToLower();
    const FString Asset = Prefix + Name + Suffix;
    // S362 immutable prop finish keeps the same original grip/pivot frame.
    if(Prefix==TEXT("SM_") && ((Name==TEXT("Arrel") && Suffix==TEXT("_sword_drawn")) || (Name==TEXT("Elia") && Suffix==TEXT("_staff"))))
    {
        const FString Prop=TEXT("/Game/Memoria/Presentation/Field3D/HeroContactS362/R5/")+Name+TEXT("/")+Asset;
        if(FPackageName::DoesPackageExist(Prop))return Prop;
    }
    // S361 changes only the hero mesh; each existing rig, clip and prop path remains active.
    if((Name==TEXT("Arrel") || Name==TEXT("Elia")) && Prefix==TEXT("SK_") && (Suffix.IsEmpty() || Suffix==TEXT("_Detail")))
    {
        if(Name==TEXT("Arrel"))
        {
            const FString Sole=TEXT("/Game/Memoria/Presentation/Field3D/HeroFinishS362/R4/Arrel/SK_Arrel");
            if(FPackageName::DoesPackageExist(Sole))return Sole;
        }
        const FString Finish=TEXT("/Game/Memoria/Presentation/Field3D/HeroFinishS362/R2/")+Name+TEXT("/SK_")+Name;
        if(FPackageName::DoesPackageExist(Finish))return Finish;
        const FString Hero=TEXT("/Game/Memoria/Presentation/Field3D/HeroPolishS361/R4/")+Name+TEXT("/SK_")+Name;
        if(FPackageName::DoesPackageExist(Hero))return Hero;
    }
    // S360 anatomical NPC rigs and compatible contact-aware loops.
    for(const TCHAR* Person:{TEXT("Traveler"),TEXT("Bureauagent"),TEXT("Guard"),TEXT("Villagerf"),TEXT("Villagerm"),TEXT("Fisherman"),TEXT("Elder"),TEXT("Child"),TEXT("Scholar"),TEXT("Elia"),TEXT("Malet")})
        if(Id.Equals(Person,ESearchCase::IgnoreCase))
        {
            const bool Ambient=FString(Person)!=TEXT("Elia") && FString(Person)!=TEXT("Malet");
            if(Prefix==TEXT("SK_") && (Suffix.IsEmpty() || Suffix==TEXT("_Detail")))
            {
                const FString A=TEXT("SK_")+FString(Person)+(Ambient?TEXT("_Detail"):TEXT(""));
                return TEXT("/Game/Memoria/Presentation/Field3D/HumanoidAnatomyS360/R7/")+FString(Person)+TEXT("/")+A+TEXT(".")+A;
            }
            if(Prefix==TEXT("A_") && (Suffix==TEXT("_Idle") || Suffix==TEXT("_Walk")))
            {
                const FString A=TEXT("A_")+FString(Person)+Suffix;
                return TEXT("/Game/Memoria/Presentation/Field3D/HumanoidAnatomyS360/R7/")+FString(Person)+TEXT("/")+A+TEXT(".")+A;
            }
        }
    if(Prefix==TEXT("SK_") && Suffix.IsEmpty())
    {
        const FString Body=(Id.Equals(TEXT("Husk"),ESearchCase::IgnoreCase) || Id.Equals(TEXT("AshWalker"),ESearchCase::IgnoreCase))?TEXT("AshWalker"):(Id.Equals(TEXT("Thief"),ESearchCase::IgnoreCase) || Id.Equals(TEXT("BeltScavenger"),ESearchCase::IgnoreCase))?TEXT("BeltScavenger"):TEXT("");
        if(!Body.IsEmpty())return TEXT("/Game/Memoria/Presentation/Field3D/HumanoidAnatomyS360/R10/")+Body+TEXT("/SK_")+Body+TEXT(".SK_")+Body;
    }
    // S358 clothing/body revisions remain immutable fallbacks.
    if(Prefix==TEXT("SK_"))
    {
        for(const TCHAR* Ambient:{TEXT("Traveler"),TEXT("Bureauagent"),TEXT("Guard"),TEXT("Villagerf"),TEXT("Villagerm"),TEXT("Fisherman"),TEXT("Elder"),TEXT("Child"),TEXT("Scholar")})
            if(Id.Equals(Ambient,ESearchCase::IgnoreCase) && (Suffix.IsEmpty() || Suffix==TEXT("_Detail")))
                return TEXT("/Game/Memoria/Presentation/Field3D/HumanoidBodiesS358/R3/")+FString(Ambient)+TEXT("/SK_")+Ambient+TEXT("_Detail.SK_")+Ambient+TEXT("_Detail");
        if(Suffix.IsEmpty())
        {
            FString Body;
            if(Id.Equals(TEXT("Husk"),ESearchCase::IgnoreCase) || Id.Equals(TEXT("AshWalker"),ESearchCase::IgnoreCase))Body=TEXT("AshWalker");
            else if(Id.Equals(TEXT("Thief"),ESearchCase::IgnoreCase) || Id.Equals(TEXT("BeltScavenger"),ESearchCase::IgnoreCase))Body=TEXT("BeltScavenger");
            else if(Id.Equals(TEXT("Elia"),ESearchCase::IgnoreCase))Body=TEXT("Elia");
            else if(Id.Equals(TEXT("Malet"),ESearchCase::IgnoreCase))Body=TEXT("Malet");
            if(!Body.IsEmpty())return TEXT("/Game/Memoria/Presentation/Field3D/HumanoidBodiesS358/R3/")+Body+TEXT("/SK_")+Body+TEXT(".SK_")+Body;
        }
    }
    // S357 changes only the two humanoid bodies; identity, safe clips and original props keep their paths.
    if (Prefix == TEXT("SK_") && Suffix.IsEmpty())
    {
        const bool Ash = Id.Equals(TEXT("Husk"), ESearchCase::IgnoreCase) || Id.Equals(TEXT("AshWalker"), ESearchCase::IgnoreCase);
        const bool Scav = Id.Equals(TEXT("Thief"), ESearchCase::IgnoreCase) || Id.Equals(TEXT("BeltScavenger"), ESearchCase::IgnoreCase);
        if (Ash || Scav)
        {
            const FString Revision = Ash ? TEXT("AshWalker") : TEXT("BeltScavenger");
            const FString RevisedAsset = TEXT("SK_") + Revision;
            return TEXT("/Game/Memoria/Presentation/Field3D/HumanoidFacesS357/R4/") + Revision + TEXT("/") + RevisedAsset + TEXT(".") + RevisedAsset;
        }
    }
    // S356 revision bodies share original humanoid rigs; animation/prop fallbacks stay below.
    for (const TCHAR* Revision : {TEXT("Elia"), TEXT("Malet"), TEXT("AshWalker"), TEXT("BeltScavenger"), TEXT("VoidWisp"), TEXT("MemoryLeech")})
        if (Id.Equals(Revision, ESearchCase::IgnoreCase))
        {
            const FString RevisedAsset = Prefix + FString(Revision) + Suffix;
            return (FString(Revision)==TEXT("AshWalker") || FString(Revision)==TEXT("BeltScavenger")
                ? TEXT("/Game/Memoria/Presentation/Field3D/CharacterUpgradeS356/HeadRefinement/")
                : TEXT("/Game/Memoria/Presentation/Field3D/CharacterUpgradeS356/Accepted/")) + FString(Revision) + TEXT("/") + RevisedAsset + TEXT(".") + RevisedAsset;
        }
    return TEXT("/Game/Memoria/Presentation/Field3D/") + Name + TEXT("/") + Asset + TEXT(".") + Asset;
}
template<class T> T* RiggedAsset(const FString& Id, const FString& Prefix, const FString& Suffix = TEXT(""))
{
    if (auto* Revised = LoadObject<T>(nullptr, *RiggedPath(Id, Prefix, Suffix), nullptr, LOAD_NoWarn | LOAD_Quiet)) return Revised;
    const FString Name = Id.Left(1).ToUpper() + Id.Mid(1).ToLower();
    const FString Asset = Prefix + Name + Suffix;
    const FString Original = TEXT("/Game/Memoria/Presentation/Field3D/") + Name + TEXT("/") + Asset + TEXT(".") + Asset;
    return LoadObject<T>(nullptr, *Original, nullptr, LOAD_NoWarn | LOAD_Quiet);
}
bool HasRigged(const FString& Id)
{
    return RiggedAsset<USkeletalMesh>(Id, TEXT("SK_")) &&
        RiggedAsset<UAnimSequence>(Id, TEXT("A_"), TEXT("_Idle")) &&
        RiggedAsset<UAnimSequence>(Id, TEXT("A_"), TEXT("_Walk"));
}
FString HD(const FString& Id, const FString& Name) { return FString::Printf(TEXT("/Game/Memoria/Presentation/FieldHD/SPR_%s_%s"), *Id, *Name); }
// Saved field looks. The graphs match the old editor-only materials, and the cooker packs these assets.
const TCHAR* EliaHairPackageName = TEXT("/Game/Memoria/Presentation/FieldLooks/M_EliaHair");
const TCHAR* EliaHairObjectPath = TEXT("/Game/Memoria/Presentation/FieldLooks/M_EliaHair.M_EliaHair");
const TCHAR* TobiasCoatPackageName = TEXT("/Game/Memoria/Presentation/FieldLooks/M_TobiasCoat");
const TCHAR* TobiasCoatObjectPath = TEXT("/Game/Memoria/Presentation/FieldLooks/M_TobiasCoat.M_TobiasCoat");
#if WITH_EDITOR
// M_NaturalHuman and M_HeroFinish have no tint parameter. These saved copies keep the vertex-color
// person and recolour only cloth (coat) or the hair band (vertex alpha .10–.35).
bool SaveFieldLook(const TCHAR* PackageName, bool bHair)
{
    UPackage* Package = CreatePackage(PackageName);
    if (!Package) return false;
    const FString AssetName = FPaths::GetBaseFilename(PackageName);
    UMaterial* Surface = NewObject<UMaterial>(Package, *AssetName, RF_Public | RF_Standalone);
    if (!Surface) return false;
    Surface->SetShadingModel(MSM_DefaultLit);
    Surface->SetUsageByFlag(MATUSAGE_SkeletalMesh, true);
    if (bHair) Surface->SetUsageByFlag(MATUSAGE_MorphTargets, true);
    Surface->TwoSided = true;
    auto* Regions = NewObject<UMaterialExpressionVertexColor>(Surface);
    auto* UV = NewObject<UMaterialExpressionTextureCoordinate>(Surface);
    auto* Category = NewObject<UMaterialExpressionComponentMask>(Surface);
    Surface->GetExpressionCollection().AddExpression(Regions);
    Surface->GetExpressionCollection().AddExpression(UV);
    Surface->GetExpressionCollection().AddExpression(Category);
    Category->Input.Connect(4, Regions);
    Category->R = true;
    auto AddCustom = [&](const FString& Code, ECustomMaterialOutputType Type, std::initializer_list<TPair<FName, UMaterialExpression*>> Inputs)
    {
        auto* Node = NewObject<UMaterialExpressionCustom>(Surface);
        Surface->GetExpressionCollection().AddExpression(Node);
        Node->Code = Code;
        Node->OutputType = Type;
        for (const TPair<FName, UMaterialExpression*>& Pair : Inputs)
        {
            FCustomInput Input;
            Input.InputName = Pair.Key;
            Input.Input.Connect(0, Pair.Value);
            Node->Inputs.Add(Input);
        }
        return Node;
    };
    UMaterialExpressionCustom* Color = nullptr;
    UMaterialExpressionCustom* Rough = nullptr;
    UMaterialExpressionCustom* Spec = nullptr;
    if (bHair)
    {
        Color = AddCustom(TEXT("float hair=step(.10,A)*(1-step(.35,A));float3 silver=float3(0.80,0.82,0.84);return lerp(C.rgb,silver,hair);"), CMOT_Float3, {{TEXT("C"), Regions}, {TEXT("A"), Category}});
        Rough = AddCustom(TEXT("return A<.1?.59:(A<.35?.43:(A<.65?.82:(A<.9?.32:.18)));"), CMOT_Float1, {{TEXT("A"), Category}});
        Spec = AddCustom(TEXT("return A<.1?.25:(A<.35?.30:(A<.65?.22:.48));"), CMOT_Float1, {{TEXT("A"), Category}});
    }
    else
    {
        Color = AddCustom(TEXT("float cloth=1-step(.10,abs(Category-.75));float weave=1-cloth*.035*(.5+.5*sin(UV.x*447)*sin(UV.y*393));float3 ink=float3(0.10,0.075,0.055);return lerp(C.rgb,ink,cloth)*weave;"), CMOT_Float3, {{TEXT("C"), Regions}, {TEXT("Category"), Category}, {TEXT("UV"), UV}});
        Rough = AddCustom(TEXT("return Category>.92?.40:(Category<.12?.66:(Category<.62?.73:.89));"), CMOT_Float1, {{TEXT("Category"), Category}});
        Spec = AddCustom(TEXT("return Category<.12?.28:(Category>.92?.48:.20);"), CMOT_Float1, {{TEXT("Category"), Category}});
    }
    if (UMaterialEditorOnlyData* Editor = Surface->GetEditorOnlyData())
    {
        Editor->BaseColor.Connect(0, Color);
        Editor->Roughness.Connect(0, Rough);
        Editor->Specular.Connect(0, Spec);
    }
    Surface->PostEditChange();
    if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
    const FMaterialResource* Resource = Surface->GetMaterialResource(GMaxRHIShaderPlatform);
    if (!Resource || Resource->GetCompileErrors().Num()) return false;
    const FString File = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(File), true);
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    Args.SaveFlags = SAVE_NoError;
    return UPackage::SavePackage(Package, Surface, *File, Args);
}
#endif
UMaterialInterface* FieldLook(const TCHAR* PackageName, const TCHAR* ObjectPath, bool bHair)
{
    static TStrongObjectPtr<UMaterialInterface> HairHeld;
    static TStrongObjectPtr<UMaterialInterface> CoatHeld;
    TStrongObjectPtr<UMaterialInterface>& Held = bHair ? HairHeld : CoatHeld;
    if (Held.IsValid()) return Held.Get();
#if WITH_EDITOR
    // Cook is a commandlet. It must load the assets already on disk, not author new ones.
    if (!IsRunningCommandlet() && !FPackageName::DoesPackageExist(PackageName))
    {
        static bool bTriedHair = false;
        static bool bTriedCoat = false;
        bool& bTried = bHair ? bTriedHair : bTriedCoat;
        if (!bTried)
        {
            bTried = true;
            SaveFieldLook(PackageName, bHair);
        }
    }
#endif
    if (UMaterialInterface* Loaded = LoadObject<UMaterialInterface>(nullptr, ObjectPath, nullptr, LOAD_NoWarn | LOAD_Quiet))
    {
        Held.Reset(Loaded);
        return Loaded;
    }
    return nullptr;
}
// The confession seal is a thin plate on the coat, not a box held in front of the chest.
// X is forward in the imported frame. The engine cube is 100cm, so scale X .012 is 1.2cm thick.
FVector TobiasBadgeOffset(USkeletalMeshComponent* Skeletal, FName Bone)
{
    const FVector Fallback(11.f, 0.f, 8.f);
    if (!Skeletal || Bone.IsNone()) return Fallback;
    const FVector Origin = Skeletal->GetSocketTransform(Bone, RTS_Component).GetLocation();
    TArray<FFinalSkinVertex> Vertices;
    Skeletal->GetCPUSkinnedVertices(Vertices, 0);
    float MaxForward = -1.e9f;
    FVector Sum = FVector::ZeroVector;
    int32 Count = 0;
    for (const FFinalSkinVertex& Vertex : Vertices)
    {
        const FVector P(Vertex.Position.X, Vertex.Position.Y, Vertex.Position.Z);
        if (P.X < Origin.X - 1.f) continue;
        if (FMath::Abs(P.Y - Origin.Y) > 12.f) continue;
        if (P.Z < Origin.Z - 4.f || P.Z > Origin.Z + 18.f) continue;
        MaxForward = FMath::Max(MaxForward, P.X);
        Sum += P;
        ++Count;
    }
    if (Count < 12) return Fallback;
    const FVector Center = Sum / static_cast<float>(Count);
    constexpr float HalfThickness = 0.6f;
    constexpr float Clearance = 0.25f;
    return FVector(MaxForward + HalfThickness + Clearance - Origin.X, Center.Y - Origin.Y, Center.Z - Origin.Z);
}
}
UMemoriaFieldCharacterComponent::UMemoriaFieldCharacterComponent()
{
    PrimaryComponentTick.bCanEverTick = true; PrimaryComponentTick.TickGroup = TG_PostPhysics;
    Walk.SetNum(0);
    // Hard references so the cooker packs the saved looks. Quiet when the editor has not written them yet.
    ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> Hair(EliaHairObjectPath, LOAD_Quiet | LOAD_NoWarn);
    ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> Coat(TobiasCoatObjectPath, LOAD_Quiet | LOAD_NoWarn);
    EliaHairMaterial = Hair.Get();
    TobiasCoatMaterial = Coat.Get();
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
    if (Skeletal) { FMemoriaHeroFinish::ClearBlink(Skeletal); Skeletal->DestroyComponent(); Skeletal = nullptr; }
    for (const auto& Prop : Props) if (Prop) Prop->DestroyComponent();
    for (UStaticMeshComponent* Part : {Scabbard.Get(), Blade.Get()}) if (Part) Part->DestroyComponent();
    Props.Reset(); Sheathed = Scabbard = Blade = nullptr; bSwordDrawn = false; Phase = Weight = Age = 0.f;
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
        Phase = FMath::Fmod(Phase + float(Distance) / GaitStride, 1.f);
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
            const auto* World=GetWorld();
            const auto* Combat=World?World->GetSubsystem<UMemoriaFieldCombatSubsystem>():nullptr;
            const auto* Game=World?World->GetGameInstance():nullptr;
            const auto* Settings=Game?Game->GetSubsystem<UMemoriaSettingsSubsystem>():nullptr;
            Anim->IdlePersonality=MemoriaCharacterPersonalityS362::Sample(CharacterId,Age,Weight,ActionClip!=nullptr,
                Combat && Combat->LiveMonsterCount()>0,Settings && Settings->IsReduceMotion());
        }
        // New dedicated lids follow transient presentation time. Active/held death stays neutral.
        const bool bDeath = ActionClip && ActionClip->GetName().Contains(TEXT("Death"));
        FMemoriaHeroFinish::ApplyBlink(Skeletal,
            FMemoriaHeroFinish::BlinkWeight(Age, GetTypeHash(CharacterId), bDeath), bDeath);
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
    MemoriaPresentationMetrics::FScopedCpu Cpu(MemoriaPresentationMetrics::EScope::CharacterUpdate);
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    Age += DeltaTime;
    if (ActionClip)
    {
        // Blend in over 0.08 s and out over the last 0.12 s; a held clip (death) rests on its final pose.
        const float Length = GetActionLength();
        ActionTime = FMath::Min(ActionTime + DeltaTime * ActionRate, Length * ActionHoldAt);
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
bool UMemoriaFieldCharacterComponent::PlayAction(const TCHAR* Clip, float Rate, bool bHold, float HoldAt)
{
    // A character with the sword set (Arrel) plays its cuts in place of the melee set.
    UAnimSequence* Sequence = MemoriaCombatClips::Load(CharacterId, MemoriaCombatClips::ForAction(CharacterId, Clip));
    if (!Sequence || !Skeletal || Sequence->GetSkeleton() != Skeletal->GetSkeletalMeshAsset()->GetSkeleton()) return false;
    ActionClip = Sequence; ActionTime = 0.f; ActionRate = FMath::Max(.05f, Rate); ActionWeight = 0.f; bActionHold = bHold;
    ActionHoldAt = bHold ? FMath::Clamp(HoldAt, .01f, 1.f) : 1.f;
    ApplyFrame(); return true;
}
void UMemoriaFieldCharacterComponent::StopAction()
{
    ActionClip = nullptr; ActionTime = ActionWeight = 0.f; bActionHold = false;
    ApplyFrame(); FMemoriaHeroFinish::ClearBlink(Skeletal);
}
void UMemoriaFieldCharacterComponent::OnUnregister()
{
    FMemoriaHeroFinish::ClearBlink(Skeletal);
    Super::OnUnregister();
}
bool UMemoriaFieldCharacterComponent::InitializeFoe(const FMemoriaFoeLook& Look)
{
    Height = Look.Height; CharacterId = TEXT("Mannequin"); bFoeModel = false;
    if (Skeletal) { FMemoriaHeroFinish::ClearBlink(Skeletal); Skeletal->DestroyComponent(); Skeletal = nullptr; }
    FoeMaterials.Reset();
    if (Blade) { Blade->DestroyComponent(); Blade = nullptr; }
    // S326: Codex's model, its idle and walk and the clips retargeted onto it; the material's Hit flash is
    // driven like M_FieldFoe's, and the thief takes its dagger in hand.
    if (Look.Model)
    {
        const FString Id = Look.Model;
        auto* Mesh = RiggedAsset<USkeletalMesh>(Id, TEXT("SK_"));
        CharacterId = Id; MeshYawOffset = 0.f;
        if (CreateSkeletal(Mesh, MemoriaCombatClips::Load(Id, Look.Idle ? Look.Idle : TEXT("Idle")), MemoriaCombatClips::Load(Id, Look.Walk ? Look.Walk : TEXT("Walk"))))
        {
            for (int32 I = 0; I < Skeletal->GetNumMaterials(); ++I)
                if (UMaterialInterface* Base = Skeletal->GetMaterial(I))
                {
                    auto* Material = UMaterialInstanceDynamic::Create(Base, Skeletal); Skeletal->SetMaterial(I, Material); FoeMaterials.Add(Material);
                    Material->SetVectorParameterValue(TEXT("SurfaceTint"), Look.SurfaceTint);
                    Material->SetScalarParameterValue(TEXT("Weathering"), FMath::Clamp(Look.Weathering, 0.f, 1.f));
                    if (Look.bRetint)
                    {
                        Material->SetVectorParameterValue(TEXT("Glow"), Look.ModelGlow); Material->SetScalarParameterValue(TEXT("CrackStrength"), Look.ModelCrack);
                        Material->SetVectorParameterValue(TEXT("RimColor"), Look.ModelRim); Material->SetScalarParameterValue(TEXT("RimStrength"), Look.ModelRimStrength);
                    }
                }
            if (auto* Dagger = RiggedAsset<UStaticMesh>(Id == TEXT("BeltScavenger") ? TEXT("Thief") : Id, TEXT("SM_"), TEXT("_dagger")))
            { Skeletal->TickAnimation(0.f, false); Skeletal->RefreshBoneTransforms(); Blade = AttachGrip(Dagger, 1.f); }
            if (Card) Card->SetHiddenInGame(true);
            bFoeModel = true; bHighResolution = true; WalkFrames = 30; ApplyFrame(); return true;
        }
        CharacterId = TEXT("Mannequin");
    }
    auto Seq = [](const TCHAR* Path) { return LoadObject<UAnimSequence>(nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet); };
    auto Clip = [&](const TCHAR* Name, const TCHAR* Fallback) { UAnimSequence* S = Name ? MemoriaCombatClips::Load(CharacterId, Name) : nullptr; return S ? S : Seq(Fallback); };
    const FString MeshPath = Look.bQuinn ? TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple") : MemoriaCombatClips::MannequinMesh() + TEXT(".SKM_Manny_Simple");
    auto* Mesh = LoadObject<USkeletalMesh>(nullptr, *MeshPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
    // Epic's mannequin faces +Y in mesh space; the field figures face +X at yaw 0.
    MeshYawOffset = -90.f;
    if (!CreateSkeletal(Mesh, Clip(Look.Idle, TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle")),
        Clip(Look.Walk, TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Walk/MF_Unarmed_Walk_Fwd.MF_Unarmed_Walk_Fwd")))) return false;
    // Every slot takes M_FieldFoe (a skeletal-mesh material; BasicShapeMaterial rendered as the default).
    if (auto* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Memoria/Presentation/Combat/M_FieldFoe.M_FieldFoe"), nullptr, LOAD_NoWarn | LOAD_Quiet))
        for (int32 I = 0; I < Skeletal->GetNumMaterials(); ++I)
        {
            auto* Material = UMaterialInstanceDynamic::Create(Base, Skeletal);
            Material->SetVectorParameterValue(TEXT("SurfaceTint"), Look.SurfaceTint);
            Material->SetScalarParameterValue(TEXT("Weathering"), FMath::Clamp(Look.Weathering, 0.f, 1.f));
            Material->SetVectorParameterValue(TEXT("Color"), Look.Color); Material->SetVectorParameterValue(TEXT("Glow"), Look.Glow);
            Material->SetScalarParameterValue(TEXT("CrackStrength"), Look.Crack); Material->SetScalarParameterValue(TEXT("RimStrength"), Look.Rim);
            Skeletal->SetMaterial(I, Material); FoeMaterials.Add(Material);
        }
    if (Look.BladeScale > 0.f)
    {
        Skeletal->TickAnimation(0.f, false); Skeletal->RefreshBoneTransforms();
        Blade = AttachGrip(LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Memoria/Presentation/Field3D/Arrel/SM_Arrel_sword_drawn.SM_Arrel_sword_drawn"), nullptr, LOAD_NoWarn | LOAD_Quiet), Look.BladeScale);
    }
    if (Card) Card->SetHiddenInGame(true);
    bHighResolution = true; WalkFrames = 30; ApplyFrame(); return true;
}
void UMemoriaFieldCharacterComponent::SetHitFlash(float Amount)
{ for (const auto& Material : FoeMaterials) if (Material) Material->SetScalarParameterValue(TEXT("Hit"), Amount); }
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
    FMemoriaHeroFinish::ClearBlink(Skeletal);
    auto* Anim = Cast<UMemoriaFieldAnimInstance>(Skeletal->GetAnimInstance());
    if (!Anim) { Skeletal->DestroyComponent(); Skeletal = nullptr; return false; }
    Anim->Idle = Idle; Anim->Walk = WalkClip;
    GaitStride=StrideLength;
    if(WalkClip->GetPathName().Contains(TEXT("HumanoidAnatomyS360/")))
    {
        const float Step=CharacterId.Equals(TEXT("Child"),ESearchCase::IgnoreCase)?17.f:
            (CharacterId.Equals(TEXT("AshWalker"),ESearchCase::IgnoreCase) || CharacterId.Equals(TEXT("Husk"),ESearchCase::IgnoreCase))?23.f:
            (CharacterId.Equals(TEXT("BeltScavenger"),ESearchCase::IgnoreCase) || CharacterId.Equals(TEXT("Thief"),ESearchCase::IgnoreCase))?26.f:30.f;
        // One grounded stance traverses one authored step. Scale its full cycle to the live figure.
        GaitStride=2.f*Step*Height/180.f;
    }
    return true;
}
// Animation assets and their skeleton must agree before replacing a working card.
float UMemoriaFieldCharacterComponent::AmbientHeight(const FString& Id, float ArrelWorldHeight)
{
    // Authored heights in cm (models/MANIFEST.md S334, models/S347_NPC_MANIFEST.md); Arrel's source model is 180.
    static const TMap<FString, float> Authored = {
        {TEXT("traveler"), 174.f}, {TEXT("bureauagent"), 176.f}, {TEXT("guard"), 178.f},
        {TEXT("villagerf"), 165.f}, {TEXT("villagerm"), 175.f}, {TEXT("fisherman"), 172.f},
        {TEXT("elder"), 165.f}, {TEXT("child"), 120.f}, {TEXT("scholar"), 172.f}};
    const float* Cm = Authored.Find(Id.Replace(TEXT("_"), TEXT("")).ToLower());
    return Cm ? ArrelWorldHeight * *Cm / 180.f : ArrelWorldHeight;
}
bool UMemoriaFieldCharacterComponent::InitializeTobias()
{
    // 23화: a third-grade registry clerk. The bureau agent's rig is the coat; scholar is the fallback if that mesh is absent.
    const TCHAR* Donor = TEXT("bureauagent");
    auto LoadDonor = [&](const TCHAR* Id, USkeletalMesh*& Mesh, UAnimSequence*& Idle, UAnimSequence*& WalkClip)
    {
        Mesh = RiggedAsset<USkeletalMesh>(Id, TEXT("SK_"), TEXT("_Detail"));
        if (!Mesh) Mesh = RiggedAsset<USkeletalMesh>(Id, TEXT("SK_"));
        Idle = RiggedAsset<UAnimSequence>(Id, TEXT("A_"), TEXT("_Idle"));
        WalkClip = RiggedAsset<UAnimSequence>(Id, TEXT("A_"), TEXT("_Walk"));
    };
    USkeletalMesh* Mesh = nullptr; UAnimSequence* Idle = nullptr; UAnimSequence* WalkClip = nullptr;
    LoadDonor(Donor, Mesh, Idle, WalkClip);
    if (!Mesh || !Idle || !WalkClip) { Donor = TEXT("scholar"); LoadDonor(Donor, Mesh, Idle, WalkClip); }
    CharacterId = TEXT("Tobias"); MeshYawOffset = 0.f;
    if (!CreateSkeletal(Mesh, Idle, WalkClip)) return false;
    Skeletal->TickAnimation(0.f, false); Skeletal->RefreshBoneTransforms();
    // The bureau mesh has one M_NaturalHuman slot: vertex color is the person, and alpha near 0.75 is cloth.
    // A flat FieldFoe instance would erase the face. Recolour only that cloth. No weapon and no lantern.
    if (UMaterialInterface* Coat = FieldLook(TobiasCoatPackageName, TobiasCoatObjectPath, false))
    {
        TobiasCoatMaterial = Coat;
        const TArray<FSkeletalMaterial>& Slots = Skeletal->GetSkeletalMeshAsset()->GetMaterials();
        for (int32 Index = 0; Index < Slots.Num() && Index < Skeletal->GetNumMaterials(); ++Index)
        {
            const FString Name = Slots[Index].MaterialSlotName.ToString() + Slots[Index].ImportedMaterialSlotName.ToString() + GetNameSafe(Slots[Index].MaterialInterface);
            if (Name.Contains(TEXT("Face")) || Name.Contains(TEXT("Eye")) || Name.Contains(TEXT("Hair")) || Name.Contains(TEXT("Brow")) || Name.Contains(TEXT("Head"))) continue;
            Skeletal->SetMaterial(Index, Coat);
        }
    }
    auto* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"), nullptr, LOAD_NoWarn | LOAD_Quiet);
    auto* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"), nullptr, LOAD_NoWarn | LOAD_Quiet);
    auto Kit = [&](const TCHAR* Name, const FVector& Offset, const FVector& Scale, const FLinearColor& Ink)
    {
        if (!Cube || !Skeletal || !Skeletal->GetSkeletalMeshAsset()) return;
        const auto& Ref = Skeletal->GetSkeletalMeshAsset()->GetRefSkeleton();
        FName Bone = NAME_None;
        for (const TCHAR* Candidate : {TEXT("spine_03"), TEXT("spine_02"), TEXT("spine_01"), TEXT("pelvis")})
            if (Ref.FindBoneIndex(Candidate) != INDEX_NONE) { Bone = Candidate; break; }
        if (Bone.IsNone()) return;
        const FTransform BoneTransform = Skeletal->GetSocketTransform(Bone, RTS_Component);
        auto* Part = NewObject<UStaticMeshComponent>(GetOwner(), Name);
        Part->SetStaticMesh(Cube); Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetGenerateOverlapEvents(false); Part->SetCastShadow(true); Part->SetLightingChannels(true, true, false);
        Part->SetupAttachment(Skeletal, Bone);
        Part->SetRelativeTransform(FTransform(FQuat::Identity, BoneTransform.GetLocation() + Offset, Scale).GetRelativeTransform(BoneTransform));
        if (Base)
        {
            auto* InkMid = UMaterialInstanceDynamic::Create(Base, Part);
            InkMid->SetVectorParameterValue(TEXT("Color"), Ink); InkMid->SetVectorParameterValue(TEXT("BaseColor"), Ink);
            Part->SetMaterial(0, InkMid);
        }
        Part->RegisterComponent(); Props.Add(Part);
    };
    // A thin plate on the coat, parented to the same chest bone the kit attaches to. The bag stays on the back.
    FName Chest = NAME_None;
    if (Skeletal->GetSkeletalMeshAsset())
    {
        const auto& ChestRef = Skeletal->GetSkeletalMeshAsset()->GetRefSkeleton();
        for (const TCHAR* Candidate : {TEXT("spine_03"), TEXT("spine_02"), TEXT("spine_01"), TEXT("pelvis")})
            if (ChestRef.FindBoneIndex(Candidate) != INDEX_NONE) { Chest = Candidate; break; }
    }
    Kit(TEXT("TobiasBadge"), TobiasBadgeOffset(Skeletal, Chest), FVector(0.012f, 0.05f, 0.065f), FLinearColor(0.86f, 0.84f, 0.76f));
    Kit(TEXT("TobiasBag"), FVector(-16.f, 0.f, 4.f), FVector(0.15f, 0.18f, 0.24f), FLinearColor(0.15f, 0.10f, 0.06f));
    Kit(TEXT("TobiasStrap"), FVector(-14.f, 0.f, 18.f), FVector(0.035f, 0.20f, 0.045f), FLinearColor(0.11f, 0.08f, 0.05f));
    bHighResolution = true; WalkFrames = 30; ApplyFrame(); return true;
}
bool UMemoriaFieldCharacterComponent::InitializeRigged(const FString& Id)
{
    if (Id.Equals(TEXT("tobias"), ESearchCase::IgnoreCase)) return InitializeTobias();
    // S354: tailored ambient geometry shares the original skeleton, clips and painted material.
    auto* Mesh = RiggedAsset<USkeletalMesh>(Id, TEXT("SK_"), TEXT("_Detail"));
    if (!Mesh) Mesh = RiggedAsset<USkeletalMesh>(Id, TEXT("SK_"));
    CharacterId = Id.Left(1).ToUpper() + Id.Mid(1).ToLower(); MeshYawOffset = 0.f;
    if (!CreateSkeletal(Mesh, RiggedAsset<UAnimSequence>(Id, TEXT("A_"), TEXT("_Idle")), RiggedAsset<UAnimSequence>(Id, TEXT("A_"), TEXT("_Walk")))) return false;
    // Bind rigid grips against the actual idle pose, including the FBX import basis.
    Skeletal->TickAnimation(0.f, false); Skeletal->RefreshBoneTransforms();
    bHighResolution = true; WalkFrames = 30;
    // Coordinates are in the imported reference-pose frame: X forward, Y right, Z up.
    if (Id == TEXT("arrel"))
    {
        Sheathed = AttachProp(Id, TEXT("sword_sheathed"), TEXT("pelvis"), FVector(0, -21, 13));
        // S312: the drawn pair keeps the sheathed prop's frame, so the scabbard hangs exactly where it was.
        Scabbard = AttachProp(Id, TEXT("scabbard"), TEXT("pelvis"), FVector(0, -21, 13), false);
        Blade = AttachGrip(RiggedAsset<UStaticMesh>(Id, TEXT("SM_"), TEXT("_sword_drawn")));
        if (!Scabbard || !Blade)
        {
            for (UStaticMeshComponent* Part : {Scabbard.Get(), Blade.Get()}) if (Part) Part->DestroyComponent();
            Scabbard = Blade = nullptr;
        }
        else { Scabbard->SetHiddenInGame(true); Blade->SetHiddenInGame(true); }
    }
    else if (Id == TEXT("elia"))
    {
        // 23화: silver hair, and she does not carry a staff. The face and cloak stay on their own slots.
        if (UMaterialInterface* Hair = FieldLook(EliaHairPackageName, EliaHairObjectPath, true))
        {
            EliaHairMaterial = Hair;
            const TArray<FSkeletalMaterial>& Slots = Skeletal->GetSkeletalMeshAsset()->GetMaterials();
            for (int32 Index = 0; Index < Slots.Num() && Index < Skeletal->GetNumMaterials(); ++Index)
            {
                const UMaterialInterface* Current = Skeletal->GetMaterial(Index);
                const FString Name = Slots[Index].MaterialSlotName.ToString() + Slots[Index].ImportedMaterialSlotName.ToString() + GetNameSafe(Current);
                if (Name.Contains(TEXT("EliaSkin")) || Name.Contains(TEXT("Face")) || Name.Contains(TEXT("Eye"))) continue;
                if (Name.Contains(TEXT("HeroFinish")) || Name.Contains(TEXT("Hair")))
                    Skeletal->SetMaterial(Index, Hair);
            }
        }
    }
    else if (Id == TEXT("malet"))
    {
        AttachProp(Id, TEXT("ledger"), TEXT("hand_r"), FVector(3, 0, -3));
        AttachProp(Id, TEXT("vials"), TEXT("pelvis"), FVector(10, 18, 0));
    }
    ApplyFrame(); return true;
}
UStaticMeshComponent* UMemoriaFieldCharacterComponent::AttachProp(const FString& Id, const FString& Prop, FName Bone, const FVector& Offset, bool bListed)
{
    auto* Mesh = RiggedAsset<UStaticMesh>(Id, TEXT("SM_"), TEXT("_") + Prop);
    if (!Mesh || !Skeletal) return nullptr;
    const auto& Ref = Skeletal->GetSkeletalMeshAsset()->GetRefSkeleton();
    const int32 Index = Ref.FindBoneIndex(Bone); if (Index == INDEX_NONE) return nullptr;
    const FTransform BoneTransform = Skeletal->GetSocketTransform(Bone, RTS_Component);
    auto* Part = NewObject<UStaticMeshComponent>(GetOwner(), NAME_None);
    Part->SetStaticMesh(Mesh); Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Part->SetGenerateOverlapEvents(false); Part->SetLightingChannels(true, true, false);
    Part->SetupAttachment(Skeletal, Bone);
    // The prop FBX origin is its grip. Cancel the idle grip rotation, then follow the animated bone.
    FTransform Desired(FRotator(0, 90, 0).Quaternion(), BoneTransform.GetLocation() + Offset);
    if(Id==TEXT("arrel") && (Prop==TEXT("sword_sheathed") || Prop==TEXT("scabbard"))) Desired=FMemoriaHeroPropMount::FinishScabbardMount(Desired);
    Part->SetRelativeTransform(Desired.GetRelativeTransform(BoneTransform));
    if (Prop == TEXT("staff")) Part->SetRelativeScale3D(FVector(.85f));
    Part->RegisterComponent();
    if (bListed) Props.Add(Part);
    return Part;
}
UStaticMeshComponent* UMemoriaFieldCharacterComponent::AttachGrip(UStaticMesh* Mesh, float PropScale)
{
    if (!Mesh || !Skeletal) return nullptr;
    const auto& Ref = Skeletal->GetSkeletalMeshAsset()->GetRefSkeleton();
    for (const TCHAR* Bone : {TEXT("hand_r"), TEXT("index_01_r"), TEXT("middle_01_r"), TEXT("pinky_01_r")})
        if (Ref.FindBoneIndex(Bone) == INDEX_NONE) return nullptr;
    auto At = [&](const TCHAR* Bone) { return Skeletal->GetSocketTransform(Bone, RTS_Component).GetLocation(); };
    // The grip is built from the hand itself rather than a hand-tuned socket, so it holds for any rig with
    // UE finger names: the handle runs across the knuckles with the pommel on the little-finger side (the
    // prop's +Z), the guard's width (+X) follows the back of the hand, and the handle sits inside the fist.
    const FTransform Hand = Skeletal->GetSocketTransform(TEXT("hand_r"), RTS_Component);
    const FVector Index = At(TEXT("index_01_r")), Pinky = At(TEXT("pinky_01_r")), Knuckles = (Index + Pinky) * .5f;
    const FVector Z = (Pinky - Index).GetSafeNormal();
    const FVector Along = At(TEXT("middle_01_r")) - Hand.GetLocation();
    const FVector X = (Along - Z * FVector::DotProduct(Along, Z)).GetSafeNormal();
    FVector Palm = Ref.FindBoneIndex(TEXT("thumb_01_r")) != INDEX_NONE ? At(TEXT("thumb_01_r")) - Knuckles : FVector::ZeroVector;
    Palm = (Palm - Z * FVector::DotProduct(Palm, Z) - X * FVector::DotProduct(Palm, X)).GetSafeNormal();
    const FTransform Desired(FRotationMatrix::MakeFromZX(Z, X).ToQuat(), Knuckles + X * GripReach + Palm * GripDepth, FVector(PropScale));
    auto* Part = NewObject<UStaticMeshComponent>(GetOwner(), NAME_None);
    Part->SetStaticMesh(Mesh); Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Part->SetGenerateOverlapEvents(false); Part->SetLightingChannels(true, true, false);
    Part->SetupAttachment(Skeletal, TEXT("hand_r"));
    Part->SetRelativeTransform(Desired.GetRelativeTransform(Hand));
    Part->RegisterComponent();
    return Part;
}
void UMemoriaFieldCharacterComponent::SetSwordDrawn(bool bDrawn)
{
    if (!HasSword() || bDrawn == bSwordDrawn) return;
    bSwordDrawn = bDrawn;
    if (Sheathed) Sheathed->SetHiddenInGame(bDrawn);
    Scabbard->SetHiddenInGame(!bDrawn); Blade->SetHiddenInGame(!bDrawn);
}
