param([string]$Compiler = 'g++')
$ErrorActionPreference = 'Stop'
$taskRoot = Split-Path -Parent $PSScriptRoot
$destination = Join-Path $taskRoot 'outputs\SteamVRCapsIME'
$bin = Join-Path $destination 'bin\win64'
New-Item -ItemType Directory -Path $bin -Force | Out-Null
$common = @('-std=c++17','-O2','-Wall','-Wextra','-Werror','-Wno-cast-function-type','-static','-static-libgcc','-static-libstdc++')
& $Compiler @common '-shared' (Join-Path $taskRoot 'src\driver.cpp') '-o' (Join-Path $bin 'driver_capsime.dll') '-luser32'
if ($LASTEXITCODE -ne 0) { throw 'DLL build failed' }
& $Compiler @common '-municode' (Join-Path $taskRoot 'src\host.cpp') '-o' (Join-Path $bin 'CapsImeHost.exe') '-luser32'
if ($LASTEXITCODE -ne 0) { throw 'Host build failed' }
Copy-Item -LiteralPath (Join-Path $taskRoot 'driver\driver.vrdrivermanifest') -Destination $destination
foreach ($file in @('Install.ps1','Uninstall.ps1','patch_steam.py')) {
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot $file) -Destination $destination
}
Copy-Item -LiteralPath (Join-Path $taskRoot 'README.md') -Destination $destination
Copy-Item -LiteralPath (Join-Path $taskRoot 'third_party\openvr\LICENSE') -Destination (Join-Path $destination 'OpenVR-LICENSE.txt')
Write-Output "Built $destination"
