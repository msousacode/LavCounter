 > Registro histórico: o modo simulado foi removido em 07/10/2026. O firmware atual usa exclusivamente o VL53L0X real; as instruções de ativação abaixo não se aplicam à versão atual.

# Validação do mock VL53L1X

Change: `mock-sensor-vl53l1x`, branch `mock-sensor`, 02/10/2026.

## Testes de software

- `npm test`: aprovado. Núcleo, armazenamento, integração física e integração mock passaram.
- `npm run test:web`: 6 testes aprovados no Edge, incluindo os três comandos, resposta perdida com reconciliação, recarregamento, modo físico, operação sem recursos externos e largura móvel de 390 px.
- `openspec validate mock-sensor-vl53l1x --strict`: aprovado.
- Interface gerada por `npm run build:web`; captura revisada em `build/web-mock-mobile.png`, sem transbordamento horizontal.

`tests/mock-firmware.cpp` executa o sketch real com periféricos de host e o adaptador mock de produção. Verifica ausência inicial, limite temporal, repetição sem duplicação, falha antes/depois da validação, tolerância de saída, GPIOs, autenticação, entrada inválida, RTC indisponível, gravação/retry, paginação e reinício. Journal e configuração físicos preexistentes permanecem iguais. O fake do VL53L1X emite erro de compilação se for incluído no modo mock.

## Compilação ESP32

Build mock aprovado: 1.112.072 bytes de flash (84%) e 51.368 bytes de RAM estática (15%). Binário em `build/firmware-mock/LavCounter.ino.bin`, log em `build/compile-mock.log`. Build físico aprovado: 1.115.388 bytes de flash (85%) e 51.432 bytes de RAM estática (15%). Binário em `build/firmware-physical/LavCounter.ino.bin`, log em `build/compile-physical.log`. A lista de dependências do pré-processamento mock contém `MockPresenceSensor.h` e não contém `VL53L1X.h`.

## Bancada

COM4 foi detectada como porta USB serial e confirmada pelo usuário como a ESP32 de teste. O computador está em outra rede; consulta a `http://192.168.4.1/api/status` expirou sem conexão HTTP. Não foi realizado upload, troca de Wi-Fi nem ensaio físico. A validação de ciclo pela API na placa, persistência sob reinício real e observação elétrica dos LEDs permanecem pendentes, conforme [roteiro de bancada do mock](MOCK-SENSOR.md#roteiro-de-bancada).

Os testes em host não são evidência de funcionamento óptico, I2C, Wi-Fi ou LEDs reais. A implementação permite registrar essa pendência sem apresentar o ensaio físico como aprovado.
