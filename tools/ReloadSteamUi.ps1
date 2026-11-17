param([string]$SteamRoot)
$ErrorActionPreference = 'Stop'
if (!$SteamRoot) { $SteamRoot = (Get-ItemProperty 'HKCU:\Software\Valve\Steam' -ErrorAction SilentlyContinue).SteamPath }
if (!$SteamRoot) { $SteamRoot = 'C:\Program Files (x86)\Steam' }
$health = Invoke-RestMethod 'http://127.0.0.1:27391/health' -TimeoutSec 3
if (!$health.ready) { throw 'Start SteamVR with the Caps IME driver before reloading its UI' }
$expected = [IO.Path]::GetFullPath((Join-Path $SteamRoot 'bin\cef\cef.win64\steamwebhelper.exe'))
$helpers = @(Get-Process -Name steamwebhelper -ErrorAction SilentlyContinue)
foreach ($process in $helpers) {
    if ($process.Path -ne $expected) { throw 'Unexpected Steam webhelper path; nothing stopped' }
}
# Explicit maintenance action: Steam's UI closes briefly; unsent Steam text may be lost.
foreach ($process in $helpers) { Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue }
Write-Output 'Steam will recreate its UI. Reopen the SteamVR desktop keyboard after it returns.'
