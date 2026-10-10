# Sincronização de sessões na nuvem

## Purpose

Enviar sessões de lavagem persistidas ao Cloud Firestore com isolamento por cliente e dispositivo, permitindo retomada confiável sem tornar a operação local dependente da nuvem.

## Requirements

### Requirement: Configuração opcional e autorização
O sistema SHALL preparar envio HTTPS ao Cloud Firestore, sem usar Realtime Database, e SHALL iniciar sem configuração remota ou credenciais reais. SHALL exigir destino, cliente e autenticação válidos antes de enviar, validar o servidor TLS e não divulgar segredos no painel, APIs ou logs. A autorização remota SHALL restringir a identidade autenticada ao cliente e dispositivo provisionados; `clientId`, `deviceId` e chave de API isoladamente SHALL não conceder acesso.

#### Scenario: Acesso ainda não informado
- **WHEN** o firmware opera sem os dados de acesso que serão fornecidos posteriormente
- **THEN** informa nuvem não configurada, não realiza envio e mantém coleta, persistência e painel local

#### Scenario: Autorização recusada ou certificado inválido
- **WHEN** o servidor rejeita a autorização ou a validação TLS falha
- **THEN** o sistema mantém registros pendentes e informa erro sem anunciar sincronização nem revelar segredos

### Requirement: Identidade e isolamento por cliente
O sistema SHALL atribuir `deviceId` estável derivado do MAC de fábrica em eFuse e gerar automaticamente `clientId` único no primeiro uso, persistindo-o antes de expor cadastro ou envio. SHALL reutilizar clientId após reinício/upgrade. SHALL separar sessões por cliente e dispositivo, preservando `sessionId` como texto e o caminho remoto existente com modo `physical`. O firmware SHALL operar exclusivamente com o sensor real. SHALL manter clientId independente do e-mail e UID da conta e não usar identificadores como tokens secretos. SHALL impedir troca silenciosa de cliente/destino enquanto houver histórico vinculado; falha de leitura/gravação da identidade SHALL bloquear vínculo/envio sem interromper funções locais.

#### Scenario: Primeiro uso sem cadastro
- **WHEN** o cliente liga um aparelho novo sem configuração Firebase ou conta SaaS
- **THEN** o aparelho gera e persiste clientId sem exigir entrada do usuário e opera apenas localmente

#### Scenario: Identidade não pode ser persistida
- **WHEN** a gravação inicial de clientId falha ou uma identidade existente está corrompida
- **THEN** o aparelho informa erro, não oferece vínculo/envio com identidade instável e mantém coleta local

#### Scenario: Várias placas e clientes
- **WHEN** placas de clientes diferentes possuem sessões com o mesmo ID local
- **THEN** seus documentos remotos são distintos, com associação explícita ao cliente, placa e modo de origem

#### Scenario: Reinício e atualização para sensor real
- **WHEN** a mesma placa reinicia ou recebe atualização que remove o modo simulado
- **THEN** mantém seus identificadores de cliente e placa, os dados e confirmações físicos, sem importar sessões de simulação para o histórico físico

#### Scenario: Tentativa de reassociação
- **WHEN** uma configuração aponta histórico já vinculado a outro cliente ou destino
- **THEN** o envio fica bloqueado com diagnóstico de vínculo incompatível até procedimento explícito de reprovisionamento

### Requirement: Disparos automáticos e manuais
O sistema SHALL tentar sincronizar pendências ao conectar/reconectar ao Wi-Fi com acesso remoto configurado e premium ativo confirmado pelo servidor, quando premium é ativado e quando uma validação ou encerramento é persistido. SHALL permitir solicitar sincronização pelo painel autenticado somente com autorização premium válida. SHALL manter o ponto de acesso e funções locais disponíveis, serializar solicitações concorrentes e aplicar retentativas espaçadas para falhas transitórias, sem espera de rede no processamento de detecção.

#### Scenario: Conexão com pendências
- **WHEN** a ESP32 conecta ao Wi-Fi do estabelecimento com sessões persistidas ainda pendentes, configuração válida e premium ativo autorizado
- **THEN** inicia envio automaticamente, sem depender de navegador aberto

#### Scenario: Botão durante envio
- **WHEN** o operador solicita sincronização enquanto outra já está em execução
- **THEN** recebe o estado do trabalho existente sem iniciar envios paralelos

#### Scenario: Wi-Fi sem internet
- **WHEN** há conexão ao roteador mas o Firestore está inacessível
- **THEN** informa falha remota, mantém pendências e funções locais e tenta novamente com intervalo limitado

### Requirement: Sessões persistidas e envio idempotente
O sistema SHALL enviar apenas sessões validadas duravelmente persistidas, incluindo histórico anterior à ativação, e SHALL preservar campos v2, qualidade temporal, parâmetros e origem simulada/física. Validação e encerramento SHALL representar o mesmo documento remoto. SHALL atualizar o documento quando a sessão encerrar ou for identificada como interrompida no boot, sem inventar horários e sem duplicar contagens. Reenvios SHALL não regredir versões já confirmadas.

#### Scenario: Validação seguida de encerramento
- **WHEN** a validação é enviada e o encerramento da mesma sessão é persistido posteriormente
- **THEN** o documento existente recebe o encerramento, mesmo que o ID já tenha sido sincronizado

#### Scenario: Resposta perdida
- **WHEN** o servidor grava uma sessão mas a confirmação não chega à ESP32
- **THEN** a retentativa usa o mesmo documento e mantém uma única sessão remota

#### Scenario: Registro pendente apenas em RAM
- **WHEN** uma validação está na fila local aguardando gravação
- **THEN** não é enviada nem contabilizada como sincronizada até sua persistência

### Requirement: Confirmação durável e recuperação
O sistema SHALL registrar confirmação apenas após sucesso remoto e persistência local do marcador de sincronização. SHALL reconstruir pendências após reinício, manter o histórico local intacto e usar memória e trabalhos limitados por sessão. Erros de gravação local, journal degradado ou resposta remota SHALL permanecer visíveis e não resultar em descarte silencioso.

#### Scenario: Reinício após envio sem marcador
- **WHEN** a energia é interrompida depois do sucesso remoto e antes de persistir a confirmação local
- **THEN** a sessão é reenviada de forma idempotente após reinício

#### Scenario: Corrupção de confirmação
- **WHEN** o marcador de sincronização não pode ser lido com integridade
- **THEN** o sistema assume pendências conservadoramente e não apaga nem considera todo o histórico sincronizado

#### Scenario: Coleta durante rede lenta
- **WHEN** uma tentativa de envio enfrenta DNS, TLS ou HTTP lentos
- **THEN** o sensor continua sendo processado dentro do intervalo de amostragem configurado e novas sessões continuam sendo persistidas

### Requirement: Contrato de cadastro SaaS e liberação premium
O sistema SHALL preparar contrato de cadastro no qual clientId e deviceId da URL são associados ao UID da conta criada por e-mail/senha no SaaS, sem código temporário de ativação. SHALL não tratar IDs recebidos na URL como credenciais ou prova de premium. O envio SHALL exigir autenticação válida, vínculo correspondente e premium ativo confirmado pelo servidor com validade verificável; sem confirmação ou com autorização vencida SHALL preservar sessões apenas localmente. Credenciais de usuário SHALL permanecer no SaaS. Nesta entrega, os provedores remotos SHALL estar preparados e indisponíveis por padrão até configuração posterior, sem exigir implementação do SaaS/pagamento.

#### Scenario: Cadastro com identidade do aparelho
- **WHEN** o cliente acessa o cadastro a partir da URL Premium
- **THEN** o contrato SaaS permite associar a conta autenticada ao clientId/deviceId recebidos sem código adicional no aparelho, preservando clientId se o e-mail mudar

#### Scenario: Wi-Fi disponível sem premium
- **WHEN** o aparelho tem Wi-Fi e destino Firebase configurados mas premium não foi confirmado ativo
- **THEN** nenhuma sessão é enviada automática ou manualmente e a coleta local continua

#### Scenario: Ativação após período local
- **WHEN** o servidor confirma vínculo válido e premium ativo
- **THEN** o aparelho sincroniza as sessões persistidas antes da ativação e as novas atualizações, sem apagar histórico

#### Scenario: Premium expirado ou revogado
- **WHEN** a validade da autorização termina sem renovação ou o servidor informa revogação
- **THEN** novos envios param, as sessões e pendências locais permanecem e o painel informa a restrição premium

#### Scenario: Tentativa de liberar por parâmetro local
- **WHEN** um cliente fornece flag premium ou altera IDs na URL sem autorização remota válida
- **THEN** isso não habilita o envio nem concede acesso aos documentos de outro cliente
