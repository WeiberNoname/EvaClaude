param([switch]$Render)
$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$game = Join-Path $projectRoot 'Builds\Windows\Eva.exe'
if (!(Test-Path -LiteralPath $game)) { throw 'Package the standalone game before running this check.' }
$logDir = Join-Path $projectRoot 'Saved\Logs'
New-Item -ItemType Directory -Path $logDir -Force | Out-Null
$logFile = Join-Path $logDir 'StandaloneChapter.log'
$mode = if ($Render) { '-RenderOffscreen -EvaChapterShots' } else { '-nullrhi -EvaChapterTest' }
$arguments = '{0} -unattended -nosplash -abslog="{1}"' -f $mode,$logFile
$testProcess = Start-Process -FilePath $game -WorkingDirectory (Split-Path $game) -ArgumentList $arguments -WindowStyle Hidden -PassThru -Wait
if ($testProcess.ExitCode -ne 0) { throw "Chapter run failed with exit code $($testProcess.ExitCode). See $logFile" }
$result = Select-String -LiteralPath $logFile -Pattern 'EVA_CHAPTER_RESULT success=1 .*checkpoint=1'
if (!$result) { throw "Chapter did not report successful equipment, story, and checkpoint validation. See $logFile" }
Write-Host $result.Line
