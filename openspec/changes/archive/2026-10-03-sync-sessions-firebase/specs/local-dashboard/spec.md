# Spec Delta

## ADDED Requirements

### Requirement: Comando e diagnóstico da nuvem
O painel SHALL oferecer botão “Sincronizar com a nuvem” e apresentar configuração ausente, estado premium ativo/inativo/desconhecido/expirado, Wi-Fi desconectado, envio em andamento, pendências de versões persistidas, última conclusão e erro sanitizado. SHALL distinguir solicitação aceita de envio confirmado, proteger rotas com autenticação local existente e manter métricas locais identificadas como locais. O botão SHALL ser indisponível com motivo claro quando faltar autorização premium válida, configuração ou conexão, e durante execução SHALL impedir solicitações duplicadas.

#### Scenario: Solicitação aceita
- **WHEN** o operador autenticado aciona o botão com premium ativo confirmado, acesso configurado e Wi-Fi conectado
- **THEN** o painel indica trabalho em andamento e só anuncia conclusão quando houver confirmação remota e local das versões do lote

#### Scenario: Nuvem não configurada
- **WHEN** os dados de acesso ainda não foram fornecidos
- **THEN** o painel informa “Nuvem não configurada”, explica a indisponibilidade do botão e preserva todas as funções locais

#### Scenario: Comando sem autenticação
- **WHEN** um cliente não autenticado solicita estado ou envio remoto por API local
- **THEN** o servidor exige autenticação sem expor dados ou iniciar sincronização

#### Scenario: Falha parcial
- **WHEN** algumas sessões são confirmadas e uma falha impede concluir o lote
- **THEN** o painel distingue confirmações de pendências e mostra a falha sem alterar os totais locais

### Requirement: Acesso ao cadastro premium
O painel SHALL oferecer ação “Premium” que abre URL HTTPS do SaaS configurado com clientId e deviceId persistidos em parâmetros codificados, sem e-mail, senha, tokens ou código temporário de ativação. SHALL informar indisponibilidade quando URL SaaS ou identidade válida estiver ausente. A ação SHALL estar disponível sem assinatura ativa; clicar ou completar o cadastro SHALL não anunciar premium ativo antes da confirmação remota de autorização.

#### Scenario: Abrir cadastro
- **WHEN** o operador autenticado clica em Premium com URL SaaS configurada e identidade válida
- **THEN** abre o cadastro com os IDs do aparelho, reutilizando os mesmos IDs em cliques futuros e sem exigir código adicional

#### Scenario: Serviço futuro sem URL
- **WHEN** a URL SaaS ainda não foi informada
- **THEN** o painel explica que o cadastro premium está indisponível e mantém operação local

## MODIFIED Requirements

### Requirement: Separação da futura oferta SaaS
O sistema SHALL manter coleta e métricas locais independentes da camada premium. SHALL não exigir assinatura, conta, conexão com nuvem ou cálculos financeiros para operar localmente. SHALL permitir envio de sessões à nuvem somente com premium ativo confirmado pelo servidor. Recursos premium ainda sem integração configurada SHALL ser explicitamente indisponíveis, sem apresentar assinatura ou métricas financeiras simuladas.

#### Scenario: Uso sem serviço premium
- **WHEN** o dispositivo opera sem conta SaaS ou premium ativo
- **THEN** todas as funções básicas locais permanecem disponíveis, as sessões são salvas apenas localmente e nenhuma métrica financeira simulada é apresentada

#### Scenario: Premium habilitado
- **WHEN** há integração configurada e autorização premium ativa confirmada pelo servidor
- **THEN** a sincronização é liberada mantendo coleta, persistência e métricas locais independentes da nuvem
