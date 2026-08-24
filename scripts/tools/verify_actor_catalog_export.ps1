[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$GodotPath,
    [string]$ProjectRoot = "",
    [int]$TimeoutSeconds = 180
)

$ErrorActionPreference = "Stop"
if ([string]::IsNullOrWhiteSpace($ProjectRoot)) {
    $ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
} else {
    $ProjectRoot = (Resolve-Path -LiteralPath $ProjectRoot).Path
}

if (-not (Test-Path -LiteralPath $GodotPath -PathType Leaf)) {
    throw "Godot executable not found: $GodotPath"
}

$fatalPattern = "(?im)Infinite loop|SCRIPT ERROR|Parse Error|Assertion failed|Invalid call|Invalid access|Lambda capture .* was freed|\[SMOKE\]\[(?:FAIL|SUITE_FAIL)\]|\[SAVE_PATH_GUARD\]\[FATAL\]"
$tempBase = [System.IO.Path]::GetFullPath([System.IO.Path]::GetTempPath()).TrimEnd('\', '/')
$tempLeaf = "memoria-mwe-export-" + [guid]::NewGuid().ToString("N")
$tempRoot = Join-Path $tempBase $tempLeaf
[void](New-Item -ItemType Directory -Path $tempRoot -ErrorAction Stop)

function Invoke-GodotChecked {
    param(
        [string]$Arguments,
        [string]$WorkingDirectory,
        [string]$CaseName,
        [string]$RequiredMarker = "",
        [bool]$ScanFatal = $true
    )

    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $GodotPath
    $startInfo.WorkingDirectory = $WorkingDirectory
    $startInfo.UseShellExecute = $false
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $startInfo.CreateNoWindow = $true
    $startInfo.Arguments = $Arguments

    $process = [System.Diagnostics.Process]::new()
    $process.StartInfo = $startInfo
    [void]$process.Start()
    $stdoutTask = $process.StandardOutput.ReadToEndAsync()
    $stderrTask = $process.StandardError.ReadToEndAsync()
    if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
        $process.Kill($true)
        throw "$CaseName timed out after $TimeoutSeconds seconds"
    }
    $process.WaitForExit()
    $output = (($stdoutTask.GetAwaiter().GetResult() + "`n" + $stderrTask.GetAwaiter().GetResult()).Trim())
    $fatalMatches = if ($ScanFatal) { [regex]::Matches($output, $fatalPattern) } else { @() }
    $hasMarker = [string]::IsNullOrEmpty($RequiredMarker) -or $output.Contains($RequiredMarker)
    if ($process.ExitCode -ne 0 -or -not $hasMarker -or $fatalMatches.Count -gt 0) {
        Write-Host $output
        throw "$CaseName failed: exit=$($process.ExitCode) marker=$hasMarker fatal=$($fatalMatches.Count)"
    }
    return $output
}

try {
    $packPath = Join-Path $tempRoot "memoria-actor-catalog-export.pck"
    $escapedProject = $ProjectRoot.Replace('"', '\"')
    $escapedPack = $packPath.Replace('"', '\"')
    # Export serialization is exit/PCK checked, while the exported smoke below
    # receives the fatal scan. Some existing editor plugin resources emit
    # engine-level VisualShader ERROR lines during pack construction even when
    # Godot completes the pack successfully; those are surfaced separately.
    $exportOutput = Invoke-GodotChecked `
        -Arguments "--headless --path `"$escapedProject`" --export-pack `"Windows Desktop (Demo)`" `"$escapedPack`"" `
        -WorkingDirectory $ProjectRoot `
        -CaseName "actor_catalog_export_pack" `
        -ScanFatal $false
    $exportErrorCount = ([regex]::Matches($exportOutput, "(?m)^ERROR:")).Count
    if ($exportErrorCount -gt 0) {
        Write-Warning "actor_catalog_export_pack completed with existing resource errors=$exportErrorCount"
    }

    if (-not (Test-Path -LiteralPath $packPath -PathType Leaf)) {
        throw "Export completed without producing the PCK: $packPath"
    }
    $packLength = (Get-Item -LiteralPath $packPath).Length
    if ($packLength -le 0) {
        throw "Exported PCK is empty: $packPath"
    }

    $runtimeMarker = "ACTOR_CATALOG_SMOKE_PASS suite=actor_catalog actors=4 schema=1 runtime_path=res://data/world_state/actors.json"
    [void](Invoke-GodotChecked `
        -Arguments "--headless --main-pack `"$escapedPack`" --scene res://scripts/tools/smoke_actor_catalog.tscn -- --smoke-test" `
        -WorkingDirectory $tempRoot `
        -CaseName "actor_catalog_export_runtime" `
        -RequiredMarker $runtimeMarker)

    Write-Host "ACTOR_CATALOG_EXPORT_PASS preset=Windows_Desktop_Demo path=res://data/world_state/actors.json pack_bytes=$packLength export_log_errors=$exportErrorCount runtime_fatal_scan=enabled"
}
finally {
    if (Test-Path -LiteralPath $tempRoot -PathType Container) {
        $resolvedRoot = (Resolve-Path -LiteralPath $tempRoot).Path
        $resolvedParent = [System.IO.Path]::GetFullPath((Split-Path -Parent $resolvedRoot)).TrimEnd('\', '/')
        $resolvedLeaf = Split-Path -Leaf $resolvedRoot
        if ($resolvedParent -ne $tempBase -or -not $resolvedLeaf.StartsWith("memoria-mwe-export-")) {
            throw "Refusing unsafe export temp cleanup target: $resolvedRoot"
        }
        Remove-Item -LiteralPath $resolvedRoot -Recurse -Force
    }
}
