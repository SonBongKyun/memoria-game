#include "Chapter/MemoriaChapterPresentation.h"
#include "Chapter/MemoriaChapterCardWidget.h"
#include "Combat/MemoriaCombatHudWidget.h"
#include "Combat/MemoriaExplorationHudWidget.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Combat/MemoriaFieldMonster.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Framework/MemoriaVerdanTuning.h"
#include "Interaction/MemoriaEliaCompanion.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Domain/MemoriaChapterMemories.h"
#include "Domain/MemoriaPlayerMemoryDomain.h"
#include "Audio/MemoriaAudioSubsystem.h"
#include "Achievements/MemoriaAchievementSubsystem.h"
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
FString Ko(const FString& En) { return MemoriaChapterMaps::Korean(En); }
// The story scenes a chapter's road leads into before they are ported (drift_shelter.gd -> Chapter 5).
FString SceneTitle(const FString& Scene) { return Scene.Contains(TEXT("ch5_classifier")) ? TEXT("The Classifier") : FString(); }
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
    auto* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Memoria/Presentation/Depth/M_Surface.M_Surface"), nullptr, LOAD_NoWarn | LOAD_Quiet);
    if (!Base) Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    auto* Material = UMaterialInstanceDynamic::Create(Base, this);
    Material->SetVectorParameterValue(TEXT("Tint"), Linear(Srgb));
    Material->SetVectorParameterValue(TEXT("Color"), Linear(Srgb));
    Material->SetScalarParameterValue(TEXT("Mode"), 3.f);
    Material->SetScalarParameterValue(TEXT("Roughness"), Roughness);
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
void AMemoriaChapterPresentation::BuildDecorations()
{
    // _setup_map_decorations: the source lays flat translucent ColorRects under the actors. Here each stands as
    // a simple prop named by the script's variable: the waystation's leaning water tank, the cracks in the belt
    // road, the shelter's campfire and its light, the rubble heaps.
    auto* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    auto* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    FRandomStream Rng(Spec->Chapter * 104729);
    const float S = MemoriaChapterMaps::Scale;
    // S335: Codex's S334 props (-run=MemoriaAmbientModels) stand in place of the primitives when they are
    // imported: centimetre models with their origin on the ground at the centre.
    auto Model = [](const TCHAR* Name)
    { return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Memoria/Presentation/Field3D/Props/SM_%s.SM_%s"), Name, Name), nullptr, LOAD_NoWarn | LOAD_Quiet); };
    UStaticMesh* TankModel = Model(TEXT("WaterTank")); UStaticMesh* FireModel = Model(TEXT("Campfire")); UStaticMesh* RubbleModel = Model(TEXT("Rubble"));
    auto Place = [this](UStaticMesh* Asset, const FVector& Ground, float Yaw)
    {
        auto* Prop = NewObject<UStaticMeshComponent>(this);
        Prop->SetupAttachment(GetRootComponent()); Prop->SetMobility(EComponentMobility::Movable);
        Prop->SetStaticMesh(Asset); Prop->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        // The figures stand on z -8, the top of the ground slabs.
        Prop->SetWorldLocation(FVector(Ground.X, Ground.Y, -8.f)); Prop->SetWorldRotation(FRotator(0, Yaw, 0));
        Prop->RegisterComponent(); AddInstanceComponent(Prop); ++ModelPropCount;
    };
    for (const auto& D : Spec->Decorations)
    {
        const FVector Center = MemoriaChapterMaps::ToWorld(D.Origin + D.Size * .5);
        if (D.bLight)
        {
            auto* Light = NewObject<UPointLightComponent>(this);
            Light->SetupAttachment(GetRootComponent()); Light->SetMobility(EComponentMobility::Movable);
            Light->SetWorldLocation(MemoriaChapterMaps::ToWorld(D.Origin) + FVector(0, 0, 45.f));
            Light->SetLightColor(Linear(D.Color)); Light->SetIntensity(9000.f * D.Energy);
            Light->SetAttenuationRadius(110.f * D.Scale); Light->SetCastShadows(false);
            Light->RegisterComponent(); AddInstanceComponent(Light); ++DecorationCount;
            continue;
        }
        auto* Mesh = NewObject<UStaticMeshComponent>(this);
        Mesh->SetupAttachment(GetRootComponent()); Mesh->SetMobility(EComponentMobility::Movable);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        const FVector2D World = D.Size * S;
        if (D.Kind == TEXT("tank"))
        {
            // A cylinder as wide as the rect, as tall as the rect is long, standing on the rect's lower end and
            // leaning by the source's rotation. It is the one prop Arrel cannot walk through.
            const float Radius = World.X * .5f, Tall = World.Y;
            // The source's translucent shade would stand as a black mass here; the metal is lifted to read as a tank.
            const FVector Foot = MemoriaChapterMaps::ToWorld(FVector2D(D.Origin.X + D.Size.X * .5, D.Origin.Y + D.Size.Y - D.Size.X * .5));
            Mesh->SetStaticMesh(Cylinder); Mesh->SetMaterial(0, Surface(D.Color * 1.7f));
            Mesh->SetWorldScale3D(FVector(Radius / 50.f, Radius / 50.f, Tall / 100.f));
            Mesh->SetWorldLocation(Foot + FVector(0, 0, Tall * .5f - 10.f));
            Mesh->SetCollisionProfileName(TEXT("BlockAll"));
            // With the model, the cylinder stays only as the unseen block; the model carries its own lean.
            if (TankModel) { Mesh->SetHiddenInGame(true); Mesh->SetCastShadow(false); Place(TankModel, Foot, 0.f); }
            else Mesh->SetWorldRotation(FRotator(0, 0, FMath::RadiansToDegrees(D.Rotation)));
        }
        else if (D.Kind == TEXT("fire"))
        {
            // The campfire: a small bright ember block; its PointLight2D follows as the next decoration.
            Mesh->SetStaticMesh(Cube);
            auto* Ember = Surface(D.Color); Ember->SetVectorParameterValue(TEXT("Color"), Linear(D.Color) * 6.f);
            Ember->SetVectorParameterValue(TEXT("Tint"), Linear(D.Color) * 6.f);
            Mesh->SetMaterial(0, Ember); Mesh->SetCastShadow(false);
            Mesh->SetWorldScale3D(FVector(World.X / 100.f, World.Y / 100.f, .14f));
            Mesh->SetWorldRotation(FRotator(0, 45.f, 0));
            Mesh->SetWorldLocation(Center + FVector(0, 0, -1.f));
            // With the model, the ember block glows in the middle of its stone ring and crossed logs.
            if (FireModel) { Place(FireModel, Center, 20.f); Mesh->SetWorldScale3D(FVector(.16f, .16f, .08f)); Mesh->SetWorldLocation(Center + FVector(0, 0, 0.f)); }
        }
        else if (D.Kind == TEXT("crack"))
        {
            // A dark seam lying on the road.
            Mesh->SetStaticMesh(Cube); Mesh->SetMaterial(0, Surface(D.Color)); Mesh->SetCastShadow(false);
            Mesh->SetWorldScale3D(FVector(FMath::Max(World.X, 5.f) / 100.f, World.Y / 100.f, .02f));
            Mesh->SetWorldRotation(FRotator(0, Rng.FRandRange(-9.f, 9.f), 0));
            Mesh->SetWorldLocation(Center + FVector(0, 0, -7.f));
        }
        else
        {
            // Rubble and any other heap: the model turned at random, or a low stone.
            if (D.Kind == TEXT("rubble") && RubbleModel)
            { Place(RubbleModel, Center, Rng.FRandRange(0.f, 360.f)); Mesh->MarkAsGarbage(); ++DecorationCount; continue; }
            const float Tall = Rng.FRandRange(10.f, 20.f);
            Mesh->SetStaticMesh(Cube); Mesh->SetMaterial(0, Surface(D.Color));
            Mesh->SetWorldScale3D(FVector(World.X / 100.f, World.Y / 100.f, Tall / 100.f));
            Mesh->SetWorldRotation(FRotator(Rng.FRandRange(-6.f, 6.f), Rng.FRandRange(0.f, 60.f), 0));
            Mesh->SetWorldLocation(Center + FVector(0, 0, Tall * .5f - 8.f));
        }
        Mesh->RegisterComponent(); AddInstanceComponent(Mesh); ++DecorationCount;
    }
}
void AMemoriaChapterPresentation::BuildAmbientNpcs()
{
    // The revisit's ambient NPCs, standing still at their tiles: Codex's S334 models (-run=MemoriaAmbientModels,
    // S335), or without them the source's procedural pixel figures (Unreal/Tools/export_ambient_npcs.py) on
    // cards. They are built hidden; Tick shows them once their gate opens.
    for (const auto& Npc : Spec->AmbientNpcs)
    {
        auto* Figure = NewObject<UMemoriaFieldCharacterComponent>(this);
        AddInstanceComponent(Figure); Figure->SetupAttachment(GetRootComponent()); Figure->RegisterComponent();
        Figure->SetWorldLocation(MemoriaChapterMaps::ToWorld(Npc.Position));
        // "bureau_agent" -> "bureauagent": the component title-cases the id into the sprite's name.
        if (!Figure->InitializeCharacter(Npc.Preset.Replace(TEXT("_"), TEXT("")), ArrelHeight)) { Figure->DestroyComponent(); continue; }
        Figure->Face(TEXT("Down")); Figure->SetVisibility(false, true);
        AmbientNpcs.Add(Figure);
        // Each sets out a little after the one before, so they are never in step.
        FAmbientMind Mind; Mind.Home = Mind.Target = Figure->GetComponentLocation(); Mind.Wait = 1.5f + 1.3f * NpcMinds.Num();
        NpcMinds.Add(Mind);
    }
}
bool AMemoriaChapterPresentation::CanStand(const FVector& World) const
{
    // An open tile, and clear of the one prop that blocks (the water tank).
    const FVector2D P = MemoriaChapterMaps::ToSource(World);
    const int32 Type = Spec->TileAt(FMath::FloorToInt32(P.X / Spec->TileSize), FMath::FloorToInt32(P.Y / Spec->TileSize));
    if (Type < 0 || Spec->IsSolid(Type)) return false;
    for (const auto& D : Spec->Decorations)
        if (D.Kind == TEXT("tank") && P.X > D.Origin.X - 28 && P.Y > D.Origin.Y - 28 && P.X < D.Origin.X + D.Size.X + 28 && P.Y < D.Origin.Y + D.Size.Y + 28) return false;
    return true;
}
void AMemoriaChapterPresentation::TickAmbientNpcs(float DeltaSeconds)
{
    auto* Combat = GetWorld()->GetSubsystem<UMemoriaFieldCombatSubsystem>();
    const FVector Arrel = Player->GetActorLocation();
    for (int32 I = 0; I < AmbientNpcs.Num() && I < NpcMinds.Num(); ++I)
    {
        auto* Figure = AmbientNpcs[I].Get(); FAmbientMind& Mind = NpcMinds[I];
        if (!Figure) continue;
        const FVector At = Figure->GetComponentLocation();
        const FVector ToArrel = (Arrel - At) * FVector(1, 1, 0);
        FVector Step = FVector::ZeroVector; float Want = Mind.Yaw;
        // The nearest live foe, if a fight is on.
        const AMemoriaFieldMonster* Foe = nullptr;
        if (Combat)
            for (const auto& Weak : Combat->GetMonsters())
                if (const auto* M = Weak.Get(); M && !M->IsDead() && (!Foe || FVector::DistSquared2D(M->GetActorLocation(), At) < FVector::DistSquared2D(Foe->GetActorLocation(), At))) Foe = M;
        if (Foe)
        {
            // A fight: it stands where it is and watches the foe.
            Mind.bWalking = false; Mind.Wait = FMath::Max(Mind.Wait, 2.f);
            Want = ((Foe->GetActorLocation() - At) * FVector(1, 1, 0)).Rotation().Yaw;
        }
        else if (ToArrel.Size() < NpcNotice)
        {
            // Arrel is close: it stops and turns to him.
            Mind.bWalking = false; Mind.Wait = FMath::Max(Mind.Wait, 1.2f);
            if (!ToArrel.IsNearlyZero()) Want = ToArrel.Rotation().Yaw;
        }
        else if (Mind.bWalking)
        {
            const FVector To = (Mind.Target - At) * FVector(1, 1, 0);
            const float Left = To.Size();
            if (Left < 3.f) { Mind.bWalking = false; Mind.Wait = NpcRng.FRandRange(2.5f, 6.f); }
            else { Step = To / Left * FMath::Min(Left, NpcSpeed * DeltaSeconds); Want = To.Rotation().Yaw; }
        }
        else if ((Mind.Wait -= DeltaSeconds) <= 0.f)
        {
            // A new spot within reach of its place: open ground the whole way, and not on top of Arrel.
            Mind.Wait = 1.5f;
            for (int32 Try = 0; Try < 6; ++Try)
            {
                const float Angle = NpcRng.FRandRange(0.f, 2.f * PI), Reach = NpcRng.FRandRange(60.f, NpcRoam);
                const FVector Spot = Mind.Home + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0) * Reach;
                bool bClear = FVector::Dist2D(Spot, Arrel) > NpcNotice;
                for (const float Along : {.25f, .5f, .75f, 1.f}) bClear = bClear && CanStand(FMath::Lerp(At, Spot, Along));
                if (bClear) { Mind.Target = FVector(Spot.X, Spot.Y, At.Z); Mind.bWalking = true; break; }
            }
        }
        Mind.Yaw = FMath::FixedTurn(Mind.Yaw, Want, 320.f * DeltaSeconds);
        if (!Step.IsZero()) { Figure->SetWorldLocation(At + Step); NpcTravel += Step.Size(); }
        Figure->AdvanceLocomotion(Step, DeltaSeconds);
        // A rigged figure turns freely; a card keeps the four facings its walk gave it.
        if (Figure->IsRigged()) Figure->SetAim(Mind.Yaw);
    }
}
int32 AMemoriaChapterPresentation::GetRiggedNpcCount() const
{
    int32 Count = 0; for (const auto& Npc : AmbientNpcs) Count += Npc && Npc->IsRigged() ? 1 : 0; return Count;
}
int32 AMemoriaChapterPresentation::GetVisibleNpcCount() const
{
    int32 Count = 0; for (const auto& Npc : AmbientNpcs) Count += Npc && Npc->IsVisible() ? 1 : 0; return Count;
}
bool AMemoriaChapterPresentation::AreEncountersOpen() const
{
    return Spec && !Spec->Encounters.IsEmpty() && !Spec->EncountersGate.IsEmpty() && GateOpen(Spec->EncountersGate) && !bComplete && !bDeparting;
}
void AMemoriaChapterPresentation::UpdateEncounters()
{
    // RandomEncounter.update on a closed chapter's map: the source's distance model with the map's own range and
    // pool. The foes rise in the field (void entries as three husks, the others as two thieves, as in Verdan).
    auto* Narrative = GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
    auto* Combat = GetWorld()->GetSubsystem<UMemoriaFieldCombatSubsystem>();
    if (!AreEncountersOpen() || !Narrative || !Combat || !Combat->GetPlayer()) return;
    if (!bEncounterReady)
    {
        Encounter.Reset(EncounterRng.Real(Spec->EncounterMin, Spec->EncounterMax)); bEncounterReady = true;
        Encounter.MinSteps = Spec->EncounterMin; Encounter.MaxSteps = Spec->EncounterMax; Encounter.PoolSize = Spec->Encounters.Num();
    }
    // A fight holds the distance, as the source's separate battle scene did.
    const bool bExploring = Narrative->GetState() == EMemoriaSliceState::Exploration && Combat->LiveMonsterCount() == 0 && !Combat->IsDefeated();
    const auto Step = Encounter.Advance(PlayerSource(), bExploring, EncounterRng, false, Spec->TileSize);
    auto* Run = GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>();
    if (Step.bWarningStarted)
    {
        Narrative->Record(TEXT("encounter:warning"));
        Narrative->ShowNotice(Run && Run->GetRunSnapshot().CurrentLocale == TEXT("ko") ? TEXT("기억 소음이 닫힌다") : TEXT("Memory noise closes in"));
    }
    if (Step.bTriggered && Spec->Encounters.IsValidIndex(Step.EnemyIndex))
    {
        // S342: the pool's own foe, by its source name, in its pack.
        const EMemoriaFoeKind Kind = FoeKindByName(Spec->Encounters[Step.EnemyIndex].Name, Spec->Encounters[Step.EnemyIndex].bVoid);
        Combat->SpawnWave(FoePackSize(Kind), Player->GetActorLocation(), 420.f, Kind);
        if (Run) Run->RecordBattleStarted();
        Narrative->Record(TEXT("encounter:field_started:") + Spec->Encounters[Step.EnemyIndex].Name);
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
bool AMemoriaChapterPresentation::SceneReady() const
{
    // The departure's flags (canon_ch5_classifier_ready) stand and no later canon flag has moved the story on.
    const auto& Exit = Spec->Exit;
    if (Exit.NextScene.IsEmpty() || Exit.Flags.IsEmpty()) return false;
    for (const FString& F : Exit.Flags) if (!Flag(F)) return false;
    return StoryMovedOn().IsEmpty();
}
bool AMemoriaChapterPresentation::RoadOpen() const
{
    // The chapter is closed and its next map is ported.
    return Flag(Spec->Exit.Completes) && MemoriaChapterMaps::Find(Spec->Exit.NextMap) != nullptr;
}
FString AMemoriaChapterPresentation::StoryMovedOn() const
{
    for (const FString& Blocked : Spec->ResumeBlocked) if (!Spec->Exit.Flags.Contains(Blocked) && Flag(Blocked)) return Blocked;
    return FString();
}
bool AMemoriaChapterPresentation::GateOpen(const FString& Gate) const
{
    // _can_resume_ch4_exploration: the section's flag, and none of the later canon flags that move the story on.
    for (const FString& Blocked : Spec->ResumeBlocked) if (Flag(Blocked)) return false;
    return Gate.IsEmpty() || Flag(Gate);
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
    // S330: a continued save stands where it was saved; otherwise _position_player's spawn.
    FVector2D Place = Spec->Spawn;
    if (auto* Narrative = GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>()) Narrative->ConsumeMapPosition(Place);
    Player->SetActorLocation(MemoriaChapterMaps::ToWorld(Place) + FVector(0, 0, Player->GetActorLocation().Z));
    BuildTerrain(); BuildLight(); BuildMarkers(); BuildDecorations(); BuildAmbientNpcs();
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
    // _ready_sequence: a readied story scene launches at once; after it, the chain is over and the card names
    // the chapter the story reached (canon_ch6_seam_ready: "Next: Chapter 6, The Seam").
    if (SceneReady()) { StepAt = -1.f; SceneAt = .5f; }
    else if (const FString Moved = StoryMovedOn(); !Moved.IsEmpty())
    {
        StepAt = -1.f; bComplete = true;
        const bool bKo = Run && Run->GetRunSnapshot().CurrentLocale == TEXT("ko");
        const int32 Reached = int32(Run ? Run->GetRunSnapshot().CurrentChapter : Spec->Chapter);
        const FString Next = Moved == TEXT("canon_ch6_seam_ready") ? TEXT("The Seam") : FString();
        Card->Show(bKo ? FString::Printf(TEXT("%d장 완료"), Reached) : FString::Printf(TEXT("CHAPTER %d COMPLETE"), Reached), bKo ? Ko(Next) : Next,
            bKo ? FString::Printf(TEXT("%d장으로 가는 길은 아직 준비 중입니다"), Reached + 1) : FString::Printf(TEXT("The road to Chapter %d is still being prepared"), Reached + 1), SceneDelay);
        if (auto* Narrative = Game->GetSubsystem<UMemoriaNarrativeSubsystem>(); Narrative && !Next.IsEmpty())
            Narrative->ShowNotice(Localized(FString::Printf(TEXT("Next: Chapter %d, %s"), Reached + 1, *Next)));
    }
    // S330: a save loaded in a map whose chapter is closed (the source's autosave at the departure). The source
    // goes on from there through its world atlas, which is not ported; here the exit stays a road onward.
    bInExit = Spec->Exit.Rect.Contains(PlayerSource());
    if (RoadOpen() && StepAt >= 0.f && !bFirst)
        if (auto* Narrative = Game->GetSubsystem<UMemoriaNarrativeSubsystem>())
        {
            const bool bKo = Run && Run->GetRunSnapshot().CurrentLocale == TEXT("ko");
            const FString Where = bKo ? Ko(Spec->Exit.NextMap) : PlaceName(Spec->Exit.NextMap);
            Narrative->ShowNotice(bKo ? FString::Printf(TEXT("%d장 완료. 출구의 길이 다음 목적지로 이어집니다: %s"), Spec->Chapter, *Where)
                                      : FString::Printf(TEXT("Chapter %d complete. The road at the exit goes on: %s"), Spec->Chapter, *Where));
        }
    PreviousPosition = Player->GetActorLocation();
    if (auto* Narrative = Game->GetSubsystem<UMemoriaNarrativeSubsystem>()) Narrative->Record(TEXT("chapter:presented:") + Map);
    if (auto* Achievements = Game->GetSubsystem<UMemoriaAchievementSubsystem>()) Achievements->RecordMapVisit(Map);
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
        if (Step.MemoriesChapter > 0) GrantChapterMemories(Step.MemoriesChapter);
        if (!Step.Group.IsEmpty()) Narrative->StartChapterField(Step.Group, MemoriaChapterMaps::GroupAsset(*Spec, Step.Group), Spec->DialogueFile);
        return;
    }
}
void AMemoriaChapterPresentation::GrantChapterMemories(int32 Chapter)
{
    // _start_chN_sequence: MemoryManager.add_chapter_memories(N). NotificationToast raises one toast per
    // new memory; here they wait for the arrival chain to end, where the field shows notices long enough to read.
    auto* Run = GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>();
    auto* Narrative = GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
    if (!Run || !Run->HasActiveRun()) return;
    const int32 Before = Run->GetPlayerMemory()->GetDefinitions().Num();
    if (Run->AddChapterMemories(Chapter) != EMemoriaMemoryResult::Success) { if (Narrative) Narrative->Record(TEXT("error:chapter_memories")); return; }
    const auto& Definitions = Run->GetPlayerMemory()->GetDefinitions();
    const bool bKo = Run->GetRunSnapshot().CurrentLocale == TEXT("ko");
    for (int32 I = Before; I < Definitions.Num(); ++I)
    {
        FMemoriaMemoryLocalizedText Text;
        const FString Title = bKo && MemoriaChapterMemories::Korean(Definitions[I].Id, Text) ? Text.Title : Definitions[I].Title;
        MemoryToasts.Add(bKo ? FString::Printf(TEXT("기억 획득: %s"), *Title) : FString::Printf(TEXT("Memory acquired: %s"), *Title));
    }
    if (Narrative) Narrative->Record(FString::Printf(TEXT("chapter:memories:%d:%d"), Chapter, Definitions.Num() - Before));
}
void AMemoriaChapterPresentation::OnFieldFinished(const FString& Group)
{
    auto* Narrative = GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
    // The departure closes the chapter: its end handler's chapter number, flags and notice, then the
    // completion card, and the road on to the next chapter map when it is ported.
    if (bDeparting && Group == Spec->Exit.Group)
    {
        bDeparting = false; bComplete = true;
        const auto& Exit = Spec->Exit;
        auto* Run = GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>();
        if (Run) Run->SetCurrentChapter(Exit.NextChapter);
        for (const FString& F : Exit.Flags) SetFlag(F);
        const bool bKo = Run && Run->GetRunSnapshot().CurrentLocale == TEXT("ko");
        if (!Exit.NoticeEn.IsEmpty() && Narrative) Narrative->ShowNotice(bKo && !Exit.NoticeKo.IsEmpty() ? Exit.NoticeKo : Exit.NoticeEn);
        const int32 Next = Spec->Chapter + 1;
        const bool bRoad = MemoriaChapterMaps::Find(Exit.NextMap) != nullptr;
        const FString Where = !Exit.NextMap.IsEmpty() ? (bKo ? Ko(Exit.NextMap) : PlaceName(Exit.NextMap)) : (bKo ? Ko(SceneTitle(Exit.NextScene)) : SceneTitle(Exit.NextScene));
        const FString Eyebrow = bKo ? FString::Printf(TEXT("%d장 완료"), Spec->Chapter) : FString::Printf(TEXT("CHAPTER %d COMPLETE"), Spec->Chapter);
        const FString Road = bRoad ? (bKo ? FString::Printf(TEXT("%d장으로 이어집니다"), Next) : FString::Printf(TEXT("The road goes on to Chapter %d"), Next))
                                   : (bKo ? FString::Printf(TEXT("%d장으로 가는 길은 아직 준비 중입니다"), Next) : FString::Printf(TEXT("The road to Chapter %d is still being prepared"), Next));
        Card->Show(Eyebrow, Where, Road, bRoad ? TravelDelay : SceneDelay);
        // change_scene_chapter_complete: the card, then the next map; or the story scene after the card.
        if (bRoad) TravelAt = Clock + TravelDelay;
        else if (!Exit.NextScene.IsEmpty()) SceneAt = Clock + SceneDelay;
        if (Narrative) Narrative->Record(TEXT("chapter:complete:") + Map);
        // _on_departure_ended: SaveManager.autosave_on_chapter_transition, in this map at Arrel's place.
        if (Narrative) Narrative->AutosaveChapterMap(Map, PlayerSource());
        return;
    }
    for (const auto& Step : Spec->Sequence)
        if (Step.Group == Group)
        {
            // The group's dialogue_ended handler: its flags and toasts, then the next link after a beat.
            for (const FString& F : Step.Flags) SetFlag(F);
            for (const FString& Toast : Step.Toasts) if (Narrative) Narrative->ShowNotice(Localized(Toast));
            // The field shows notices only between dialogues, so the memory toasts wait for the chain's last link.
            if (&Step == &Spec->Sequence.Last())
            {
                for (const FString& Toast : MemoryToasts) if (Narrative) Narrative->ShowNotice(Toast);
                MemoryToasts.Reset();
            }
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
    // body_entered: the exit acts on stepping into it, not on standing in it.
    const bool bWasInExit = bInExit; bInExit = Exit.Rect.Contains(P);
    if (!bDeparting && bInExit && Flag(Exit.Requires) && !Flag(Exit.Completes))
    {
        SetFlag(Exit.Completes); bDeparting = true;
        Narrative->StartChapterField(Exit.Group, MemoriaChapterMaps::GroupAsset(*Spec, Exit.Group), Spec->DialogueFile);
        return;
    }
    // S330: in a closed chapter the exit is the road onward (see BeginPlay).
    if (!bDeparting && bInExit && !bWasInExit && RoadOpen() && !(Combat && Combat->LiveMonsterCount() > 0))
    { bComplete = true; Narrative->TravelToChapterMap(Exit.NextMap); return; }
    auto* Run = GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>();
    if (Run && GateOpen(Spec->ObjectsGate))
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
    if (Combat && GateOpen(Spec->BattlesGate))
        for (int32 I = 0; I < Spec->Battles.Num(); ++I)
        {
            const auto& Battle = Spec->Battles[I];
            const FString Id = FString::Printf(TEXT("battle_%s_%d"), *Map, I + 1);
            // The source's one-time battle areas become a foe rising in the field (void ones as husks).
            if (!Flag(Id) && Battle.Rect.Contains(P))
            {
                SetFlag(Id);
                // S342: the area's own foe, one fewer than its roaming pack.
                const EMemoriaFoeKind Kind = FoeKindByName(Battle.Name, Battle.bVoid);
                Combat->SpawnWave(FMath::Max(1, FoePackSize(Kind) - 1), Player->GetActorLocation(), 380.f, Kind);
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
    for (const auto& Pair : Markers) if (Pair.Value) Pair.Value->SetHiddenInGame(Flag(Pair.Key) || !GateOpen(Spec->ObjectsGate));
    // The ambient NPCs stand only on the revisit (belt_waystation.gd: "The first canonical visit is abandoned").
    const bool bNpcs = !Spec->AmbientNpcsGate.IsEmpty() && GateOpen(Spec->AmbientNpcsGate);
    for (const auto& Npc : AmbientNpcs) if (Npc && Npc->IsVisible() != bNpcs) Npc->SetVisibility(bNpcs, true);
    if (bNpcs) TickAmbientNpcs(DeltaSeconds);
    TickEnvironment(DeltaSeconds);
    UpdateEncounters();
    if (StepAt >= 0.f && Clock >= StepAt) { StepAt = -1.f; StartNextStep(); }
    if (SceneAt >= 0.f && Clock >= SceneAt)
    {
        // The story scene waits for any dialogue or menu to close.
        auto* Narrative = GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
        if (Narrative && Narrative->GetState() != EMemoriaSliceState::Exploration) SceneAt = Clock + .3f;
        else { SceneAt = -1.f; if (Narrative) Narrative->EnterStoryScene(Spec->Exit.NextScene); }
    }
    if (TravelAt >= 0.f && Clock >= TravelAt)
    {
        TravelAt = -1.f;
        if (auto* Narrative = GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>()) Narrative->TravelToChapterMap(Spec->Exit.NextMap);
        return;
    }
    CheckTriggers();
}
void AMemoriaChapterPresentation::EndPlay(const EEndPlayReason::Type Reason)
{
    if (auto* Narrative = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>() : nullptr) Narrative->OnFieldFinished.Remove(FinishedHandle);
    for (UUserWidget* Widget : {static_cast<UUserWidget*>(CombatHud.Get()), static_cast<UUserWidget*>(ExplorationHud.Get()), static_cast<UUserWidget*>(Card.Get())})
        if (Widget) Widget->RemoveFromParent();
    Super::EndPlay(Reason);
}
