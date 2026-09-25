# API e histórico

Todas as rotas, incluindo consultas, exigem HTTP Basic com `lavcounter` / `lav123`. Respostas JSON usam UTF-8 e `Cache-Control: no-store`. A API não oferece CORS. Erros de validação retornam 400, configuração desatualizada 409 e falhas de persistência 500/503. Rotas inexistentes também exigem autenticação antes de retornar 404.

| Rota | Comportamento |
| --- | --- |
| GET / | HTML/CSS/JS autocontidos |
| GET /api/status | Estado, volume ativo, vazão, pulsos, configuração ativa e falhas de RTC/armazenamento |
| GET /api/config | values, defaults, revision, fields com unidades/limites/ajuda e erro de configuração |
| POST /api/config | Objeto com revision atual e values contendo todos os campos; valida e persiste atomicamente |
| GET /api/rtc | valid, dateTime local e error |
| POST /api/rtc | dateTime no formato YYYY-MM-DDTHH:mm:ss, ano 2000–2099 |
| GET /api/sessions | Histórico paginado; limit 1–50 (padrão 20), cursor e category opcionais |

Categorias: `washing`, `unidentified`, `discarded`. `category` vazio consulta todas. Resposta de sessões: `events`, `nextCursor`, `hasMore`, `degraded`. Cursor é opaco para o cliente e deve ser reutilizado como retornado. Ordem de registro crescente. A leitura é incremental, cedendo execução à aquisição entre registros. Use páginas até `hasMore=false` para exportar um backup; mantenha o JSON completo.

Exemplo de ajuste de relógio:

```json
{"dateTime":"2026-09-25T14:30:00"}
```

Para configuração, obtenha GET /api/config, copie `values`, altere os campos desejados e envie junto da `revision` recebida. A revisão impede sobrescrever alterações feitas em outra aba. Somente sucesso confirma que os dados foram persistidos. Eventos anteriores retêm seus snapshots.

## Registro

Arquivo `/events-v1.jsonl`, um objeto por linha. Cada evento inclui:

- `id`: sequência persistente representada como string para não perder precisão no navegador.
- `schemaVersion`, `firmwareVersion`, `classifierVersion`.
- `startedAt`, `finishedAt`, `closedAt`, `timeBasis=local-rtc`, `timeQuality` e `clockAdjusted`.
- `pulses`, `volumeL`, `durationMs`, `flowMs`, `pauseMs`, `maxPauseMs`, `pauseCount`, `resumeCount`, `avgFlowLpm`.
- `category`, `score`, `evidence` (seis booleanos), `reasons`, `acquisitionDegraded`.
- `config`: valores efetivamente utilizados e sua revisão.

Horários inválidos e vazão média sem tempo de fluxo são `null`. `rtc-projected` indica projeção a partir da âncora RTC, `adjusted` indica ajuste durante sessão e `unavailable` indica algum horário indisponível. Descartes e usos não identificados permanecem no mesmo arquivo com categoria explícita.

## Falhas e retenção

O firmware verifica bytes gravados e mantém até oito eventos pendentes, tentando a cada cinco segundos. IDs permitem evitar duplicação no reenvio. Uma linha incompleta é preservada e sinalizada; as demais continuam acessíveis. Armazenamento cheio não apaga histórico nem produz confirmação falsa. `queuedEvents`, `lostEvents`, `storageError` e `storageDegraded` devem ser acompanhados no Status.

SPIFFS não é formatado automaticamente. Para primeira instalação, gere uma imagem vazia com o `mkspiffs` distribuído pelo core ESP32, conforme partição compilada; grave-a explicitamente apenas em dispositivo novo ou após backup. A partição `default` de 4 MB do core 3.3.10 usa offset `0x290000`, tamanho `0x160000`. Não aplique esses endereços a outra tabela de partição. O firmware de produção não disponibiliza exclusão em massa pela web.

Não há importação automática de `/sessions.jsonl` legado. Rollback requer firmware original e backup correspondente; não apague NVS isoladamente mantendo histórico novo, pois a sequência de IDs reside na NVS.
