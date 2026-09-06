#include "MemoriaStartingCatalogImport.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "HAL/FileManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
THIRD_PARTY_INCLUDES_START
#include <openssl/sha.h>
THIRD_PARTY_INCLUDES_END

namespace MemoriaCatalogImport
{
namespace
{
FString Quote(const FString& S)
{
    FString Out = TEXT("\"");
    for (TCHAR C : S)
    {
        switch (C)
        {
        case '"': Out += TEXT("\\\""); break;
        case '\\': Out += TEXT("\\\\"); break;
        case '\b': Out += TEXT("\\b"); break;
        case '\f': Out += TEXT("\\f"); break;
        case '\n': Out += TEXT("\\n"); break;
        case '\r': Out += TEXT("\\r"); break;
        case '\t': Out += TEXT("\\t"); break;
        default: if (C < 32) { Out += FString::Printf(TEXT("\\u%04x"), uint32(C)); } else { Out += C; }
        }
    }
    return Out + TEXT("\"");
}
bool Keys(const TSharedPtr<FJsonObject>& O, std::initializer_list<const TCHAR*> Expected)
{
    if (!O || O->Values.Num() != int32(Expected.size())) { return false; }
    for (const TCHAR* K : Expected)
    {
        bool Found=false;
        for (const auto& Pair : O->Values) { Found |= Pair.Key.ToView().Equals(K,ESearchCase::CaseSensitive); }
        if (!Found) { return false; }
    }
    return true;
}
bool Str(const TSharedPtr<FJsonObject>& O, const TCHAR* K, FString& V, bool bNonempty = false)
{
    if (!O || !O->HasTypedField<EJson::String>(K) || !O->TryGetStringField(K,V) || (bNonempty && V.IsEmpty())) { return false; }
    for (TCHAR C : V) { if (C == 0) { return false; } }
    return true;
}
bool Hex(const FString& S, int32 Length)
{
    if (S.Len() != Length) { return false; }
    for (TCHAR C : S) { if (!((C >= '0' && C <= '9') || (C >= 'a' && C <= 'f'))) { return false; } }
    return true;
}
bool Integer(const TSharedPtr<FJsonObject>& O, const TCHAR* K, int64& V, int64 Low, int64 High)
{
    double N = 0;
    if (!O->HasTypedField<EJson::Number>(K) || !O->TryGetNumberField(K, N) || !FMath::IsFinite(N) || N < double(Low) || N > double(High) || N != FMath::FloorToDouble(N)) { return false; }
    V = int64(N); return true;
}
TSharedPtr<FJsonValue> ObjectValue(const TSharedPtr<FJsonObject>& O) { return MakeShared<FJsonValueObject>(O); }
TSharedPtr<FJsonObject> Semantic(const UMemoriaMemoryCatalog& C)
{
    auto O = MakeShared<FJsonObject>();
    O->SetNumberField(TEXT("schema_version"), 1); O->SetStringField(TEXT("content_kind"), TEXT("player_memory.starting_catalog"));
    TArray<TSharedPtr<FJsonValue>> Entries;
    for (int32 I = 0; I < C.Definitions.Num(); ++I)
    {
        const auto& D = C.Definitions[I]; auto R = MakeShared<FJsonObject>();
        R->SetStringField(TEXT("id"), D.Id); R->SetStringField(TEXT("title"), D.Title); R->SetStringField(TEXT("description"), D.Description);
        R->SetNumberField(TEXT("grade"), uint8(D.RawGrade)); R->SetNumberField(TEXT("burn_power"), double(D.BurnPower));
        R->SetStringField(TEXT("story_effect"), D.StoryEffect); R->SetStringField(TEXT("related_npc"), D.RelatedNpc);
        if (!C.LocalizedText.IsValidIndex(I) || !C.LocalizedText[I].MemoryId.Equals(D.Id,ESearchCase::CaseSensitive) || !C.LocalizedText[I].Locale.Equals(TEXT("ko"),ESearchCase::CaseSensitive)) { return nullptr; }
        const auto& L = C.LocalizedText[I]; auto K = MakeShared<FJsonObject>();
        K->SetStringField(TEXT("title"), L.Title); K->SetStringField(TEXT("description"), L.Description);
        if (L.bHasStoryEffect) { K->SetStringField(TEXT("story_effect"), L.StoryEffect); } else { K->SetField(TEXT("story_effect"), MakeShared<FJsonValueNull>()); }
        R->SetObjectField(TEXT("localization_ko"), K); Entries.Add(ObjectValue(R));
    }
    if (C.Definitions.Num() != C.LocalizedText.Num()) { return nullptr; }
    O->SetArrayField(TEXT("entries"), Entries); return O;
}
}
FString Sha256(const FString& Text)
{
    FTCHARToUTF8 Bytes(*Text); uint8 Hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const uint8*>(Bytes.Get()), Bytes.Length(), Hash);
    return BytesToHex(Hash, SHA256_DIGEST_LENGTH).ToLower();
}
FString Canonical(const TSharedPtr<FJsonValue>& V)
{
    if (!V) { return TEXT("INVALID"); }
    switch (V->Type)
    {
    case EJson::Null: return TEXT("null");
    case EJson::Boolean: return V->AsBool() ? TEXT("true") : TEXT("false");
    case EJson::String: return Quote(V->AsString());
    case EJson::Number:
    {
        const double N = V->AsNumber();
        return FMath::IsFinite(N) && FMath::Abs(N) <= 9007199254740991.0 && N == FMath::FloorToDouble(N)
            ? FString::Printf(TEXT("%lld"), int64(N)) : TEXT("INVALID_NUMBER");
    }
    case EJson::Array:
    {
        TArray<FString> Items; for (const auto& E : V->AsArray()) { Items.Add(Canonical(E)); }
        return TEXT("[") + FString::Join(Items, TEXT(",")) + TEXT("]");
    }
    case EJson::Object:
    {
        TArray<FString> Names; for (const auto& Pair : V->AsObject()->Values) { Names.Add(FString(Pair.Key.ToView())); }
        Names.Sort([](const FString& A, const FString& B) { return A.Compare(B, ESearchCase::CaseSensitive) < 0; });
        TArray<FString> Items; for (const auto& K : Names) { Items.Add(Quote(K) + TEXT(":") + Canonical(V->AsObject()->TryGetField(K))); }
        return TEXT("{") + FString::Join(Items, TEXT(",")) + TEXT("}");
    }
    default: return TEXT("INVALID");
    }
}
FString Fingerprint(const UMemoriaMemoryCatalog& Catalog)
{
    auto S = Semantic(Catalog); return S ? Sha256(Canonical(ObjectValue(S)) + TEXT("\n")) : TEXT("");
}
bool ReadIr(const FString& Path, UMemoriaMemoryCatalog& C, FString& Error, bool bVerifySources)
{
    auto Fail = [&](const TCHAR* Why) { Error = Why; return false; };
    TArray<uint8> Bytes;
    if (!FFileHelper::LoadFileToArray(Bytes, *Path) || Bytes.Num() > 4 * 1024 * 1024) { return Fail(TEXT("Cannot read bounded IR")); }
    FUTF8ToTCHAR Decoded(reinterpret_cast<const ANSICHAR*>(Bytes.GetData()),Bytes.Num());
    FString Text(Decoded.Length(),Decoded.Get());
    FTCHARToUTF8 Back(*Text);
    if (Back.Length() != Bytes.Num() || FMemory::Memcmp(Back.Get(), Bytes.GetData(), Bytes.Num()) != 0) { return Fail(TEXT("Invalid UTF-8")); }
    TSharedPtr<FJsonObject> O;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), O) || !(Canonical(ObjectValue(O)) + TEXT("\n")).Equals(Text,ESearchCase::CaseSensitive))
    { return Fail(TEXT("Noncanonical, duplicate-key or malformed IR")); }
    if (!Keys(O, {TEXT("schema_version"),TEXT("content_kind"),TEXT("extractor_version"),TEXT("source_repository_revision"),TEXT("sources"),TEXT("entries"),TEXT("semantic_sha256")}))
    { return Fail(TEXT("Unknown or missing IR fields")); }
    int64 Schema;
    if (!Integer(O,TEXT("schema_version"),Schema,1,1) || !Str(O,TEXT("content_kind"),C.ContentKind) || !C.ContentKind.Equals(TEXT("player_memory.starting_catalog"),ESearchCase::CaseSensitive) ||
        !Str(O,TEXT("extractor_version"),C.ExtractorVersion) || !C.ExtractorVersion.Equals(TEXT("memoria-starting-memory/1"),ESearchCase::CaseSensitive) ||
        !Str(O,TEXT("source_repository_revision"),C.SourceRepositoryRevision) || !Hex(C.SourceRepositoryRevision,40) ||
        !Str(O,TEXT("semantic_sha256"),C.SemanticSha256) || !Hex(C.SemanticSha256,64)) { return Fail(TEXT("Unsupported metadata")); }
    C.CatalogSchemaVersion = int32(Schema); C.SourceIrSha256 = Sha256(Text); C.ContentRevision = C.SemanticSha256;
    const TArray<TSharedPtr<FJsonValue>>* Sources;
    const TArray<FString> ExpectedSources = {TEXT("scripts/systems/memory_manager.gd"),TEXT("scripts/utils/journey_oath.gd"),TEXT("scenes/main/main.gd"),TEXT("scripts/core/game_manager.gd")};
    if (!O->TryGetArrayField(TEXT("sources"),Sources) || Sources->Num() != ExpectedSources.Num()) { return Fail(TEXT("Source inventory mismatch")); }
    C.Sources.Reset();
    for (int32 I=0; I<Sources->Num(); ++I)
    {
        auto S = (*Sources)[I]->Type == EJson::Object ? (*Sources)[I]->AsObject() : nullptr; FMemoriaCatalogSource P;
        if (!Keys(S,{TEXT("path"),TEXT("sha256_utf8_lf")}) || !Str(S,TEXT("path"),P.Path) || !P.Path.Equals(ExpectedSources[I],ESearchCase::CaseSensitive) ||
            !Str(S,TEXT("sha256_utf8_lf"),P.Sha256Utf8Lf) || !Hex(P.Sha256Utf8Lf,64)) { return Fail(TEXT("Invalid source provenance")); }
        if (bVerifySources)
        {
            FString Source;
            if (!FFileHelper::LoadFileToString(Source, *(FPaths::ProjectDir()/TEXT("../../")/P.Path))) { return Fail(TEXT("Source unavailable")); }
            Source.ReplaceInline(TEXT("\r\n"),TEXT("\n"));
            if (Sha256(Source) != P.Sha256Utf8Lf) { return Fail(TEXT("Source hash disagrees with IR")); }
        }
        C.Sources.Add(P);
    }
    const TArray<TSharedPtr<FJsonValue>>* Entries;
    if (!O->TryGetArrayField(TEXT("entries"),Entries) || Entries->IsEmpty()) { return Fail(TEXT("Empty/malformed entries")); }
    C.Definitions.Reset(); C.LocalizedText.Reset();
    for (const auto& V : *Entries)
    {
        auto R = V->Type == EJson::Object ? V->AsObject() : nullptr;
        if (!Keys(R,{TEXT("id"),TEXT("title"),TEXT("description"),TEXT("grade"),TEXT("burn_power"),TEXT("story_effect"),TEXT("related_npc"),TEXT("localization_ko")}))
        { return Fail(TEXT("Unknown definition field, including mutable state")); }
        FMemoriaMemoryDefinition D; int64 Grade;
        if (!Str(R,TEXT("id"),D.Id,true) || !Str(R,TEXT("title"),D.Title,true) || !Str(R,TEXT("description"),D.Description,true) ||
            !Str(R,TEXT("story_effect"),D.StoryEffect) || !Str(R,TEXT("related_npc"),D.RelatedNpc) ||
            !Integer(R,TEXT("grade"),Grade,0,4) || !Integer(R,TEXT("burn_power"),D.BurnPower,1,MAX_int32)) { return Fail(TEXT("Invalid definition value")); }
        if (C.Definitions.ContainsByPredicate([&](const auto& Prior){return Prior.Id.Equals(D.Id,ESearchCase::CaseSensitive);})) { return Fail(TEXT("Exact duplicate ID")); }
        D.RawGrade = static_cast<EMemoriaMemoryGrade>(Grade); D.SourcePath = C.Sources[0].Path; D.SourceHash = C.Sources[0].Sha256Utf8Lf;
        D.TextId = TEXT("player_memory/") + D.Id;
        const TSharedPtr<FJsonObject>* K;
        if (!R->TryGetObjectField(TEXT("localization_ko"),K) || !Keys(*K,{TEXT("title"),TEXT("description"),TEXT("story_effect")})) { return Fail(TEXT("Invalid localization")); }
        FMemoriaMemoryLocalizedText L; L.MemoryId = D.Id; L.Locale = TEXT("ko");
        if (!Str(*K,TEXT("title"),L.Title,true) || !Str(*K,TEXT("description"),L.Description,true)) { return Fail(TEXT("Missing localization text")); }
        L.bHasStoryEffect = (*K)->Values[TEXT("story_effect")]->Type != EJson::Null;
        if (L.bHasStoryEffect && !Str(*K,TEXT("story_effect"),L.StoryEffect)) { return Fail(TEXT("Invalid localized effect")); }
        C.Definitions.Add(D); C.LocalizedText.Add(L);
    }
    if (Fingerprint(C) != C.SemanticSha256) { return Fail(TEXT("Semantic fingerprint mismatch")); }
    return true;
}
bool Import(const FString& Path, bool bCheckOnly, TSharedPtr<FJsonObject>& Report, FString& Error)
{
    TStrongObjectPtr<UMemoriaMemoryCatalog> Candidate(NewObject<UMemoriaMemoryCatalog>());
    if (!ReadIr(Path,*Candidate,Error)) { return false; }
    auto* Existing = FPackageName::DoesPackageExist(PackagePath) ? LoadObject<UMemoriaMemoryCatalog>(nullptr,ObjectPath,nullptr,LOAD_NoWarn|LOAD_Quiet) : nullptr;
    Report = MakeShared<FJsonObject>(); Report->SetStringField(TEXT("asset"),ObjectPath);
    Report->SetStringField(TEXT("ir_sha256"),Candidate->SourceIrSha256); Report->SetStringField(TEXT("semantic_sha256"),Candidate->SemanticSha256);
    Report->SetNumberField(TEXT("entry_count"),Candidate->Definitions.Num());
    if (Existing)
    {
        if (Fingerprint(*Existing) != Existing->SemanticSha256) { Error=TEXT("Generated asset was edited; preserve designer changes for review"); return false; }
        bool Same = true;
        for (TFieldIterator<FProperty> P(UMemoriaMemoryCatalog::StaticClass(), EFieldIterationFlags::None); P; ++P)
        { Same &= P->Identical_InContainer(Existing,Candidate.Get()); }
        if (Same) { Report->SetStringField(TEXT("result"),TEXT("UNCHANGED")); Report->SetBoolField(TEXT("saved"),false); return true; }
    }
    if (bCheckOnly) { Error=TEXT("Asset missing or differs; check-only does not import"); return false; }
    const FString Filename = FPackageName::LongPackageNameToFilename(PackagePath,FPackageName::GetAssetPackageExtension());
    if (!Existing && FPaths::FileExists(Filename)) { Error=TEXT("Existing package is not a readable catalog; refusing overwrite"); return false; }
    auto* Package = Existing ? Existing->GetOutermost() : CreatePackage(PackagePath);
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename),true);
    auto* Asset = Existing ? Existing : NewObject<UMemoriaMemoryCatalog>(Package,TEXT("DA_StartingMemoryCatalog"),RF_Public|RF_Standalone);
    for (TFieldIterator<FProperty> P(UMemoriaMemoryCatalog::StaticClass(), EFieldIterationFlags::None); P; ++P)
    { P->CopyCompleteValue_InContainer(Asset,Candidate.Get()); }
    if (!Existing) { FAssetRegistryModule::AssetCreated(Asset); }
    Asset->MarkPackageDirty(); FSavePackageArgs Args; Args.TopLevelFlags=RF_Public|RF_Standalone;
    if (!UPackage::SavePackage(Package,Asset,*Filename,Args)) { Error=TEXT("Catalog package save failed"); return false; }
    Report->SetStringField(TEXT("result"),Existing ? TEXT("UPDATED") : TEXT("CREATED")); Report->SetBoolField(TEXT("saved"),true);
    return true;
}
}
