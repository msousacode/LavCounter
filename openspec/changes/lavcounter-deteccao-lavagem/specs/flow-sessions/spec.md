# Spec Delta

## Purpose

Registrar sessões automáticas de uso de água com métricas de fluxo e horários confiáveis para classificação posterior.

## ADDED Requirements

### Requirement: Sessões automáticas e retomadas
O firmware SHALL monitorar fluxo sem tag ou botão, iniciar uma sessão ao detectar pulsos válidos, entrar em WAITING_RESUME após ausência configurada de pulsos e encerrar após timeout de inatividade contado desde o último pulso válido. O usuário SHALL poder editar esse timeout pela web sem alteração de código. O timeout SHALL acionar a classificação combinada, sem definir sozinho a categoria. Retomada anterior ao limite SHALL preservar a sessão; no limite ou depois SHALL iniciar outra sessão após finalizar a anterior uma única vez.

#### Scenario: Pausa longa dentro do limite
- **WHEN** o fluxo retorna antes do timeout configurado
- **THEN** a mesma sessão continua, acumulando uma pausa e uma retomada sem perder volume

#### Scenario: Encerramento e fronteira
- **WHEN** a inatividade atinge o timeout, inclusive quando chega novo pulso exatamente no limite
- **THEN** a sessão anterior é encerrada uma vez e o novo pulso pertence a uma nova sessão

### Requirement: Métricas de fluxo
Cada sessão SHALL registrar pulsos válidos, volume fracionário em litros, duração total, tempo efetivo com fluxo, tempo pausado, quantidade de pausas e retomadas e vazão média durante fluxo em L/min. A duração total SHALL ir do primeiro ao último pulso, excluindo a espera terminal; pausas internas SHALL ser incluídas. O intervalo de medição e a calibração SHALL ser documentados e configuráveis, com contagem segura durante atendimento web.

#### Scenario: Volume inferior a um litro
- **WHEN** são recebidos 225 pulsos válidos com calibração de 450 pulsos/L
- **THEN** o volume registrado é 0,5 L, sem truncamento para zero

#### Scenario: Sessão intermitente
- **WHEN** há dois trechos de fluxo separados por pausa e depois encerramento
- **THEN** o evento informa uma pausa interna, uma retomada e duração total igual ao tempo com fluxo mais tempo pausado, dentro da resolução documentada

### Requirement: RTC e tempo decorrido
O sistema SHALL usar RTC para início, último fluxo e encerramento, mantendo duração e timeouts independentes de ajustes do calendário. Horários inválidos SHALL ser nulos com indicação de qualidade; falha do RTC não SHALL impedir aquisição de fluxo nem fabricar datas válidas.

#### Scenario: RTC ajustado durante sessão
- **WHEN** o calendário é corrigido durante uma sessão
- **THEN** duração e timeout permanecem corretos e o evento sinaliza descontinuidade temporal

#### Scenario: RTC indisponível
- **WHEN** o RTC falha na leitura do início ou fim
- **THEN** a sessão mantém métricas e marca o horário afetado como indisponível
