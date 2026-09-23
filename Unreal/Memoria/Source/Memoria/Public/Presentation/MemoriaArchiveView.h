#pragma once
#include "CoreMinimal.h"
class UMemoriaRunSubsystem;

struct MEMORIA_API FMemoriaArchiveRow
{
    FString Id, Title, Description, StoryEffect, StateLabel, GradeLabel;
    int32 Grade = 0;
    bool bBurned = false, bResidue = false, bFaded = false;
    double Erosion = 0.0; // Source display ratio, clamped to [0, 1].
    int64 BurnPower = 0;
    FLinearColor Accent = FLinearColor::White;
};
struct MEMORIA_API FMemoriaArchiveView
{
    FString Title, EmptyText;
    TArray<FString> FilterLabels;
    TArray<FMemoriaArchiveRow> Rows;
    int32 OwnedCount = 0, RemainingCount = 0, BurnedCount = 0;
    bool bKo = false;
};
namespace MemoriaArchive
{
    MEMORIA_API FMemoriaArchiveView Build(const UMemoriaRunSubsystem& Run, int32 FilterGrade = -1);
}
