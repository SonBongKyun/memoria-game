#pragma once
#include "CoreMinimal.h"
// S368: the illustrations shown as a memory burns (the source's battle_scene.gd MEMORY_BURN_CUTIN_PATHS and
// _resolve_memory_burn_cutin): an exact id first, then keywords in the id and title. Imported by
// -run=MemoriaAfterimageAssets -BurnArt into /Game/Memoria/Presentation/Combat/T_MemoryBurn_<Name>.
namespace MemoriaBurnArt
{
struct FEntry { const TCHAR* Name; const TCHAR* File; const TCHAR* Id; };
inline const FEntry Entries[6] = {
    {TEXT("first_sword"), TEXT("assets/cg/generated/memory_burn_first_sword.png"), TEXT("identity_first_sword")},
    {TEXT("elia_song"), TEXT("assets/cg/generated/memory_burn_elia_song.png"), TEXT("daily_campfire_song")},
    {TEXT("reaching_hand"), TEXT("assets/cg/generated/memory_burn_reaching_hand.png"), TEXT("rel_hand_reaching")},
    {TEXT("arrel_name"), TEXT("assets/cg/generated/memory_burn_arrel_name.png"), TEXT("core_name_origin")},
    {TEXT("compass"), TEXT("assets/cg/generated/memory_burn_compass.png"), TEXT("identity_compass")},
    {TEXT("void_walker"), TEXT("assets/cg/generated/memory_burn_void_walker.png"), TEXT("identity_void_walker")}};
inline FString Package(const TCHAR* Name) { return FString::Printf(TEXT("/Game/Memoria/Presentation/Combat/T_MemoryBurn_%s"), Name); }
inline FString ObjectPath(const TCHAR* Name) { return FString::Printf(TEXT("%s.T_MemoryBurn_%s"), *Package(Name), Name); }
// The entry's name for a memory, or nullptr: the exact id, then the source's keyword order.
inline const TCHAR* Resolve(const FString& Id, const FString& Title)
{
    const FString Clean = Id.TrimStartAndEnd();
    for (const FEntry& Entry : Entries) if (Clean.Equals(Entry.Id, ESearchCase::CaseSensitive)) return Entry.Name;
    const FString Lookup = (Clean + TEXT(" ") + Title).ToLower();
    if (Lookup.Contains(TEXT("sword"))) return TEXT("first_sword");
    if (Lookup.Contains(TEXT("song")) || Lookup.Contains(TEXT("campfire"))) return TEXT("elia_song");
    if (Lookup.Contains(TEXT("hand")) || Lookup.Contains(TEXT("reach"))) return TEXT("reaching_hand");
    if (Lookup.Contains(TEXT("name")) || Lookup.Contains(TEXT("arrel")) || Lookup.Contains(TEXT("origin"))) return TEXT("arrel_name");
    if (Lookup.Contains(TEXT("compass"))) return TEXT("compass");
    if (Lookup.Contains(TEXT("void")) || Lookup.Contains(TEXT("bl-07")) || Lookup.Contains(TEXT("bl07"))) return TEXT("void_walker");
    return nullptr;
}
}
