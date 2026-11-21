param(
    [Parameter(Mandatory)][string]$SteamVrRoot,
    [string]$Package
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (!$Package) { $Package = Join-Path $root 'outputs\SteamVRCapsIME-Setup' }
. (Join-Path $Package 'SetupCommon.ps1')
Assert-CapsImeStopped

function Invoke-SetupBatch([string]$Batch, [string]$SteamRoot) {
    $info = New-Object System.Diagnostics.ProcessStartInfo
    $info.FileName = Join-Path $env:SystemRoot 'System32\cmd.exe'
    $info.Arguments = '/d /s /c ""{0}" -SteamRoot "{1}""' -f $Batch, $SteamRoot
    $info.WorkingDirectory = Split-Path -Parent $Batch
    $info.UseShellExecute = $false
    $info.CreateNoWindow = $true
    $info.RedirectStandardInput = $true
    $process = [Diagnostics.Process]::Start($info)
    try {
        $process.StandardInput.Close()
        $process.WaitForExit()
        if ($process.ExitCode -ne 0) { throw "$Batch failed with exit code $($process.ExitCode)." }
    } finally { $process.Dispose() }
}

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
    Invoke-SetupBatch (Join-Path $Package 'Install.bat') $steam
    if ([IO.File]::ReadAllText($keyboard) -cne $original) { throw 'Original keyboard was not restored.' }
    $drivers = (Get-Content -LiteralPath $env:VR_PATHREG_OVERRIDE -Raw -Encoding utf8 | ConvertFrom-Json).external_drivers
    if ($drivers.Count -ne 2 -or $drivers -notcontains $installed -or $drivers -notcontains $unrelated) {
        throw 'Unexpected migrated registrations.'
    }

    & powershell.exe -NoProfile -File (Join-Path $Package 'Install.ps1') -SteamRoot $steam
    if ($LASTEXITCODE -ne 0) { throw 'Repeat install failed.' }
    $note = Join-Path $installed 'bin\win64\caps-ime.log'
    [IO.File]::WriteAllText($note, 'diagnostic data')
    $uninstaller = Join-Path $installed 'Uninstall.bat'
    Invoke-SetupBatch $uninstaller $steam
    $drivers = (Get-Content -LiteralPath $env:VR_PATHREG_OVERRIDE -Raw -Encoding utf8 | ConvertFrom-Json).external_drivers
    if (@($drivers).Count -ne 1 -or $drivers -notcontains $unrelated) { throw 'Unexpected registrations after uninstall.' }
    if (!(Test-Path -LiteralPath $note) -or (Test-Path -LiteralPath (Join-Path $installed 'bin\win64\driver_capsime.dll'))) {
        throw 'Unexpected files after uninstall.'
    }

    Remove-Item -LiteralPath $note
    Invoke-SetupBatch (Join-Path $Package 'Install.bat') $steam
    Invoke-SetupBatch $uninstaller $steam
    if (Test-Path -LiteralPath $installed) { throw 'Uninstall did not remove the empty installation directory.' }

    Write-Output 'PASS: batch install and uninstall, legacy migration, stale keyboard restore, repeat install, and log retention.'
} finally {
    $env:LOCALAPPDATA = $beforeLocal
    $env:VR_PATHREG_OVERRIDE = $beforeRegistry
}
