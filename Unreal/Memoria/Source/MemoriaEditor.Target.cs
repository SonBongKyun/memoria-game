using UnrealBuildTool;
using System.Collections.Generic;

public class MemoriaEditorTarget : TargetRules
{
    public MemoriaEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V6;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
        ExtraModuleNames.AddRange(new string[] { "Memoria", "MemoriaTests" });
    }
}
