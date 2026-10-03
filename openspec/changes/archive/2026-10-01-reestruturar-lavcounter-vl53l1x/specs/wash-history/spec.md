# Histórico de lavagens

## Purpose

Registrar lavagens inferidas por presença com identidade estável e horário do RTC, permitindo consultar contagens locais confiáveis.

## ADDED Requirements

### Requirement: Registro de validação e encerramento
O sistema SHALL atribuir identificador estável à sessão, registrar a validação quando ela ocorrer e complementar o encerramento sob o mesmo identificador. Histórico e contadores SHALL deduplicar por sessão. Registros SHALL incluir versão, parâmetros utilizados, duração monotônica e horários de início, validação e encerramento quando disponíveis, sem métricas hidráulicas.

#### Scenario: Lavagem ainda presente
- **WHEN** uma sessão atinge a validação e o veículo permanece no box
- **THEN** ela aparece uma vez nas contagens e seu encerramento fica pendente

#### Scenario: Reenvio e encerramento
- **WHEN** a persistência é repetida ou a sessão validada recebe encerramento
- **THEN** o histórico consolidado e as contagens continuam representando uma única lavagem

### Requirement: Relógio e qualidade temporal
O sistema SHALL usar RTC para datas, explicitar horários desconhecidos e ajustes e manter a detecção baseada em tempo monotônico. Contagens diárias SHALL usar a data de validação; validações sem data SHALL permanecer no total, separadas do dia atual.

#### Scenario: RTC inválido ou ajustado
- **WHEN** o RTC está inválido ou é ajustado durante uma sessão
- **THEN** a temporização não muda e o registro informa qualidade temporal, sem inventar data para horários indisponíveis

### Requirement: Persistência e recuperação
O sistema SHALL preservar registros confirmados após reinício, distinguir pendências e perdas, não formatar nem apagar histórico automaticamente e manter dados hidráulicos separados. Após reinício SHALL aguardar ausência confirmada antes de habilitar novas sessões para evitar recontar um veículo que permaneceu presente; sessões interrompidas SHALL não receber horário de saída inventado.

#### Scenario: Reinício com veículo presente
- **WHEN** o dispositivo reinicia após uma lavagem persistida e o veículo continua presente
- **THEN** a lavagem permanece no total e nenhuma nova sessão começa até uma ausência pelo mínimo configurado

#### Scenario: Armazenamento indisponível
- **WHEN** a gravação falha ou a fila se esgota
- **THEN** o painel distingue contagem observada de registros confirmados e informa pendências ou perdas, sem anunciar persistência bem-sucedida

#### Scenario: Atualização do firmware hidráulico
- **WHEN** existem configurações e eventos do modelo anterior
- **THEN** o sistema preserva esses dados, inicia configuração de presença independente e não soma eventos hidráulicos nas novas contagens
