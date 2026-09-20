param([string]$Engine = 'C:\Program Files\Epic Games\UE_5.7')
$ErrorActionPreference = 'Stop'
$project = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\Eva.uproject'))
& "$Engine\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" $project -unattended -nullrhi -nosplash '-ExecCmds=Automation RunTests Eva.Rules' '-TestExit=Automation Test Queue Empty' "-ReportExportPath=$PSScriptRoot\..\Saved\TestReport" -log
if ($LASTEXITCODE -ne 0) { throw 'Unreal automation tests failed.' }
