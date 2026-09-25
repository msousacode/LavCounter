# Spec Delta

## Purpose

Separar lavagens prováveis, usos duvidosos e eventos mínimos de água com critérios ajustáveis e justificativa consultável.

## ADDED Requirements

### Requirement: Classificação combinada após encerramento
O sistema SHALL atribuir exatamente uma categoria a cada evento encerrado: lavagem, uso não identificado ou evento descartado. Descarte SHALL exigir simultaneamente duração, volume e tempo efetivo abaixo dos respectivos mínimos. Demais eventos SHALL ser avaliados combinando duração, volume, tempo efetivo, pausas, retomadas e vazão; nenhum critério isolado SHALL bastar para lavagem. Eventos que não satisfazem a combinação SHALL permanecer como uso não identificado. Métricas, motivos, versão do classificador e configuração utilizada SHALL acompanhar o resultado.

#### Scenario: Duração isolada
- **WHEN** a sessão supera quatro minutos, mas não satisfaz os critérios combinados de lavagem
- **THEN** não é classificada como lavagem apenas pela duração

#### Scenario: Ruído mínimo
- **WHEN** duração, volume e tempo efetivo ficam todos abaixo dos mínimos
- **THEN** o evento é marcado como evento descartado com os motivos, mantendo registro auditável

#### Scenario: Uso duvidoso relevante
- **WHEN** há volume relevante, mas padrão insuficiente para lavagem
- **THEN** o evento é registrado como uso não identificado, sem incrementar o total de lavagens

#### Scenario: Lavagem com e sem pausa
- **WHEN** um evento contínuo ou intermitente satisfaz a combinação configurada
- **THEN** é classificado como lavagem, sem obrigatoriedade de uma pausa

### Requirement: Configuração por estabelecimento
O sistema SHALL permitir ao usuário autenticado consultar e alterar pela tela Parametrização, sem editar código, recompilar ou reinstalar firmware, calibração, medição, limites mínimos, espera por retomada e critérios de classificação, validar os valores e persistir a configuração. Alterações SHALL valer apenas para novas sessões; cada sessão SHALL usar uma configuração consistente do início ao fim. Valores padrão SHALL ser apresentados como iniciais para calibração local, sem promessa de precisão validada em campo.

#### Scenario: Mudança durante sessão
- **WHEN** uma configuração válida é salva durante uso de água
- **THEN** a sessão corrente usa os valores anteriores e a próxima usa a nova revisão, inclusive após reinício

#### Scenario: Configuração inválida
- **WHEN** são enviados valores negativos, calibração zero ou timeout menor que a detecção de pausa
- **THEN** o sistema rejeita a alteração e preserva a configuração válida

#### Scenario: Timeout não é classificação isolada
- **WHEN** a inatividade alcança o timeout definido pelo usuário
- **THEN** o evento é encerrado e avaliado por todos os critérios; uma lavagem válida não é rejeitada somente porque seu fluxo terminou

#### Scenario: Recalibração sem alterar firmware
- **WHEN** o usuário salva novos limiares e reinicia o dispositivo
- **THEN** novas sessões usam esses valores sem recompilação, mantendo inalterados os eventos e configurações históricas
