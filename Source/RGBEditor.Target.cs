using UnrealBuildTool;
using System.Collections.Generic;

public class RGBEditorTarget : TargetRules
{
    public RGBEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V6;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("RGB");
    }
}
