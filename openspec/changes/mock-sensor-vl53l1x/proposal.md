# Mock do VL53L1X pelo painel local

## Why

O VL53L1X ainda não chegou, impedindo validar o ciclo de lavagens na ESP32. Uma entrada simulada pelo painel permitirá testar temporização, LEDs, contagens e registros usando a lógica existente, sem depender do sensor conectado.

## What Changes

- Adicionar modo mock explícito de firmware, mantendo o sensor físico como padrão e sem fallback automático.
- Disponibilizar botões autenticados para veículo presente, box vazio e falha do sensor. Conforme escolha do usuário, os comandos simulam amostras e respeitam os tempos configurados; não forçam validação ou encerramento.
- Reutilizar máquina de estados, semáforo, RTC e persistência reais da ESP32.
- Identificar claramente a simulação no painel e nas APIs; separar histórico, contagens e configuração de teste dos dados físicos.
- Documentar ativação, compilação e roteiro de validação na branch `mock-sensor`.

## Capabilities

### New Capabilities

- `sensor-simulation`: comandos de simulação, ativação explícita, ciclo temporal e isolamento dos dados de teste.

### Modified Capabilities

- `vehicle-presence`: permitir aquisição simulada explicitamente selecionada, preservando a aquisição física como padrão.
- `local-dashboard`: apresentar controles e diagnóstico do mock apenas quando habilitado.
- `wash-history`: identificar e separar registros simulados dos registros físicos.

## Impact

Firmware (`PresenceSensor.h`, integração em `LavCounter.ino`, configuração e armazenamento), API local, `web/index.html` e cabeçalho gerado pelo script existente. Testes nativos, testes Playwright e documentação de operação serão ampliados. Nenhum serviço externo ou nova biblioteca é necessário; RTC e LEDs continuam sendo os dispositivos reais. Esta change contém apenas planejamento; a implementação exige solicitação posterior.
