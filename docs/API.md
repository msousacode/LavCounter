# API local — contrato v2

O status autenticado inclui `presenceSample` (`present`, `absent`, `unknown`), `presenceReason`, `distanceValid` e `sensorRangeStatus` (status traduzido do VL53L1X; bruto de 4 bits do VL53L0X). Razões: `within_threshold`, `beyond_threshold`, `signal_insufficient`, `invalid_range`, `i2c_error`, `timeout`, `stale`, `initializing`. No VL53L1X, status 2 com aquisição íntegra retorna `absent`, `signal_insufficient`, `distanceValid=false`, `distanceMm=null`, `sensorHealthy=true` e erro vazio. Falhas reais retornam `unknown`, saúde falsa e distância nula. Diagnósticos expiram após 500 ms sem nova amostra. Ausência operacional ainda precisa cumprir o tempo configurado para encerrar a sessão; não comprova fisicamente baia vazia.

Todas as rotas exigem HTTP Basic (`lavcounter` / `lav123`), inclusive consultas e rotas inexistentes. A rede SoftAP é LavCounter, senha lavcounter123; URL http://192.168.4.1. JSON usa UTF-8 e Cache-Control: no-store. O serviço é local, sem TLS ou dependência de internet.

| Rota | Contrato |
| --- | --- |
| GET / | Painel autocontido |
| GET /api/status | schemaVersion=2, source=local, state, distanceMm (null se inválida), sensorModel, sensorHealthy, sensorError, ledColor, ledBlinking, session |
| POST /api/mock-sensor | Somente firmware mock: sample=present/absent/fault; 200 confirma entrada, 400 rejeita corpo inválido, 404 no físico |
| GET /api/config | values, defaults, revision, fields com unidade/limites/ajuda, error |
| POST /api/config | revision e values com os três parâmetros inteiros |
| GET/POST /api/rtc | valid, dateTime e error; escrita recebe dateTime local YYYY-MM-DDTHH:mm:ss, 2000–2099 |
| GET /api/sessions | events consolidados, nextCursor como string, hasMore, degraded; limit 1–50 (padrão 20), cursor inicial 0 |

Status inclui observedWashes, persistedWashes, todayWashes (null se RTC inválido), undatedWashes, queuedEvents, lostEvents, storageError, storageDegraded e uso/capacidade. O total observado inclui validações vistas nesta execução e as recuperadas no boot. O total persistido só inclui registros confirmados. Contagem diária e sem data incluem registros persistidos e em fila; perdas explicitamente sinalizadas podem torná-las incompletas. A passagem da fila ao disco não aumenta a contagem novamente. A data é a da validação, não a do encerramento. Dados sem data não são atribuídos retroativamente ao dia atual.

`sensorModel` informa VL53L0X, VL53L1X ou ToF nao identificado. Status inclui `sensorMode` sempre igual a `physical`. `distanceMm` contém a distância válida do VL53L0X ou VL53L1X, ou `null` quando o sensor está indisponível ou a leitura é inválida. Sessões incluem `sensorMode`; registros físicos antigos são normalizados na leitura. O campo `mockSample` e a rota `/api/mock-sensor` foram removidos.

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
# Nuvem e Premium

Todas as novas rotas exigem o mesmo HTTP Basic local. `GET /api/cloud-sync` retorna identidade, configured, wifiConnected, premiumStatus, state, lastError sanitizado, pendingVersions/confirmedVersions, countsComplete e lastCompletedAt (null sem RTC confiável). Contadores completos descrevem apenas a revisão atual de cada sessão. Durante varredura, são null; cycleConfirmed/cyclePending mostram o andamento parcial. Um envio aceito não comprova sucesso remoto.

`POST /api/cloud-sync` aceita corpo vazio ou `{}` sem destino/IDs/flag premium: 202 solicitado, 200 trabalho existente, 403 premium não autorizado, 503 configuração/conectividade/identidade indisponível, 400 parâmetros. `GET /api/premium-link` retorna URL HTTPS com clientId/deviceId persistidos ou 503 com motivo. Premium inativo não bloqueia cadastro; URL base ainda não fornecida é indisponível. Nenhuma senha/token segue na URL; não existe código temporário de ativação.

Fonte de upload: journal v2 durável, não fila RAM. Ciclo captura limite do arquivo, consolida validação/encerramento e usa SHA-256 da serialização canônica do snapshot como syncRevision. Journals adicionais `/cloud-v1.jsonl` e `/cloud-mock-v1.jsonl` guardam version=1, binding, sessionId textual, revision e checksum SHA-256 canônico desses campos. Somente resposta REST 2xx completa, com nome e revisão correspondentes, seguida de gravação da confirmação permite considerar a versão confirmada. Falha na gravação mantém pendência e reenvia o mesmo documento. Truncamento/corrupção de confirmação causa reenvio conservador; não há limpeza/formatação automática. A falta de espaço é visível e exige preservação/exportação e manutenção explícita.

Boot de sessão validada sem encerramento deriva interrupted=true sem inventar closedAt; isso é uma nova revisão remota. Encerramento após envio da validação permanece pendente mesmo com mesmo sessionId. Histórico hidráulico é preservado sem upload; modos físico/mock têm documentos, journals e vínculos separados. Consulte [NUVEM.md](NUVEM.md) para configuração e contrato futuro de autorização.

O payload Firestore inclui syncStage inteiro (1 validada, 2 interrompida, 3 encerrada). O contrato de regras/backend exige progressão monotônica e equivalência da revisão para reenvio no mesmo stage; isso impede escrita antiga atrasada de regredir sessão já encerrada.

No firmware privado de integração, `GET /api/cloud-sync` também informa `testMode`, `testUntil` (segundos UTC) e `authUid` (identificador público do usuário de bancada). A autorização de teste substitui apenas a exigência de compra/premium remoto durante o prazo explícito; autenticação Firebase, vínculo ao aparelho, TLS, confirmação durável e regras Firestore continuam obrigatórios. `POST /api/cloud-sync` não aceita parâmetros para liberar ou prolongar o teste. Sem configuração privada, `testMode=false`, `testUntil=0` e `authUid=""`. Consulte [NUVEM.md](NUVEM.md) para preparação, publicação das regras e encerramento do teste.

## Wi-Fi externo pelo painel

`GET /api/cloud-wifi` exige HTTP Basic e informa `wifiConfigured`, `wifiConnected`, `wifiSsid`, `wifiState` (`not_configured`, `connecting`, `connected`, `error`) e `wifiError`. Esses campos também aparecem em `GET /api/cloud-sync`. Conexão Wi-Fi não comprova acesso à internet; autenticação/TLS e envio ao Firebase têm diagnóstico próprio.

`POST /api/cloud-wifi` aceita somente `ssid` e `password` como strings. SSID tem de 1 a 32 bytes UTF-8; senha de 8 a 63 bytes ou vazia para rede aberta. Retorna 202 após persistir os dois campos em um único registro NVS no namespace `lavWifi`, 400 para entrada inválida e 500 para falha de gravação. A senha não aparece nas respostas. A rede salva é comum aos perfis físico/simulado, tem prioridade sobre o arquivo privado e é reutilizada no boot. A tentativa de conexão é iniciada pelo loop após responder ao painel, mantendo o AP LavCounter ativo.

Sem Wi-Fi externo, Sincronizar com a nuvem abre o formulário de rede e senha. Conectar e sincronizar aguarda conexão e autorização Firebase; o coordenador dispara o envio ao conectar. Configurar Wi-Fi permite corrigir ou trocar a rede. AP+STA pode mudar de canal e desconectar momentaneamente o navegador: reconecte ao LavCounter e reabra o painel; a conexão e sincronização continuam no aparelho.

## Formatos de exibição

O painel usa `dd/MM/YYYY HH:mm` para início, encerramento, validação, relógio local, última sincronização e expiração da autorização. Datas do RTC são exibidas sem conversão de fuso; a expiração UTC é exibida em America/Sao_Paulo. Duração no histórico e na sessão ativa usa `hh:mm:ss`, descartando a fração de segundo e preservando horas acima de 24. O formulário do relógio oferece o seletor nativo de data e hora, valida o calendário e envia a representação ISO local com segundos zero. APIs, histórico e Firestore mantêm seus formatos originais e a precisão armazenada.

Eventos: Sessão / Início, Encerramento, Duração, Resultado, Validação.

O formulário de ajuste do RTC usa `datetime-local`, com seletor nativo de calendário e horário do navegador/celular, precisão de minutos e anos entre 2000 e 2099. Carrega e envia a hora local em ISO, sem conversão de fuso; o POST acrescenta segundos zero. A apresentação do campo segue o idioma do navegador; os demais horários do painel permanecem em `dd/MM/YYYY HH:mm`. Erro 503 indica falha de escrita/leitura do RTC, distinta de formato inválido.


