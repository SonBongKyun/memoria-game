#pragma once
#include "CoreMinimal.h"
class UAnimSequence;
struct FMemoriaCombatClip { const TCHAR* Name; const TCHAR* MannequinPath; };
// S311: the combat clips every 3D combatant uses. The mannequin (the monster stand-in) plays Epic's
// originals; each rigged character plays its retargeted copy from -run=MemoriaCombatRetarget.
namespace MemoriaCombatClips
{
    MEMORIA_API const TArray<FMemoriaCombatClip>& Clips();
    MEMORIA_API FString MannequinMesh();
    // /Game/Memoria/Presentation/Field3D/<Id>/Combat/A_<Id>_<Name>. For "Mannequin": Epic's original for the
    // S311 set, else the retargeted foe clip.
    MEMORIA_API FString ClipPath(const FString& Id, const FString& Name);
    MEMORIA_API UAnimSequence* Load(const FString& Id, const FString& Name);
    // S312: Arrel's sword set, retargeted from Quaternius' UAL2 by -run=MemoriaSwordRetarget.
    MEMORIA_API const TArray<const TCHAR*>& SwordClips();
    // S313: UAL2 clips retargeted onto the mannequin foes by -run=MemoriaFoeAssets (Combat/Foes/A_Mannequin_<Clip>).
    MEMORIA_API const TArray<const TCHAR*>& FoeClips();
    // The clip an action uses for this character: the sword set when it has one, else the S311 melee set.
    MEMORIA_API const TCHAR* ForAction(const FString& Id, const TCHAR* Clip);
    // Clip names.
    inline const TCHAR* Attack(int32 Step) { return Step == 0 ? TEXT("Attack_01") : Step == 1 ? TEXT("Attack_02") : TEXT("Attack_03"); }
    inline const TCHAR* Charged() { return TEXT("ChargedAttack"); }
    inline const TCHAR* Dash() { return TEXT("Dash"); }
    inline const TCHAR* Hit() { return TEXT("HitReact_Front_Lgt_01"); }
    inline const TCHAR* Death() { return TEXT("Death_Front_01"); }
    inline const TCHAR* Block() { return TEXT("Block"); }  // S315: only characters with the sword set have one
}
