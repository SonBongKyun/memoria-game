[CmdletBinding()]
param(
    [string]$GodotPath = "",
    [int]$TimeoutSeconds = 90
)

$ErrorActionPreference = "Stop"
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path

if ([string]::IsNullOrWhiteSpace($GodotPath)) {
    foreach ($candidate in @("godot", "godot4", "Godot_v4.6.2-stable_win64_console")) {
        $command = Get-Command $candidate -ErrorAction SilentlyContinue
        if ($null -ne $command) {
            $GodotPath = $command.Source
            break
        }
    }
}

if ([string]::IsNullOrWhiteSpace($GodotPath) -or -not (Test-Path -LiteralPath $GodotPath)) {
    throw "Godot executable not found. Pass -GodotPath with the Godot console executable."
}

$cases = @(
    [pscustomobject]@{
        Name = "smoke_test_runner_contract"
        Scene = "res://scripts/tools/smoke_test_runner_contract.tscn"
        Marker = "SMOKE_TEST_RUNNER_CONTRACT_PASS"
    },
    [pscustomobject]@{
        Name = "memory_world_engine"
        Scene = "res://scripts/tools/smoke_memory_world_engine.tscn"
        Marker = "MEMORY_WORLD_ENGINE_SMOKE_PASS"
    },
    [pscustomobject]@{
        Name = "save_migration_fixtures"
        Scene = "res://scripts/tools/smoke_save_migration_fixtures.tscn"
        Marker = "SAVE_MIGRATION_FIXTURES_SMOKE_PASS"
    },
    [pscustomobject]@{
        Name = "actor_registry"
        Scene = "res://scripts/tools/smoke_actor_registry.tscn"
        Marker = "ACTOR_REGISTRY_SMOKE_PASS"
    }
)

$fatalPattern = "(?im)Infinite loop|SCRIPT ERROR|Parse Error|Assertion failed|Invalid call|Invalid access|Lambda capture .* was freed|\[SMOKE\]\[(?:FAIL|SUITE_FAIL)\]"

foreach ($case in $cases) {
    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $GodotPath
    $startInfo.WorkingDirectory = $projectRoot
    $startInfo.UseShellExecute = $false
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $startInfo.CreateNoWindow = $true
    $escapedRoot = $projectRoot.Replace('"', '\"')
    $startInfo.Arguments = "--headless --path `"$escapedRoot`" --scene $($case.Scene)"

    $process = [System.Diagnostics.Process]::new()
    $process.StartInfo = $startInfo
    [void]$process.Start()
    $stdoutTask = $process.StandardOutput.ReadToEndAsync()
    $stderrTask = $process.StandardError.ReadToEndAsync()

    if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
        $process.Kill($true)
        throw "Smoke timed out after $TimeoutSeconds seconds [$($case.Name)]: $($case.Scene)"
    }
    $process.WaitForExit()
    $output = (($stdoutTask.GetAwaiter().GetResult() + "`n" + $stderrTask.GetAwaiter().GetResult()).Trim())
    $fatalMatches = [regex]::Matches($output, $fatalPattern)
    $hasMarker = $output.Contains($case.Marker)

    if ($process.ExitCode -ne 0 -or -not $hasMarker -or $fatalMatches.Count -gt 0) {
        Write-Host $output
        $reasons = @()
        if ($process.ExitCode -ne 0) { $reasons += "exit=$($process.ExitCode)" }
        if (-not $hasMarker) { $reasons += "missing=$($case.Marker)" }
        if ($fatalMatches.Count -gt 0) {
            $fatalNames = @($fatalMatches | ForEach-Object Value) | Sort-Object -Unique
            $reasons += "fatal=$($fatalNames -join ',')"
        }
        throw "Smoke failed [$($case.Name)] ($($reasons -join '; '))"
    }

    Write-Host "[SMOKE][PASS][$($case.Name)] marker=$($case.Marker)"
}

Write-Host "MEMORY_WORLD_ENGINE_SUITE_PASS cases=$($cases.Count) fatal_scan=enabled"
