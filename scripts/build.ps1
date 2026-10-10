param([string]$ArduinoCli = 'C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe')
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
Push-Location $projectRoot
try {
    $cores = & $ArduinoCli core list --format json | ConvertFrom-Json
    $espCore = $cores.platforms | Where-Object { $_.id -eq 'esp32:esp32' }
    if ($espCore.installed_version -ne '3.3.10') { throw 'Instale esp32:esp32@3.3.10 antes de compilar.' }
    $libraries = & $ArduinoCli lib list --format json | ConvertFrom-Json
    $dependencies = @(@{name='ArduinoJson'; version='7.4.2'})
    $dependencies += @{name='VL53L0X'; version='1.3.1'}
    $dependencies += @{name='VL53L1X'; version='1.3.1'}
    foreach ($dependency in $dependencies) {
        $installed = $libraries.installed_libraries.library | Where-Object { $_.name -eq $dependency.name }
        if ($installed.version -ne $dependency.version) { throw "Instale $($dependency.name)@$($dependency.version) antes de compilar." }
    }
    node scripts/embed-web.cjs
    if ($LASTEXITCODE -ne 0) { throw 'Falha ao incorporar a interface.' }
    New-Item -ItemType Directory -Force build | Out-Null
    & $ArduinoCli compile --jobs 2 --fqbn 'esp32:esp32:esp32:PartitionScheme=default,FlashSize=4M' --build-path "$projectRoot\build\firmware-physical" "$projectRoot\firmware\LavCounter" 2>&1 | Tee-Object "build/compile-physical.log"
    if ($LASTEXITCODE -ne 0) { throw "Compilação falhou. Consulte build/compile-physical.log." }
} finally { Pop-Location }
