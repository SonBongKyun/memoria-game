param([string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.8', [switch]$Continue)
$ErrorActionPreference = 'Stop'
$projectPath = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../Memoria/Memoria.uproject'))
$editorPath = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe'
if (!(Test-Path -LiteralPath $editorPath)) { throw "Unreal Editor not found: $editorPath" }
# Explicit interactive standalone slice, with no new asset authoring or source writes.
$mapPath = if ($Continue) { '/Game/Tests/Campaign/L_VerdanHost' } else { '/Game/Tests/Campaign/L_Ch2VerdanSlice' }
$playArguments = @($projectPath, $mapPath, '-game', '-windowed', '-ResX=1280', '-ResY=720', '-log')
if ($Continue) { $playArguments += '-MemoriaContinue' }
& $editorPath @playArguments
