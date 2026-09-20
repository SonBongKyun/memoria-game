using UnrealBuildTool;
using System.IO;

public class MemoriaTests : ModuleRules
{
    public MemoriaTests(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;
        bUseUnity = false;
        AddEngineThirdPartyPrivateStaticDependencies(Target, "OpenSSL");
        PrivateDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "Memoria", "UnrealEd", "UMG", "UMGEditor", "Slate", "SlateCore", "AssetRegistry", "InputCore", "EnhancedInput", "Paper2D", "Json", "JsonUtilities", "MeshDescription", "StaticMeshDescription", "SkeletalMeshDescription", "SkeletalMeshUtilitiesCommon" });
        PrivateIncludePaths.Add(Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../Tests/Shared")));
        PrivateIncludePaths.Add(Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../Tests/Generated")));
    }
}
