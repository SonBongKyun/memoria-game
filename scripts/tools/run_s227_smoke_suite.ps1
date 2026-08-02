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
    throw "Godot executable not found. Pass -GodotPath with the Godot 4.6.2 console executable."
}

$cases = @(
    [pscustomobject]@{
        Scene = "res://scripts/tools/smoke_burn_directive_stabilization.tscn"
        Marker = "BURN_DIRECTIVE_STABILIZATION_SMOKE_PASS"
    },
    [pscustomobject]@{
        Scene = "res://scripts/tools/smoke_early_loop.tscn"
        Marker = "EARLY_LOOP_SMOKE_PASS"
    },
    [pscustomobject]@{
        Scene = "res://scripts/tools/smoke_tactical_directives.tscn"
        Marker = "TACTICAL_DIRECTIVES_SMOKE_PASS"
    },
    [pscustomobject]@{
        Scene = "res://scripts/tools/smoke_story_combat.tscn"
        Marker = "STORY_COMBAT_SMOKE_PASS"
    },
    [pscustomobject]@{
        Scene = "res://scripts/tools/smoke_visual_clarity.tscn"
        Marker = "VISUAL_CLARITY_SMOKE_PASS"
    },
    [pscustomobject]@{
        Scene = "res://scripts/tools/smoke_crash_guards.tscn"
        Marker = "CRASH_GUARDS_SMOKE_PASS"
    }
)

$fatalPattern = "(?im)Infinite loop|SCRIPT ERROR|Parse Error|Assertion failed|Invalid call|Invalid access"

foreach ($case in $cases) {
    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $GodotPath
    $startInfo.WorkingDirectory = $projectRoot
    $startInfo.UseShellExecute = $false
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $startInfo.CreateNoWindow = $true
    $escapedRoot = $projectRoot.Replace('"', '\"')
    $startInfo.Arguments = "--headless --path `"$escapedRoot`" `"$($case.Scene)`""

    $process = [System.Diagnostics.Process]::new()
    $process.StartInfo = $startInfo
    [void]$process.Start()
    $stdoutTask = $process.StandardOutput.ReadToEndAsync()
    $stderrTask = $process.StandardError.ReadToEndAsync()

    if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
        $process.Kill($true)
        throw "Timed out after $TimeoutSeconds seconds: $($case.Scene)"
    }
    $process.WaitForExit()
    $output = (($stdoutTask.GetAwaiter().GetResult() + "`n" + $stderrTask.GetAwaiter().GetResult()).Trim())
    $hasMarker = $output.Contains($case.Marker)
    $fatalMatches = [regex]::Matches($output, $fatalPattern)

    if ($process.ExitCode -ne 0 -or -not $hasMarker -or $fatalMatches.Count -gt 0) {
        Write-Host $output
        $reasons = @()
        if ($process.ExitCode -ne 0) { $reasons += "exit=$($process.ExitCode)" }
        if (-not $hasMarker) { $reasons += "missing=$($case.Marker)" }
        if ($fatalMatches.Count -gt 0) {
            $reasons += "fatal=$((@($fatalMatches | ForEach-Object Value) | Sort-Object -Unique) -join ',')"
        }
        throw "Smoke failed ($($reasons -join '; ')): $($case.Scene)"
    }

    Write-Host "$($case.Marker) verified; no fatal diagnostics"
}

Write-Host "S227_SMOKE_SUITE_PASS cases=$($cases.Count) fatal_scan=enabled"
