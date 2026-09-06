#include "Misc/AutomationTest.h"
#include "MemoryParityFixtures.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FMemoriaMemoryParityTest, "Memoria.Memory.SourceParity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

void FMemoriaMemoryParityTest::GetTests(TArray<FString>& OutNames, TArray<FString>& OutCommands) const
{
    for (const auto& Fixture : Memoria::Tests::Fixtures())
    {
        const FString Id(UTF8_TO_TCHAR(Fixture.Id.c_str()));
        OutNames.Add(Id); OutCommands.Add(Id);
    }
}
bool FMemoriaMemoryParityTest::RunTest(const FString& Parameters)
{
    for (const auto& Fixture : Memoria::Tests::Fixtures())
    {
        if (!Parameters.Equals(UTF8_TO_TCHAR(Fixture.Id.c_str()), ESearchCase::CaseSensitive)) { continue; }
        const auto Errors = Memoria::Tests::Run(Fixture);
        for (const auto& Error : Errors) { AddError(UTF8_TO_TCHAR(Error.c_str())); }
        return Errors.empty();
    }
    AddError(TEXT("Unknown memory fixture; no test was executed"));
    return false;
}
#endif
