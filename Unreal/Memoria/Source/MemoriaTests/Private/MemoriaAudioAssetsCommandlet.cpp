#include "MemoriaAudioAssetsCommandlet.h"
#include "Audio/MemoriaAudioCatalog.h"
#include "Factories/SoundFactory.h"
#include "Sound/SoundWave.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
namespace
{
struct FAudioImport { FString Package, File; bool bLoop = false; };
}
UMemoriaAudioAssetsCommandlet::UMemoriaAudioAssetsCommandlet()
{ IsClient = false; IsServer = false; IsEditor = true; LogToConsole = true; }
int32 UMemoriaAudioAssetsCommandlet::Main(const FString&)
{
    const FString Root = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../.."));
    const FString Sources = Root / TEXT("Unreal/ArtSource/Audio");
    FString Text; TSharedPtr<FJsonObject> Manifest;
    if (!FFileHelper::LoadFileToString(Text, *(Sources / TEXT("manifest.json"))) ||
        !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Manifest) || !Manifest)
    { UE_LOG(LogTemp, Error, TEXT("MEMORIA_AUDIO missing manifest; run generate_audio_sources.py")); return 1; }
    const TSharedPtr<FJsonObject> Cues = Manifest->GetObjectField(TEXT("cues"));
    TArray<FAudioImport> Imports;
    // The runtime catalog and the rendered manifest must describe the same cue set.
    for (const auto& Cue : MemoriaAudio::Cues())
    {
        const TSharedPtr<FJsonObject>* Entry = nullptr;
        if (!Cues->TryGetObjectField(Cue.Id, Entry)) { UE_LOG(LogTemp, Error, TEXT("MEMORIA_AUDIO cue %s not rendered"), Cue.Id); return 1; }
        Imports.Add({MemoriaAudio::CuePackage(Cue.Id), Sources / (*Entry)->GetStringField(TEXT("file")), false});
    }
    for (const auto& Track : MemoriaAudio::Tracks()) Imports.Add({MemoriaAudio::TrackPackage(Track.Id), Root / Track.Source, true});
    for (const auto& Pair : Cues->Values)
    {
        const FName Id(*FString(Pair.Key.ToView()));
        if (!MemoriaAudio::FindCue(Id) && !MemoriaAudio::FindTrack(Id)) { UE_LOG(LogTemp, Error, TEXT("MEMORIA_AUDIO rendered cue %s has no runtime entry"), *Id.ToString()); return 1; }
    }
    // Additive: existing packages are kept, never refreshed. Check every new source before the first write.
    Imports.RemoveAll([](const FAudioImport& Import)
    {
        const bool bKept = FPackageName::DoesPackageExist(Import.Package);
        if (bKept) UE_LOG(LogTemp, Display, TEXT("MEMORIA_AUDIO_KEPT %s"), *Import.Package);
        return bKept;
    });
    for (const auto& Import : Imports)
        if (!FPaths::FileExists(Import.File)) { UE_LOG(LogTemp, Error, TEXT("MEMORIA_AUDIO missing source %s"), *Import.File); return 1; }
    auto* Factory = NewObject<UFactory>(GetTransientPackage(), USoundFactory::StaticClass());
    for (const auto& Import : Imports)
    {
        const FString Name = FPaths::GetBaseFilename(Import.Package); bool bCancelled = false;
        auto* Package = CreatePackage(*Import.Package);
        auto* Wave = Cast<USoundWave>(Factory->FactoryCreateFile(USoundWave::StaticClass(), Package, *Name, RF_Public | RF_Standalone, Import.File, nullptr, GWarn, bCancelled));
        if (!Wave || bCancelled) { UE_LOG(LogTemp, Error, TEXT("MEMORIA_AUDIO import failed %s"), *Import.File); return 1; }
        Wave->bLooping = Import.bLoop; Wave->PostEditChange();
        FAssetRegistryModule::AssetCreated(Wave); Wave->MarkPackageDirty();
        FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
        const FString File = FPackageName::LongPackageNameToFilename(Import.Package, FPackageName::GetAssetPackageExtension());
        if (!UPackage::SavePackage(Package, Wave, *File, Args)) return 1;
        UE_LOG(LogTemp, Display, TEXT("MEMORIA_AUDIO_ASSET %s %.3fs loop=%d"), *Wave->GetPathName(), Wave->Duration, Import.bLoop ? 1 : 0);
    }
    return 0;
}
