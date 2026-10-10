# VL53L0X — 07/10/2026

O sensor físico do projeto passou a ser o VL53L0X, com biblioteca Pololu 1.3.1. O VL53L1X pertence às revisões anteriores; seus registros de validação foram preservados como histórico. A pinagem permanece SDA GPIO21, SCL GPIO22 e endereço I²C 0x29, compartilhando o barramento com o DS3231 em 0x68.

O adaptador usa medições contínuas a cada 100 ms, orçamento de 50 ms e consulta de disponibilidade antes da leitura. O resultado de 12 bytes é lido com verificação do tamanho da resposta I²C e decodificação da distância em milímetros. Somente status de hardware 11, distância positiva diferente de 65535 e confirmação da interrupção sem erro permitem classificar presença ou ausência. Amostras ausentes expiram em 500 ms; desconexão ou falta persistente de amostras permite reinicialização, sem inferir box vazio.

Referências do driver e da interpretação de status:

- [Biblioteca Pololu VL53L0X](https://github.com/pololu/vl53l0x-arduino).
- [API ST distribuída pela Adafruit: decodificação de status](https://github.com/adafruit/Adafruit_VL53L0X/blob/master/src/core/src/vl53l0x_api_core.cpp).

A biblioteca Pololu não implementa a estimativa de sigma da API completa ST. Alcance, precisão, alimentação do módulo específico e tempo real sob falhas de rede/I²C devem ser conferidos em bancada, conforme `BANCADA.md`. Nenhum upload ou ensaio óptico foi realizado nesta alteração.

Os testes do adaptador cobrem os 16 status de hardware, limiar inclusivo, distância zero/65535, timeout, resposta curta I²C, erro na confirmação de interrupção, amostra expirada, desconexão e recuperação inclusive quando nunca houve amostra inicial. O fake antigo do VL53L1X foi substituído. Os fakes de Preferences/Wi-Fi e a expectativa AP+STA foram compatibilizados com o provisionamento Wi-Fi já existente, sem alterar o código de rede de produção.

`scripts/test-native.ps1`: sete executáveis aprovados. `npm run test:web`: nove testes aprovados, incluindo a mensagem de falha do VL53L0X e o modo simulado; três falhas em expectativas desatualizadas sobre o botão offline (agora abre configuração Wi-Fi), data inválida no histórico (agora exibida como horário indisponível) e formato do campo RTC (agora `dd/MM/YYYY HH:mm`). Nesta alteração, a única modificação no painel é o nome do sensor no título de simulação; esses comportamentos não foram alterados.

`scripts/build.ps1`: compilação física aprovada com core ESP32 3.3.10 e VL53L0X 1.3.1. Flash: 1.296.508 / 1.310.720 bytes (98%), com 14.212 bytes livres. RAM estática: 54.352 / 327.680 bytes (16%), com 273.328 bytes livres. Log em `build/compile-physical.log`; binário em `build/firmware-physical/LavCounter.ino.bin`. A margem de flash é pequena e deve ser reavaliada antes de adicionar funcionalidades. RAM estática não comprova heap disponível durante TLS. O modo simulado foi verificado no sketch nativo, sem recompilação do binário mock para a placa nesta etapa.

## Console serial

Adicionados `ajuda`, `sensor`, `sensor on`, `sensor off` e `i2c`, conforme [instruções do console](CONSOLE-SENSOR.md). A saída inclui distância bruta, validade, status de hardware, idade da amostra, limiar, estado da detecção e erro. Nenhuma segunda aquisição óptica é iniciada; o console observa a aquisição existente. A consulta I²C verifica um dos dois endereços por passagem do loop. A saída respeita o espaço disponível na UART, com blocos de até 64 bytes; entrada limitada a 47 caracteres e processamento de até 32 bytes por passagem.

Após a integração, os sete executáveis de `scripts/test-native.ps1` passaram novamente. Novos casos no sketch real verificam leitura/status/idade, periodicidade de 1 s, interrupção da saída contínua, CRLF, descarte de comando longo com recuperação, resposta/ausência I²C e transmissão sem espera quando o receptor está bloqueado. O perfil mock também compilou e passou no teste nativo. Não houve upload nem teste físico do console.

Compilação física final com console: **aprovada** via `scripts/build.ps1`. Flash: 1.298.168 / 1.310.720 bytes (99%), com 12.552 bytes livres. RAM estática: 54.816 / 327.680 bytes (16%), com 272.864 bytes livres. Binário atualizado em `build/firmware-physical/LavCounter.ino.bin`; log em `build/compile-physical.log`. A primeira tentativa parou na geração de `WebPage.h` com erro de abertura do arquivo; a repetição concluiu a geração e a compilação com sucesso. Nenhuma alteração na interface web foi necessária.
