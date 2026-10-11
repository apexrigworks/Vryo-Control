@echo off
rem Vryo Control: update from GitHub and rebuild in one step.
rem Run from PowerShell in the project folder:  .\scripts\update-vryo.bat
rem (everything is inside one block so Windows reads it before git pull can change this file)
cd /d "%~dp0.."
(
    echo Closing Vryo Control if it is running...
    taskkill /F /IM VryoControl.exe >nul 2>&1
    echo Getting the latest code...
    git pull
    echo Removing the old build...
    if exist "OpenRGB Windows 64-bit" rmdir /S /Q "OpenRGB Windows 64-bit"
    if exist release rmdir /S /Q release
    echo Building...
    call scripts\build-windows.bat 6.8.3 2022 64
    exit /b %ERRORLEVEL%
)
