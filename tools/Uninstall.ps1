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
$marked = @('virtualkeyboard.js','sp.js') | Where-Object {
    [IO.File]::ReadAllText((Join-Path $ui $_)).Contains('/* SteamVR Caps IME v1 */')
}
if ($marked.Count -eq 2) {
    & python (Join-Path $packageRoot 'patch_steam.py') --steamui $ui --backup (Join-Path $packageRoot 'backups') --remove
    if ($LASTEXITCODE -ne 0) { throw 'Adapter changed. Refusing to overwrite Steam files.' }
} elseif ($marked.Count -ne 0) { throw 'Only one adapter found. Inspect Steam UI before uninstalling.' }
$reg = Join-Path $SteamVrRoot 'bin\win64\vrpathreg.exe'
& $reg removedriver $packageRoot
if ($LASTEXITCODE -ne 0) { throw 'Driver unregistration failed' }
& (Join-Path $packageRoot 'bin\win64\CapsImeHost.exe') --stop
Write-Output 'Removed. Restart Steam and SteamVR to unload the cached UI and DLL.'
