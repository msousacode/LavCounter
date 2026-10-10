# Console serial do sensor

Carregue o firmware na ESP32 e abra o Monitor Serial em **115200 baud**. Identificação, aquisição e detecção são automáticas, inclusive com o monitor fechado. O console mostra uma linha a cada segundo sem comandos. Para comandos, selecione Nova linha ou Ambos NL e CR.

| Comando | Resultado |
| --- | --- |
| ajuda / help | Lista os comandos |
| sensor | Mostra a última aquisição |
| sensor on | Retoma a saída a cada segundo |
| sensor off | Silencia apenas o console; a detecção continua |
| i2c | Consulta 0x29 (ToF) e 0x68 (RTC), um endereço por passagem |

Exemplos de trechos da saída:

```text
[VL53L1X] id=0xEACC valido=sim bruto_mm=1000 status=0 ... ocupacao=present motivo=within_threshold saudavel=sim erro=
[VL53L1X] id=0xEACC valido=nao bruto_mm=2600 status=2 ... ocupacao=absent motivo=signal_insufficient saudavel=sim erro=
[VL53L1X] id=0xEACC valido=nao bruto_mm=700 status=7 ... ocupacao=unknown motivo=invalid_range saudavel=nao erro=Leitura invalida do VL53L1X
```

`valido` representa a validade da distância, não a classificação de ocupação. `bruto_mm` guarda a última distância recebida, inclusive quando inválida; não use esse número como medida se `valido=nao`. `saudavel` indica se há classificação operacional recente. `ocupacao` é present/absent/unknown; `motivo` explica a classificação. `idade_ms` é a idade da última medição completa confirmada, ou sem_amostra. Estado e limiar são os do fluxo de reconhecimento, usando o snapshot da sessão ativa.

VL53L0X: ID 0x00EE, status bruto válido 11, distância maior que zero e diferente de 65535. VL53L1X: ID 0xEACC, status traduzido válido 0. Em ambos, distância válida até o limiar é presença, e acima é ausência. Somente no VL53L1X status traduzido 2 admissível indica ausência operacional, independentemente da distância bruta inválida. Essa regra não comprova baia fisicamente vazia; objetos com retorno fraco também podem gerar sinal insuficiente.

Status 7 e demais status não aceitos indicam unknown. Falha de leitura I²C, resposta curta, erro de calibração/DSS/confirmacão ou timeout prevalecem sobre status 2 e nunca confirmam saída. Amostra sem atualização expira em 500 ms. Falhas de comunicação provocam nova identificação/inicialização após 5 s. Uma resposta em 0x29 não confirma o modelo: veja o ID.

O console observa a aquisição existente, sem segunda medição. Entrada limitada a 47 caracteres, processamento de até 32 bytes por passagem e saída em blocos de até 64 bytes conforme espaço UART, sem espera pelo receptor. Testes de presença que atinjam a permanência configurada podem registrar lavagens reais no histórico.
