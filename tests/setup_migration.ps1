param(
    [Parameter(Mandatory)][string]$SteamVrRoot,
    [string]$Package
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (!$Package) { $Package = Join-Path $root 'outputs\SteamVRCapsIME-Setup' }
. (Join-Path $Package 'SetupCommon.ps1')
Assert-CapsImeStopped

$testRoot = Join-Path $root "work\setup-test-$([Guid]::NewGuid().ToString('N'))"
$beforeLocal = $env:LOCALAPPDATA
$beforeRegistry = $env:VR_PATHREG_OVERRIDE
try {
    $env:LOCALAPPDATA = Join-Path $testRoot ('Local Data ' + [char]0x672C)
    $env:VR_PATHREG_OVERRIDE = Join-Path $testRoot 'openvrpaths.vrpath'
    $steam = Join-Path $testRoot 'Test Steam'
    $keyboard = Get-CapsImeKeyboardFile $steam
    $legacy = Join-Path $testRoot 'old extracted driver'
    $unrelated = Join-Path $testRoot 'other-driver'
    $installed = Get-CapsImeInstallDirectory
    New-Item -ItemType Directory -Path $legacy,$unrelated,(Split-Path -Parent $keyboard) -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $Package 'driver.vrdrivermanifest'),(Join-Path $Package 'caps-adapter.js') -Destination $legacy
    [IO.File]::WriteAllText((Join-Path $unrelated 'driver.vrdrivermanifest'), '{"name":"testing_other_driver"}')
    $anchor = 'const g=u.s_keyToggleData[t];if(g)'
    $original = 'header var Rt=p(90759) TypeKeyInternal(e){let{strKey:t ' + $anchor + ' trailer'
    [IO.File]::WriteAllText($keyboard, $original.Replace($anchor, [IO.File]::ReadAllText((Join-Path $legacy 'caps-adapter.js')) + $anchor))
    @{ version=1; jsonid='vrpathreg'; runtime=@($SteamVrRoot); config=@($testRoot); log=@($testRoot); external_drivers=@($unrelated,$legacy) } |
        ConvertTo-Json | Set-Content -LiteralPath $env:VR_PATHREG_OVERRIDE -Encoding ascii

    if ((Get-CapsImeRegisteredDirectory (Get-CapsImeRegistrationTool)) -ne $legacy) {
        throw 'vrpathreg did not read the isolated test registry.'
    }
    & powershell.exe -NoProfile -File (Join-Path $Package 'Install.ps1') -SteamRoot $steam
    if ($LASTEXITCODE -ne 0) { throw 'Migration failed.' }
    if ([IO.File]::ReadAllText($keyboard) -cne $original) { throw 'Original keyboard was not restored.' }
    $drivers = (Get-Content -LiteralPath $env:VR_PATHREG_OVERRIDE -Raw -Encoding utf8 | ConvertFrom-Json).external_drivers
    if ($drivers.Count -ne 2 -or $drivers -notcontains $installed -or $drivers -notcontains $unrelated) {
        throw 'Unexpected migrated registrations.'
    }

    & powershell.exe -NoProfile -File (Join-Path $Package 'Install.ps1') -SteamRoot $steam
    if ($LASTEXITCODE -ne 0) { throw 'Repeat install failed.' }
    $note = Join-Path $installed 'bin\win64\caps-ime.log'
    [IO.File]::WriteAllText($note, 'diagnostic data')
    & powershell.exe -NoProfile -File (Join-Path $installed 'Uninstall.ps1') -SteamRoot $steam
    if ($LASTEXITCODE -ne 0) { throw 'Uninstall failed.' }
    $drivers = (Get-Content -LiteralPath $env:VR_PATHREG_OVERRIDE -Raw -Encoding utf8 | ConvertFrom-Json).external_drivers
    if (@($drivers).Count -ne 1 -or $drivers -notcontains $unrelated) { throw 'Unexpected registrations after uninstall.' }
    if (!(Test-Path -LiteralPath $note) -or (Test-Path -LiteralPath (Join-Path $installed 'bin\win64\driver_capsime.dll'))) {
        throw 'Unexpected files after uninstall.'
    }
    Write-Output 'PASS: legacy migration, stale keyboard restore, repeat install, and uninstall via isolated SteamVR registration.'
} finally {
    $env:LOCALAPPDATA = $beforeLocal
    $env:VR_PATHREG_OVERRIDE = $beforeRegistry
}
