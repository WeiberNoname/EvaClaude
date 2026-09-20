param([switch]$Editor)
$ErrorActionPreference='Stop'
$projectRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$logFile=Join-Path $projectRoot 'Saved\Logs\WorldVerification.log'
New-Item -ItemType Directory -Force -Path (Split-Path $logFile) | Out-Null
if($Editor) {
    $game='C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
    $arguments='"{0}\Eva.uproject" -game -EvaWorldTest -RenderOffscreen -unattended -nosplash -abslog="{1}"' -f $projectRoot,$logFile
} else {
    $game=Join-Path $projectRoot 'Builds\Windows\Eva.exe'
    $arguments='-EvaWorldTest -RenderOffscreen -unattended -nosplash -abslog="{0}"' -f $logFile
}
$run=Start-Process -FilePath $game -WorkingDirectory $projectRoot -ArgumentList $arguments -WindowStyle Hidden -PassThru -Wait
if($run.ExitCode -ne 0) { throw "World verification failed: $($run.ExitCode). See $logFile" }
$result=Select-String -LiteralPath $logFile -Pattern 'EVA_WORLD_RESULT success=1 districts=15 contracts=1'
if(!$result) { throw "Missing successful world result. See $logFile" }
Write-Host $result.Line
