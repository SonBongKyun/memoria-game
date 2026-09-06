#pragma once
#include "Narrative/MemoriaNarrativeData.h"
#include "Dom/JsonObject.h"

namespace MemoriaNarrativeImport
{
FString Package(bool bVN);
FString ObjectPath(bool bVN);
FString Fingerprint(const UMemoriaFieldAsset& Asset);
FString Fingerprint(const UMemoriaVNAsset& Asset);
bool ReadIr(const FString& Path, UMemoriaFieldAsset& Asset, FString& Error, bool bVerifySources=true);
bool ReadIr(const FString& Path, UMemoriaVNAsset& Asset, FString& Error, bool bVerifySources=true);
bool Import(const FString& Path, bool bVN, bool bCheckOnly, TSharedPtr<FJsonObject>& Report, FString& Error);
}
