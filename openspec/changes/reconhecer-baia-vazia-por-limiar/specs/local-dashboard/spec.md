# Painel local — delta

## ADDED Requirements

### Requirement: Diagnóstico de ocupação separado da distância
O sistema SHALL expor no status autenticado e apresentar no painel a classificação presença, ausência ou desconhecido, a razão da classificação, o modelo identificado e a validade da distância, separados da saúde operacional. `distanceMm` SHALL permanecer nulo quando não houver distância válida, inclusive na ausência operacional por status 2. O painel SHALL distinguir ausência por sinal insuficiente de falha de aquisição e SHALL não exibir distância bruta inválida como distância confiável. O console automático SHALL apresentar a mesma classificação e razão sem iniciar uma segunda aquisição nem exigir comandos para detectar.

#### Scenario: Ausência sem distância válida
- **WHEN** o sensor identificado é VL53L1X e sua medição admissível tem status 2
- **THEN** o status informa ausência e motivo sinal insuficiente, distância inválida/nula e saúde operacional disponível; o painel informa que nenhum alvo foi reconhecido pelo critério operacional, sem apresentar distância inventada

#### Scenario: Falha com último valor bruto
- **WHEN** ocorre falha de comunicação, timeout, amostra vencida ou status 7 após uma leitura anterior
- **THEN** o status e o painel informam desconhecido e o motivo da falha, com distância válida indisponível, sem anunciar baia vazia a partir da leitura anterior

#### Scenario: Diagnóstico protegido
- **WHEN** um cliente sem autenticação consulta o status de ocupação
- **THEN** o serviço exige a mesma autenticação local das demais informações do aparelho
