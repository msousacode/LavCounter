# API local — contrato v2

Todas as rotas exigem HTTP Basic (`lavcounter` / `lav123`), inclusive consultas e rotas inexistentes. A rede SoftAP é LavCounter, senha lavcounter123; URL http://192.168.4.1. JSON usa UTF-8 e Cache-Control: no-store. O serviço é local, sem TLS ou dependência de internet.

| Rota | Contrato |
| --- | --- |
| GET / | Painel autocontido |
| GET /api/status | schemaVersion=2, source=local, state, distanceMm (null se inválida), sensorHealthy, sensorError, ledColor, ledBlinking, session |
| GET /api/config | values, defaults, revision, fields com unidade/limites/ajuda, error |
| POST /api/config | revision e values com os três parâmetros inteiros |
| GET/POST /api/rtc | valid, dateTime e error; escrita recebe dateTime local YYYY-MM-DDTHH:mm:ss, 2000–2099 |
| GET /api/sessions | events consolidados, nextCursor como string, hasMore, degraded; limit 1–50 (padrão 20), cursor inicial 0 |

Status inclui observedWashes, persistedWashes, todayWashes (null se RTC inválido), undatedWashes, queuedEvents, lostEvents, storageError, storageDegraded e uso/capacidade. O total observado inclui validações vistas nesta execução e as recuperadas no boot. O total persistido só inclui registros confirmados. Contagem diária e sem data incluem registros persistidos e em fila; perdas explicitamente sinalizadas podem torná-las incompletas. A passagem da fila ao disco não aumenta a contagem novamente. A data é a da validação, não a do encerramento. Dados sem data não são atribuídos retroativamente ao dia atual.

Exemplo de configuração (use a revisão recebida, não uma constante):

```json
{"revision":1,"values":{"presenceMaxMm":1500,"washMinSeconds":120,"absenceMinSeconds":10}}
```

Valores inválidos retornam 400, revisão desatualizada 409, gravação falha 500 e RTC/histórico indisponível 503. Sucesso de gravação da configuração significa persistência confirmada. Sessão ativa mantém o snapshot anterior. O filtro category foi removido e retorna 400. Campos hidráulicos não fazem parte do contrato v2.

## Journal e consultas

Novo arquivo `/presence-v2.jsonl`; os arquivos antigos permanecem intactos e não são somados. Cada sessão recebe sessionId decimal como string; validação e encerramento usam o mesmo ID e type `validated` ou `closed`. Campos: schemaVersion=2, firmwareVersion, validated, startedAt, validatedAt, closedAt, durationMs, config, timeBasis=local-rtc, timeQuality e clockAdjusted. Horários desconhecidos são null. Uma sessão encerrada sem validação tem validated=false. A sessão corrente no status também pode ter type=candidate.

A consulta consolida os dois registros por ID e acrescenta interrupted=true para uma sessão persistida sem encerramento encontrada no boot. Seu horário de saída permanece null. O cursor é o último ID retornado, reutilizado literalmente pelo cliente. Atualize desde cursor 0 para observar encerramentos de sessões já exibidas. A ordem é crescente. A leitura usa memória limitada e cede execução à aquisição.

A fila contém até oito registros; grava um registro por passagem do loop e repete tentativas após falhas a cada cinco segundos. Falhas, linhas incompletas e perdas são explícitas. Reenvios são consolidados sem duplicar sessões. O journal segue IDs crescentes e a consulta rejeita regressões, sinalizando degradação. Não existe exclusão automática ou formatação pelo firmware. Uma perda de energia pode perder itens da fila e contagens só observadas. Faça backup do filesystem e NVS juntos; os IDs residem na NVS.

## Migração e rollback

Antes de atualizar uma unidade, exporte o histórico pela API antiga e preserve firmware, SPIFFS e NVS. A nova chave `presenceConfig` não interpreta a chave antiga `config`. A sequência nextId é mantida e conferida contra o maior ID do novo histórico. O firmware anterior pode voltar a usar os dados preservados; registros por presença não são convertidos em registros hidráulicos. Não apague NVS isoladamente. Dispositivos sem SPIFFS precisam de inicialização explícita conforme sua tabela de partições, após backup.
