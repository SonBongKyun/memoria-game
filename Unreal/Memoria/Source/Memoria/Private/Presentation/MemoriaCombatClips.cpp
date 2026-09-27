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
