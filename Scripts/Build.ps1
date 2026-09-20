param([string]$Engine = 'C:\Program Files\Epic Games\UE_5.7')
$ErrorActionPreference = 'Stop'
$project = Join-Path $PSScriptRoot '..\Eva.uproject'
$project = [IO.Path]::GetFullPath($project)
python "$PSScriptRoot\generate_models.py"
if ($LASTEXITCODE -ne 0) { throw 'Model generation failed.' }
python "$PSScriptRoot\generate_audio.py"
if ($LASTEXITCODE -ne 0) { throw 'Audio generation failed.' }
& "$Engine\Engine\Build\BatchFiles\Build.bat" EvaEditor Win64 Development "-Project=$project" -WaitMutex -NoHotReloadFromIDE
if ($LASTEXITCODE -ne 0) { throw 'Unreal compilation failed.' }
& "$Engine\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" $project -run=pythonscript "-script=$PSScriptRoot\bootstrap_content.py" -unattended -nullrhi -AllowCommandletAudio -nosplash
if ($LASTEXITCODE -ne 0) { throw 'Content generation failed.' }

& "$Engine\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" $project -run=pythonscript "-script=$PSScriptRoot\import_models.py" -unattended -nullrhi -nosplash
if ($LASTEXITCODE -ne 0) { throw 'Model import failed.' }
