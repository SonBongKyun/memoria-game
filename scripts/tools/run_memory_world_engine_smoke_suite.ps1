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

# No smoke scene may name the production root or perform direct FileAccess
# writes. The single write implementation belongs to smoke_save_sandbox.gd and
# is protected by SaveManager.guard_test_write_target at runtime.
$smokeSources = Get-ChildItem -LiteralPath (Join-Path $projectRoot "scripts\tools") -Filter "smoke_*.gd" -File
foreach ($sourceFile in $smokeSources) {
    $sourceText = Get-Content -LiteralPath $sourceFile.FullName -Raw
    if ($sourceText.Contains("user://saves")) {
        throw "Production save literal found in smoke source: $($sourceFile.Name)"
    }
    if ($sourceFile.Name -ne "smoke_save_sandbox.gd" -and $sourceText.Contains("FileAccess.WRITE")) {
        throw "Direct FileAccess write found outside smoke sandbox: $($sourceFile.Name)"
    }
}
Write-Host "SAVE_PATH_ISOLATION_STATIC_PASS smoke_sources=$($smokeSources.Count) direct_write_helpers=1"

$cases = @(
    [pscustomobject]@{
        Name = "smoke_test_runner_contract"
        Scene = "res://scripts/tools/smoke_test_runner_contract.tscn"
        Marker = "SMOKE_TEST_RUNNER_CONTRACT_PASS"
        ExpectedExit = 0
        RequiredLog = ""
        ExtraArgs = ""
    },
    [pscustomobject]@{
        Name = "production_save_path_guard"
        Scene = "res://scripts/tools/smoke_production_save_path_guard.tscn"
        Marker = ""
        ExpectedExit = 1
        RequiredLog = "[SAVE_PATH_GUARD][FATAL] operation=configure_test_root target=user://saves"
        ExtraArgs = ""
    },
    [pscustomobject]@{
        Name = "outside_temp_save_path_guard"
        Scene = "res://scripts/tools/smoke_production_save_path_guard.tscn"
        Marker = ""
        ExpectedExit = 1
        RequiredLog = "[SAVE_PATH_GUARD][FATAL] operation=configure_test_root target=user://not_allowed_smoke_saves"
        ExtraArgs = "--guard-outside-temp"
    },
    [pscustomobject]@{
        Name = "crash_guards_isolated"
        Scene = "res://scripts/tools/smoke_crash_guards.tscn"
        Marker = "CRASH_GUARDS_ISOLATED_SMOKE_PASS"
        ExpectedExit = 0
        RequiredLog = "isolated_save_root=true"
        ExtraArgs = ""
    },
    [pscustomobject]@{
        Name = "memory_world_engine"
        Scene = "res://scripts/tools/smoke_memory_world_engine.tscn"
        Marker = "MEMORY_WORLD_ENGINE_SMOKE_PASS"
        ExpectedExit = 0
        RequiredLog = ""
        ExtraArgs = ""
    },
    [pscustomobject]@{
        Name = "save_migration_fixtures"
        Scene = "res://scripts/tools/smoke_save_migration_fixtures.tscn"
        Marker = "SAVE_MIGRATION_FIXTURES_SMOKE_PASS"
        ExpectedExit = 0
        RequiredLog = "user_slots_touched=0"
        ExtraArgs = ""
    },
    [pscustomobject]@{
        Name = "actor_catalog"
        Scene = "res://scripts/tools/smoke_actor_catalog.tscn"
        Marker = "ACTOR_CATALOG_SMOKE_PASS"
        ExpectedExit = 0
        RequiredLog = "atomic_rejection=true"
        ExtraArgs = ""
    },
    [pscustomobject]@{
        Name = "actor_registry"
        Scene = "res://scripts/tools/smoke_actor_registry.tscn"
        Marker = "ACTOR_REGISTRY_SMOKE_PASS"
        ExpectedExit = 0
        RequiredLog = "unknown_access=validated"
        ExtraArgs = ""
    },
    [pscustomobject]@{
        Name = "world_event_schema"
        Scene = "res://scripts/tools/smoke_world_event_schema.tscn"
        Marker = "WORLD_EVENT_SCHEMA_SMOKE_PASS"
        ExpectedExit = 0
        RequiredLog = "replay_on_load=0"
        ExtraArgs = ""
    }
)

$fatalPattern = "(?im)Infinite loop|SCRIPT ERROR|Parse Error|Assertion failed|Invalid call|Invalid access|Lambda capture .* was freed|\[SMOKE\]\[(?:FAIL|SUITE_FAIL)\]|\[SAVE_PATH_GUARD\]\[FATAL\]"

foreach ($case in $cases) {
    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $GodotPath
    $startInfo.WorkingDirectory = $projectRoot
    $startInfo.UseShellExecute = $false
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $startInfo.CreateNoWindow = $true
    $escapedRoot = $projectRoot.Replace('"', '\"')
    $startInfo.Arguments = "--headless --path `"$escapedRoot`" --scene $($case.Scene) -- --smoke-test $($case.ExtraArgs)"

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

    if ($case.ExpectedExit -eq 1) {
        $hasRequiredLog = $output.Contains($case.RequiredLog)
        $hasUnexpectedPass = $output.Contains("_PASS")
        if ($process.ExitCode -ne 1 -or -not $hasRequiredLog -or $hasUnexpectedPass) {
            Write-Host $output
            throw "Expected path-guard failure did not satisfy the contract [$($case.Name)] exit=$($process.ExitCode)"
        }
        Write-Host "[SMOKE][PASS][$($case.Name)] expected_exit=1 target_logged=true"
        continue
    }

    $fatalMatches = [regex]::Matches($output, $fatalPattern)
    $hasMarker = $output.Contains($case.Marker)
    $hasRequiredLog = [string]::IsNullOrEmpty($case.RequiredLog) -or $output.Contains($case.RequiredLog)

    if ($process.ExitCode -ne 0 -or -not $hasMarker -or -not $hasRequiredLog -or $fatalMatches.Count -gt 0) {
        Write-Host $output
        $reasons = @()
        if ($process.ExitCode -ne 0) { $reasons += "exit=$($process.ExitCode)" }
        if (-not $hasMarker) { $reasons += "missing=$($case.Marker)" }
        if (-not $hasRequiredLog) { $reasons += "missing_log=$($case.RequiredLog)" }
        if ($fatalMatches.Count -gt 0) {
            $fatalNames = @($fatalMatches | ForEach-Object Value) | Sort-Object -Unique
            $reasons += "fatal=$($fatalNames -join ',')"
        }
        throw "Smoke failed [$($case.Name)] ($($reasons -join '; '))"
    }

    Write-Host "[SMOKE][PASS][$($case.Name)] marker=$($case.Marker)"
}

Write-Host "MEMORY_WORLD_ENGINE_SUITE_PASS cases=$($cases.Count) fatal_scan=enabled save_isolation=guarded"
