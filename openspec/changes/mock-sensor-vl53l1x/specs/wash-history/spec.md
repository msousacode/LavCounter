## ADDED Requirements

### Requirement: Histórico simulado identificado e isolado
O sistema SHALL identificar sessões simuladas nas APIs e no painel e persistir seus registros separadamente do histórico físico, reutilizando regras de validação, consolidação, qualidade temporal e recuperação. Contagens e paginação SHALL abranger apenas o modo ativo. Dados físicos existentes SHALL ser preservados e continuar legíveis sem migração destrutiva. RTC indisponível SHALL não impedir simulações, mantendo horários desconhecidos e contagem sem data.

#### Scenario: Persistência e reinício do mock
- **WHEN** uma lavagem simulada é persistida e a ESP32 reinicia
- **THEN** ela permanece uma única vez no histórico e nas contagens de teste, identificada como simulada

#### Scenario: Alternância entre firmwares
- **WHEN** o operador alterna entre firmware mock e físico sem apagar o armazenamento
- **THEN** cada modo consulta somente seus próprios registros e nenhuma lavagem simulada entra no total físico

#### Scenario: Simulação sem RTC válido
- **WHEN** uma lavagem simulada é validada sem data confiável
- **THEN** ela integra o total sem data, sem horários inventados nem contagem diária indevida
