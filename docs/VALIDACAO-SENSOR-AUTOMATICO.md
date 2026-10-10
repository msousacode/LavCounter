# Sensor automático — 09/10/2026

O firmware aceita VL53L0X e VL53L1X físicos com bibliotecas Pololu 1.3.1. Identifica primeiro o VL53L0X pelo registro 0xC0 (0xEE); caso diferente, consulta o identificador de 16 bits do VL53L1X em 0x010F (0xEACC). Dispositivos desconhecidos são rejeitados. A inscrição comercial VL53L0/1X V2 não determina qual chip está instalado.

A aquisição contínua usa período de 100 ms e orçamento de 50 ms. VL53L1X usa modo Long e leitura sem espera por uma nova amostra. Os status válidos são 11 (hardware VL53L0X) e 0 (API Pololu VL53L1X). Distância inválida, falta de amostra ou falha I²C entram como Unknown, sem contar como ausência. Falhas provocam nova identificação/inicialização após 5 s. O driver VL53L1X é reconstruído na inicialização para não reutilizar a calibração anterior após desconexão/reset.

O fluxo normal de presença inicia no setup e permanece independente do console. As leituras seriais agora aparecem automaticamente a cada segundo; sensor off apenas silencia a saída. O console mantém limites de entrada/saída e não inicia outra aquisição. GET /api/status acrescenta sensorModel. Pinagem: SDA21/SCL22; LEDs vermelho23/amarelo18/verde19.

scripts/test-native.ps1 passou após o ajuste final. Casos novos verificam identificação VL53L1X, limiar inclusivo, leitura inválida, timeout, resposta curta, falha na confirmação de interrupção, falta de amostra, recuperação e desconexão. Casos VL53L0X e de detecção/contagem permanecem aprovados. O console é verificado sem enviar comandos para iniciar a saída.

Referência do driver: https://github.com/pololu/vl53l1x-arduino/tree/1.3.1

Compilação física aprovada via scripts/build.ps1: flash 1.300.488 / 1.310.720 bytes (99%, margem 10.232 bytes); RAM estática 54.880 / 327.680 bytes (16%). Binário: build/firmware-physical/LavCounter.ino.bin. Log: build/compile-physical.log. Durante a compilação, a cópia de PresenceSensor.h na pasta gerada foi sincronizada com o ajuste final antes de compilar o sketch; os hashes SHA-256 do fonte e da cópia compilada foram conferidos e são iguais. A margem de flash é pequena.

Não houve upload nem teste físico. Ausência de resposta em 0x29 continua exigindo verificar alimentação, pinagem, solda e XSHUT do módulo específico.
