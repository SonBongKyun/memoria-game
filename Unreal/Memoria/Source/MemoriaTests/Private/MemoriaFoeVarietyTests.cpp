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
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Animation/AnimSequence.h"
#include "Rendering/SkeletalMeshModel.h"
#include "Rendering/SkeletalMeshLODModel.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Rendering/SkeletalMeshLODRenderData.h"
#include "Rendering/SkinWeightVertexBuffer.h"
#include "SkeletalRenderPublic.h"
#include "Presentation/MemoriaCombatClips.h"
#include "Presentation/MemoriaFieldAnimInstance.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
// Inspect the live skinned vertices, not imported mesh bounds or bone presence: both can remain
// valid when an animation deforms the rendered body underground or outside the detail camera.
void CheckSkinnedBody(FAutomationTestBase* Test,USkeletalMeshComponent* Skin,AMemoriaFieldMonster* F,APlayerController* PC,const FString& Id,const TCHAR* Pose,bool bFallen=false)
{
    if (!Skin || !F || !PC) { Test->AddError(TEXT("S356 skin inspection requires the live actor and camera"));return; }
    TArray<FFinalSkinVertex> Vertices;Skin->GetCPUSkinnedVertices(Vertices,0);
    if (!Test->TestTrue(TEXT("S356 live deformation has CPU-skinned vertices"),Vertices.Num()>100))return;
    const FVector Actor=F->GetActorLocation();const double Height=F->Spec().Height;
    int32 Width=0,ScreenHeight=0;PC->GetViewportSize(Width,ScreenHeight);
    FBox Body(ForceInit);FBox2D Projection(ForceInit);int32 Near=0,Above=0,Visible=0,Finite=0;
    for (const auto& Vertex:Vertices)
    {
        const FVector Point=Skin->GetComponentTransform().TransformPosition(FVector(Vertex.Position));
        if (Point.ContainsNaN())continue;
        ++Finite;Body+=Point;
        if (FVector::Dist2D(Point,Actor)<=Height*1.65 && FMath::Abs(Point.Z-Actor.Z)<=Height*2.0)++Near;
        if (Point.Z>=Actor.Z-35.f)++Above;
        FVector2D Pixel;
        if (PC->ProjectWorldLocationToScreen(Point,Pixel))
        {
            Projection+=Pixel;
            if (Pixel.X>=0 && Pixel.X<Width && Pixel.Y>=0 && Pixel.Y<ScreenHeight)++Visible;
        }
    }
    const double Count=Vertices.Num();const FVector Size=Body.IsValid?Body.GetSize():FVector::ZeroVector;
    const FVector Center=Body.IsValid?Body.GetCenter()-Actor:FVector::ZeroVector;
    const FVector2D Pixels=Projection.bIsValid?Projection.GetSize():FVector2D::ZeroVector;
    const auto* Anim=Cast<UMemoriaFieldAnimInstance>(Skin->GetAnimInstance());
    const auto* Figure=F->GetFigure();
    Test->AddInfo(FString::Printf(TEXT("S356_SKIN_BODY %s pose=%s action=%s time=%.4f weight=%.4f vertices=%d finite=%d near=%.3f above=%.3f visible=%.3f center=%s size=%s projected=%s root_scale=%s head_scale=%s head=%s"),
        *Id,Pose,*GetNameSafe(Figure->GetActionClip()),Figure->GetActionTime(),Anim?Anim->ActionWeight:-1.f,Vertices.Num(),Finite,Near/Count,Above/Count,Visible/Count,*Center.ToString(),*Size.ToString(),*Pixels.ToString(),
        *Skin->GetSocketTransform(TEXT("root"),RTS_Component).GetScale3D().ToString(),*Skin->GetSocketTransform(TEXT("head"),RTS_Component).GetScale3D().ToString(),*(Skin->GetSocketLocation(TEXT("head"))-Actor).ToString()));
    Test->TestTrue(TEXT("S356 live body vertices remain finite, close to the encounter and above its ground"),Finite==Vertices.Num() && Near/Count>=.85 && Above/Count>=.85);
    // A held corpse must retain volume and remain on the ground, but is expected to be lying down.
    const bool PlausibleShape=bFallen?Center.Z>=-Height*.05 && Size.GetMax()>=Height*.35:Center.Z>=Height*.10 && Size.Z>=Height*.45;
    Test->TestTrue(TEXT("S356 live body has a plausible world center and extent while animated"),Body.IsValid && Center.Size2D()<=Height*.95 && Center.Z<=Height*1.25 && Size.Z<=Height*2.0 && PlausibleShape);
    const bool ReadableProjection=bFallen?Pixels.GetMax()>=FMath::Min(Width,ScreenHeight)*.06:Pixels.X>=Width*.015 && Pixels.Y>=ScreenHeight*.075;
    Test->TestTrue(TEXT("S356 animated body itself is visible with a readable projected extent"),Width>0 && ScreenHeight>0 && Visible/Count>=.75 && ReadableProjection);

    if (Id!=TEXT("AshWalker") && Id!=TEXT("BeltScavenger"))return;
    const auto* Render=Skin->GetSkeletalMeshRenderData();
    const auto* Weights=Skin->GetSkinWeightBuffer(0);
    if (!Test->TestTrue(TEXT("S356 retained head can be inspected against its actual render skin weights"),Render && Render->LODRenderData.Num()>0 && Weights && Weights->GetNumVertices()==uint32(Vertices.Num())))return;
    const auto& LOD=Render->LODRenderData[0];const auto& Colors=LOD.StaticVertexBuffers.ColorVertexBuffer;
    const bool HasColors=Colors.GetNumVertices()==uint32(Vertices.Num()) && Colors.GetAllowCPUAccess();
    if (!Test->TestTrue(TEXT("S356 head validation requires the retained alpha source in the editor CPU colour buffer"),HasColors))return;
    const auto& Ref=Skin->GetSkeletalMeshAsset()->GetRefSkeleton();const int32 HeadIndex=Ref.FindBoneIndex(TEXT("head"));
    if (!Test->TestTrue(TEXT("S356 humanoid still has its retained head bone"),HeadIndex!=INDEX_NONE))return;
    auto IsHeadBone=[&](int32 Bone)
    {
        for (int32 Current=Bone;Current!=INDEX_NONE;Current=Ref.GetParentIndex(Current))if (Current==HeadIndex)return true;
        return false;
    };
    FBox HeadBody(ForceInit);FBox2D HeadProjection(ForceInit);int32 Heads=0,HeadFinite=0,HeadAbove=0,HeadVisible=0,InvalidMap=0;
    for (const auto& Section:LOD.RenderSections)
    {
        const TConstArrayView<FBoneIndexType> BoneMap=Section.BoneMap.IsEmpty()?LOD.GetUnifiedBoneMap():TConstArrayView<FBoneIndexType>(Section.BoneMap);
        const uint32 End=FMath::Min(uint32(Vertices.Num()),Section.BaseVertexIndex+Section.NumVertices);
        for (uint32 V=Section.BaseVertexIndex;V<End;++V)
        {
            // Alpha zero marks the preserved original face/hair/skin, rather than a new hood or collar.
            if (Colors.VertexColor(V).A>8)continue;
            uint32 TotalWeight=0,HeadWeight=0;
            for (uint32 I=0;I<Weights->GetMaxBoneInfluences();++I)
            {
                const uint32 Weight=Weights->GetBoneWeight(V,I);if (!Weight)continue;
                const uint32 LocalBone=Weights->GetBoneIndex(V,I);TotalWeight+=Weight;
                if (LocalBone>=uint32(BoneMap.Num())) { ++InvalidMap;continue; }
                if (IsHeadBone(BoneMap[LocalBone]))HeadWeight+=Weight;
            }
            if (!TotalWeight || HeadWeight*2<TotalWeight)continue;
            ++Heads;const FVector Point=Skin->GetComponentTransform().TransformPosition(FVector(Vertices[V].Position));
            if (Point.ContainsNaN())continue;
            ++HeadFinite;HeadBody+=Point;if (Point.Z>=Actor.Z-35.f)++HeadAbove;
            FVector2D Pixel;if (PC->ProjectWorldLocationToScreen(Point,Pixel))
            {
                HeadProjection+=Pixel;
                if (Pixel.X>=0 && Pixel.X<Width && Pixel.Y>=0 && Pixel.Y<ScreenHeight)++HeadVisible;
            }
        }
    }
    const FVector HeadSize=HeadBody.IsValid?HeadBody.GetSize():FVector::ZeroVector;
    const FVector HeadCenter=HeadBody.IsValid?HeadBody.GetCenter():FVector::ZeroVector;
    const FVector LiveHead=Skin->GetSocketLocation(TEXT("head"));const double HeadCount=FMath::Max(1,Heads);
    Test->AddInfo(FString::Printf(TEXT("S356_SKIN_HEAD %s pose=%s retained_alpha=%d vertices=%d invalid_bone_map=%d finite=%d above=%.3f visible=%.3f center=%s size=%s distance_to_head=%.3f projected=%s"),
        *Id,Pose,HasColors,Heads,InvalidMap,HeadFinite,HeadAbove/HeadCount,HeadVisible/HeadCount,*(HeadCenter-Actor).ToString(),*HeadSize.ToString(),FVector::Dist(HeadCenter,LiveHead),*(HeadProjection.bIsValid?HeadProjection.GetSize():FVector2D::ZeroVector).ToString()));
    Test->TestTrue(TEXT("S356 retained actual head vertices remain present with valid bone maps and volume"),InvalidMap==0 && Heads>=30 && HeadFinite==Heads && HeadBody.IsValid && HeadSize.GetMax()>=Height*.06 && HeadSize.GetMin()>=Height*.02);
    Test->TestTrue(TEXT("S356 retained head surface follows the live head above ground and inside the detail camera"),HeadBody.IsValid && FVector::Dist(HeadCenter,LiveHead)<=Height*.22 && HeadAbove/HeadCount>=.90 && HeadVisible/HeadCount>=.70);
}
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
                TSet<FString> LiveBodies;
                for (const auto& M : Combat->GetMonsters())
                {
                    if (!M.IsValid()) continue;
                    const auto& S = M->Spec();
                    Test->TestTrue(*FString::Printf(TEXT("%s stands at its own height"), S.Name), FMath::IsNearlyEqual(M->GetFigure()->GetWorldHeight(), S.Height));
                    auto* Mesh = M->GetFigure()->GetSkeletalMesh();
                    if (!Test->TestNotNull(TEXT("Variant wears its rigged body"), Mesh)) continue;
                    Test->TestTrue(TEXT("Variant body has a material slot"), Mesh->GetNumMaterials() > 0);
                    const auto* Body = Mesh->GetSkeletalMeshAsset();
                    Test->TestTrue(TEXT("Each encounter uses its named authored silhouette"), Body && Body->GetName() == TEXT("SK_") + FString(S.Model));
                    if (Body) LiveBodies.Add(Body->GetPathName());
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
                        if(FString(S.Model)==TEXT("Husk") || FString(S.Model)==TEXT("AshWalker") || FString(S.Model)==TEXT("VoidWisp") || FString(S.Model)==TEXT("MemoryLeech"))
                            Test->TestTrue(TEXT("Void and charred surfaces retain their independently controlled glow"),
                                Material->Parent->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Glow")), Glow) &&
                                Material->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Glow")), Glow, true) && Glow.Equals(S.ModelGlow, .001f) &&
                                Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("CrackStrength")), Crack, true) && FMath::IsNearlyEqual(Crack, S.ModelCrack));
                        else
                            Test->TestTrue(TEXT("Physical creature and scavenger surfaces keep their non glowing parent"),
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
                Test->TestEqual(TEXT("All six encounters have distinct geometry, rather than shared human stand-ins"), LiveBodies.Num(), 6);
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
            Foe = Spawn(EMemoriaFoeKind::RubbleRat, FVector(-140, -220, 0)); Foe->Stun(30.f);
            Phase = 90; Mark = Frame; break;
        case 90:
        case 91:
        {
            if (!F) { Test->AddError(TEXT("Creature inspection lost its actor")); return true; }
            auto* Figure = F->GetFigure(); auto* Skin = Figure->GetSkeletalMesh();
            Figure->SetAim(0.f);
            const FString Id = Phase == 90 ? TEXT("RubbleRat") : TEXT("DustCrawler");
            if (Frame == Mark + 1)
            {
                Test->TestTrue(TEXT("Creature uses its dedicated anatomy, without a human dagger"), Figure->IsFoeModel()
                    && Figure->GetCharacterId() == Id && !Figure->GetBlade() && Skin && Skin->GetSkeletalMeshAsset()->GetName() == TEXT("SK_") + Id);
                if (!Skin) return true;
                Test->TestTrue(TEXT("Creature retains authored region vertex colours"),Skin->GetSkeletalMeshAsset()->GetHasVertexColors());
                TArray<FSoftSkinVertex> Vertices;
                auto* Imported = Skin->GetSkeletalMeshAsset()->GetImportedModel();
                if (Imported && Imported->LODModels.Num()) Imported->LODModels[0].GetVertices(Vertices);
                double MeanRed=0;
                for (const auto& Vertex:Vertices) MeanRed+=double(Vertex.Color.R)/255.0;
                MeanRed/=FMath::Max(1,Vertices.Num());
                Test->TestTrue(TEXT("FBX colours remain dark linear pigments, without an sRGB double lift"),Vertices.Num()>1000 && MeanRed>.02 && MeanRed<.30);
                Test->AddInfo(FString::Printf(TEXT("CREATURE_PIGMENT %s vertices=%d linear_mean_red=%.4f"),*Id,Vertices.Num(),MeanRed));
                auto* SurfaceMaterial=Cast<UMaterialInstanceDynamic>(Skin->GetMaterial(0));
                UTexture* DetailTexture=nullptr;
                Test->TestTrue(TEXT("Creature binds its real surface normal map"),SurfaceMaterial && SurfaceMaterial->GetTextureParameterValue(FMaterialParameterInfo(TEXT("DetailNormal")),DetailTexture) && DetailTexture);
                Test->TestTrue(TEXT("Creature binds the authored fur or chitin pigment atlas"),SurfaceMaterial && SurfaceMaterial->GetTextureParameterValue(FMaterialParameterInfo(TEXT("PigmentAtlas")),DetailTexture) && DetailTexture);

                Test->TestEqual(TEXT("Species skeleton matches its articulated limbs"), Skin->GetNumBones(), Phase == 90 ? 18 : 16);
                const auto* Anim = Cast<UMemoriaFieldAnimInstance>(Skin->GetAnimInstance());
                Test->TestTrue(TEXT("Actual locomotion uses species clips"), Anim && Anim->Idle == MemoriaCombatClips::Load(Id,TEXT("Idle")) && Anim->Walk == MemoriaCombatClips::Load(Id,TEXT("Walk")));
                for (const TCHAR* Clip : {TEXT("Idle"),TEXT("Walk"),TEXT("Bite"),MemoriaCombatClips::Hit(),MemoriaCombatClips::Death()})
                {
                    auto* Sequence = MemoriaCombatClips::Load(Id,Clip);
                    Test->TestTrue(TEXT("Creature clip has a compatible skeleton and timed frames"), Sequence && Sequence->GetSkeleton() == Skin->GetSkeletalMeshAsset()->GetSkeleton() && Sequence->GetPlayLength() > .2f);
                }
                Figure->Face(TEXT("Down"));
                if (!DetailCamera.IsValid()) DetailCamera = World->SpawnActor<ACameraActor>();
                const FVector Center = F->GetActorLocation() + FVector(0,0,25);
                const FVector Eye = Center + FVector(220,-240,155);
                DetailCamera->SetActorLocation(Eye); DetailCamera->SetActorRotation((Center-Eye).Rotation());
                DetailCamera->GetCameraComponent()->SetFieldOfView(48.f); PC->SetViewTarget(DetailCamera.Get());
                LegAt = Skin->GetSocketTransform(TEXT("leg0_l"),RTS_Component);
            }
            if (Frame > Mark + 2 && Frame < Mark + 25) Figure->AdvanceLocomotion(FVector(0,2,0),1.f/60.f);
            if (Frame == Mark + 20)
            {
                Test->TestFalse(TEXT("Walking visibly articulates a leg"), Skin->GetSocketTransform(TEXT("leg0_l"),RTS_Component).Equals(LegAt,.01f));
                const auto& Bind=Skin->GetSkeletalMeshAsset()->GetRefSkeleton().GetRefBonePose();
                const FTransform BodyBind=Bind[1]*Bind[0];
                const FTransform BodyNow=Skin->GetSocketTransform(TEXT("body"),RTS_Component);
                Test->TestTrue(TEXT("Living creature stays upright relative to its imported bind pose"),FVector::DotProduct((BodyNow.GetRotation()*BodyBind.GetRotation().Inverse()).RotateVector(FVector::UpVector),FVector::UpVector)>.98f && FMath::Abs(BodyBind.GetLocation().Z-BodyNow.GetLocation().Z)<5.f);
                Test->AddInfo(FString::Printf(TEXT("CREATURE_UPRIGHT %s upDot=%.4f bindZ=%.2f liveZ=%.2f"),*Id,FVector::DotProduct((BodyNow.GetRotation()*BodyBind.GetRotation().Inverse()).RotateVector(FVector::UpVector),FVector::UpVector),BodyBind.GetLocation().Z,BodyNow.GetLocation().Z));
                Capture(Phase == 90 ? TEXT("RubbleRatDetail") : TEXT("DustCrawlerDetail"));
            }
            if (Frame == Mark + 30) Test->TestTrue(TEXT("Creature plays its actual bite"),Figure->PlayAction(TEXT("Bite")));
            if (Frame == Mark + 48) Capture(Phase == 90 ? TEXT("RubbleRatBite") : TEXT("DustCrawlerBite"));
            if (Frame == Mark + 84) Test->TestTrue(TEXT("Creature plays a held death"),Figure->PlayAction(MemoriaCombatClips::Death(),1.f,true));
            if (Frame == Mark + 170)
            {
                Test->TestTrue(TEXT("Death remains held instead of returning to idle"),Figure->IsActing() && FMath::IsNearlyEqual(Figure->GetActionTime(),Figure->GetActionLength(),.02f));
                Capture(Phase == 90 ? TEXT("RubbleRatDeath") : TEXT("DustCrawlerDeath"));
            }
            if (Frame < Mark + 180) break;
            Clear();
            if (Phase == 90)
            { Foe = Spawn(EMemoriaFoeKind::DustCrawler,FVector(-140,-220,0)); Foe->Stun(30.f); Phase=91; Mark=Frame; break; }
            Foe = Spawn(EMemoriaFoeKind::VoidWisp,FVector(-140,-220,0)); Foe->Stun(30.f); Phase=92; Mark=Frame; break;
        }
        case 92:
        case 93:
        case 94:
        case 95:
        {
            if (!F) { Test->AddError(TEXT("S356 character inspection lost its actor")); return true; }
            auto* Figure=F->GetFigure();auto* Skin=Figure->GetSkeletalMesh();
            const TCHAR* Ids[]={TEXT("VoidWisp"),TEXT("MemoryLeech"),TEXT("AshWalker"),TEXT("BeltScavenger")};
            const FString Id=Ids[Phase-92];Figure->SetAim(0.f);
            const FName MovingBone=Phase==92?TEXT("trail0_tip"):Phase==93?TEXT("segment3"):TEXT("calf_l");
            if (Frame==Mark+1)
            {
                Test->TestTrue(TEXT("S356 encounter wears its new immutable revision body"), Skin && Figure->IsFoeModel() && Figure->GetCharacterId()==Id
                    && Skin->GetSkeletalMeshAsset()->GetPathName()==(Phase>=94?FString(TEXT("/Game/Memoria/Presentation/Field3D/CharacterUpgradeS356/HeadRefinement/")):FString(TEXT("/Game/Memoria/Presentation/Field3D/CharacterUpgradeS356/Accepted/")))+Id+TEXT("/SK_")+Id+TEXT(".SK_")+Id);
                if (!Skin) return true;
                Test->TestEqual(TEXT("New anatomy uses its intended articulated rig"),Skin->GetNumBones(),Phase==92?20:Phase==93?12:77);
                if (Phase==93)
                {
                    const FVector Extent=Skin->GetSkeletalMeshAsset()->GetBounds().BoxExtent;
                    const float NormalizedLength=FMath::Max(Extent.X,Extent.Y)/FMath::Max(1.0,Extent.Z)*F->Spec().Height;
                    Test->TestTrue(TEXT("Arched leech remains compact at the unchanged runtime world height"),NormalizedLength<=225.f);
                    Test->AddInfo(FString::Printf(TEXT("S356_LEECH_BOUNDS normalized_longest_horizontal=%.2f world_height=%.2f"),NormalizedLength,F->Spec().Height));
                }
                const auto* Anim=Cast<UMemoriaFieldAnimInstance>(Skin->GetAnimInstance());
                Test->TestTrue(TEXT("S356 actual locomotion binds compatible authored clips"),Anim && Anim->Idle==MemoriaCombatClips::Load(Id,F->Spec().Idle?F->Spec().Idle:TEXT("Idle"))
                    && Anim->Walk==MemoriaCombatClips::Load(Id,F->Spec().Walk?F->Spec().Walk:TEXT("Walk")));
                for (const TCHAR* Clip:{F->Spec().Strike,MemoriaCombatClips::Hit(),MemoriaCombatClips::Death()})
                {
                    const auto* Sequence=MemoriaCombatClips::Load(Id,MemoriaCombatClips::ForAction(Id,Clip));
                    Test->TestTrue(TEXT("S356 strike, hit and held death clips agree with the live skeleton"),Sequence && Sequence->GetSkeleton()==Skin->GetSkeletalMeshAsset()->GetSkeleton() && Sequence->GetPlayLength()>.2f);
                }
                Test->TestTrue(TEXT("Only the scavenger retains the thief's physical dagger grip"),Phase==95?Figure->GetBlade()!=nullptr:Figure->GetBlade()==nullptr);
                if (Phase>=94)
                {
                    const FString Original=Phase==94?TEXT("Husk"):TEXT("Thief");
                    auto* Old=LoadObject<USkeletalMesh>(nullptr,*(TEXT("/Game/Memoria/Presentation/Field3D/")+Original+TEXT("/SK_")+Original+TEXT(".SK_")+Original));
                    auto* NewMesh=Skin->GetSkeletalMeshAsset();
                    auto* Accessories=LoadObject<UMaterialInterface>(nullptr,*(TEXT("/Game/Memoria/Presentation/Field3D/CharacterUpgradeS356/Accepted/")+Id+TEXT("/M_")+Id+TEXT("Refined.M_")+Id+TEXT("Refined")));
                    Test->TestTrue(TEXT("Accepted humanoid keeps original painted body material and independent accessories"),Old && Accessories
                        && NewMesh->GetMaterials().Num()==2 && Old->GetMaterials().Num()>0
                        && NewMesh->GetMaterials()[0].MaterialInterface.Get()==Old->GetMaterials()[0].MaterialInterface.Get()
                        && NewMesh->GetMaterials()[1].MaterialInterface.Get()==Accessories);
                    auto* BodyMID=Cast<UMaterialInstanceDynamic>(Skin->GetMaterial(0));
                    auto* AccessoryMID=Cast<UMaterialInstanceDynamic>(Skin->GetMaterial(1));
                    Test->TestTrue(TEXT("Both live humanoid sections retain their own shader and hit callbacks"),BodyMID && AccessoryMID && Old
                        && BodyMID->Parent==Old->GetMaterials()[0].MaterialInterface && AccessoryMID->Parent==Accessories);
                    const auto& Ref=Skin->GetSkeletalMeshAsset()->GetRefSkeleton();
                    bool SameBind=Old && Old->GetRefSkeleton().GetNum()==Ref.GetNum();
                    for (int32 I=0;SameBind && I<Ref.GetNum();++I) SameBind=Ref.GetBoneName(I)==Old->GetRefSkeleton().GetBoneName(I)
                        && Ref.GetParentIndex(I)==Old->GetRefSkeleton().GetParentIndex(I)
                        && Ref.GetRefBonePose()[I].Equals(Old->GetRefSkeleton().GetRefBonePose()[I],.015f);
                    Test->TestTrue(TEXT("Tailored monster outfit preserves every original humanoid bind transform"),SameBind);
                }
                const FVector Center=F->GetActorLocation()+FVector(0,0,F->Spec().Height*.45f);
                const FVector Eye=Center+FVector(370,-410,235);
                DetailCamera->SetActorLocation(Eye);DetailCamera->SetActorRotation((Center-Eye).Rotation());DetailCamera->GetCameraComponent()->SetFieldOfView(48.f);
                PC->SetViewTarget(DetailCamera.Get());LegAt=Skin->GetSocketTransform(MovingBone,RTS_Component);
                RecoveryAt=Mark+FMath::Max(32,FMath::CeilToInt(Figure->GetActionLength()/.8f*60.f)+8);
            }
            if (Frame>Mark+2 && Frame<Mark+25)Figure->AdvanceLocomotion(FVector(0,2,0),1.f/60.f);
            if (Frame==Mark+20)
            {
                Test->TestFalse(TEXT("S356 locomotion articulates actual character joints"),Skin->GetSocketTransform(MovingBone,RTS_Component).Equals(LegAt,.01f));
                CheckSkinnedBody(Test,Skin,F,PC,Id,TEXT("hit_with_locomotion"));
                Capture(*(Id+TEXT("Detail")));
            }
            // Let the real stun reaction finish before inspecting idle and distance-driven walking.
            // No StopAction/reset can conceal a broken action pose or a bad return to locomotion.
            if (Frame==RecoveryAt)
            {
                Test->TestFalse(TEXT("S356 hit reaction naturally finishes before returning to idle"),Figure->IsActing());
                CheckSkinnedBody(Test,Skin,F,PC,Id,TEXT("idle_after_hit"));
                Capture(*(Id+TEXT("RecoveredIdle")));
                LegAt=Skin->GetSocketTransform(MovingBone,RTS_Component);
            }
            if (Frame>RecoveryAt && Frame<=RecoveryAt+24)Figure->AdvanceLocomotion(FVector(0,2,0),1.f/60.f);
            if (Frame==RecoveryAt+24)
            {
                Test->TestFalse(TEXT("S356 recovered walking articulates joints without an action override"),Skin->GetSocketTransform(MovingBone,RTS_Component).Equals(LegAt,.01f));
                Test->TestFalse(TEXT("S356 recovered walking is no longer in the hit action"),Figure->IsActing());
                CheckSkinnedBody(Test,Skin,F,PC,Id,TEXT("walk_after_hit"));
                Capture(*(Id+TEXT("RecoveredWalk")));
            }
            if (Frame==RecoveryAt+30)Test->TestTrue(TEXT("S356 encounter plays its real strike"),Figure->PlayAction(F->Spec().Strike));
            if (Frame==RecoveryAt+45)
            {
                CheckSkinnedBody(Test,Skin,F,PC,Id,TEXT("strike"));
                Capture(*(Id+TEXT("Strike")));
            }
            if (Frame==RecoveryAt+85)
            {
                Test->TestTrue(TEXT("S356 encounter plays its held death"),Figure->PlayAction(MemoriaCombatClips::Death(),1.f,true));
                DeathHoldFrame=Frame+FMath::Max(90,FMath::CeilToInt(Figure->GetActionLength()*60.f)+3);
            }
            if (Frame==DeathHoldFrame)
            {
                Test->TestTrue(TEXT("S356 death stays held rather than returning to idle"),Figure->IsActing() && FMath::IsNearlyEqual(Figure->GetActionTime(),Figure->GetActionLength(),.02f));
                CheckSkinnedBody(Test,Skin,F,PC,Id,TEXT("held_death"),true);
                Capture(*(Id+TEXT("Death")));
            }
            if (DeathHoldFrame==0 || Frame<DeathHoldFrame+10)break;
            DeathHoldFrame=0;
            Clear();
            if (Phase<95)
            {
                const EMemoriaFoeKind Next[]={EMemoriaFoeKind::MemoryLeech,EMemoriaFoeKind::AshWalker,EMemoriaFoeKind::BeltScavenger};
                Foe=Spawn(Next[Phase-92],FVector(-140,-220,0));Foe->Stun(30.f);++Phase;Mark=Frame;break;
            }
            PC->SetViewTarget(Pawn);DetailCamera->Destroy();DetailCamera=nullptr;
            Foe=Spawn(EMemoriaFoeKind::VoidWisp,FVector(700,0,0));
            Hp=Combat->GetPlayerHp();Orbs=Combat->GetOrbsLaunched();Closest=1e9f;
            Phase=2;Mark=Frame;break;
        }
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
    int32 Phase = 0, Frame = 0, Mark = 0, Orbs = 0, Taken = 0, WindupAt = 0, RecoveryAt = 0, DeathHoldFrame = 0;
    int32 CleanLineupAt = 0, QuietLineupFrames = 0;
    int64 Hp = 0;
    float Closest = 0.f, Health = 0.f;
    FVector Home = FVector::ZeroVector;
    bool bFixed = false, bOldFixed = false, bOrbShot = false, bLaneShot = false, bRushShot = false, bTicked = false, bCasterCue = false, bMeleeCue = false;
    TWeakObjectPtr<AMemoriaFieldMonster> Foe;
    TWeakObjectPtr<ACameraActor> DetailCamera;
    FTransform LegAt;
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
