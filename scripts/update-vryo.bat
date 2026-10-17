@echo off
rem Vryo Control: update from GitHub and rebuild in one step.
rem Run from PowerShell in the project folder:  .\scripts\update-vryo.bat
rem The full build output is also saved to build-log.txt. The build counts as
rem successful when "OpenRGB Windows 64-bit\VryoControl.exe" exists at the end;
rem if not, the first error lines are shown again.
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
    if exist build-log.txt del build-log.txt
    echo Building, this takes a few minutes...
    cmd /c "scripts\build-windows.bat 6.8.3 2022 64 2>&1" | powershell -NoProfile -Command "$input | ForEach-Object { $_; Add-Content -Path build-log.txt -Value $_ -Encoding ascii }"
    if exist "OpenRGB Windows 64-bit\VryoControl.exe" (
        echo.
        powershell -NoProfile -ExecutionPolicy Bypass -File "scripts\create-start-menu-shortcut.ps1"
        echo ===== BUILD OK. Press the Windows key, type vryo and click Vryo Control. =====
        exit /b 0
    )
    echo.
    echo ===== BUILD FAILED. First error lines: =====
    findstr /C:"error C" /C:"fatal error" /C:"Error 2" build-log.txt > build-errors.txt
    powershell -NoProfile -Command "Get-Content build-errors.txt -TotalCount 12"
    echo ===== Copy the lines above and send them. =====
    exit /b 1
)
