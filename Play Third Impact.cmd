@echo off
set "GAME=%~dp0Builds\Windows\Eva.exe"
if not exist "%GAME%" (
  echo Package the game with Scripts\Package.ps1 first.
  pause
  exit /b 1
)
start "EVA - Third Impact" /D "%~dp0Builds\Windows" "%GAME%" -EvaImpact -windowed -ResX=1600 -ResY=900
