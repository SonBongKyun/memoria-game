#pragma once
#include "CoreMinimal.h"

// S320: a chapter field map, read from the IR that Unreal/Tools/export_chapter_maps.py extracts from the
// Godot map script (scenes/maps/<map>.gd). Positions are source pixels; ToWorld maps them into the level.
struct FMemoriaChapterRect
{
    FVector2D Origin = FVector2D::ZeroVector, Size = FVector2D::ZeroVector;
    bool Contains(const FVector2D& P) const { return P.X >= Origin.X && P.Y >= Origin.Y && P.X <= Origin.X + Size.X && P.Y <= Origin.Y + Size.Y; }
    FVector2D Center() const { return Origin + Size * .5; }
};
// One link of the arrival chain: its flag, its dialogue group, and what the group's end handler sets and toasts.
struct FMemoriaChapterStep { FString Flag, Group; TArray<FString> Flags, Toasts; };
struct FMemoriaChapterTrigger { FMemoriaChapterRect Rect; FString Group, Flag, Gate; };
struct FMemoriaChapterChest { FVector2D Origin = FVector2D::ZeroVector; FString Flag; int64 Grains = 0; TArray<TPair<FString, int64>> Items; };
struct FMemoriaChapterClue { FVector2D Origin = FVector2D::ZeroVector; FString Flag, Text; };
struct FMemoriaChapterBattle { FMemoriaChapterRect Rect; FString Name; int32 Hp = 0, Atk = 0; bool bVoid = false; };
struct FMemoriaChapterExit { FMemoriaChapterRect Rect; FString Requires, Completes, Group, NextMap; int32 NextChapter = 0; };
struct FMemoriaChapterMapSpec
{
    FString Map, AssetPrefix, DialogueFile, Source, TitleName, Subtitle, EliaRepeat, Splash;
    int32 Chapter = 0, TileSize = 32, Width = 0, Height = 0;
    TArray<int32> Tiles;                 // row-major, Width x Height
    TArray<FLinearColor> TileColors;     // per tile type (sRGB as authored)
    TArray<FString> TileDetails, TileNames;
    TArray<int32> Solid;                 // tile types that block (walls, ruins)
    FLinearColor Hue = FLinearColor::White, Light = FLinearColor::White;
    float Mood = 0.f, Brightness = 0.f, Saturation = 1.f;
    FVector2D Spawn = FVector2D::ZeroVector;
    TArray<FMemoriaChapterStep> Sequence;
    FMemoriaChapterExit Exit;
    TArray<FMemoriaChapterTrigger> Triggers;
    FString ObjectsGate, BattlesGate, EncountersGate;
    TArray<FMemoriaChapterChest> Chests;
    TArray<FMemoriaChapterClue> Clues;
    TArray<FMemoriaChapterBattle> Battles;
    int32 TileAt(int32 X, int32 Y) const { return X >= 0 && Y >= 0 && X < Width && Y < Height ? Tiles[Y * Width + X] : -1; }
    bool IsSolid(int32 Type) const { return Solid.Contains(Type); }
};
namespace MemoriaChapterMaps
{
    // The chapter maps keep the source's proportions: Arrel stands 150 units, about one and a half 32 px tiles.
    inline constexpr float Scale = 3.f;
    inline FVector ToWorld(const FVector2D& Source) { return FVector(Source.X * Scale, -Source.Y * Scale, 0.0); }
    inline FVector2D ToSource(const FVector& World) { return FVector2D(World.X / Scale, -World.Y / Scale); }
    MEMORIA_API const FMemoriaChapterMapSpec* Find(const FString& Map);
    // "waystation_arrival" -> "DA_Field_Ch3WaystationArrival" (narrative_ir.py's asset names).
    MEMORIA_API FString GroupAsset(const FMemoriaChapterMapSpec& Spec, const FString& Group);
    // The level each ported map lives in.
    MEMORIA_API FString LevelPath(const FString& Map);
    MEMORIA_API FString MapFromLevel(const FString& LevelName);
}
