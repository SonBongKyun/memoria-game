using UnrealBuildTool;

public class Memoria : ModuleRules
{
    public Memoria(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateDependencyModuleNames.AddRange(new string[] { "Json", "AnimGraphRuntime" });
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;
        PublicDependencyModuleNames.AddRange(new string[] {
            "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "Paper2D", "UMG", "Slate", "SlateCore"
        });
    }
}
