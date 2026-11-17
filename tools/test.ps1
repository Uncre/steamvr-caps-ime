$ErrorActionPreference = 'Stop'
$taskRoot = Split-Path -Parent $PSScriptRoot
Push-Location $taskRoot
try {
    New-Item -ItemType Directory -Path work -Force | Out-Null
    & python -m unittest discover -s tests -p test_patch.py
    if ($LASTEXITCODE -ne 0) { throw 'Patch tests failed' }
    & node tests\adapter.test.js
    if ($LASTEXITCODE -ne 0) { throw 'JS adapter tests failed' }
    & g++ -std=c++17 -Wall -Wextra -Werror tests\key_policy_test.cpp -o work\key_policy_test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Key policy test build failed' }
    & .\work\key_policy_test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Key policy tests failed' }
    & g++ -std=c++17 -Wall -Wextra -Werror -Wno-cast-function-type -municode -static tests\factory_test.cpp -o work\factory_test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Factory test build failed' }
    & .\work\factory_test.exe (Join-Path $taskRoot 'outputs\SteamVRCapsIME\bin\win64\driver_capsime.dll')
    if ($LASTEXITCODE -ne 0) { throw 'Factory test failed' }
} finally { Pop-Location }
