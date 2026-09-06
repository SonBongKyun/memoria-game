#include "MemoriaNarrativeCommandlet.h"
#include "Import/MemoriaNarrativeImport.h"
#include "Import/MemoriaStartingCatalogImport.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
UMemoriaNarrativeCommandlet::UMemoriaNarrativeCommandlet() { IsClient=false; IsServer=false; IsEditor=true; LogToConsole=true; }
int32 UMemoriaNarrativeCommandlet::Main(const FString& Params)
{
    FString Ir,Output,Dialect,Error;
    if (!FParse::Value(*Params,TEXT("IR="),Ir) || !FParse::Value(*Params,TEXT("Report="),Output) || !FParse::Value(*Params,TEXT("Dialect="),Dialect) || !(Dialect.Equals(TEXT("vn"),ESearchCase::CaseSensitive) || Dialect.Equals(TEXT("field"),ESearchCase::CaseSensitive)) || FPaths::FileExists(Output)) return 1;
    TSharedPtr<FJsonObject> Report;
    bool Passed=MemoriaNarrativeImport::Import(Ir,Dialect==TEXT("vn"),FParse::Param(*Params,TEXT("CheckOnly")),Report,Error);
    if (!Report) Report=MakeShared<FJsonObject>(); Report->SetBoolField(TEXT("passed"),Passed); Report->SetStringField(TEXT("error"),Error);
    Passed &= FFileHelper::SaveStringToFile(MemoriaCatalogImport::Canonical(MakeShared<FJsonValueObject>(Report))+TEXT("\n"),*Output,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    UE_LOG(LogTemp,Display,TEXT("MEMORIA_NARRATIVE_IMPORT_%s %s"),Passed?TEXT("PASS"):TEXT("FAIL"),*Error); return Passed?0:1;
}
