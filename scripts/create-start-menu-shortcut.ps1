# Vryo Control: create (or refresh) a Start menu shortcut.
#
# After this runs, press the Windows key, type "vryo" and click Vryo Control.
# No PowerShell needed to open the app.
#
# Run from PowerShell in the project folder:
#   powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\create-start-menu-shortcut.ps1
#
# update-vryo.bat runs this automatically after every good build, so the
# shortcut always points at the newest build.

$ErrorActionPreference = 'Stop'

$projectRoot = Split-Path -Parent $PSScriptRoot

# Where the build leaves the finished program. The newest one wins.
$candidates = @(
    (Join-Path $projectRoot 'OpenRGB Windows 64-bit\VryoControl.exe'),
    (Join-Path $projectRoot 'release\VryoControl.exe')
) | Where-Object { Test-Path -LiteralPath $_ }

if (-not $candidates) {
    Write-Host 'Could not find VryoControl.exe. Build the program first (.\scripts\update-vryo.bat).' -ForegroundColor Red
    exit 1
}

$exe = $candidates |
    Get-Item |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1

# The current user's Start menu "Programs" folder. Anything placed here
# shows up in Start search and in the All apps list.
$programsFolder = [Environment]::GetFolderPath('Programs')
$shortcutPath   = Join-Path $programsFolder 'Vryo Control.lnk'

$shell    = New-Object -ComObject WScript.Shell
$shortcut = $shell.CreateShortcut($shortcutPath)
$shortcut.TargetPath       = $exe.FullName
$shortcut.WorkingDirectory = $exe.DirectoryName
$shortcut.IconLocation     = $exe.FullName + ',0'
$shortcut.Description      = 'Vryo Control - GPU dashboard and lighting'
$shortcut.Save()

Write-Host ''
Write-Host 'Start menu shortcut ready:' -ForegroundColor Green
Write-Host ('  ' + $shortcutPath)
Write-Host ('  -> ' + $exe.FullName)
Write-Host ''
Write-Host 'Press the Windows key, type "vryo" and click Vryo Control.'
