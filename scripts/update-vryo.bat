@echo off
rem Vryo Control: update from GitHub and rebuild in one step.
rem Run from PowerShell in the project folder:  .\scripts\update-vryo.bat
rem The full build output is also saved to build-log.txt, and if the build
rem fails the first error lines are shown again at the end.
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
    powershell -NoProfile -Command "& 'scripts\build-windows.bat' 6.8.3 2022 64 2>&1 | Tee-Object -FilePath build-log.txt; exit $LASTEXITCODE"
    set BUILD_RESULT=%ERRORLEVEL%
    if not "%BUILD_RESULT%"=="0" (
        echo.
        echo ===== BUILD FAILED. First error lines: =====
        findstr /C:"error C" /C:"fatal error" /C:"Error 2" build-log.txt > build-errors.txt
        powershell -NoProfile -Command "Get-Content build-errors.txt -TotalCount 12"
        echo ===== Copy the lines above and send them. =====
    )
    exit /b %BUILD_RESULT%
)
