# Presença veicular

## Purpose

Detectar a permanência de veículos no box para inferir uma lavagem por sessão, com parâmetros locais e sinalização visível.

## ADDED Requirements

### Requirement: Aquisição de presença
O sistema SHALL usar leituras válidas do VL53L1X e um limiar configurável em milímetros para determinar presença. Leituras inválidas ou indisponíveis SHALL ser tratadas como desconhecidas, sem inferir ausência nem somar tempo de presença. O sistema SHALL expor a saúde do sensor.

#### Scenario: Distância dentro do limiar
- **WHEN** uma leitura válida é menor ou igual ao limiar configurado
- **THEN** o sistema reconhece presença; uma leitura válida acima do limiar reconhece ausência

#### Scenario: Falha de aquisição
- **WHEN** o sensor falha, expira ou deixa de fornecer amostras recentes
- **THEN** o diagnóstico indica indisponibilidade e as sequências de temporização contínua são interrompidas, sem encerrar a sessão nem validar lavagem

### Requirement: Validação única por sessão
O sistema SHALL iniciar uma sessão ao reconhecer presença e validar exatamente uma lavagem quando uma sequência contínua de presença atingir o tempo mínimo configurado. O sistema SHALL usar tempo monotônico, independente do RTC, e manter a marca de validação até encerrar a sessão.

#### Scenario: Limite de validação
- **WHEN** a presença contínua atinge o mínimo, inclusive no limite exato
- **THEN** a sessão passa a validada e a contagem aumenta uma única vez

#### Scenario: Permanência prolongada
- **WHEN** o veículo continua presente após a validação
- **THEN** nenhuma lavagem adicional é computada nessa sessão

#### Scenario: Presença interrompida antes de validar
- **WHEN** há ausência ou leitura desconhecida antes de atingir o mínimo
- **THEN** uma nova sequência de presença deve atingir integralmente o mínimo para validar a sessão

### Requirement: Encerramento por ausência
O sistema SHALL encerrar a sessão somente após ausência válida contínua pelo tempo mínimo configurado. Retorno anterior ao limite SHALL manter a mesma sessão e sua marca de validação. Sessão encerrada sem validação SHALL não aumentar o total.

#### Scenario: Saída confirmada e próximo veículo
- **WHEN** a ausência atinge o mínimo e posteriormente há nova presença
- **THEN** a sessão anterior é encerrada e uma nova sessão elegível a uma lavagem é iniciada

#### Scenario: Ausência breve ou falha
- **WHEN** a ausência é interrompida por presença ou leitura desconhecida antes do mínimo
- **THEN** o temporizador de ausência reinicia, preservando a identidade da sessão

### Requirement: Configuração persistente
O sistema SHALL permitir configurar distância máxima de presença, tempo mínimo de validação e tempo mínimo de ausência, com unidades, limites e validação. Salvamento SHALL confirmar somente após persistência; parâmetros de uma sessão SHALL permanecer fixos até seu encerramento.

#### Scenario: Edição durante sessão
- **WHEN** novos parâmetros válidos são salvos durante uma sessão ativa
- **THEN** eles sobrevivem ao reinício e só se aplicam à próxima sessão

#### Scenario: Entrada inválida ou falha de gravação
- **WHEN** o envio contém valores inválidos ou não pode ser persistido
- **THEN** o sistema apresenta erro e mantém a configuração anterior

### Requirement: Semáforo de operação
O sistema SHALL mostrar vermelho sem sessão, amarelo com sessão ainda não validada e verde com sessão validada. Durante a tolerância de saída SHALL manter a cor da sessão. Falha de sensor SHALL indicar diagnóstico no painel e piscar a cor correspondente, sem representar nova validação.

#### Scenario: Ciclo normal
- **WHEN** um veículo chega, permanece até validar e sai pelo tempo mínimo
- **THEN** o semáforo passa de vermelho a amarelo, depois verde e finalmente vermelho

#### Scenario: Sensor indisponível em sessão validada
- **WHEN** a aquisição falha após a validação
- **THEN** o verde pisca e a sessão conserva sua única contagem até uma saída confirmada
