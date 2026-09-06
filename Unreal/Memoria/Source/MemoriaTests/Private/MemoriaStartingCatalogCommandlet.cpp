#include "MemoriaStartingCatalogCommandlet.h"
#include "Import/MemoriaStartingCatalogImport.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

UMemoriaStartingCatalogCommandlet::UMemoriaStartingCatalogCommandlet()
{ IsClient=false; IsServer=false; IsEditor=true; LogToConsole=true; }
int32 UMemoriaStartingCatalogCommandlet::Main(const FString& Params)
{
    FString Ir, Output, Error;
    if (!FParse::Value(*Params,TEXT("IR="),Ir) || !FParse::Value(*Params,TEXT("Report="),Output))
    { UE_LOG(LogTemp,Error,TEXT("Required: -IR=<canonical file> -Report=<new evidence file> [-CheckOnly]")); return 1; }
    if (FPaths::FileExists(Output)) { UE_LOG(LogTemp,Error,TEXT("Report already exists; choose a fresh evidence path")); return 1; }
    TSharedPtr<FJsonObject> Report;
    bool Passed = MemoriaCatalogImport::Import(Ir,FParse::Param(*Params,TEXT("CheckOnly")),Report,Error);
    if (!Report) { Report=MakeShared<FJsonObject>(); }
    Report->SetBoolField(TEXT("passed"),Passed); Report->SetStringField(TEXT("error"),Error);
    Passed &= FFileHelper::SaveStringToFile(MemoriaCatalogImport::Canonical(MakeShared<FJsonValueObject>(Report))+TEXT("\n"),*Output,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    UE_LOG(LogTemp,Display,TEXT("MEMORIA_STARTING_CATALOG_%s %s"),Passed ? TEXT("PASS") : TEXT("FAIL"),*Error);
    return Passed ? 0 : 1;
}
