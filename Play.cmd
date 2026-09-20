@echo off
set "GAME=%~dp0Builds\Windows\Eva.exe"
if not exist "%GAME%" (
  echo The standalone game has not been packaged yet.
  echo Run Scripts\Package.ps1 to build it.
  pause
  exit /b 1
)
start "EVA - Last Signal" /D "%~dp0Builds\Windows" "%GAME%" -EvaWorld -windowed -ResX=1600 -ResY=900
