#pragma once
#include "CoreMinimal.h"
#include "Domain/MemoriaMemoryTypes.h"
#include "Dom/JsonObject.h"

namespace MemoriaCatalogImport
{
inline const TCHAR* PackagePath = TEXT("/Game/Memoria/Generated/Memory/DA_StartingMemoryCatalog");
inline const TCHAR* ObjectPath = TEXT("/Game/Memoria/Generated/Memory/DA_StartingMemoryCatalog.DA_StartingMemoryCatalog");
FString Sha256(const FString& Text);
FString Canonical(const TSharedPtr<FJsonValue>& Value);
FString Fingerprint(const UMemoriaMemoryCatalog& Catalog);
bool ReadIr(const FString& Path, UMemoriaMemoryCatalog& Candidate, FString& Error, bool bVerifySources = true);
bool Import(const FString& Path, bool bCheckOnly, TSharedPtr<FJsonObject>& Report, FString& Error);
}
