@echo off
set "GAME=%~dp0Builds\Windows\Eva.exe"
if not exist "%GAME%" (
  echo Package the game with Scripts\Package.ps1 first.
  pause
  exit /b 1
)
start "EVA - Free Roam" /D "%~dp0Builds\Windows" "%GAME%" -EvaWorld -windowed -ResX=1600 -ResY=900
