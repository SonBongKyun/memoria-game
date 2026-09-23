#pragma once
#include "Framework/MemoriaFieldPawn.h"
#include "GameFramework/FloatingPawnMovement.h"

// Tick the possessed production movement component; no duplicated acceleration model.
// Enhanced Input, walls and modal handoffs are exercised by the rendered replay suites.
static FString CheckVerdanMovement(FAutomationTestBase* Test, AMemoriaFieldPawn* Pawn)
{
    auto* Move=Cast<UFloatingPawnMovement>(Pawn->GetMovementComponent());
    if (!Test->TestNotNull(TEXT("Real floating movement component"),Move)) return TEXT("[]");
    const auto* FoundationMove=Cast<UFloatingPawnMovement>(GetDefault<AMemoriaFieldPawn>()->GetMovementComponent());
    Test->TestEqual(TEXT("Foundation default speed remains unchanged"),FoundationMove->MaxSpeed,1200.f);
    Test->TestEqual(TEXT("Verdan walking profile is actually applied"),Move->MaxSpeed,120.f);
    const FVector SavedPosition=Pawn->GetActorLocation(), SavedVelocity=Move->Velocity;
    const FVector Inputs[]={FVector(1,0,0),FVector(1,1,0),FVector(.5,0,0)};
    const TCHAR* Names[]={TEXT("straight"),TEXT("diagonal"),TEXT("half_stick")};
    FString Records;
    for (const int32 Hz:{30,60,120}) for (int32 Case=0;Case<3;++Case)
    {
        const float Dt=1.f/Hz, ExpectedSpeed=Case==2?60.f:120.f;
        const FVector Start(0,-300,0);
        Pawn->SetActorLocation(Start); Pawn->ConsumeMovementInputVector(); Move->StopMovementImmediately();
        double Peak=0, TimeToCruise=0;
        for (int32 Frame=0;Frame<2*Hz;++Frame)
        {
            Pawn->AddMovementInput(Inputs[Case],1.f);
            Move->TickComponent(Dt,LEVELTICK_All,nullptr);
            const double Speed=Move->Velocity.Size2D(); Peak=FMath::Max(Peak,Speed);
            if (!TimeToCruise && Speed>=ExpectedSpeed*.95) TimeToCruise=(Frame+1)*double(Dt);
            Test->TestTrue(TEXT("Axial, diagonal and partial stick stay below their speed limits"),Speed<=ExpectedSpeed+.01);
            Test->TestTrue(TEXT("Walk stays on the original plane"),FMath::Abs(Pawn->GetActorLocation().Z)<.001);
        }
        const FVector Released=Pawn->GetActorLocation();
        const double Distance=FVector::Dist2D(Start,Released);
        double StopTime=0;
        for (int32 Frame=0;Frame<Hz/2;++Frame)
        {
            Move->TickComponent(Dt,LEVELTICK_All,nullptr);
            if (!StopTime && Move->Velocity.IsNearlyZero(.001)) StopTime=(Frame+1)*double(Dt);
        }
        const double StopDistance=FVector::Dist2D(Released,Pawn->GetActorLocation());
        Test->TestTrue(TEXT("Reaches walk speed within 160ms at all tested frame rates"),TimeToCruise>0 && TimeToCruise<=.161);
        Test->TestTrue(TEXT("No diagonal speed boost; two-second travel stays in the walking envelope"),Distance>ExpectedSpeed*1.86 && Distance<ExpectedSpeed*2.01);
        Test->TestTrue(TEXT("Release stops within 120ms and seven world units"),StopTime>0 && StopTime<=.121 && StopDistance<7.0);
        Test->TestTrue(TEXT("Release fully stops the actual movement component"),Move->Velocity.IsNearlyZero(.001));
        if (!Records.IsEmpty()) Records+=TEXT(",");
        Records+=FString::Printf(TEXT("{\"hz\":%d,\"input\":\"%s\",\"peak_speed\":%.6f,\"distance_2s\":%.6f,\"time_to_cruise\":%.6f,\"stop_seconds\":%.6f,\"stop_distance\":%.6f}"),Hz,Names[Case],Peak,Distance,TimeToCruise,StopTime,StopDistance);
    }
    Pawn->ConsumeMovementInputVector(); Move->StopMovementImmediately();
    Pawn->SetActorLocation(SavedPosition); Move->Velocity=SavedVelocity;
    return TEXT("[")+Records+TEXT("]");
}
