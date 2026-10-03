## MODIFIED Requirements

### Requirement: Aquisição de presença
O sistema SHALL usar leituras válidas do VL53L1X e um limiar configurável em milímetros para determinar presença no modo físico padrão. Quando explicitamente compilado em modo mock, SHALL usar entradas simuladas de presença, ausência ou falha, mantendo as mesmas regras de detecção. Leituras inválidas ou indisponíveis SHALL ser tratadas como desconhecidas, sem inferir ausência nem somar tempo de presença. O sistema SHALL expor a saúde e o modo de aquisição do sensor.

#### Scenario: Distância dentro do limiar
- **WHEN** uma leitura física válida é menor ou igual ao limiar configurado
- **THEN** o sistema reconhece presença; uma leitura válida acima do limiar reconhece ausência

#### Scenario: Falha de aquisição
- **WHEN** o sensor falha, expira ou deixa de fornecer amostras recentes
- **THEN** o diagnóstico indica indisponibilidade e as sequências de temporização contínua são interrompidas, sem encerrar a sessão nem validar lavagem

#### Scenario: Aquisição simulada
- **WHEN** o modo mock fornece presença, ausência ou falha
- **THEN** a detecção recebe respectivamente presença, ausência ou leitura desconhecida e a API identifica a origem simulada sem apresentar distância como medição física
