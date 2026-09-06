#include "MemoriaFoundationAssetsCommandlet.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "PaperSprite.h"
#include "SpriteEditorOnlyTypes.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"
#include "Framework/MemoriaGameMode.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "FileHelpers.h"
#include "Factories/WorldFactory.h"

namespace
{
const FString Base = TEXT("/Game/Tests/Foundation/");
template <class T> T* Asset(const TCHAR* Name)
{
    UPackage* Package = CreatePackage(*(Base + Name));
    return NewObject<T>(Package, Name, RF_Public | RF_Standalone);
}
bool SaveAsset(UObject* Object)
{
    FAssetRegistryModule::AssetCreated(Object);
    Object->MarkPackageDirty();
    FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
    const FString File = FPackageName::LongPackageNameToFilename(Object->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
    const bool Saved = UPackage::SavePackage(Object->GetOutermost(), Object, *File, Args);
    UE_LOG(LogTemp, Display, TEXT("FOUNDATION_ASSET %s %s"), *Object->GetPathName(), Saved ? TEXT("SAVED") : TEXT("FAILED"));
    return Saved;
}
void Axis(UInputMappingContext* Context, UInputAction* Action, FKey Key, bool bY, bool bNegative)
{
    auto& Mapping = Context->MapKey(Action, Key);
    if (bNegative) { Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(Context)); }
    if (bY)
    {
        auto* Modifier = NewObject<UInputModifierSwizzleAxis>(Context);
        Modifier->Order = EInputAxisSwizzle::YXZ; Mapping.Modifiers.Add(Modifier);
    }
}
}
UMemoriaFoundationAssetsCommandlet::UMemoriaFoundationAssetsCommandlet()
{
    IsClient = false; IsServer = false; IsEditor = true; LogToConsole = true;
}
int32 UMemoriaFoundationAssetsCommandlet::Main(const FString& Params)
{
    // Refuse accidental replacement. Delete/re-author only by a separate explicit operation.
    const FString MapFile = FPackageName::LongPackageNameToFilename(Base + TEXT("L_FoundationTest"), FPackageName::GetMapPackageExtension());
    for (const TCHAR* Name : {TEXT("IA_Move"), TEXT("IA_Confirm"), TEXT("IA_Back"), TEXT("IA_Menu"), TEXT("IMC_Foundation"), TEXT("IMC_Modal"), TEXT("T_FootPivot"), TEXT("SPR_FootPivot"), TEXT("WBP_FoundationModal"), TEXT("L_FoundationTest")})
    {
        if (FPackageName::DoesPackageExist(Base + Name))
        { UE_LOG(LogTemp, Error, TEXT("Asset already exists; authoring refuses replacement: %s"), Name); return 1; }
    }
    bool bSaved = true;
    auto* Move = Asset<UInputAction>(TEXT("IA_Move")); Move->ValueType = EInputActionValueType::Axis2D;
    auto* Confirm = Asset<UInputAction>(TEXT("IA_Confirm"));
    auto* Back = Asset<UInputAction>(TEXT("IA_Back"));
    auto* Menu = Asset<UInputAction>(TEXT("IA_Menu"));
    auto* Context = Asset<UInputMappingContext>(TEXT("IMC_Foundation"));
    for (FKey K : {EKeys::D, EKeys::Right, EKeys::Gamepad_DPad_Right}) { Axis(Context, Move, K, false, false); }
    for (FKey K : {EKeys::A, EKeys::Left, EKeys::Gamepad_DPad_Left}) { Axis(Context, Move, K, false, true); }
    for (FKey K : {EKeys::W, EKeys::Up, EKeys::Gamepad_DPad_Up}) { Axis(Context, Move, K, true, false); }
    for (FKey K : {EKeys::S, EKeys::Down, EKeys::Gamepad_DPad_Down}) { Axis(Context, Move, K, true, true); }
    for (FKey K : {EKeys::Gamepad_LeftX, EKeys::Gamepad_LeftY})
    {
        Axis(Context, Move, K, K == EKeys::Gamepad_LeftY, false);
        auto& M = const_cast<FEnhancedActionKeyMapping&>(Context->GetMappings().Last());
        auto* DeadZone = NewObject<UInputModifierDeadZone>(Context); DeadZone->LowerThreshold = 0.5f;
        M.Modifiers.Insert(DeadZone, 0);
    }
    for (FKey K : {EKeys::E, EKeys::SpaceBar, EKeys::Enter, EKeys::Gamepad_FaceButton_Bottom}) { Context->MapKey(Confirm, K); }
    Context->MapKey(Menu, EKeys::Escape); Context->MapKey(Menu, EKeys::Gamepad_Special_Right);
    Context->MapKey(Back, EKeys::Gamepad_FaceButton_Right);
    auto* Modal = Asset<UInputMappingContext>(TEXT("IMC_Modal"));
    for (FKey K : {EKeys::Escape, EKeys::Gamepad_FaceButton_Right, EKeys::Gamepad_Special_Right}) { Modal->MapKey(Back, K); }
    for (FKey K : {EKeys::E, EKeys::SpaceBar, EKeys::Enter, EKeys::Gamepad_FaceButton_Bottom}) { Modal->MapKey(Confirm, K); }
    for (UObject* Object : TArray<UObject*>{Move, Confirm, Back, Menu, Context, Modal}) { bSaved &= SaveAsset(Object); }

    auto* Texture = Asset<UTexture2D>(TEXT("T_FootPivot"));
    TArray<FColor> Pixels; Pixels.Init(FColor::Transparent, 32 * 48);
    // Asymmetric controlled sprite: warm head, dark body, cyan feet at the pivot.
    for (int32 Y = 0; Y < 48; ++Y) for (int32 X = 0; X < 32; ++X)
    {
        if (X >= 7 && X < 25) { Pixels[Y * 32 + X] = Y < 12 ? FColor(240, 180, 65) : Y >= 44 ? FColor(30, 230, 240) : FColor(120, 70, 170); }
        if (Y >= 14 && Y < 22 && X >= 25) { Pixels[Y * 32 + X] = FColor(230, 80, 65); }
    }
    Texture->Source.Init(32, 48, 1, 1, TSF_BGRA8, reinterpret_cast<const uint8*>(Pixels.GetData()));
    Texture->CompressionSettings = TC_EditorIcon; Texture->MipGenSettings = TMGS_NoMipmaps; Texture->Filter = TF_Nearest;
    Texture->SRGB = true; Texture->PostEditChange(); bSaved &= SaveAsset(Texture);
    auto* Sprite = Asset<UPaperSprite>(TEXT("SPR_FootPivot"));
    FSpriteAssetInitParameters Init; Init.SetTextureAndFill(Texture); Init.SetPixelsPerUnrealUnit(1.0f);
    Sprite->InitializeSprite(Init); Sprite->SetPivotMode(ESpritePivotMode::Bottom_Center, FVector2D(16, 48)); bSaved &= SaveAsset(Sprite);

    UPackage* WidgetPackage = CreatePackage(*(Base + TEXT("WBP_FoundationModal")));
    auto* Widget = Cast<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(UUserWidget::StaticClass(), WidgetPackage, TEXT("WBP_FoundationModal"), BPTYPE_Normal,
        UWidgetBlueprint::StaticClass(), UWidgetBlueprintGeneratedClass::StaticClass()));
    if (!Widget) { return 1; }
    auto* Canvas = Widget->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
    Widget->WidgetTree->RootWidget = Canvas;
    auto* Border = Widget->WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
    Border->SetBrushColor(FLinearColor(0.025f, 0.035f, 0.06f, 1.0f)); Border->SetPadding(FMargin(28));
    auto* Slot = Canvas->AddChildToCanvas(Border); Slot->SetAnchors(FAnchors(0.5f)); Slot->SetAlignment(FVector2D(0.5, 0.5)); Slot->SetSize(FVector2D(620, 180)); Slot->SetPosition(FVector2D::ZeroVector);
    auto* Message = Widget->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Message"));
    Message->SetText(FText::FromString(TEXT("Foundation Test Modal\n\nEnter / A: confirm     Escape / B: close")));
    Message->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.9f, 1.0f))); Message->SetAutoWrapText(true);
    Border->SetContent(Message);
    FKismetEditorUtilities::CompileBlueprint(Widget);
    if (Widget->Status == BS_Error) { return 1; }
    bSaved &= SaveAsset(Widget);

    // WorldFactory is supported by commandlets; the interactive editor's
    // CreateNewMap helper accesses global mode tools and asserts in commandlets.
    UPackage* MapPackage = CreatePackage(*(Base + TEXT("L_FoundationTest")));
    auto* Factory = NewObject<UWorldFactory>(); Factory->bCreateWorldPartition = false;
    UWorld* World = Cast<UWorld>(Factory->FactoryCreateNew(UWorld::StaticClass(), MapPackage, TEXT("L_FoundationTest"), RF_Public | RF_Standalone, nullptr, GWarn));
    if (!World) { return 1; }
    World->GetWorldSettings()->DefaultGameMode = AMemoriaGameMode::StaticClass();
    World->SpawnActor<APlayerStart>(FVector::ZeroVector, FRotator::ZeroRotator);
    MapPackage->SetPackageFlags(PKG_ContainsMap);
    FSavePackageArgs MapArgs; MapArgs.TopLevelFlags = RF_Public | RF_Standalone;
    bSaved &= UPackage::SavePackage(MapPackage, World, *MapFile, MapArgs);
    UE_LOG(LogTemp, Display, TEXT("MEMORIA_FOUNDATION_AUTHORING_%s map=%s"), bSaved ? TEXT("PASS") : TEXT("FAIL"), *MapFile);
    return bSaved ? 0 : 1;
}
