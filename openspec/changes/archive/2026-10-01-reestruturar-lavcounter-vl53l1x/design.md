# Design

## Context

Ver `proposal.md` para motivação. O firmware atual separa detecção, configuração NVS, RTC DS3231, eventos JSONL em SPIFFS e servidor autenticado. `web/index.html` gera `WebPage.h`. A aquisição atual usa ISR de pulsos e classificação hidráulica; ambas serão substituídas. Não existem specs principais publicadas. As outras duas mudanças abertas permanecem como contexto histórico, sem serem arquivadas por esta implementação.

## Goals / Non-Goals

**Goals:** núcleo determinístico testável sem hardware; aquisição com latência limitada; contrato de sessão por presença; preservação dos dados anteriores; operação inteiramente local.

**Non-Goals:** comprovar uso de água, identificar placa/cliente, distinguir dois carros sem ausência intermediária, implementar cobrança ou SaaS, recuperar perfeitamente sessões ainda não persistidas em corte de energia.

## Decisions

1. Substituir `Detection.h` por um núcleo que recebe amostras `present`, `absent` ou `unknown` e relógio monotônico. Estados: `WAIT_CLEAR` no boot, `IDLE`, `CANDIDATE`, `VALID_WASH`. Ausência é um temporizador dentro da sessão, não uma nova sessão. Validar em `>= mínimo`, uma vez; ausência breve conserva identidade mas reinicia presença contínua antes de validar. Uma amostra desconhecida reinicia ambos os intervalos contínuos, preservando validação anterior. A alternativa de tratar erro como ausência poderia dividir uma única lavagem.

2. Criar adaptador VL53L1X independente do domínio. Reutilizar o barramento Wire do RTC; a implementação deve verificar documentação oficial do módulo/driver, fixar dependência e confirmar compatibilidade elétrica, endereço e orçamento temporal antes da integração. Polling limitado, sem espera indefinida, com leitura inválida e amostra vencida convertidas em `unknown`. Meta inicial: amostra a cada 100 ms, expiração em 500 ms; nunca contar um intervalo maior que a validade de aquisição. RTC, web e armazenamento devem ceder execução à aquisição.

3. Parâmetros iniciais de software: `presenceMaxMm=1500` (100–4000 mm), `washMinSeconds=120` (1–86400 s), `absenceMinSeconds=10` (1–3600 s), todos inteiros. São limites de configuração e hipóteses de calibração, não promessa de alcance físico. Presença usa distância menor ou igual ao limiar. Snapshot por sessão preserva revisões. Nova chave NVS `presenceConfig` evita reinterpretar configuração antiga; IDs mantêm sequência persistente. Reutilizar validação, concorrência por revisão e confirmação de gravação existentes.

4. LEDs nos pinos já usados pelo projeto: vermelho 5, amarelo 18, verde 19; verificar montagem em bancada. `WAIT_CLEAR` usa vermelho piscante e diagnóstico de inicialização; após ausência contínua mínima passa a `IDLE`. Falha do sensor pisca a cor do estado a 1 Hz; estado saudável usa luz fixa. Durante tolerância de saída, a cor representa a sessão retida.

5. Novo arquivo `/presence-v2.jsonl`, preservando `/events-v1.jsonl`. Usar registros append-only de validação e encerramento, unidos por ID de sessão e tipo de registro. Persistir validação imediatamente pela fila e encerramento depois; uma sessão não validada pode receber apenas encerramento com `validated=false`. Consolidar uma sessão por ID nas consultas; chave de deduplicação é `(id, tipo)`, não apenas ID como no armazenamento anterior. Registro inclui `schemaVersion=2`, `sessionId`, tipo, `startedAt`, `validatedAt`, `closedAt`, `durationMs`, snapshot da configuração e qualidade temporal. Nenhum campo hidráulico é transportado. Uma alternativa de gravar apenas na saída perderia validações durante permanências longas.

6. Projetar total observado como união dos IDs validados persistidos e pendentes em memória. Total persistido só inclui validações confirmadas. Após boot reconstruir agregados de registros válidos, respeitando duplicatas e linhas incompletas; não carregar todo histórico em RAM. Contagem diária utiliza a data de validação, sem reatribuir eventos sem data após ajuste do RTC. Sessão aberta encontrada após boot é apresentada como interrompida, sem inventar encerramento. `WAIT_CLEAR` evita recontagem; o custo é ignorar o veículo presente na inicialização até uma saída confirmada. Fila e perdas continuam explícitas; contagens apenas observadas podem se perder num corte de energia.

7. Reutilizar rotas `/api/status`, `/api/config`, `/api/rtc`, `/api/sessions` e autenticação. Declarar versão 2 do contrato: status com estado, saúde/distância, cor, sessão, contagens observadas/persistidas/diárias/sem data e falhas; sessões consolidadas paginadas por cursor. Remover categorias e parâmetros hidráulicos, rejeitando filtros antigos com 400. Não misturar arquivos antigos nas consultas novas. Painel mantém parametrização e RTC, troca cartões e histórico por presença e informa origem local. Separar cálculo de agregados do transporte HTTP para futura integração; não criar endpoints premium fictícios.

## Risks / Trade-offs

- Presença prolongada pode não ser lavagem real → comunicar que a contagem é inferida; calibrar no box.
- Reflexos, geometria e leituras instáveis → diagnóstico explícito, intervalos contínuos conservadores e roteiro físico de calibração.
- Dois veículos sem intervalo de ausência → uma sessão, limitação documentada desta abordagem.
- Novo contrato quebra clientes antigos → versão explícita, documentação e exemplos atualizados.
- Histórico grande e flash cheia → leitura incremental, memória limitada, fila e falhas visíveis, sem exclusão automática.
- Falha de energia antes da persistência → distinguir observado/persistido; não prometer recuperação exata.

## Migration Plan

1. Exportar histórico e preservar backup de firmware/NVS antes de atualização física; nenhuma gravação em dispositivo é parte da edição de código.
2. Implementar núcleo, adaptador, novo namespace de configuração e histórico versionado; manter arquivos e chaves antigos intactos.
3. Adaptar API, painel, testes existentes e documentação; regenerar `WebPage.h` pelo script.
4. Executar simulações/testes nativos, testes web e compilação ESP32. Documentar evidências e limites, sem declarar bancada concluída sem hardware.
5. Validar montagem e limiares conforme roteiro antes de uso real. Para rollback usar firmware anterior com seus dados preservados; não converter registros de presença em hidráulicos.
