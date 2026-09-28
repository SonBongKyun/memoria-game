#include "Presentation/MemoriaCombatClips.h"
#include "Animation/AnimSequence.h"
#include "Misc/PackageName.h"
namespace MemoriaCombatClips
{
const TArray<FMemoriaCombatClip>& Clips()
{
    static const TArray<FMemoriaCombatClip> Values = {
        {TEXT("Attack_01"), TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01")},
        {TEXT("Attack_02"), TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_02")},
        {TEXT("Attack_03"), TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_03")},
        {TEXT("ChargedAttack"), TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_ChargedAttack")},
        {TEXT("Dash"), TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jump/MM_Dash")},
        {TEXT("HitReact_Front_Lgt_01"), TEXT("/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Lgt_01")},
        {TEXT("Death_Front_01"), TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Front_01")},
    };
    return Values;
}
const TArray<const TCHAR*>& SwordClips()
{
    static const TArray<const TCHAR*> Values = {TEXT("Sword_Regular_A"), TEXT("Sword_Regular_B"), TEXT("Sword_Regular_C"),
        TEXT("Sword_Heavy_Combo"), TEXT("Sword_Dash"), TEXT("Sword_Block"), TEXT("Hit_Knockback")};
    return Values;
}
const TCHAR* ForAction(const FString& Id, const TCHAR* Clip)
{
    // The melee combo becomes the sword's three regular cuts, the dash its lunge, a blow taken its knockback.
    // Sword_Heavy_Combo and Sword_Block are retargeted for later skills and not mapped yet.
    static const TMap<FString, const TCHAR*> Sword = {
        {TEXT("Attack_01"), TEXT("Sword_Regular_A")}, {TEXT("Attack_02"), TEXT("Sword_Regular_B")}, {TEXT("Attack_03"), TEXT("Sword_Regular_C")},
        {TEXT("Dash"), TEXT("Sword_Dash")}, {TEXT("HitReact_Front_Lgt_01"), TEXT("Hit_Knockback")}};
    const TCHAR* const* Mapped = Sword.Find(Clip);
    return Mapped && Load(Id, *Mapped) ? *Mapped : Clip;
}
FString MannequinMesh() { return TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"); }
FString ClipPath(const FString& Id, const FString& Name)
{
    if (Id == TEXT("Mannequin"))
    {
        const auto* Clip = Clips().FindByPredicate([&](const FMemoriaCombatClip& C) { return Name == C.Name; });
        return Clip ? FString(Clip->MannequinPath) : FString();
    }
    return FString::Printf(TEXT("/Game/Memoria/Presentation/Field3D/%s/Combat/A_%s_%s"), *Id, *Id, *Name);
}
UAnimSequence* Load(const FString& Id, const FString& Name)
{
    const FString Path = ClipPath(Id, Name);
    return Path.IsEmpty() ? nullptr : LoadObject<UAnimSequence>(nullptr, *(Path + TEXT(".") + FPackageName::GetShortName(Path)), nullptr, LOAD_NoWarn | LOAD_Quiet);
}
}
