param([string]$Engine = 'C:\Program Files\Epic Games\UE_5.7')
$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$project = Join-Path $projectRoot 'Eva.uproject'
$archive = Join-Path $projectRoot 'Builds'
& "$Engine\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun "-project=$project" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive "-archivedirectory=$archive" -prereqs -unattended -utf8output -NoCompileEditor
if ($LASTEXITCODE -ne 0) { throw 'Standalone Windows packaging failed.' }
$game = Join-Path $archive 'Windows\Eva.exe'
if (!(Test-Path -LiteralPath $game)) { throw "Packaged executable not found: $game" }
$shortcutShell = New-Object -ComObject WScript.Shell
$shortcutPath = Join-Path $projectRoot 'Play EVA.lnk'
# Recreate the generated link: Shell target tracking can retain the pre-archive executable.
if (Test-Path -LiteralPath $shortcutPath) { Remove-Item -LiteralPath $shortcutPath -Force }
$shortcut = $shortcutShell.CreateShortcut($shortcutPath)
$launcher = Join-Path $projectRoot 'Play.cmd'
$shortcut.TargetPath = $launcher
$shortcut.WorkingDirectory = $projectRoot
$shortcut.Arguments = ''
$shortcut.Description = 'Launch EVA // LAST SIGNAL (standalone game)'
$shortcut.IconLocation = "$game,0"
$shortcut.Save()
if ($shortcutShell.CreateShortcut($shortcutPath).TargetPath -ne $launcher) { throw 'Could not verify the standalone shortcut target.' }
Write-Host "Ready: $game"
