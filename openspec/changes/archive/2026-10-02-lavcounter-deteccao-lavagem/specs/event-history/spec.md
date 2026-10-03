# Spec Delta

## Purpose

Preservar eventos encerrados e permitir consultar suas medições e classificações para testes e ajuste do LavCounter.

## ADDED Requirements

### Requirement: Histórico persistente e identificável
O sistema SHALL persistir eventos de todas as categorias com identificador único entre reinícios, horários e qualidade do RTC, métricas, classificação, motivos e versões de firmware, esquema e configuração. Consultas SHALL distinguir claramente usos não identificados e descartes, sem misturá-los à contagem de lavagens.

#### Scenario: Reinício após gravação
- **WHEN** o dispositivo reinicia após confirmar a gravação de um evento
- **THEN** o evento permanece consultável e novos eventos recebem identificadores diferentes

### Requirement: Falhas de armazenamento visíveis
O sistema SHALL verificar a gravação completa, expor falhas e não confirmar persistência quando houver erro. Não SHALL formatar automaticamente o armazenamento nem apagar histórico para ocultar falhas ou falta de espaço. Consulta SHALL tolerar registro incompleto sem invalidar os demais e informar degradação. Sessão ativa em queda de energia não tem recuperação garantida nesta versão, e essa limitação SHALL constar da documentação.

#### Scenario: Armazenamento cheio
- **WHEN** um evento não pode ser gravado por falta de espaço
- **THEN** há indicação explícita na interface e no diagnóstico, sem apagar eventos anteriores nem informar sucesso

#### Scenario: Última linha incompleta
- **WHEN** um registro incompleto está presente no histórico
- **THEN** registros íntegros continuam consultáveis e a falha é sinalizada

### Requirement: Consulta limitada
A consulta SHALL permitir paginação e filtro por categoria com limite de registros por resposta, sem carregar todo o histórico em memória.

#### Scenario: Histórico extenso
- **WHEN** o usuário consulta uma página de eventos filtrados
- **THEN** recebe apenas a página solicitada e indicação de continuação, enquanto a medição permanece ativa
