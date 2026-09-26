#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "Presentation/MemoriaVerdanArt.h"
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
    const UPaperSprite* Sprite = Card ? Card->GetSprite() : nullptr;
    return Sprite ? Sprite->GetName() + (Card->GetRelativeScale3D().X < 0 ? TEXT("#mirror") : TEXT("")) : FString();
}
void UMemoriaFieldCharacterComponent::ApplyFrame()
{
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
    Age += DeltaTime; ApplyFrame();
}
