#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Framework/MemoriaSliceHost.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Chapter/MemoriaChapterMap.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Combat/MemoriaFieldCombatTypes.h"
#include "Combat/MemoriaFieldMonster.h"
#include "Interaction/MemoriaEliaCompanion.h"
#include "Tutorial/MemoriaTutorialSubsystem.h"
#include "Achievements/MemoriaAchievementSubsystem.h"
#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "EngineUtils.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "CollisionQueryParams.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
// S342: the foes of the chapter maps' pools. Each carries its source name, HP and attack; the wisp keeps its
// distance and throws an orb that can be stepped out of; the crawler marks a lane and runs it; the leech's and
// the wisp's blows drain; the ash walker's scorches and weakens, and the antidote ends the scorch.
class FFoeVarietyReplay final : public IAutomationLatentCommand
{
public:
    explicit FFoeVarietyReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FFoeVarietyReplay() override { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 240) { Test->AddError(FString::Printf(TEXT("Foe variety timeout in phase %d"), Phase)); return true; }
        UWorld* World = GEditor->PlayWorld;
        auto* PC = World ? Cast<AMemoriaSliceController>(World->GetFirstPlayerController()) : nullptr;
        auto* Pawn = PC ? Cast<AMemoriaFieldPawn>(PC->GetPawn()) : nullptr;
        if (!Pawn || World->GetTimeSeconds() < .3) return false;
        auto* Host = World->GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
        auto* Run = World->GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>();
        if (!bFixed)
        {
            bFixed = true; bOldFixed = FApp::UseFixedTimeStep(); OldDelta = FApp::GetFixedDeltaTime();
            FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1.0 / 60.0);
            for (int32 I = 0; I < 20 && Host->GetState() == EMemoriaSliceState::VN; ++I) Host->Confirm(1);
            return false;
        }
        if (!World->GetMapName().EndsWith(TEXT("L_VerdanHost")) || World->GetTimeSeconds() == LastWorldTime) return false;
        LastWorldTime = World->GetTimeSeconds(); ++Frame;
        auto* Combat = World->GetSubsystem<UMemoriaFieldCombatSubsystem>();
        if (!Combat || !Combat->GetPlayer() || !Run || Host->GetState() != EMemoriaSliceState::Exploration) return false;
        auto Capture = [&](const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/FoeVariety") / (FString(Name) + TEXT(".png")), true, false); };
        auto Spawn = [&](EMemoriaFoeKind Kind, const FVector& Offset) { return Combat->SpawnWave(1, Pawn->GetActorLocation() + Offset, 0.f, Kind)[0]; };
        auto Clear = [&] { for (const auto& M : Combat->GetMonsters()) if (M.IsValid()) M->Destroy(); };
        using namespace MemoriaCombatTuning;
        const FVector Me = Pawn->GetActorLocation();
        AMemoriaFieldMonster* F = Foe.Get();
        const float Away = F ? FVector::Dist2D(F->GetActorLocation(), Me) : 0.f;
        switch (Phase)
        {
        case 0:
        {
            for (TActorIterator<AMemoriaEliaCompanion> It(World); It; ++It) It->SetActorHiddenInGame(true);
            Home = Me;
            // The pools' entries, as the maps' IR carries them, find their own foes with the source's numbers.
            int32 Found = 0;
            for (const TCHAR* Map : {TEXT("belt_waystation"), TEXT("drift_shelter")})
                for (const auto& E : MemoriaChapterMaps::Find(Map)->Encounters)
                {
                    const EMemoriaFoeKind Kind = FoeKindByName(E.Name, E.bVoid);
                    const FMemoriaFoeSpec& S = FoeSpec(Kind);
                    Test->TestTrue(*FString::Printf(TEXT("%s is a foe of its own"), *E.Name), E.Name == S.Name && Kind != EMemoriaFoeKind::VoidHusk && Kind != EMemoriaFoeKind::MarketThief);
                    Test->TestTrue(*FString::Printf(TEXT("%s keeps the source's HP, attack and kind"), *E.Name), S.SourceHp == E.Hp && S.SourceAtk == E.Atk && S.bVoid == E.bVoid);
                    Test->TestTrue(*FString::Printf(TEXT("%s is scaled like the thief"), *E.Name), S.Health <= E.Hp && S.Health >= E.Hp * .85f && S.Damage <= E.Atk * .75f && S.Damage >= E.Atk * .5f);
                    ++Found;
                }
            Test->TestEqual(TEXT("Six pool entries"), Found, 6);
            Test->TestTrue(TEXT("An unknown name falls back by its kind"), FoeKindByName(TEXT("Nobody"), true) == EMemoriaFoeKind::VoidHusk && FoeKindByName(TEXT("Nobody"), false) == EMemoriaFoeKind::MarketThief);
            Test->TestTrue(TEXT("How each fights"), FoeSpec(EMemoriaFoeKind::VoidWisp).Behaviour == EMemoriaFoeBehaviour::Caster && FoeSpec(EMemoriaFoeKind::DustCrawler).Behaviour == EMemoriaFoeBehaviour::Charger
                && FoeSpec(EMemoriaFoeKind::RubbleRat).Behaviour == EMemoriaFoeBehaviour::Charger && FoeSpec(EMemoriaFoeKind::MemoryLeech).Behaviour == EMemoriaFoeBehaviour::Brawler);
            Test->TestTrue(TEXT("What their blows carry"), FoeSpec(EMemoriaFoeKind::VoidWisp).Ability == EMemoriaFoeAbility::Drain && FoeSpec(EMemoriaFoeKind::MemoryLeech).Ability == EMemoriaFoeAbility::Drain
                && FoeSpec(EMemoriaFoeKind::AshWalker).Ability == EMemoriaFoeAbility::Burn && FoeSpec(EMemoriaFoeKind::AshWalker).Second == EMemoriaFoeAbility::Weaken
                && FoeSpec(EMemoriaFoeKind::BeltScavenger).Ability == EMemoriaFoeAbility::Weaken && FoeSpec(EMemoriaFoeKind::DustCrawler).Ability == EMemoriaFoeAbility::Poison);
            Test->TestTrue(TEXT("The packs"), FoePackSize(EMemoriaFoeKind::DustCrawler) == 3 && FoePackSize(EMemoriaFoeKind::VoidWisp) == 2 && FoePackSize(EMemoriaFoeKind::VoidHusk) == 3 && FoePackSize(EMemoriaFoeKind::MarketThief) == 2);
            // One of each, in a row, to be read.
            const EMemoriaFoeKind Kinds[6] = {EMemoriaFoeKind::BeltScavenger, EMemoriaFoeKind::VoidWisp, EMemoriaFoeKind::DustCrawler, EMemoriaFoeKind::MemoryLeech, EMemoriaFoeKind::RubbleRat, EMemoriaFoeKind::AshWalker};
            for (int32 I = 0; I < 6; ++I) Spawn(Kinds[I], FVector(-375.f + 150.f * I, 150.f, 0))->Stun(30.f);
            Phase = 1; Mark = Frame; break;
        }
        case 1:
            if (Frame == Mark + 200)
            {
                TArray<FLinearColor> LiveTints;
                TArray<float> LiveWear;
                for (const auto& M : Combat->GetMonsters())
                {
                    if (!M.IsValid()) continue;
                    const auto& S = M->Spec();
                    Test->TestTrue(*FString::Printf(TEXT("%s stands at its own height"), S.Name), FMath::IsNearlyEqual(M->GetFigure()->GetWorldHeight(), S.Height));
                    auto* Mesh = M->GetFigure()->GetSkeletalMesh();
                    if (!Test->TestNotNull(TEXT("Variant wears its rigged body"), Mesh)) continue;
                    Test->TestTrue(TEXT("Variant body has a material slot"), Mesh->GetNumMaterials() > 0);
                    for (int32 I = 0; I < Mesh->GetNumMaterials(); ++I)
                    {
                        auto* Material = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(I));
                        if (!Test->TestNotNull(TEXT("Variant body has its own runtime material"), Material)) continue;
                        FLinearColor Tint, Glow, Rim;
                        float Wear = -1.f, Crack = -1.f, RimStrength = -1.f;
                        const bool bTint = Material->GetVectorParameterValue(FMaterialParameterInfo(TEXT("SurfaceTint")), Tint, true);
                        const bool bWear = Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Weathering")), Wear, true);
                        Test->TestTrue(*FString::Printf(TEXT("%s body %d receives its surface tint at runtime"), S.Name, I), bTint && Tint.Equals(S.SurfaceTint, .001f));
                        Test->TestTrue(*FString::Printf(TEXT("%s body %d receives its weathering at runtime"), S.Name, I), bWear && FMath::IsNearlyEqual(Wear, S.Weathering));
                        if(FString(S.Model)==TEXT("Husk"))
                            Test->TestTrue(TEXT("Husk surface variation preserves its existing crack glow"),
                                Material->Parent->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Glow")), Glow) &&
                                Material->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Glow")), Glow, true) && Glow.Equals(S.ModelGlow, .001f) &&
                                Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("CrackStrength")), Crack, true) && FMath::IsNearlyEqual(Crack, S.ModelCrack));
                        else
                            Test->TestTrue(TEXT("Thief based surfaces keep their non glowing parent"),
                                !Material->Parent->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Glow")), Glow) &&
                                !Material->Parent->GetScalarParameterValue(FMaterialParameterInfo(TEXT("CrackStrength")), Crack));
                        Test->TestTrue(TEXT("Surface variation preserves the existing rim and projectile colour"),
                            Material->GetVectorParameterValue(FMaterialParameterInfo(TEXT("RimColor")), Rim, true) && Rim.Equals(S.ModelRim, .001f) &&
                            Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("RimStrength")), RimStrength, true) && FMath::IsNearlyEqual(RimStrength, S.ModelRimStrength));
                        if (I == 0 && bTint && bWear) { LiveTints.Add(Tint); LiveWear.Add(Wear); }
                    }
                    M->GetFigure()->SetHitFlash(.65f);
                    for (int32 I = 0; I < Mesh->GetNumMaterials(); ++I)
                    {
                        auto* Material = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(I));
                        float Hit = -1.f, Wear = -1.f;
                        FLinearColor Tint;
                        Test->TestTrue(TEXT("Each variant retains a dynamic body hit flash"), Material &&
                            Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Hit")), Hit, true) && FMath::IsNearlyEqual(Hit, .65f));
                        Test->TestTrue(TEXT("A hit leaves the variant's surface controls independent"), Material &&
                            Material->GetVectorParameterValue(FMaterialParameterInfo(TEXT("SurfaceTint")), Tint, true) && Tint.Equals(S.SurfaceTint, .001f) &&
                            Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Weathering")), Wear, true) && FMath::IsNearlyEqual(Wear, S.Weathering));
                    }
                    M->GetFigure()->SetHitFlash(0.f);
                }
                Test->TestEqual(TEXT("All six live variants provide surface profiles"), LiveTints.Num(), 6);
                for (int32 I = 0; I < LiveTints.Num(); ++I)
                    for (int32 J = I + 1; J < LiveTints.Num(); ++J)
                    {
                        Test->TestFalse(TEXT("Different kinds retain independent non-emissive tints"), LiveTints[I].Equals(LiveTints[J], .001f));
                        Test->TestFalse(TEXT("Different kinds retain independent weathering strengths"), FMath::IsNearlyEqual(LiveWear[I], LiveWear[J]));
                    }
                Capture(TEXT("FoeLineup"));
            }
            // S353: retain the original comparison shot, then also show the same live lineup after
            // the real tutorial/achievement queues have remained quiet for half a second. No UI is hidden.
            if (Frame >= Mark + 360 && CleanLineupAt == 0)
            {
                const auto* Tutorial = World->GetGameInstance()->GetSubsystem<UMemoriaTutorialSubsystem>();
                const auto* Achievements = World->GetGameInstance()->GetSubsystem<UMemoriaAchievementSubsystem>();
                if (!Tutorial || !Achievements) { Test->AddError(TEXT("Lineup overlay subsystems are missing")); return true; }
                QuietLineupFrames = !Tutorial->IsShowing() && Achievements->GetPopup().IsEmpty() ? QuietLineupFrames + 1 : 0;
                if (QuietLineupFrames >= 30)
                {
                    Test->TestEqual(TEXT("Six live foes remain for the unobscured lineup"), Combat->LiveMonsterCount(), 6);
                    Capture(TEXT("FoeLineupClean")); CleanLineupAt = Frame;
                    Test->AddInfo(FString::Printf(TEXT("Clean foe lineup captured after %d frames with tutorial and achievement overlays expired"), Frame - Mark));
                }
                if (Frame > Mark + 1200) { Test->AddError(TEXT("Lineup overlays did not expire while the foes were held")); return true; }
            }
            if (CleanLineupAt == 0 || Frame < CleanLineupAt + 6) break;
            Clear();
            // The wisp: a caster.
            Foe = Spawn(EMemoriaFoeKind::VoidWisp, FVector(700, 0, 0));
            Hp = Combat->GetPlayerHp(); Orbs = Combat->GetOrbsLaunched(); Closest = 1e9f;
            Phase = 2; Mark = Frame; break;
        case 2:
            if (!F) break;
            Closest = FMath::Min(Closest, Away);
            if (!bCasterCue && F->GetAttackCue().bActive && F->GetAttackCue().Progress >= .35f)
            {
                const FMemoriaAttackCue Cue = F->GetAttackCue();
                Test->TestTrue(TEXT("Caster cue is a directed guide with the orb's actual hit radius"), Cue.Behaviour == EMemoriaFoeBehaviour::Caster
                    && FMath::IsNearlyEqual(Cue.Radius, OrbRadius) && FMath::IsNearlyEqual(Cue.Length, 220.f));
                Test->TestTrue(TEXT("Caster cue follows its live aim and advances during windup"), FVector::DotProduct(Cue.Direction, (Me - F->GetActorLocation()).GetSafeNormal2D()) > .99f
                    && Cue.Progress >= .35f && Cue.Progress < 1.f && Cue.Origin.Equals(F->GetActorLocation() - FVector(0, 0, 8.5f), .01f));
                bCasterCue = true; Capture(TEXT("FoeCasterCue"));
            }
            if (Combat->GetOrbsLaunched() > Orbs && !bOrbShot && Combat->GetOrbs().Num() > 0 && Combat->GetOrbs()[0].Age > .25f) { bOrbShot = true; Capture(TEXT("FoeWispOrb")); }
            if (Combat->GetPlayerHp() < Hp)
            {
                Test->TestTrue(TEXT("The wisp cast from its range and never closed"), Combat->GetOrbsLaunched() == Orbs + 1 && Closest >= CasterRetreat && Closest <= CasterRange + 5.f);
                Test->TestEqual(TEXT("Its orb did the wisp's harm"), Combat->GetPlayerHp(), Hp - int64(F->Spec().Damage));
                Test->TestEqual(TEXT("No orb is left"), Combat->GetOrbs().Num(), 0);
                Test->TestTrue(TEXT("Caster cue clears when its shot has launched"), bCasterCue && !F->GetAttackCue().bActive);
                Hp = Combat->GetPlayerHp(); Orbs = Combat->GetOrbsLaunched();
                Phase = 3; Mark = Frame;
            }
            if (Frame > Mark + 900) { Test->AddError(TEXT("The wisp's orb never reached Arrel")); return true; }
            break;
        case 3:
            // The next orb is stepped out of: Arrel moves aside once it is in the air, and the wisp is held.
            if (!F) break;
            if (Combat->GetOrbsLaunched() > Orbs)
            {
                F->Stun(30.f);
                Pawn->SetActorLocation(Me + FVector(0, 380, 0));
                Hp = Combat->GetPlayerHp();
                Phase = 4; Mark = Frame;
            }
            if (Frame > Mark + 600) { Test->AddError(TEXT("The wisp never cast again")); return true; }
            break;
        case 4:
            if (Frame < Mark + int32(OrbLife * 60.f) + 10 || !F) break;
            Test->TestTrue(TEXT("An orb can be stepped out of"), Combat->GetPlayerHp() == Hp && Combat->GetOrbs().Num() == 0);
            // A wounded wisp's blow gives it back half the harm it does.
            F->TakeHit(20.f, Me, 0.f);
            Health = F->GetHealth(); Hp = Combat->GetPlayerHp();
            Test->TestTrue(TEXT("Its blow lands from range"), Combat->StrikePlayer(F, F->Spec().Damage, true));
            Test->TestTrue(TEXT("And drains: half the harm comes back to it"), FMath::IsNearlyEqual(F->GetHealth(), Health + F->Spec().Damage * DrainShare) && Combat->GetPlayerHp() == Hp - int64(F->Spec().Damage));
            Clear(); Pawn->SetActorLocation(Home);
            Phase = 5; Mark = Frame; break;
        case 5:
            if (Frame < Mark + 40) break;
            // The crawler: a charger.
            Foe = Spawn(EMemoriaFoeKind::DustCrawler, FVector(-640, 0, 0));
            Taken = Combat->GetStrikesTaken();
            Phase = 6; Mark = Frame; break;
        case 6:
            if (!F) break;
            if (F->GetState() == EMemoriaMonsterState::Windup && !bLaneShot)
            {
                bLaneShot = true; WindupAt = Frame;
                Test->TestTrue(TEXT("It commits to its lane within reach of a rush"), Away <= RushFrom + 5.f && Away > RushHit);
                Test->TestTrue(TEXT("The lane points at Arrel"), FVector::DotProduct(F->GetRushDirection(), (Me - F->GetActorLocation()).GetSafeNormal2D()) > .99f);
                const FMemoriaAttackCue Cue = F->GetAttackCue();
                Test->TestTrue(TEXT("Charge cue covers its full committed rush and hit radius"), Cue.bActive && Cue.Behaviour == EMemoriaFoeBehaviour::Charger
                    && FMath::IsNearlyEqual(Cue.Length, RushDistance) && FMath::IsNearlyEqual(Cue.Radius, RushHit) && Cue.Direction.Equals(F->GetRushDirection(), .001f));
            }
            if (bLaneShot && Frame == WindupAt + 20) Capture(TEXT("FoeCrawlerLane"));
            if (bLaneShot && Frame == WindupAt + 22)
            {
                Test->TestTrue(TEXT("Charge cue counts down without rotating its committed lane"), F->GetAttackCue().bActive
                    && F->GetAttackCue().Progress > .3f && F->GetAttackCue().Progress < 1.f && F->GetAttackCue().Direction.Equals(F->GetRushDirection(), .001f));
                Capture(TEXT("FoeChargerCue"));
            }
            if (F->GetState() == EMemoriaMonsterState::Rush && !bRushShot && Away < 200.f) { bRushShot = true; Capture(TEXT("FoeCrawlerRush")); }
            if (bLaneShot && F->GetState() == EMemoriaMonsterState::Recover)
            {
                Test->TestTrue(TEXT("Charge cue clears during recovery"), !F->GetAttackCue().bActive);
                Test->TestEqual(TEXT("The rush struck Arrel once"), Combat->GetStrikesTaken(), Taken + 1);
                Test->TestTrue(TEXT("And poisoned him"), Combat->IsPoisoned());
                Test->TestTrue(TEXT("It ran on past him"), FVector::DotProduct(F->GetActorLocation() - Me, F->GetRushDirection()) > 0.f);
                Clear();
                Phase = 7; Mark = Frame;
            }
            if (Frame > Mark + 900) { Test->AddError(TEXT("The crawler never finished its rush")); return true; }
            break;
        case 7:
            if (Frame < Mark + 20) break;
            // The ash walker's blow scorches and weakens; the antidote ends the scorch and the poison.
            Foe = Spawn(EMemoriaFoeKind::AshWalker, FVector(110, 0, 0));
            Phase = 8; Mark = Frame; break;
        case 8:
            if (Frame < Mark + 3 || !F) break;
            if (Frame == Mark + 3) F->Stun(60.f);
            if (Frame < Mark + 8) break;
            Test->TestTrue(TEXT("The walker's blow lands"), Combat->StrikePlayer(F, F->Spec().Damage));
            Test->TestTrue(TEXT("It scorches and weakens"), Combat->IsScorched() && Combat->GetScorchTicksLeft() == ScorchTicks && Combat->IsWeakened());
            Hp = Combat->GetPlayerHp();
            while (Run->ConsumeItem(TEXT("antidote"))) {}
            Run->GrantFieldItem(TEXT("antidote"), 1);
            Phase = 9; Mark = Frame; break;
        case 9:
            if (Frame == Mark + 20) Capture(TEXT("FoeWalkerScorch"));
            if (Combat->GetScorchTicksLeft() == ScorchTicks - 1 && !bTicked)
            {
                bTicked = true;
                Test->TestTrue(TEXT("The scorch bites"), Combat->GetPlayerHp() < Hp);
                Test->TestTrue(TEXT("The antidote is used"), Combat->UseQuickItem(1, Me));
                Test->TestTrue(TEXT("It ends the scorch and the poison"), !Combat->IsScorched() && !Combat->IsPoisoned());
                Phase = 10; Mark = Frame;
            }
            if (Frame > Mark + 300) { Test->AddError(TEXT("The scorch never ticked")); return true; }
            break;
        case 10:
        {
            if (Frame < Mark + 40 || !F) break;
            // Unlock through the public memory domain, then cure the newly added status with Anchor Pulse.
            FMemoriaMemoryDefinition Hands; Hands.Id = TEXT("daily_elia_hands"); Hands.BurnPower = 1;
            auto* Memory = Run->GetPlayerMemory();
            Test->TestTrue(TEXT("The hands memory is added for the cure regression"), Memory->Add(Hands, Run->GetMemoryContext()) == EMemoriaMemoryResult::Success);
            Test->TestTrue(TEXT("The hands memory burns to unlock Anchor Pulse"), Memory->Burn(Hands.Id, EMemoriaBurnMode::Normal, false, Run->GetMemoryContext()) == EMemoriaMemoryResult::Success);
            Test->TestTrue(TEXT("A fresh walker blow scorches before Anchor Pulse"), Combat->StrikePlayer(F, F->Spec().Damage) && Combat->IsScorched());
            Test->TestTrue(TEXT("Anchor Pulse is used"), Combat->UseEliaSkill(3));
            Test->TestTrue(TEXT("Anchor Pulse cures scorch, poison and weaken"), !Combat->IsScorched() && !Combat->IsPoisoned() && !Combat->IsWeakened());
            Phase = 11; Mark = Frame; break;
        }
        case 11:
            if (Frame < Mark + 40 || !F) break;
            Test->TestTrue(TEXT("The last foe scorches Arrel before victory"), Combat->StrikePlayer(F, F->Spec().Damage) && Combat->IsScorched());
            F->TakeHit(999.f, Me, 0.f);
            Test->TestTrue(TEXT("Victory clears every harmful status"), !Combat->IsScorched() && !Combat->IsPoisoned() && !Combat->IsWeakened());
            Hp = Combat->GetPlayerHp(); Phase = 12; Mark = Frame; break;
        case 12:
        {
            if (Frame < Mark + FMath::CeilToInt(PoisonInterval * 60.f) + 30) break;
            Test->TestEqual(TEXT("No scorch damage follows victory"), Combat->GetPlayerHp(), Hp);
            Clear();
            // A long frame with a pawn in front of a wall must still sweep against the wall.
            const FVector TestCentre = Home + FVector(0, 5000, 0);
            Pawn->SetActorLocation(TestCentre);
            F = Spawn(EMemoriaFoeKind::DustCrawler, FVector(-150, 0, 0));
            auto* Collision = Cast<UBoxComponent>(F->GetRootComponent());
            Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
            F->Tick(.01f); F->Tick(.01f); F->Tick(F->Spec().Windup + .01f);
            Test->TestTrue(TEXT("Collision regression enters the real rush"), F->GetState() == EMemoriaMonsterState::Rush);
            auto* Wall = World->SpawnActor<AActor>();
            auto* Box = NewObject<UBoxComponent>(Wall); Wall->SetRootComponent(Box); Wall->AddInstanceComponent(Box);
            Box->SetBoxExtent(FVector(40, 160, 80)); Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            Box->SetCollisionObjectType(ECC_WorldStatic); Box->SetCollisionResponseToAllChannels(ECR_Block); Box->RegisterComponent();
            Wall->SetActorLocation(TestCentre + FVector(60, 0, 0));
            F->Tick(.5f);
            Test->TestTrue(TEXT("A pawn does not let the rush cross the wall behind it"), F->GetActorLocation().X + 30.f <= TestCentre.X + 20.f + .1f);
            Test->TestEqual(TEXT("Rush restores its pawn collision response"), Collision->GetCollisionResponseToChannel(ECC_Pawn), ECR_Block);
            Wall->Destroy(); Clear();
            // Proximity must not strike through a wall that stops the actual rush segment.
            F = Spawn(EMemoriaFoeKind::DustCrawler, FVector(-60, 0, 0));
            F->Tick(.01f); F->Tick(.01f);
            Test->TestTrue(TEXT("Thin-wall regression enters the real windup before the wall is placed"), F->GetState() == EMemoriaMonsterState::Windup);
            Wall = World->SpawnActor<AActor>();
            Box = NewObject<UBoxComponent>(Wall); Wall->SetRootComponent(Box); Wall->AddInstanceComponent(Box);
            Box->SetBoxExtent(FVector(5, 160, 80)); Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            Box->SetCollisionObjectType(ECC_WorldStatic); Box->SetCollisionResponseToAllChannels(ECR_Block); Box->RegisterComponent();
            Wall->SetActorLocation(TestCentre + FVector(-20, 0, 0));
            const FMemoriaAttackCue BlockedCue = F->GetAttackCue();
            const FVector CueEnd = BlockedCue.Origin + BlockedCue.Direction * BlockedCue.Length;
            Test->TestTrue(TEXT("A charge warning detects the wall during windup and stops at its body clearance"), BlockedCue.bActive && BlockedCue.bBlocked
                && BlockedCue.Length < RushDistance && CueEnd.X + 30.f <= TestCentre.X - 25.f + .1f);
            F->Tick(F->Spec().Windup + .01f);
            Test->TestTrue(TEXT("Thin-wall regression releases the real rush after its blocked warning"), F->GetState() == EMemoriaMonsterState::Rush);
            Hp = Combat->GetPlayerHp(); Taken = Combat->GetStrikesTaken();
            F->Tick(.5f);
            Test->TestTrue(TEXT("The thin wall stops the rush before Arrel"), F->GetActorLocation().X + 30.f <= TestCentre.X - 25.f + .1f);
            Test->TestEqual(TEXT("The blocked rush does not harm Arrel through the thin wall"), Combat->GetPlayerHp(), Hp);
            Test->TestEqual(TEXT("The blocked rush does not count a strike through the thin wall"), Combat->GetStrikesTaken(), Taken);
            Wall->Destroy(); Clear();
            Phase = 13; Mark = Frame; break;
        }
        case 13:
        {
            if (Frame < Mark + 40) break;
            // The same long frame in an open lane crosses Arrel even though both endpoints are outside reach.
            F = Spawn(EMemoriaFoeKind::DustCrawler, FVector(-150, 0, 0));
            F->Tick(.01f); F->Tick(.01f); F->Tick(F->Spec().Windup + .01f);
            Test->TestTrue(TEXT("Overshoot control enters the real rush"), F->GetState() == EMemoriaMonsterState::Rush);
            Hp = Combat->GetPlayerHp(); Taken = Combat->GetStrikesTaken();
            F->Tick(.5f);
            Test->TestTrue(TEXT("The open-lane long frame runs beyond Arrel"), FVector::DotProduct(F->GetActorLocation() - Me, F->GetRushDirection()) > RushHit);
            Test->TestEqual(TEXT("The traversed open lane strikes Arrel once during an overshoot"), Combat->GetStrikesTaken(), Taken + 1);
            Test->TestEqual(TEXT("The traversed open lane deals the crawler's harm during an overshoot"), Combat->GetPlayerHp(), Hp - int64(F->Spec().Damage));
            Clear();
            F = Spawn(EMemoriaFoeKind::DustCrawler, FVector(-150, 0, 0));
            F->Tick(.01f); F->Tick(.01f); F->Tick(F->Spec().Windup + .01f);
            const FVector RushStart = F->GetActorLocation();
            Hp = Combat->GetPlayerHp(); Taken = Combat->GetStrikesTaken();
            F->Tick(.7f); F->Tick(.5f);
            Test->TestTrue(TEXT("A long final rush frame stops at the advertised 520 cm and enters recovery"),
                FMath::IsNearlyEqual(FVector::Dist2D(F->GetActorLocation(), RushStart), RushDistance, .1f) && F->GetState() == EMemoriaMonsterState::Recover);
            Test->TestTrue(TEXT("The lifetime-clipped rush still sweeps Arrel exactly once"), Combat->GetStrikesTaken() == Taken + 1
                && Combat->GetPlayerHp() == Hp - int64(F->Spec().Damage));
            Clear(); Pawn->SetActorLocation(Home);
            Phase = 14; Mark = Frame; break;
        }
        case 14:
            if (Frame < Mark + 40) break;
            Foe = Spawn(EMemoriaFoeKind::AshWalker, FVector(110, 0, 0));
            Test->TestTrue(TEXT("Melee cue is absent before a windup"), !Foe->GetAttackCue().bActive);
            Phase = 15; Mark = Frame; break;
        case 15:
            if (!F) break;
            if (!bMeleeCue && F->GetAttackCue().bActive && F->GetAttackCue().Progress >= .35f)
            {
                const FMemoriaAttackCue Cue = F->GetAttackCue();
                Test->TestTrue(TEXT("Melee cue covers the same radius as StrikePlayer"), Cue.Behaviour == EMemoriaFoeBehaviour::Brawler
                    && FMath::IsNearlyEqual(Cue.Radius, F->Spec().Reach + 40.f) && FMath::IsNearlyZero(Cue.Length));
                Test->TestTrue(TEXT("Melee cue is a ground warning with a live windup clock"), Cue.Progress >= .35f && Cue.Progress < 1.f
                    && Cue.Origin.Equals(F->GetActorLocation() - FVector(0, 0, 8.5f), .01f));
                bMeleeCue = true; Capture(TEXT("FoeMeleeCue")); WindupAt = Frame;
            }
            if (bMeleeCue && Frame >= WindupAt + 3)
            {
                F->Stun(30.f);
                Test->TestTrue(TEXT("Interrupting a melee windup clears its warning immediately"), !F->GetAttackCue().bActive);
                Clear(); Phase = 16; Mark = Frame;
            }
            if (Frame > Mark + 300) { Test->AddError(TEXT("The melee attack never showed its cue")); return true; }
            break;
        case 16:
        {
            if (Frame < Mark + 10) break;
            // Direct public launch/tick calls isolate collision from AI timing. The remote lane
            // contains only these fixtures, at the same height as a real wisp's projectile.
            const FVector Centre = Home + FVector(0, 5000, 0);
            const FVector OrbHeight(0, 0, FoeSpec(EMemoriaFoeKind::VoidWisp).Height * .6f);
            auto Reset = [&]
            {
                Combat->Revive(1.f); Pawn->SetActorLocation(Centre);
                AMemoriaFieldMonster* Owner = Spawn(EMemoriaFoeKind::VoidWisp, FVector(-800, 0, 0));
                Owner->Stun(60.f); return Owner;
            };
            auto WallAt = [&](float X)
            {
                AActor* Wall = World->SpawnActor<AActor>();
                auto* Box = NewObject<UBoxComponent>(Wall); Wall->SetRootComponent(Box); Wall->AddInstanceComponent(Box);
                Box->SetBoxExtent(FVector(5, 160, 180)); Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
                Box->SetCollisionObjectType(ECC_WorldStatic); Box->SetCollisionResponseToAllChannels(ECR_Block); Box->RegisterComponent();
                Wall->SetActorLocation(Centre + FVector(X, 0, 90)); return Wall;
            };
            auto Shot = [&](AMemoriaFieldMonster* Owner, float Distance)
            { Combat->LaunchOrb(Owner, Centre + FVector(-Distance, 0, 0) + OrbHeight, FVector::ForwardVector); };
            auto Snapshot = [&] { Hp = Combat->GetPlayerHp(); Taken = Combat->GetStrikesTaken(); };
            auto Unharmed = [&](const TCHAR* Message)
            { Test->TestTrue(Message, Combat->GetPlayerHp() == Hp && Combat->GetStrikesTaken() == Taken); };
            auto Struck = [&](const TCHAR* Message, AMemoriaFieldMonster* Owner)
            { Test->TestTrue(Message, Combat->GetPlayerHp() == Hp - int64(Owner->Spec().Damage) && Combat->GetStrikesTaken() == Taken + 1); };

            AMemoriaFieldMonster* Owner = Reset();
            AActor* Wall = WallAt(-80.f);
            FCollisionQueryParams Query(SCENE_QUERY_STAT(MemoriaOrbFixture), false, Owner); Query.AddIgnoredActor(Pawn);
            FHitResult WallHit;
            Test->TestTrue(TEXT("Orb cover fixture actually blocks visibility at projectile height"), World->LineTraceSingleByChannel(WallHit,
                Centre + FVector(-200, 0, 0) + OrbHeight, Centre + OrbHeight, ECC_Visibility, Query) && WallHit.GetActor() == Wall);
            Shot(Owner, 200.f); Snapshot(); Combat->Tick(.9f);
            Unharmed(TEXT("Cover before Arrel absorbs a crossing orb during a long frame"));
            Test->TestEqual(TEXT("Cover consumes the orb"), Combat->GetOrbs().Num(), 0); Wall->Destroy();

            Owner = Reset(); Wall = WallAt(110.f);
            Shot(Owner, 200.f); Snapshot(); Combat->Tick(.9f);
            Struck(TEXT("Arrel before cover receives the first contact instead of the later wall"), Owner);
            Test->TestEqual(TEXT("Player contact consumes the orb once"), Combat->GetOrbs().Num(), 0); Wall->Destroy();

            Owner = Reset();
            auto* Decoy = Spawn(EMemoriaFoeKind::MarketThief, FVector(-100, 0, 0)); Decoy->Stun(60.f);
            CastChecked<UBoxComponent>(Decoy->GetRootComponent())->SetBoxExtent(FVector(30, 160, 180));
            Shot(Owner, 200.f); Snapshot(); Combat->Tick(.9f);
            Struck(TEXT("An open-lane orb crosses Arrel once and ignores other pawn bodies"), Owner);
            Test->TestEqual(TEXT("Long-frame crossing leaves no orb beyond Arrel"), Combat->GetOrbs().Num(), 0);

            Owner = Reset();
            Test->TestTrue(TEXT("The crossing regression starts a real dodge"), Combat->RequestDash(FVector::RightVector));
            Shot(Owner, 200.f); Snapshot(); Combat->Tick(.9f);
            Unharmed(TEXT("A crossing orb preserves dodge immunity"));
            Test->TestTrue(TEXT("A dodged orb continues through the old player position"), Combat->GetOrbs().Num() == 1
                && Combat->GetOrbs()[0].Location.X > Centre.X + OrbRadius);
            Combat->Tick(OrbLife); Unharmed(TEXT("The dodged orb does not strike later from its old crossing"));
            Test->TestEqual(TEXT("The dodged orb eventually expires"), Combat->GetOrbs().Num(), 0);

            Owner = Reset();
            Shot(Owner, OrbSpeed * OrbLife + OrbRadius + 20.f); Snapshot(); Combat->Tick(OrbLife + .5f);
            Unharmed(TEXT("An expiring orb cannot travel beyond its remaining lifetime to hit Arrel"));
            Test->TestEqual(TEXT("An expired orb is removed"), Combat->GetOrbs().Num(), 0);
            Owner = Reset();
            Shot(Owner, OrbSpeed * (OrbLife - .1f) + OrbRadius); Snapshot(); Combat->Tick(OrbLife + .5f);
            Struck(TEXT("The live portion of an expiring long frame still resolves its earlier contact"), Owner);

            Owner = Reset(); Shot(Owner, 200.f); Owner->Destroy(); Snapshot(); Combat->Tick(.9f);
            Unharmed(TEXT("An orphaned orb cannot harm Arrel"));
            Test->TestEqual(TEXT("An orphaned orb fades immediately"), Combat->GetOrbs().Num(), 0);
            Owner = Reset(); Shot(Owner, 200.f); Owner->TakeHit(999.f, Centre, 0.f); Snapshot(); Combat->Tick(.9f);
            Unharmed(TEXT("A dead thrower's orb cannot harm Arrel"));
            Test->TestEqual(TEXT("A dead thrower's orb fades immediately"), Combat->GetOrbs().Num(), 0);
            Test->TestTrue(TEXT("A dead thrower has no attack cue"), !Owner->GetAttackCue().bActive);
            Combat->Revive(1.f); Pawn->SetActorLocation(Home);
            return true;
        }
        }
        return false;
    }
private:
    FAutomationTestBase* Test;
    double Started, OldDelta = 0, LastWorldTime = -1;
    uint64 LastFrame = MAX_uint64;
    int32 Phase = 0, Frame = 0, Mark = 0, Orbs = 0, Taken = 0, WindupAt = 0;
    int32 CleanLineupAt = 0, QuietLineupFrames = 0;
    int64 Hp = 0;
    float Closest = 0.f, Health = 0.f;
    FVector Home = FVector::ZeroVector;
    bool bFixed = false, bOldFixed = false, bOrbShot = false, bLaneShot = false, bRushShot = false, bTicked = false, bCasterCue = false, bMeleeCue = false;
    TWeakObjectPtr<AMemoriaFieldMonster> Foe;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoeVarietyTest, "MemoriaVisual.FoeVariety", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFoeVarietyTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FFoeVarietyReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
