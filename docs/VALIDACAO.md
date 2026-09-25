# Validação da implementação — 25/09/2026

## Compilação ESP32

Comando reproduzível: `./scripts/build.ps1`.

- Arduino CLI 1.5.1; core ESP32 3.3.10; ArduinoJson 7.4.2.
- FQBN `esp32:esp32:esp32:PartitionScheme=default,FlashSize=4M`.
- Resultado final: **sucesso**.
- Flash de aplicação: **1.109.768 / 1.310.720 bytes (84%)**.
- RAM estática: **85.208 / 327.680 bytes (26%)**, restando 242.472 bytes para uso dinâmico/local.
- Log: `build/compile.log`; binário: `build/firmware/LavCounter.ino.bin`.
- A tentativa de build da base antiga foi interrompida para reduzir concorrência; não é usada como evidência. A primeira compilação da versão integrada passou, seguida da compilação final acima.

## Testes de software

`./scripts/test-native.ps1` usa Zig 0.14.1 como compilador C++ para executar o código real no computador:

- Núcleo: limites de configuração, volume fracionário, sessões, pausas, retomadas, timeout na fronteira, contador de 64 bits, classificação e limiares das seis evidências, filtro e overflow.
- Calendário: BCD, dia da semana/data, horas 12/24, flag de oscilação, datas inválidas, ano bissexto, viradas e 1.200 meses entre 2000 e 2099.
- ConfigStore/Event/Storage reais com NVS/SPIFFS simulados: persistência, falha atômica, IDs entre reinícios, JSON, gravação parcial, armazenamento cheio e paginação de 66 registros.
- Sketch/handlers reais com transporte e hardware simulados: autenticação em todas as sete rotas e fallback, configuração durante sessão, persistência após reinício simulado, correção do relógio, timeout exato, falha I2C e overflow de aquisição.

`npm run test:web`: **5 testes aprovados** no Microsoft Edge headless com API simulada. Cobrem histórico, texto seguro, salvar/cancelar/restaurar, erro por campo, falha de persistência, layout móvel, RTC e edição/envio dos **18 parâmetros reais**. Capturas em `build/web-config.png`, `build/web-events.png` e `build/web-mobile.png`. O layout foi inspecionado visualmente.

Dependências de teste: `npm ci`; Zig 0.14.1 em `tools/zig-x86_64-windows-0.14.1` ou parâmetro `-Zig`; ArduinoJson instalada no diretório Arduino ou parâmetro `-ArduinoJson`. O compilador de testes não é incorporado ao firmware.

## Limite da validação

`arduino-cli board list --format json` retornou `detected_ports: []`. Nenhum firmware foi gravado em dispositivo. Conexão Wi-Fi real, RTC físico, contagem elétrica e calibração com água ainda precisam do [roteiro de bancada](BANCADA.md).

Testes de navegador simulam a API; testes nativos executam os handlers de produção com transporte simulado. Eles não comprovam o enlace Wi-Fi nem a persistência elétrica do hardware. A tarefa de acesso local no ESP32 e a aceitação física permanecem abertas.

## Progresso OpenSpec

24/27 tarefas concluídas. Permanecem abertas 6.3 (acesso real ao AP), 7.2 (bancada com sensor e RTC) e 7.3 (aceitação física). Nenhum upload foi realizado. A implementação e a validação de software estão concluídas; a mudança não foi arquivada.
