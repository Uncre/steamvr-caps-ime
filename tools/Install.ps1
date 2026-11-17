param([string]$SteamRoot, [string]$SteamVrRoot)
$ErrorActionPreference = 'Stop'
$packageRoot = $PSScriptRoot
if (!(Test-Path -LiteralPath (Join-Path $packageRoot 'driver.vrdrivermanifest'))) { throw 'Run the copy in outputs\SteamVRCapsIME' }
if (!$SteamRoot) { $SteamRoot = (Get-ItemProperty 'HKCU:\Software\Valve\Steam' -ErrorAction SilentlyContinue).SteamPath }
if (!$SteamRoot) { $SteamRoot = 'C:\Program Files (x86)\Steam' }
if (!$SteamVrRoot) {
    $vrpaths = Join-Path ([Environment]::GetFolderPath('LocalApplicationData')) 'openvr\openvrpaths.vrpath'
    $SteamVrRoot = (Get-Content -LiteralPath $vrpaths -Raw | ConvertFrom-Json).runtime[0]
}
$ui = Join-Path $SteamRoot 'steamui'
$patch = Join-Path $packageRoot 'patch_steam.py'
$backup = Join-Path $packageRoot 'backups'
$reg = Join-Path $SteamVrRoot 'bin\win64\vrpathreg.exe'
if (!(Test-Path -LiteralPath $reg)) { throw "SteamVR not found: $reg" }
& python $patch --steamui $ui --backup $backup --check
if ($LASTEXITCODE -ne 0) { throw 'Unsupported Steam UI. Nothing installed.' }
& python $patch --steamui $ui --backup $backup
if ($LASTEXITCODE -ne 0) { throw 'Steam UI patch failed' }
& $reg adddriver $packageRoot
if ($LASTEXITCODE -ne 0) { throw 'Driver registration failed. Run Uninstall.ps1 to restore the UI.' }
Write-Output 'Installed. Restart the Steam client, then SteamVR. Use Japanese Microsoft IME in the target app.'
