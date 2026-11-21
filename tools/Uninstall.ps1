param([string]$SteamRoot, [string]$SteamVrRoot)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'SetupCommon.ps1')
$destination = Get-CapsImeInstallDirectory

$mutex = New-Object System.Threading.Mutex($false, 'Local\SteamVR.CapsIme.Setup')
$acquired = $false
try {
    try { $acquired = $mutex.WaitOne(0) } catch [System.Threading.AbandonedMutexException] { $acquired = $true }
    if (!$acquired) { throw 'Another Caps IME setup is running.' }
    Assert-CapsImeStopped
    $registrationTool = Get-CapsImeRegistrationTool $SteamVrRoot
    $registered = Get-CapsImeRegisteredDirectory $registrationTool
    if ($registered -and $registered -ne $destination) {
        throw "Caps IME is registered at a different location: $registered"
    }

    $keyboard = Get-CapsImeKeyboardFile $SteamRoot
    if (Test-Path -LiteralPath $keyboard -PathType Leaf) {
        $original = Get-CapsImeOriginalKeyboard $keyboard @($destination)
        Set-CapsImeUtf8File $keyboard $original
    }
    if ($registered) { Invoke-CapsImeRegistration $registrationTool removedriver $destination }
    foreach ($file in $CapsImeFiles) {
        $path = Join-Path $destination $file
        if (Test-Path -LiteralPath $path -PathType Leaf) { Remove-Item -LiteralPath $path -Force }
    }
    foreach ($dir in @((Join-Path $destination 'bin\win64'), (Join-Path $destination 'bin'), $destination, (Split-Path -Parent $destination))) {
        if ((Test-Path -LiteralPath $dir) -and !(Get-ChildItem -LiteralPath $dir -Force)) {
            Remove-Item -LiteralPath $dir
        }
    }
    Write-Output 'Caps IME uninstalled. Existing diagnostic logs and backups are kept at the install location.'
    Write-Output 'Restart Steam to clear its cached keyboard.'
} finally {
    if ($acquired) { $mutex.ReleaseMutex() }
    $mutex.Dispose()
}
