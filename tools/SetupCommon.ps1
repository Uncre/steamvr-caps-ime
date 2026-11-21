function Get-CapsImeInstallDirectory {
    Join-Path ([IO.Path]::GetFullPath($env:LOCALAPPDATA)) 'SteamVRCapsIME\capsime'
}

function Get-CapsImePathsFile {
    if ($env:VR_PATHREG_OVERRIDE) { return $env:VR_PATHREG_OVERRIDE }
    Join-Path $env:LOCALAPPDATA 'openvr\openvrpaths.vrpath'
}

function Get-CapsImeRegistrationTool([string]$SteamVrRoot) {
    if (!$SteamVrRoot) {
        $pathsFile = Get-CapsImePathsFile
        if (Test-Path -LiteralPath $pathsFile -PathType Leaf) {
            $paths = Get-Content -LiteralPath $pathsFile -Raw -Encoding utf8 | ConvertFrom-Json
            $SteamVrRoot = @($paths.runtime) | Select-Object -First 1
        }
        if (!$SteamVrRoot) {
            $SteamVrRoot = (Get-ItemProperty -LiteralPath 'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 250820' -ErrorAction SilentlyContinue).InstallLocation
        }
    }
    if (!$SteamVrRoot) { throw 'SteamVR was not found. Start SteamVR once, exit it, and run setup again.' }
    $tool = Join-Path $SteamVrRoot 'bin\win64\vrpathreg.exe'
    if (!(Test-Path -LiteralPath $tool -PathType Leaf)) { throw "SteamVR registration tool not found: $tool" }
    $tool
}

function Get-CapsImeRegisteredDirectory([string]$RegistrationTool) {
    $encoding = [Console]::OutputEncoding
    try {
        [Console]::OutputEncoding = [Text.Encoding]::UTF8
        $output = @(& $RegistrationTool finddriver capsime)
        $code = $LASTEXITCODE
    } finally {
        [Console]::OutputEncoding = $encoding
    }
    switch ($code) {
        0 {
            if ($output.Count -ne 1 -or ![IO.Path]::IsPathRooted($output[0])) {
                throw "Unexpected vrpathreg output: $output"
            }
            return [IO.Path]::GetFullPath($output[0]).TrimEnd('\', '/')
        }
        1 { return }
        2 { throw 'SteamVR already has multiple capsime registrations. Remove the duplicate before running setup.' }
        default { throw "Could not read SteamVR driver registration. Start SteamVR once and exit before setup. $output" }
    }
}

function Invoke-CapsImeRegistration([string]$RegistrationTool, [string]$Command, [string]$DriverDirectory) {
    if ($DriverDirectory -match '[^\x00-\x7F]') {
        # vrpathreg on Windows passes these paths through the ANSI code page.
        # Keep the registry in UTF-8 and replace it atomically while SteamVR is stopped.
        $path = Get-CapsImePathsFile
        $registry = Get-Content -LiteralPath $path -Raw -Encoding utf8 | ConvertFrom-Json
        $drivers = @($registry.external_drivers | Where-Object {
            $_ -and $_.TrimEnd('\', '/') -ne $DriverDirectory
        })
        if ($Command -eq 'adddriver') { $drivers += $DriverDirectory }
        $registry | Add-Member -NotePropertyName external_drivers -NotePropertyValue $drivers -Force
        Set-CapsImeUtf8File $path ($registry | ConvertTo-Json -Depth 100)
        return
    }
    & $RegistrationTool $Command $DriverDirectory | Out-Host
    if ($LASTEXITCODE -ne 0) { throw "SteamVR registration command failed: $Command $DriverDirectory" }
}

function Assert-CapsImeStopped {
    if (Get-Process -Name vrserver,CapsImeHost -ErrorAction SilentlyContinue) {
        throw 'Exit SteamVR and close the Caps IME diagnostic host before running setup.'
    }
}

function Get-CapsImeKeyboardFile([string]$SteamRoot) {
    if (!$SteamRoot) { $SteamRoot = (Get-ItemProperty 'HKCU:\Software\Valve\Steam' -ErrorAction SilentlyContinue).SteamPath }
    if (!$SteamRoot) { $SteamRoot = 'C:\Program Files (x86)\Steam' }
    Join-Path $SteamRoot 'steamui\virtualkeyboard.js'
}

function Get-CapsImeOriginalKeyboard([string]$KeyboardFile, [string[]]$DriverDirectories) {
    $utf8 = New-Object System.Text.UTF8Encoding($false, $true)
    $text = $utf8.GetString([IO.File]::ReadAllBytes($KeyboardFile))
    $anchor = 'const g=u.s_keyToggleData[t];if(g)'
    foreach ($directory in $DriverDirectories) {
        if (!$directory) { continue }
        $adapter = Join-Path $directory 'caps-adapter.js'
        if (!(Test-Path -LiteralPath $adapter -PathType Leaf)) { continue }
        $patch = $utf8.GetString([IO.File]::ReadAllBytes($adapter)) + $anchor
        if ($text.Contains($patch)) { $text = $text.Replace($patch, $anchor) }
    }
    if ($text.Contains('/* SteamVR Caps IME')) {
        throw 'Unrecognized Caps IME patch in the Steam keyboard. Restart Steam to restore its keyboard file, then retry.'
    }
    $text
}

function Set-CapsImeUtf8File([string]$Path, [string]$Content) {
    $utf8 = New-Object System.Text.UTF8Encoding($false, $true)
    if ($Content -ceq $utf8.GetString([IO.File]::ReadAllBytes($Path))) { return }
    $temporary = "$Path.capsime-setup-$([guid]::NewGuid().ToString('N')).tmp"
    try {
        [IO.File]::WriteAllText($temporary, $Content, $utf8)
        [IO.File]::Replace($temporary, $Path, [NullString]::Value)
    } finally {
        if (Test-Path -LiteralPath $temporary) { Remove-Item -LiteralPath $temporary }
    }
}

# Keep diagnostic logs, backups and unrelated files out of install payloads and rollback.
$CapsImeFiles = @(
    'driver.vrdrivermanifest', 'caps-adapter.js', 'bin\win64\driver_capsime.dll',
    'bin\win64\CapsImeHost.exe', 'SetupCommon.ps1', 'Uninstall.bat', 'Uninstall.ps1',
    'ReloadSteamUi.ps1', 'README.md', 'LICENSE', 'OpenVR-LICENSE.txt'
)
