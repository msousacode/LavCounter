## ADDED Requirements

### Requirement: Controles locais de simulação
O painel SHALL apresentar no modo mock a indicação persistente de simulação, a entrada atual e botões Veículo presente, Box vazio e Falha do sensor. SHALL mostrar que os tempos configurados continuam valendo e atualizar estado, LEDs, contagens e histórico a partir das respostas da ESP32. SHALL indicar erros de comando sem anunciar sucesso indevido e recuperar a entrada atual após recarregar a página. No modo físico SHALL ocultar os controles e a API SHALL rejeitar comandos de mock.

#### Scenario: Simulação offline
- **WHEN** o operador autenticado acessa o painel mock sem internet e seleciona Veículo presente
- **THEN** recebe confirmação da entrada e acompanha a validação somente após o tempo mínimo configurado

#### Scenario: Falha de comunicação
- **WHEN** o envio de um comando falha
- **THEN** o painel informa o erro e consulta novamente a ESP32 para reconciliar a entrada, sem presumir que o comando foi aplicado

#### Scenario: Recarregamento do painel
- **WHEN** a página é recarregada durante uma sessão simulada
- **THEN** mostra entrada e sessão mantidas pela ESP32 sem reiniciá-las

#### Scenario: Firmware físico
- **WHEN** o dispositivo está em modo físico
- **THEN** o painel não oferece controles de simulação e chamadas diretas de mock são rejeitadas
