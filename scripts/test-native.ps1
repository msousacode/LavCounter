param([string]$Zig = "$PSScriptRoot\..\tools\zig-x86_64-windows-0.14.1\zig.exe", [string]$ArduinoJson = "$env:USERPROFILE\Documents\Arduino\libraries\ArduinoJson\src")
$ErrorActionPreference='Stop'
Push-Location (Split-Path $PSScriptRoot -Parent)
try {
 New-Item -ItemType Directory -Force build | Out-Null
 $flags=@('cc','-x','c++','-std=c++17','-fno-exceptions','-fno-rtti','-fno-threadsafe-statics')
 & $Zig @flags tests/core.cpp -o build/core-tests.exe
 if($LASTEXITCODE -ne 0){throw 'Compilação de testes do núcleo falhou.'}
 & build/core-tests.exe
 if($LASTEXITCODE -ne 0){throw 'Testes do núcleo falharam.'}
 & $Zig @flags -I tests/fakes tests/presence-sensor.cpp -o build/presence-sensor-tests.exe
 if($LASTEXITCODE -ne 0){throw 'Compilação de testes do sensor falhou.'}
 & build/presence-sensor-tests.exe
 if($LASTEXITCODE -ne 0){throw 'Testes do sensor falharam.'}
 $jsonFlags=@('-I','tests/fakes','-I',$ArduinoJson,'-DLAVCOUNTER_CLOUD_IGNORE_PRIVATE=1','-DARDUINOJSON_ENABLE_ARDUINO_STRING=1','-DARDUINOJSON_ENABLE_ARDUINO_STREAM=0','-DARDUINOJSON_ENABLE_ARDUINO_PRINT=0','-DARDUINOJSON_ENABLE_STD_STRING=0','-DARDUINOJSON_ENABLE_STD_STREAM=0','-DARDUINOJSON_ENABLE_STRING_VIEW=0')
 & $Zig @flags @jsonFlags tests/cloud.cpp -o build/cloud-tests.exe
 if($LASTEXITCODE -ne 0){throw 'Compilação de testes de nuvem falhou.'}
 & build/cloud-tests.exe
 if($LASTEXITCODE -ne 0){throw 'Testes de nuvem falharam.'}
 & $Zig @flags @jsonFlags tests/firebase.cpp -o build/firebase-tests.exe
 if($LASTEXITCODE -ne 0){throw 'Compilação de testes Firebase falhou.'}
 & build/firebase-tests.exe
 if($LASTEXITCODE -ne 0){throw 'Testes Firebase falharam.'}
 & $Zig @flags @jsonFlags tests/firebase-firmware.cpp -o build/firebase-firmware-tests.exe
 if($LASTEXITCODE -ne 0){throw 'Compilação do firmware Firebase de teste falhou.'}
 & build/firebase-firmware-tests.exe
 if($LASTEXITCODE -ne 0){throw 'Testes do firmware Firebase falharam.'}
 & $Zig @flags @jsonFlags tests/storage.cpp -o build/storage-tests.exe
 if($LASTEXITCODE -ne 0){throw 'Compilação de testes de armazenamento falhou.'}
 & build/storage-tests.exe
 if($LASTEXITCODE -ne 0){throw 'Testes de armazenamento falharam.'}
 & $Zig @flags @jsonFlags tests/firmware.cpp -o build/firmware-tests.exe
 if($LASTEXITCODE -ne 0){throw 'Compilação de testes de integração falhou.'}
 & build/firmware-tests.exe
 if($LASTEXITCODE -ne 0){throw 'Testes de integração falharam.'}
} finally {Pop-Location}
