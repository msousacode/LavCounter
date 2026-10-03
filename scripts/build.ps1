param([string]$ArduinoCli = 'C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe', [switch]$MockSensor)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
Push-Location $projectRoot
try {
    $cores = & $ArduinoCli core list --format json | ConvertFrom-Json
    $espCore = $cores.platforms | Where-Object { $_.id -eq 'esp32:esp32' }
    if ($espCore.installed_version -ne '3.3.10') { throw 'Instale esp32:esp32@3.3.10 antes de compilar.' }
    $libraries = & $ArduinoCli lib list --format json | ConvertFrom-Json
    $dependencies = @(@{name='ArduinoJson'; version='7.4.2'})
    if (-not $MockSensor) { $dependencies += @{name='VL53L1X'; version='1.3.1'} }
    foreach ($dependency in $dependencies) {
        $installed = $libraries.installed_libraries.library | Where-Object { $_.name -eq $dependency.name }
        if ($installed.version -ne $dependency.version) { throw "Instale $($dependency.name)@$($dependency.version) antes de compilar." }
    }
    node scripts/embed-web.cjs
    if ($LASTEXITCODE -ne 0) { throw 'Falha ao incorporar a interface.' }
    New-Item -ItemType Directory -Force build | Out-Null
    $mode = if ($MockSensor) { 'mock' } else { 'physical' }
    $mockValue = if ($MockSensor) { 1 } else { 0 }
    & $ArduinoCli compile --jobs 2 --fqbn 'esp32:esp32:esp32:PartitionScheme=default,FlashSize=4M' --build-property "compiler.cpp.extra_flags=-DLAVCOUNTER_SENSOR_MOCK=$mockValue" --build-path "$projectRoot\build\firmware-$mode" "$projectRoot\firmware\LavCounter" 2>&1 | Tee-Object "build/compile-$mode.log"
    if ($LASTEXITCODE -ne 0) { throw "Compilação falhou. Consulte build/compile-$mode.log." }
} finally { Pop-Location }
