param([switch]$Editor, [string]$Engine = 'C:\Program Files\Epic Games\UE_5.7')
$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$logFile = Join-Path $projectRoot 'Saved\Logs\FieldVerification.log'
New-Item -ItemType Directory -Force -Path (Split-Path $logFile) | Out-Null
$flags = '-EvaFieldTest -RenderOffscreen -windowed -ForceRes -ResX=1600 -ResY=900 -unattended -nosplash'
if ($Editor) {
    $game = Join-Path $Engine 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
    $arguments = '"{0}\Eva.uproject" -game {1} -abslog="{2}"' -f $projectRoot,$flags,$logFile
} else {
    $game = Join-Path $projectRoot 'Builds\Windows\Eva.exe'
    $arguments = '{0} -abslog="{1}"' -f $flags,$logFile
}
$run = Start-Process -FilePath $game -WorkingDirectory $projectRoot -ArgumentList $arguments -WindowStyle Hidden -PassThru
if (!$run.WaitForExit(120000)) { $run.Kill($true); throw "A.T. field verification timed out. See $logFile" }
if ($run.ExitCode -ne 0) { throw "A.T. field verification failed: $($run.ExitCode). See $logFile" }
$result = Select-String -LiteralPath $logFile -Pattern 'EVA_FIELD_RESULT success=1'
if (!$result) { throw "Missing successful A.T. field result. See $logFile" }
Write-Host $result.Line
