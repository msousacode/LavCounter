# Proposal

## Why

As sessões contabilizadas estão apenas na ESP32. Enviá-las ao Firebase permite preservar uma cópia remota e reunir dados de vários estabelecimentos com isolamento por cliente, mantendo a operação local independente da internet.

## What Changes

- Preparar envio unidirecional ao Cloud Firestore (não Firebase Realtime Database), via HTTPS, com configuração de acesso inicialmente ausente.
- Adicionar conexão Wi-Fi em modo estação mantendo o ponto de acesso local, sincronização automática ao conectar/reconectar e ao persistir atualizações, além de botão autenticado “Sincronizar com a nuvem”.
- Gerar automaticamente um `clientId` UUID no primeiro uso e persistir na NVS, compartilhado pelos perfis físico/mock, e identificar placa por `deviceId` derivado do MAC de fábrica em eFuse; nenhum identificador será usado como segredo de autenticação.
- Adicionar ação “Premium” com URL de cadastro SaaS contendo `clientId` e `deviceId`. Preparar contrato para associar esses IDs à conta criada por e-mail/senha, mantendo UID da conta e e-mail separados do clientId. Não exigir código temporário de ativação.
- Manter o aparelho exclusivamente local enquanto não houver premium confirmado pelo servidor. Preparar consulta de autorização premium e bloquear envio automático/manual quando inativo, desconhecido ou expirado; configuração Firebase e Wi-Fi sozinhas não liberam upload.
- Enviar sessões validadas persistidas e atualizar o mesmo documento no encerramento, com reenvio idempotente, confirmação durável e retomada após falhas/reinício.
- Preservar separação entre dados físicos e simulados também na nuvem e nos marcadores de sincronização.
- Exibir configuração ausente, conexão, pendências, progresso e erros sem expor credenciais ou alterar contagens locais.
- Documentar contrato de configuração e autorização para os dados de acesso que serão fornecidos em outro prompt. Não provisionar nem publicar recursos Firebase nesta mudança.

## Capabilities

### New Capabilities

- `cloud-session-sync`: identidade automática, contrato de vínculo SaaS/premium, sincronização automática/manual autorizada, isolamento, envio idempotente, retomada e configuração futura do Firestore.

### Modified Capabilities

- `local-dashboard`: ação Premium, comando e diagnóstico de sincronização opcionais, preservando todas as funções locais sem conta ou internet e atualizando a separação da oferta SaaS.

## Impact

Firmware Arduino ESP32 em `firmware/LavCounter/LavCounter.ino`, leitura durável do histórico em `Storage.h`, configuração e identidade em módulos próprios, painel-fonte `web/index.html` e geração de `WebPage.h`. Novas rotas locais autenticadas de sincronização, conectividade STA+AP e transporte TLS com dependências do core ESP32 existente; testes nativos com fakes e testes Playwright. Documentação de API, configuração futura, regras de isolamento e validação de bancada. Histórico v2, NVS existente e dados legados serão preservados; não há migração destrutiva ou download da nuvem.

Esta mudança prepara o firmware e os contratos do serviço futuro; implementação do SaaS, cadastro Firebase Authentication, pagamento e publicação do backend premium ficam fora do escopo. Sem esses serviços configurados, o envio permanece bloqueado.
