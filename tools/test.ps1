param([string]$Destination)
$ErrorActionPreference = 'Stop'
$taskRoot = Split-Path -Parent $PSScriptRoot
if (!$Destination) { $Destination = Join-Path $taskRoot 'outputs\SteamVRCapsIME-Setup' }
Push-Location $taskRoot
try {
    New-Item -ItemType Directory -Path work -Force | Out-Null
    & python -m unittest discover -s tests -p test_patch.py
    if ($LASTEXITCODE -ne 0) { throw 'Patch tests failed' }
    & node tests\adapter.test.js
    if ($LASTEXITCODE -ne 0) { throw 'JS adapter tests failed' }
    & g++ -std=c++17 -Wall -Wextra -Werror tests\http_policy_test.cpp -o work\http_policy_test.exe
    if ($LASTEXITCODE -ne 0) { throw 'HTTP policy test build failed' }
    & .\work\http_policy_test.exe
    if ($LASTEXITCODE -ne 0) { throw 'HTTP policy tests failed' }
    & g++ -std=c++17 -Wall -Wextra -Werror -Wno-cast-function-type -municode -static tests\factory_test.cpp -o work\factory_test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Factory test build failed' }
    & .\work\factory_test.exe (Join-Path $Destination 'bin\win64\driver_capsime.dll')
    if ($LASTEXITCODE -ne 0) { throw 'Factory test failed' }
} finally { Pop-Location }
