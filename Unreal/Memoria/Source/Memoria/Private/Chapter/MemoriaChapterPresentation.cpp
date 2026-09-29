#include "Chapter/MemoriaChapterPresentation.h"
#include "Chapter/MemoriaChapterCardWidget.h"
#include "Combat/MemoriaCombatHudWidget.h"
#include "Combat/MemoriaExplorationHudWidget.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Framework/MemoriaVerdanTuning.h"
#include "Interaction/MemoriaEliaCompanion.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Audio/MemoriaAudioSubsystem.h"
#include "Camera/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PaperSpriteComponent.h"
namespace
{
FLinearColor Linear(const FLinearColor& Srgb) { return FLinearColor::FromSRGBColor(Srgb.ToFColor(false)); }
// The small set of authored strings the chapter maps raise outside dialogue, in Korean.
FString Ko(const FString& En)
{
    static const TMap<FString, FString> Table = {
        {TEXT("Obtained: Blank Book"), TEXT("획득: 백서")},
        {TEXT("The Belt"), TEXT("벨트")}, {TEXT("Weight of Pages"), TEXT("페이지의 무게")},
        {TEXT("drift_shelter"), TEXT("표류 쉼터")}, {TEXT("belt_waystation"), TEXT("벨트 중간역")},
        {TEXT("A faded Bureau sign: 'RELAY STATION 14, All combustion events must be reported within 72 hours.'"),
         TEXT("빛바랜 관리국 표지판: '중계소 14, 모든 연소는 72시간 이내에 보고할 것.'")}};
    const FString* Found = Table.Find(En); return Found ? *Found : En;
}
FString PlaceName(const FString& Map)
{ FString Out; for (const FString& Part : [&] { TArray<FString> P; Map.ParseIntoArray(P, TEXT("_")); return P; }()) Out += (Out.IsEmpty() ? TEXT("") : TEXT(" ")) + Part.Left(1).ToUpper() + Part.Mid(1); return Out; }
}
AMemoriaChapterPresentation::AMemoriaChapterPresentation()
{
    PrimaryActorTick.bCanEverTick = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}
UMaterialInstanceDynamic* AMemoriaChapterPresentation::Surface(const FLinearColor& Srgb, float Roughness)
{
    auto* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    auto* Material = UMaterialInstanceDynamic::Create(Base, this);
    Material->SetVectorParameterValue(TEXT("Color"), Linear(Srgb));
    return Material;
}
UInstancedStaticMeshComponent* AMemoriaChapterPresentation::Layer(const TCHAR* Mesh, UMaterialInstanceDynamic* Material, bool bCollide)
{
    auto* Instances = NewObject<UInstancedStaticMeshComponent>(this);
    Instances->SetupAttachment(GetRootComponent());
    Instances->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, Mesh));
    if (Material) Instances->SetMaterial(0, Material);
    Instances->SetCollisionProfileName(bCollide ? TEXT("BlockAll") : TEXT("NoCollision"));
    Instances->SetCastShadow(!bCollide);
    Instances->RegisterComponent(); AddInstanceComponent(Instances);
    return Instances;
}
int32 AMemoriaChapterPresentation::GetBlockerCount() const { return Blockers ? Blockers->GetInstanceCount() : 0; }
FVector2D AMemoriaChapterPresentation::PlayerSource() const { return Player.IsValid() ? MemoriaChapterMaps::ToSource(Player->GetActorLocation()) : FVector2D::ZeroVector; }
void AMemoriaChapterPresentation::BuildTerrain()
{
    // The tile grid, 32 px tiles at the map's 3x scale: flat ground for the open tiles, raised walls
    // (the border tall, the building walls low enough for the quarter view to see over), rubble on ruins.
    // An invisible block stands on every solid tile, as TilePainter.add_collisions does.
    const float T = Spec->TileSize * MemoriaChapterMaps::Scale;
    const TCHAR* Cube = TEXT("/Engine/BasicShapes/Cube.Cube");
    TArray<UInstancedStaticMeshComponent*> Ground;
    for (int32 Type = 0; Type < Spec->TileColors.Num(); ++Type) Ground.Add(Layer(Cube, Surface(Spec->TileColors[Type]), false));
    Blockers = Layer(Cube, nullptr, true); Blockers->SetHiddenInGame(true);
    const int32 Wall = Spec->TileNames.IndexOfByKey(TEXT("WALL")), Ruin = Spec->TileNames.IndexOfByKey(TEXT("RUIN"));
    FRandomStream Rng(Spec->Chapter * 7919);
    for (int32 Y = 0; Y < Spec->Height; ++Y)
        for (int32 X = 0; X < Spec->Width; ++X)
        {
            const int32 Type = Spec->TileAt(X, Y);
            if (!Ground.IsValidIndex(Type)) continue;
            const FVector Center = MemoriaChapterMaps::ToWorld(FVector2D((X + .5f) * Spec->TileSize, (Y + .5f) * Spec->TileSize));
            const bool bBorder = X == 0 || Y == 0 || X == Spec->Width - 1 || Y == Spec->Height - 1;
            if (Type == Wall)
            {
                // The border reads as a low rim and the building walls stay below the quarter view's line of sight.
                const float H = bBorder ? 50.f : 100.f;
                Ground[Type]->AddInstance(FTransform(FRotator::ZeroRotator, Center + FVector(0, 0, H * .5f - 8.f), FVector(T / 100.f, T / 100.f, H / 100.f)), true);
            }
            else
            {
                // Ground: a thin slab, its top at z -8 where the figures stand; a hair of height jitter keeps the grid from reading as a board.
                Ground[Type]->AddInstance(FTransform(FRotator::ZeroRotator, Center + FVector(0, 0, -13.f - Rng.FRand() * 1.5f), FVector(T / 100.f, T / 100.f, .1f)), true);
                if (Type == Ruin)
                    for (int32 I = 0; I < 3; ++I)
                    {
                        const FVector Offset(Rng.FRandRange(-.3f, .3f) * T, Rng.FRandRange(-.3f, .3f) * T, 0);
                        const float S = Rng.FRandRange(.22f, .45f) * T / 100.f, H = Rng.FRandRange(.25f, .7f);
                        Ground[Type]->AddInstance(FTransform(FRotator(Rng.FRandRange(-8.f, 8.f), Rng.FRandRange(0.f, 90.f), 0), Center + Offset + FVector(0, 0, H * 50.f - 8.f), FVector(S, S, H)), true);
                    }
            }
            if (Spec->IsSolid(Type)) Blockers->AddInstance(FTransform(FRotator::ZeroRotator, Center + FVector(0, 0, 60.f), FVector(T / 100.f, T / 100.f, 1.6f)), true);
        }
}
void AMemoriaChapterPresentation::BuildLight()
{
    // The atmosphere budget: the key light in the map's light colour, a cool fill, and a haze in its hue.
    for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It) It->Destroy();
    FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    if (auto* Key = GetWorld()->SpawnActor<ADirectionalLight>(FVector(0, 0, 800), FRotator(-52, 35, 0), Params))
    {
        auto* L = Key->GetComponent(); L->SetMobility(EComponentMobility::Movable);
        L->SetLightColor(Linear(Spec->Light)); L->SetIntensity(6.f + 4.f * Spec->Brightness); L->SetLightingChannels(true, true, false);
        L->ForwardShadingPriority = 1; // the key light is the one forward shading, translucency and fog use
    }
    if (auto* Fill = GetWorld()->SpawnActor<ADirectionalLight>(FVector(0, 0, 800), FRotator(-35, -140, 0), Params))
    {
        auto* L = Fill->GetComponent(); L->SetMobility(EComponentMobility::Movable); L->SetCastShadows(false);
        L->SetLightColor(Linear(FLinearColor(.55f, .62f, .78f))); L->SetIntensity(1.6f); L->SetLightingChannels(true, true, false);
        L->ForwardShadingPriority = 0; L->SetAtmosphereSunLight(false);
    }
    if (auto* Fog = GetWorld()->SpawnActor<AExponentialHeightFog>(FVector(0, 0, -40), FRotator::ZeroRotator, Params))
    {
        auto* F = Fog->GetComponent(); F->SetFogDensity(.012f + .02f * Spec->Mood); F->SetFogHeightFalloff(.5f);
        F->SetFogInscatteringColor(Linear(Spec->Hue) * .35f);
    }
}
void AMemoriaChapterPresentation::BuildMarkers()
{
    // make_discovery_marker: a small glowing plinth on each chest and clue, shown once the gate opens.
    auto* Plinth = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    auto Add = [&](const FString& Flag, const FVector2D& Origin, const FLinearColor& Tint)
    {
        auto* Mesh = NewObject<UStaticMeshComponent>(this);
        Mesh->SetupAttachment(GetRootComponent()); Mesh->SetStaticMesh(Plinth); Mesh->SetMaterial(0, Surface(Tint, .4f));
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetWorldLocation(MemoriaChapterMaps::ToWorld(Origin + FVector2D(Spec->TileSize * .5f, Spec->TileSize * .5f)) + FVector(0, 0, 4.f));
        Mesh->SetWorldScale3D(FVector(.35f, .35f, .24f));
        Mesh->RegisterComponent(); AddInstanceComponent(Mesh); Markers.Add(Flag, Mesh);
    };
    for (const auto& Chest : Spec->Chests) Add(Chest.Flag, Chest.Origin, FLinearColor(.75f, .6f, .25f));
    for (const auto& Clue : Spec->Clues) Add(Clue.Flag, Clue.Origin, FLinearColor(.3f, .45f, .8f));
}
bool AMemoriaChapterPresentation::Flag(const FString& Id) const
{
    const auto* Run = GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>();
    return !Id.IsEmpty() && Run && Run->GetRunSnapshot().GetFlag(Id);
}
void AMemoriaChapterPresentation::SetFlag(const FString& Id)
{
    if (auto* Run = GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>(); Run && !Id.IsEmpty()) Run->SetStoryFlag(Id, true);
    if (auto* Narrative = GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>()) Narrative->Record(TEXT("flag:") + Id);
}
FString AMemoriaChapterPresentation::Localized(const FString& Text) const
{
    const auto* Run = GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>();
    return Run && Run->GetRunSnapshot().CurrentLocale == TEXT("ko") ? Ko(Text) : Text;
}
void AMemoriaChapterPresentation::BeginPlay()
{
    Super::BeginPlay();
    Spec = MemoriaChapterMaps::Find(Map);
    auto* PC = GetWorld()->GetFirstPlayerController();
    Player = PC ? Cast<AMemoriaFieldPawn>(PC->GetPawn()) : nullptr;
    if (!Spec || !Player.IsValid()) { UE_LOG(LogTemp, Error, TEXT("MEMORIA_CHAPTER missing map or pawn: %s"), *Map); SetActorTickEnabled(false); return; }
    Player->ApplyVerdanMovementProfile(); Player->SetWalkSpeed(WalkSpeed);
    Player->SetActorLocation(MemoriaChapterMaps::ToWorld(Spec->Spawn) + FVector(0, 0, Player->GetActorLocation().Z));
    BuildTerrain(); BuildLight(); BuildMarkers();
    // The quarter-view camera and the painted/rigged figures, as in Verdan.
    auto* Camera = Player->GetFieldCamera(); Camera->ProjectionMode = ECameraProjectionMode::Perspective;
    Camera->SetFieldOfView(MemoriaVerdanTuning::CameraFOV); Camera->SetRelativeLocation(MemoriaVerdanTuning::CameraOffset);
    Camera->SetRelativeRotation(FRotator(MemoriaVerdanTuning::CameraPitch, 90, 0));
    if (auto* Sprite = Player->GetFieldSprite()) { Sprite->SetHiddenInGame(true); Sprite->SetCastShadow(false); }
    ArrelFigure = NewObject<UMemoriaFieldCharacterComponent>(this);
    AddInstanceComponent(ArrelFigure); ArrelFigure->SetupAttachment(Player->GetRootComponent()); ArrelFigure->RegisterComponent();
    ArrelFigure->InitializeCharacter(TEXT("arrel"), ArrelHeight);
    auto* Game = GetGameInstance();
    auto* Run = Game->GetSubsystem<UMemoriaRunSubsystem>();
    FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    if (Run && Run->GetRunSnapshot().Player.bEliaWithParty)
        if (auto* Elia = GetWorld()->SpawnActor<AMemoriaEliaCompanion>(Player->GetActorLocation() + FVector(-30, -20, 0), FRotator::ZeroRotator, Params))
            Elia->Follow(Player.Get());
    if (auto* Combat = GetWorld()->GetSubsystem<UMemoriaFieldCombatSubsystem>())
    {
        Combat->RegisterPlayer(Player.Get(), ArrelFigure);
        CombatHud = CreateWidget<UMemoriaCombatHudWidget>(PC, UMemoriaCombatHudWidget::StaticClass());
        CombatHud->Bind(Combat); CombatHud->SetVisibility(ESlateVisibility::HitTestInvisible); CombatHud->AddToViewport(4);
    }
    ExplorationHud = CreateWidget<UMemoriaExplorationHudWidget>(PC, UMemoriaExplorationHudWidget::StaticClass());
    ExplorationHud->SetPlace(PlaceName(Map), Ko(Map)); ExplorationHud->SetVisibility(ESlateVisibility::HitTestInvisible); ExplorationHud->AddToViewport(3);
    Card = CreateWidget<UMemoriaChapterCardWidget>(PC, UMemoriaChapterCardWidget::StaticClass());
    Card->SetVisibility(ESlateVisibility::HitTestInvisible); Card->AddToViewport(20);
    if (auto* Narrative = Game->GetSubsystem<UMemoriaNarrativeSubsystem>())
        FinishedHandle = Narrative->OnFieldFinished.AddUObject(this, &AMemoriaChapterPresentation::OnFieldFinished);
    // _ready_sequence: the chapter title on the first arrival, then the first unseen link of the chain.
    const bool bFirst = Spec->Sequence.Num() > 0 && !Flag(Spec->Sequence[0].Flag);
    if (bFirst)
    {
        const bool bKo = Run && Run->GetRunSnapshot().CurrentLocale == TEXT("ko");
        Card->Show(bKo ? FString::Printf(TEXT("%d장"), Spec->Chapter) : FString::Printf(TEXT("CHAPTER %d"), Spec->Chapter),
            bKo ? Ko(Spec->TitleName) : Spec->TitleName, bKo ? Ko(Spec->Subtitle) : Spec->Subtitle, 3.f);
    }
    StepAt = bFirst ? 3.3f : .3f;
    PreviousPosition = Player->GetActorLocation();
    if (auto* Narrative = Game->GetSubsystem<UMemoriaNarrativeSubsystem>()) Narrative->Record(TEXT("chapter:presented:") + Map);
}
void AMemoriaChapterPresentation::StartNextStep()
{
    auto* Narrative = GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
    if (!Narrative) return;
    for (const auto& Step : Spec->Sequence)
    {
        if (Flag(Step.Flag)) continue;
        // Wait out any dialogue, fight or menu before the next link starts.
        if (Narrative->GetState() != EMemoriaSliceState::Exploration) { StepAt = Clock + .3f; return; }
        SetFlag(Step.Flag);
        if (!Step.Group.IsEmpty()) Narrative->StartChapterField(Step.Group, MemoriaChapterMaps::GroupAsset(*Spec, Step.Group), Spec->DialogueFile);
        return;
    }
}
void AMemoriaChapterPresentation::OnFieldFinished(const FString& Group)
{
    auto* Narrative = GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
    // The departure closes the chapter: the next chapter number, then the completion card.
    if (bDeparting && Group == Spec->Exit.Group)
    {
        bDeparting = false; bComplete = true;
        if (auto* Run = GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>()) Run->SetCurrentChapter(Spec->Exit.NextChapter);
        const auto* Run = GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>();
        const bool bKo = Run && Run->GetRunSnapshot().CurrentLocale == TEXT("ko");
        // The next map is not ported yet: the card says where the road leads.
        Card->Show(bKo ? FString::Printf(TEXT("%d장 완료"), Spec->Chapter) : FString::Printf(TEXT("CHAPTER %d COMPLETE"), Spec->Chapter),
            bKo ? Ko(Spec->Exit.NextMap) : PlaceName(Spec->Exit.NextMap),
            bKo ? FString::Printf(TEXT("%d장으로 가는 길은 아직 준비 중입니다"), Spec->Exit.NextChapter) : FString::Printf(TEXT("The road to Chapter %d is still being prepared"), Spec->Exit.NextChapter), 6.f);
        if (Narrative) Narrative->Record(TEXT("chapter:complete:") + Map);
        return;
    }
    for (const auto& Step : Spec->Sequence)
        if (Step.Group == Group)
        {
            // The group's dialogue_ended handler: its flags and toasts, then the next link after a beat.
            for (const FString& F : Step.Flags) SetFlag(F);
            for (const FString& Toast : Step.Toasts) if (Narrative) Narrative->ShowNotice(Localized(Toast));
            StepAt = Clock + StepDelay;
            return;
        }
}
void AMemoriaChapterPresentation::CheckTriggers()
{
    auto* Narrative = GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
    auto* Combat = GetWorld()->GetSubsystem<UMemoriaFieldCombatSubsystem>();
    if (!Narrative || Narrative->GetState() != EMemoriaSliceState::Exploration || bComplete || (Combat && Combat->IsDefeated())) return;
    const FVector2D P = PlayerSource();
    for (const auto& Trigger : Spec->Triggers)
        if (!Flag(Trigger.Flag) && (Trigger.Gate.IsEmpty() || Flag(Trigger.Gate)) && Trigger.Rect.Contains(P))
        { SetFlag(Trigger.Flag); Narrative->StartChapterField(Trigger.Group, MemoriaChapterMaps::GroupAsset(*Spec, Trigger.Group), Spec->DialogueFile); return; }
    const auto& Exit = Spec->Exit;
    if (!bDeparting && Exit.Rect.Contains(P) && Flag(Exit.Requires) && !Flag(Exit.Completes))
    {
        SetFlag(Exit.Completes); bDeparting = true;
        Narrative->StartChapterField(Exit.Group, MemoriaChapterMaps::GroupAsset(*Spec, Exit.Group), Spec->DialogueFile);
        return;
    }
    auto* Run = GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>();
    if (Run && (Spec->ObjectsGate.IsEmpty() || Flag(Spec->ObjectsGate)))
    {
        const FVector2D Tile(Spec->TileSize, Spec->TileSize);
        for (const auto& Chest : Spec->Chests)
            if (!Flag(Chest.Flag) && FMemoriaChapterRect{Chest.Origin, Tile}.Contains(P))
            {
                SetFlag(Chest.Flag); Run->AddGrains(Chest.Grains);
                for (const auto& Item : Chest.Items) Run->GrantFieldItem(Item.Key, Item.Value);
                Narrative->ShowNotice(FString::Printf(TEXT("+%lld Grains"), Chest.Grains));
                if (auto* Audio = GetGameInstance()->GetSubsystem<UMemoriaAudioSubsystem>()) Audio->PlaySfx(TEXT("ui_select"));
            }
        for (const auto& Clue : Spec->Clues)
            if (!Flag(Clue.Flag) && FMemoriaChapterRect{Clue.Origin, Tile}.Contains(P))
            { SetFlag(Clue.Flag); Narrative->ShowNotice(Localized(Clue.Text)); }
    }
    if (Combat && (Spec->BattlesGate.IsEmpty() || Flag(Spec->BattlesGate)))
        for (int32 I = 0; I < Spec->Battles.Num(); ++I)
        {
            const auto& Battle = Spec->Battles[I];
            const FString Id = FString::Printf(TEXT("battle_%s_%d"), *Map, I + 1);
            // The source's one-time battle areas become a foe rising in the field (void ones as husks).
            if (!Flag(Id) && Battle.Rect.Contains(P))
            {
                SetFlag(Id);
                Combat->SpawnWave(1, Player->GetActorLocation(), 380.f, Battle.bVoid ? EMemoriaFoeKind::VoidHusk : EMemoriaFoeKind::MarketThief);
            }
        }
}
void AMemoriaChapterPresentation::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Clock += DeltaSeconds;
    if (!Player.IsValid() || !Spec) return;
    const FVector Position = Player->GetActorLocation();
    ArrelFigure->AdvanceLocomotion(Position - PreviousPosition, DeltaSeconds);
    PreviousPosition = Position;
    for (const auto& Pair : Markers) if (Pair.Value) Pair.Value->SetHiddenInGame(Flag(Pair.Key) || !(Spec->ObjectsGate.IsEmpty() || Flag(Spec->ObjectsGate)));
    if (StepAt >= 0.f && Clock >= StepAt) { StepAt = -1.f; StartNextStep(); }
    CheckTriggers();
}
void AMemoriaChapterPresentation::EndPlay(const EEndPlayReason::Type Reason)
{
    if (auto* Narrative = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>() : nullptr) Narrative->OnFieldFinished.Remove(FinishedHandle);
    for (UUserWidget* Widget : {static_cast<UUserWidget*>(CombatHud.Get()), static_cast<UUserWidget*>(ExplorationHud.Get()), static_cast<UUserWidget*>(Card.Get())})
        if (Widget) Widget->RemoveFromParent();
    Super::EndPlay(Reason);
}
