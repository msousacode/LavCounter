# Design

## Context

Ver `proposal.md` para motivação. O firmware usa Arduino ESP32 core 3.3.10, ArduinoJson 7.4.2, WebServer síncrono e detecção cooperativa em `servicePresence()`. Hoje `setup()` só chama `WiFi.softAP`; não existe STA ou transporte remoto. `EventStorage` persiste validação e encerramento em JSONL e consolida por ID. `page()` inclui a fila RAM e, portanto, não é fonte adequada para upload durável; `scan(false)` exclui essa fila. O boot deriva `interrupted` para sessões sem encerramento. IDs são reservados em NVS e serializados como texto. O perfil mock separa journal e namespace, mas reutiliza a placa. Não há alteração necessária nos requisitos existentes de `wash-history`; as novas garantias remotas estão em `cloud-session-sync`.

## Goals / Non-Goals

**Goals:** separar seleção/persistência local do trabalho de rede; preparar integração real REST com configuração ausente; suportar atualizações de sessões antigas sem perder encerramentos; manter uso de RAM limitado e processamento do sensor no ritmo configurado.

**Non-Goals:** download, edição remota, aplicação SaaS, provisionamento de contas/regras em produção, painel de credenciais, migração entre clientes ou recuperação após apagamento de NVS/flash. O botão fica no painel web, não é um botão físico.

## Decisions

### 1. Firestore REST e autenticação substituível

Usar Cloud Firestore REST com uma gravação por sessão e nome determinístico. O corpo converte JSON v2 para os tipos `fields` do Firestore (strings, booleanos, inteiros decimais, null e mapas); não enviar JSON comum como se fosse documento REST. Manter horários RTC locais como strings, sem inventar UTC. Inteiros grandes e IDs permanecem exatos. Preservar todo o snapshot consolidado e adicionar `clientId`, `deviceId`, `cloudSchemaVersion` e `syncRevision`.

Preparar módulos `CloudConfig`, `CloudAuth`, `PremiumAccess`, `DeviceIdentity`, `FirestoreTransport` e `CloudSync`. `CloudConfig` começa desabilitado e recebe configuração privada não versionada, com exemplo sem segredos: SSID/senha, projectId, databaseId, URL SaaS/serviço premium e material do provedor de autenticação. O clientId vem da identidade local persistida, não da configuração privada. Validar IDs como segmentos de caminho e bloquear configuração parcial. `CloudAuth` fornece ID token Firebase válido e renovação via provedor injetável; nesta entrega o provedor padrão indisponível não permite envio. O cliente REST real deve ser exercitado com provedor falso, não substituído integralmente por um stub. Dados de login/provisionamento reais ficam para o prompt futuro. Nunca incorporar chave privada de service account à placa.

Firebase ID tokens estão sujeitos a Security Rules; contrato futuro de regras deve verificar vínculo autenticado de cliente/dispositivo e modo, incluindo payload consistente com o caminho. Documentar esse contrato e testar isolamento com avaliador/fake local; publicação e validação em projeto real ficam pendentes de acesso. Rejeitar 401/403 com diagnóstico, solicitar renovação ao provedor em 401 uma vez e bloquear repetições rápidas se falhar. Usar TLS com CA/bundle confiável, nunca `setInsecure`; obter hora UTC de rede para validação TLS sem sobrescrever o RTC local. Hora/TLS indisponíveis suspendem envio com erro claro.

Alternativas: Realtime Database contradiz a solicitação; SDK adicional amplia dependências; service account concede privilégios inadequados; token estático sem expiração/renovação não é contrato suficiente para operação contínua.

### 2. Cliente e placa são identidades diferentes

Ler MAC de fábrica com `esp_efuse_mac_get_default` (verificar include no core fixado) e formatar seis bytes completos como `esp32-` + 12 dígitos hexadecimais minúsculos, sem usar MAC configurável de interface ou truncar em 32 bits. Gerar clientId como UUID v4 com fonte aleatória adequada do ESP32 no primeiro uso e gravar/validar em namespace NVS de identidade comum aos modos físico e mock, separado da configuração de detecção. Reutilizar após reinício e upgrade; não regenerar se a NVS estiver indisponível/corrompida: informar erro e bloquear ativação/envio, mantendo operação local. Em primeira criação, só expor URL após confirmar a gravação. Não gerar novo UUID em cada clique, conexão ou tentativa.

Esse clientId identifica a instalação original do aparelho antes de existir conta. Duas placas compradas separadamente geram clientIds distintos; agrupamento futuro de instalações sob uma conta não exige reescrever seus históricos e fica fora do escopo. UID da conta identifica o usuário, e-mail é dado alterável de cadastro, deviceId identifica a placa. Nenhum deles é credencial.

Caminho de documento: `clients/{clientId}/devices/{deviceId}/modes/{sensorMode}/sessions/{sessionId}`. Um documento por sessão; validação e encerramento sobrescrevem o snapshot dessa mesma sessão por upsert REST. Somente um emissor serializado por placa/modo pode escrever. Persistir vínculo de destino (projeto, database, cliente, placa, modo) ao ativar o envio pela primeira vez; divergência bloqueia envio sem reassociar histórico antigo. IDs atuais pressupõem preservação da NVS; factory reset exige reprovisionamento explícito antes de reusar sequência no mesmo caminho.

Alternativas: MAC como cliente impediria agrupamento de placas; MAC como token não autentica; documento só por sessionId colidiria entre dispositivos e modos.

### 2.1. URL de cadastro e premium confirmado pelo servidor

A ação Premium monta `https://<saas>/cadastro?clientId=<uuid>&deviceId=<id>`, com parâmetros codificados, origem HTTPS previamente configurada e nenhum token, e-mail ou senha na URL. O usuário preenche e-mail/senha no SaaS futuro, que cria uma conta Firebase Authentication e associa seu UID ao clientId/deviceId recebidos; o e-mail não muda os caminhos das sessões. Não há código temporário de ativação, pareamento, QR secreto ou confirmação extra no aparelho. IDs são contexto de associação, não autorização de leitura/escrita. Política de resolução de vínculos existentes deve preservar o vínculo original, sem sobrescrever outro proprietário silenciosamente.

Preparar PremiumAccess como provedor de autorização autenticada no servidor: recebe identidade persistida e credencial de dispositivo provisionada pelo contrato CloudAuth; retorna status (`inactive`, `unknown`, `active`, `expired`), vínculo e validade da autorização. O provedor padrão retorna unknown e mantém upload bloqueado. A credencial de dispositivo e o endpoint concretos serão entregues na integração futura; a senha do usuário permanece no SaaS e não é armazenada no firmware. O contrato exige verificar identidade autenticada, vínculo e assinatura premium no backend/regras, sem confiar em parâmetro premium da URL, flag local ou clientId recebido como prova de autorização.

Antes de cada ciclo e requisição de upload, exigir autorização válida, não expirada, correspondente ao vínculo e premium active. Revalidar em conexão, reconexão e vencimento; falha de validação sem autorização ainda válida impede envio. Revogação recebida encerra novos envios, preservando o que já foi confirmado. O servidor também recusa escrita premium inativa/expirada, cobrindo autorização local desatualizada. O worker realiza consultas remotas sem bloquear detecção. Sem premium, não enviar sessões, mesmo com Wi-Fi e Firebase configurados; consultas de ativação não transportam histórico. Após ativar, enviar inclusive sessões persistidas no período local. Cadastro/pagamento/backend reais não são implementados nesta mudança; interfaces e fakes permitem verificar todo o gate no firmware.

### 3. Confirmação por snapshot, sem cursor de último ID

Selecionar snapshots validados por leitura durável consolidada e incremental do journal, com estado de iteração limitado, serviço cooperativo da presença e sem carregar todo histórico em RAM. Adaptar armazenamento para expor leitura por passos; não chamar `page()` de upload. Capturar limite do arquivo ao iniciar o ciclo; novas gravações disparam outro ciclo. Corrupções usam diagnóstico existente e só registros válidos completos são enviados.

Calcular `syncRevision` como SHA-256 de serialização canônica dos campos do snapshot, inclusive `type`, horários, duração persistida, origem e `interrupted`; nunca incluir duração viva nem metadados de confirmação. Guardar journal separado de confirmações com versão, vínculo, sessionId e digest, somente depois de resposta REST 2xx completa. Confirmações exigem newline e integridade; truncamentos/corrupções invalidam a entrada afetada e causam reenvio conservador. Consulta por sessão usa memória constante e percurso cooperativo, aceitando custo de leitura maior nesta versão. Evitar writes de flash para polling/retentativa; gravar apenas novas confirmações. Não compactar/apagar histórico automaticamente.

Se o encerramento acontecer enquanto uma validação está em envio, confirmar apenas o digest do snapshot enviado; o encerramento segue pendente e será enviado no próximo ciclo. No boot, alteração derivada para `interrupted=true` gera nova revisão. Se a confirmação local falhar após sucesso remoto, repetir o mesmo upsert; nunca anunciar conclusão durável antes de gravar marcador. Novo envio da mesma revisão produz o mesmo conteúdo. Uma única requisição em voo e ordem local impedem regressão por concorrência do dispositivo.

Alternativas: último ID perde atualização de encerramento; marcar no journal original compromete histórico; reenvio de tudo em cada polling desperdiça tráfego; upload só de encerradas adia lavagens já contabilizadas e ignora interrompidas.

### 4. Rede isolada da detecção

Modo AP+STA mantém rede local, com credenciais de STA opcionais via configuração privada e reconexão com backoff. O coordenador recebe eventos de conexão e persistência, faz seleção incremental no contexto principal e envia snapshot imutável para worker FreeRTOS. O worker executa DNS/TLS/HTTP com timeouts e resposta limitada, sem acessar detector, SPIFFS, NVS ou WebServer. Uma fila de entrada e uma de resultado de capacidade 1 evitam concorrência e crescimento ilimitado. O contexto principal confirma resultados e publica estado. Eventos Wi-Fi apenas sinalizam estado, sem executar gravações nos callbacks.

Começar backoff transitório em 5 s, dobrar até 5 min com jitter; usar tempo monotônico. Respeitar Retry-After válido em 429 com limites. Falhas 5xx/rede são transitórias; 400/403/configuração inválida interrompem ciclo até correção ou solicitação manual explícita. Reconexão ou comando manual pode antecipar espera transitória sem criar worker adicional. Novos registros continuam sendo persistidos durante envio. Em memória insuficiente ou erro do worker, falhar de forma observável, sem executar HTTP no loop como fallback.

Alternativa: HTTP síncrono em `loop()` ou no handler web bloquearia sensor por segundos. A interface REST de uma sessão limita payload, heap e custo de retentativa.

### 5. API e painel

`GET /api/cloud-sync` autenticado retorna configured, deviceId, clientId não secreto, wifiConnected, premiumStatus, state (`not_configured`, `premium_required`, `premium_unknown`, `premium_expired`, `offline`, `idle`, `syncing`, `backoff`, `error`), pendingVersions, confirmedVersions, lastCompletedAt (null sem horário confiável), lastError sanitizado e diagnóstico de storage. Contadores referem-se à revisão atual consolidada de cada sessão, não a tentativas nem à soma de versões antigas. Durante cálculo incremental, representar contadores desconhecidos como null; não inventar zero. “Sem pendências” só depois de concluir varredura válida.

`POST /api/cloud-sync` autenticado e sem parâmetros de destino retorna 202 ao aceitar um trabalho, 200 com estado existente se ocupado, 403 para premium inativo/desconhecido/expirado e 503 para configuração/conectividade ausente. O comando não espera HTTP remoto. `GET /api/premium-link` autenticado retorna URL de cadastro com IDs persistidos e motivo de indisponibilidade; URL base ausente ou identidade inválida retorna 503, sem inventar endereço. Não aceitar destino ou IDs alternativos enviados pelo navegador. A ação Premium abre essa URL mesmo sem premium ativo; o botão de sincronização fica indisponível com motivo claro até autorização. O painel consulta progresso e apresenta feedback acessível; métricas locais continuam usando API v2 existente. Editar `web/index.html` e gerar `WebPage.h` com o script existente.

Alternativa: colocar tokens no navegador transferiria responsabilidade e segredos ao operador; resposta síncrona impediria acompanhamento e operação local.

## Risks / Trade-offs

- [Rede/TLS consome heap e CPU] → worker isolado, payload e filas limitados, timeouts, ensaio no ESP32 com rede lenta e medição do intervalo de amostragem.
- [Marcadores aumentam uso de flash e leituras] → journal separado por modo, gravação apenas após sucesso novo, diagnóstico de capacidade; se cheio, manter pendência, sem apagar sessões. Otimização/compactação futura fora do escopo.
- [MAC identifica mas não autentica] → autorização Firebase por vínculo provisionado; bloquear envio até existir provedor válido. Identidade de cliente explícita.
- [Flash/NVS apagada permite reutilização de ID] → preservar em upgrade; documentar reprovisionamento para reset. Não prometer idempotência após perda do estado local.
- [Configuração privada sem provisionamento remoto pronto] → estado inicial não configurado, fakes verificam integração; conexão real e regras precisam de dados futuros.
- [AP+STA pode mudar canal do AP] → validar reconexão do navegador e preservar endereço/acesso local na bancada.

## Migration Plan

1. Entregar código desabilitado por padrão com configuração de exemplo e testes locais, preservando journals, configuração e IDs existentes.
2. Compilar ambos os perfis; executar regressões nativas/web e roteiro de rede lenta sem depender de credenciais reais.
3. Em prompt futuro, preencher destino, URL SaaS/endpoint premium, credenciais STA e provedores Firebase/PremiumAccess, associar o clientId automático à conta, provisionar vínculo/regras e validar TLS, assinatura ativa, isolamento e escrita em projeto autorizado.
4. Primeiro ciclo envia histórico v2 do modo ativo e confirma revisões; não migra dados hidráulicos nem mistura mock.
5. Rollback: desabilitar nuvem ou retornar firmware anterior sem apagar flash/NVS; journals de confirmação são adicionais e ignorados pelo firmware antigo.

## Open Questions

Valores reais de projectId, databaseId, URL SaaS/endpoint premium, SSID/senha e material dos provedores de autenticação/autorização serão fornecidos posteriormente. clientId será gerado na placa. CA/hora UTC e regras serão validadas contra esse destino antes de ativação; isso não altera os contratos descritos. Cadastro, assinatura e serviço remoto serão implementados em trabalho futuro independente.

## References

- Espressif: MAC de fábrica em eFuse e API de leitura: https://docs.espressif.com/projects/esp-idf/en/v5.0.2/esp32/api-reference/system/misc_system_api.html
- Firebase: autenticação REST por Firebase ID token e aplicação de Security Rules: https://firebase.google.com/docs/firestore/use-rest-api
- Firebase: condições de autorização e `request.auth`: https://firebase.google.com/docs/firestore/security/rules-conditions
