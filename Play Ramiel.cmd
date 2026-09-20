@echo off
set "GAME=%~dp0Builds\Windows\Eva.exe"
if not exist "%GAME%" (
  echo Run Scripts\Package.ps1 first.
  pause
  exit /b 1
)
start "EVA - Ramiel" /D "%~dp0Builds\Windows" "%GAME%" -EvaRamiel -windowed -ResX=1600 -ResY=900
