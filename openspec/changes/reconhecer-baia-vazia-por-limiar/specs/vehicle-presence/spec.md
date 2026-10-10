# Presença veicular — delta

## MODIFIED Requirements

### Requirement: Aquisição de presença
O sistema SHALL identificar automaticamente o VL53L0X ou VL53L1X real, adquirir sem comandos manuais e usar um limiar configurável em milímetros, com padrão de 1500 mm. Distância válida menor ou igual ao limiar SHALL representar presença; distância válida maior SHALL representar ausência. Uma medição recente e completa do VL53L1X com status 2 (sinal insuficiente), sem falha de transporte, confirmação ou timeout, SHALL representar ausência operacional, sem considerar sua distância bruta válida. Essa exceção SHALL não se aplicar aos status brutos do VL53L0X. Status 7 e demais resultados não aceitos, sensor desconhecido, amostra expirada, falha de comunicação, resposta incompleta, erro na confirmação e timeout SHALL representar desconhecido, sem inferir ausência nem somar tempo de presença. O sistema SHALL expor separadamente saúde operacional, validade da distância e classificação de presença, sem modo simulado ou comandos de simulação no painel.

#### Scenario: Distância dentro do limiar
- **WHEN** uma leitura válida é menor ou igual ao limiar configurado
- **THEN** o sistema reconhece presença; uma leitura válida acima do limiar reconhece ausência

#### Scenario: Limite exato de 1,5 m
- **WHEN** o limiar é 1500 mm e chegam leituras válidas de 1499, 1500 e 1501 mm
- **THEN** as duas primeiras representam presença e a última representa ausência

#### Scenario: Ausência com sinal insuficiente
- **WHEN** chega uma medição completa e recente do VL53L1X com status 2, comunicação e confirmação concluídas e sem timeout
- **THEN** a classificação é ausência, a distância é inválida e a aquisição permanece operacional, qualquer que seja o valor bruto retornado

#### Scenario: Falha de aquisição
- **WHEN** o sensor falha, expira ou deixa de fornecer amostras recentes, inclusive ao ocorrer timeout ou falha de transferência/confirmacão junto a status 2
- **THEN** o diagnóstico indica indisponibilidade e as sequências de temporização contínua são interrompidas, sem encerrar a sessão nem validar lavagem

#### Scenario: Status não aceito
- **WHEN** uma medição do VL53L1X tem status 7 ou outro status diferente de 0 e 2, ou uma leitura VL53L0X não satisfaz seu critério de validade
- **THEN** a classificação é desconhecido, mesmo que a distância bruta esteja acima ou abaixo do limiar

### Requirement: Encerramento por ausência
O sistema SHALL encerrar a sessão após ausência operacional contínua pelo tempo mínimo configurado, considerando tanto distância válida acima do limiar quanto status 2 admissível do VL53L1X. Esses dois tipos de ausência SHALL poder compor a mesma sequência contínua. Retorno anterior ao limite SHALL manter a mesma sessão e sua marca de validação. Presença ou desconhecido SHALL reiniciar o intervalo de ausência. Sessão encerrada sem validação SHALL não aumentar o total. Ao encerrar SHALL voltar a IDLE e permitir nova sessão com novo identificador, sem aumentar novamente o total de uma sessão já validada.

#### Scenario: Saída confirmada e próximo veículo
- **WHEN** a ausência atinge o mínimo e posteriormente há nova presença
- **THEN** a sessão anterior é encerrada e uma nova sessão elegível a uma lavagem é iniciada

#### Scenario: Ausência breve ou falha
- **WHEN** a ausência é interrompida por presença ou leitura desconhecida antes do mínimo
- **THEN** o temporizador de ausência reinicia, preservando a identidade da sessão

#### Scenario: Saída sem fundo mensurável
- **WHEN** uma sessão validada recebe somente medições admissíveis com status 2 durante todo o tempo de ausência
- **THEN** encerra uma única vez no limite exato, mantém sua única contagem e retorna a IDLE

#### Scenario: Alternância de evidências de ausência
- **WHEN** leituras válidas acima do limiar alternam com status 2 admissível, sem lacuna expirada, presença ou desconhecido
- **THEN** o intervalo de ausência continua sem reiniciar e encerra no tempo configurado

#### Scenario: Saída antes de validar
- **WHEN** a sessão candidata não alcança a permanência mínima e completa a ausência mínima
- **THEN** encerra sem contabilizar lavagem e volta a IDLE

### Requirement: Semáforo de operação
O sistema SHALL mostrar vermelho sem sessão, amarelo com sessão ainda não validada e verde com sessão validada. Durante a tolerância de saída SHALL manter a cor da sessão. Ausência operacional por status 2 admissível SHALL não causar indicação de falha ou piscar por si só; ao completar a ausência SHALL retornar ao vermelho. Falha de sensor ou classificação desconhecida SHALL indicar diagnóstico no painel e piscar a cor correspondente, sem representar nova validação.

#### Scenario: Ciclo normal
- **WHEN** um veículo chega, permanece até validar e sai pelo tempo mínimo
- **THEN** o semáforo passa de vermelho a amarelo, depois verde e finalmente vermelho

#### Scenario: Sensor indisponível em sessão validada
- **WHEN** a aquisição falha após a validação
- **THEN** o verde pisca e a sessão conserva sua única contagem até uma saída confirmada

#### Scenario: Baia vazia com sinal insuficiente
- **WHEN** não há sessão ativa e chegam medições admissíveis com status 2
- **THEN** o sistema permanece em IDLE com vermelho fixo e informa ausência operacional, sem apresentar uma distância válida

## ADDED Requirements

### Requirement: Inicialização pronta para detectar
O sistema SHALL iniciar em IDLE após ligar ou reiniciar, sem exigir confirmação prévia de baia vazia. A primeira presença válida SHALL iniciar uma candidata; a permanência mínima contínua SHALL continuar obrigatória para validar e contabilizar. Leituras desconhecidas SHALL não iniciar sessões nem contabilizar lavagens.

#### Scenario: Reinício com objeto próximo
- **WHEN** o aparelho reinicia e recebe uma primeira distância válida dentro do limiar
- **THEN** parte de IDLE, inicia CANDIDATE e somente contabiliza após cumprir integralmente a permanência mínima

#### Scenario: Reinício sem aquisição válida
- **WHEN** o aparelho reinicia e recebe somente resultados desconhecidos
- **THEN** permanece em IDLE, sem sessão ou contagem nova, com diagnóstico e vermelho piscante de falha
