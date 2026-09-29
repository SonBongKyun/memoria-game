#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MemoriaTutorialSubsystem.generated.h"

// S323: tutorial_hints.gd. A hint shows once, the first time its moment comes, then never again. The hint
// slides in at the top of the screen, holds for four seconds, and any key or click lets it go. The source's
// hints that name turn-based systems the action field no longer has (BREAK, directives, resonance, the
// approach, equipment, the pulse) are not carried over; the first-battle hint names the action controls.
// Unlike the source, a key that dismisses a hint is not swallowed: in real-time combat it is also a strike.
// Shown hints persist per profile in GameUserSettings.ini (never in tests or commandlets).
UCLASS()
class MEMORIA_API UMemoriaTutorialSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    static const TArray<FString>& HintIds();
    static FString Text(const FString& Id, bool bKo);
    // show_hint: false when already shown or unknown. A new hint replaces the one on screen.
    bool ShowHint(const FString& Id);
    bool HasShown(const FString& Id) const { return Shown.Contains(Id); }
    const TArray<FString>& GetShown() const { return Shown; }
    const FString& GetCurrent() const { return Current; }
    bool IsShowing() const { return !Current.IsEmpty(); }
    bool IsLeaving() const { return OutAge >= 0.f; }
    float GetAge() const { return Age; }
    float GetOutAge() const { return OutAge; }
    // _dismiss: the fade out; the hint is gone when it ends.
    void Dismiss();
    // Real time, so a hint holds its four seconds through the burn picker's slowed world.
    void Advance(float DeltaSeconds);
    void ResetShown() { Shown.Reset(); Current.Reset(); OutAge = -1.f; Save(); }
    static constexpr float SlideSeconds = .35f;
    static constexpr float HoldSeconds = 4.f;
    static constexpr float OutSeconds = .25f;
private:
    TArray<FString> Shown;
    FString Current;
    float Age = 0.f, OutAge = -1.f;
    bool bPersist = false;
    void Save() const;
};
