param([string]$SteamRoot, [string]$SteamVrRoot)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'SetupCommon.ps1')

$destination = Get-CapsImeInstallDirectory
if ($PSScriptRoot -eq $destination) { throw 'Run Install.ps1 from the extracted release.' }
foreach ($file in $CapsImeFiles) {
    if (!(Test-Path -LiteralPath (Join-Path $PSScriptRoot $file) -PathType Leaf)) {
        throw "Incomplete release: $file is missing."
    }
}
$manifest = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'driver.vrdrivermanifest') -Raw | ConvertFrom-Json
if ($manifest.name -cne 'capsime') { throw 'Unexpected driver name in release manifest.' }

$mutex = New-Object System.Threading.Mutex($false, 'Local\SteamVR.CapsIme.Setup')
$acquired = $false
try {
    try { $acquired = $mutex.WaitOne(0) } catch [System.Threading.AbandonedMutexException] { $acquired = $true }
    if (!$acquired) { throw 'Another Caps IME setup is running.' }
    Assert-CapsImeStopped
    $registrationTool = Get-CapsImeRegistrationTool $SteamVrRoot
    $previous = Get-CapsImeRegisteredDirectory $registrationTool
    $keyboard = Get-CapsImeKeyboardFile $SteamRoot
    $original = Get-CapsImeOriginalKeyboard $keyboard @($previous, $destination, $PSScriptRoot)
    if ([regex]::Matches($original, [regex]::Escape('const g=u.s_keyToggleData[t];if(g)')).Count -ne 1 -or
        !$original.Contains('var Rt=p(90759)') -or !$original.Contains('TypeKeyInternal(e){let{strKey:t')) {
        throw 'This Steam client build has an unsupported keyboard layout. Setup made no changes.'
    }
    Set-CapsImeUtf8File $keyboard $original

    $rollback = Join-Path ([IO.Path]::GetTempPath()) "SteamVRCapsIME-$([guid]::NewGuid().ToString('N'))"
    $saved = @()
    $written = @()
    $removedOld = $false
    $addedNew = $false
    try {
        New-Item -ItemType Directory -Path $rollback | Out-Null
        foreach ($file in $CapsImeFiles) {
            $target = Join-Path $destination $file
            if (Test-Path -LiteralPath $target -PathType Leaf) {
                $backup = Join-Path $rollback $file
                New-Item -ItemType Directory -Path (Split-Path -Parent $backup) -Force | Out-Null
                Copy-Item -LiteralPath $target -Destination $backup
                $saved += $file
            }
        }
        foreach ($file in $CapsImeFiles) {
            $target = Join-Path $destination $file
            New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force | Out-Null
            $written += $file
            Copy-Item -LiteralPath (Join-Path $PSScriptRoot $file) -Destination $target -Force
        }
        if ($previous -ne $destination) {
            if ($previous) {
                Invoke-CapsImeRegistration $registrationTool removedriver $previous
                $removedOld = $true
            }
            Invoke-CapsImeRegistration $registrationTool adddriver $destination
            $addedNew = $true
        }
        if ((Get-CapsImeRegisteredDirectory $registrationTool) -ne $destination) {
            throw 'SteamVR did not report the expected driver location.'
        }
    } catch {
        if ($addedNew) { Invoke-CapsImeRegistration $registrationTool removedriver $destination }
        if ($removedOld) { Invoke-CapsImeRegistration $registrationTool adddriver $previous }
        foreach ($file in $written) {
            $target = Join-Path $destination $file
            if ($file -in $saved) {
                Copy-Item -LiteralPath (Join-Path $rollback $file) -Destination $target -Force
            } elseif (Test-Path -LiteralPath $target -PathType Leaf) {
                Remove-Item -LiteralPath $target
            }
        }
        throw
    } finally {
        foreach ($file in $saved) { Remove-Item -LiteralPath (Join-Path $rollback $file) -ErrorAction SilentlyContinue }
        foreach ($dir in @((Join-Path $rollback 'bin\win64'), (Join-Path $rollback 'bin'), $rollback)) {
            if ((Test-Path -LiteralPath $dir) -and !(Get-ChildItem -LiteralPath $dir -Force)) {
                Remove-Item -LiteralPath $dir -ErrorAction SilentlyContinue
            }
        }
    }
    Write-Output "Installed: $destination"
    Write-Output 'Start SteamVR to use Caps IME. You can delete the extracted release folder.'
    Write-Output "Uninstall: $(Join-Path $destination 'Uninstall.ps1')"
} finally {
    if ($acquired) { $mutex.ReleaseMutex() }
    $mutex.Dispose()
}
