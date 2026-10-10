# Histórico de lavagens — delta

## MODIFIED Requirements

### Requirement: Persistência e recuperação
O sistema SHALL preservar registros confirmados após reinício, distinguir pendências e perdas, não formatar nem apagar histórico automaticamente e manter dados hidráulicos separados. Após reinício SHALL iniciar em IDLE, permitindo nova sessão pela primeira presença válida, sem exigir ausência prévia; sessões interrompidas SHALL não receber horário de saída inventado. A documentação SHALL explicitar que um veículo que permaneceu durante o reinício pode formar uma nova sessão e ser contado novamente após uma nova permanência mínima. A identidade de uma sessão nova SHALL ser distinta da identidade persistida anterior.

#### Scenario: Reinício com veículo presente
- **WHEN** o dispositivo reinicia após uma lavagem persistida e o veículo continua presente
- **THEN** a lavagem permanece no total, o aparelho inicia em IDLE e uma presença válida pode iniciar outra candidata com novo ID, sem contabilização imediata

#### Scenario: Armazenamento indisponível
- **WHEN** a gravação falha ou a fila se esgota
- **THEN** o painel distingue contagem observada de registros confirmados e informa pendências ou perdas, sem anunciar persistência bem-sucedida

#### Scenario: Atualização do firmware hidráulico
- **WHEN** existem configurações e eventos do modelo anterior
- **THEN** o sistema preserva esses dados, inicia configuração de presença independente e não soma eventos hidráulicos nas novas contagens

#### Scenario: Encerramento por ausência operacional
- **WHEN** uma lavagem já validada encerra após ausência confirmada inclusive por status 2 admissível
- **THEN** o encerramento complementa o mesmo ID da validação e as contagens observada e persistida continuam representando uma lavagem
