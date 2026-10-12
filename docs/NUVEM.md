# Identidade, cadastro e nuvem

## Liberação atual (12/10/2026)

Na integração Firebase privada habilitada, o Premium permanece liberado no firmware, sem prazo local. `FirebaseManagedPremium` substitui o provedor temporário na montagem do sketch. `defaultConfig()` seleciona `firebaseManagedPremium=true` e `testUntil=0`; a data antiga do arquivo privado deixa de bloquear envios. As regras do Firebase decidem se cada escrita pode ser aceita, inclusive o prazo configurado pelo administrador. Essa configuração não foi consultada nem alterada nesta mudança.

Token válido, renovação automática, TLS, relógio UTC confiável, vínculo exato de projeto/banco/dispositivo/modo e confirmação durável continuam obrigatórios. A permissão local significa autorização para tentar sincronizar, não comprovação de Premium remoto nem sucesso de upload. Sem integração privada habilitada, o comportamento padrão permanece bloqueado. O painel mostra a liberação controlada pelo Firebase; `/api/cloud-sync` acrescenta `firebaseManagedPremium` e mantém `testMode=true` por compatibilidade. O provedor temporário abaixo permanece apenas para compatibilidade/testes; seu prazo não se aplica à integração atual.

O primeiro boot gera clientId UUID v4 e confirma sua gravação no namespace NVS `lavIdentity`. É reutilizado em reinícios e upgrades do firmware físico. deviceId usa os seis bytes do MAC eFuse, no formato `esp32-` e 12 hexadecimais. NVS ilegível/corrompida bloqueia cadastro/envio sem gerar identidade substituta silenciosamente. Preserve NVS/flash ao atualizar. Factory reset exige reprovisionamento explícito.

Uma compra independente gera identidade de instalação independente. O SaaS futuro associa UID da conta criada com e-mail/senha a clientId e deviceId recebidos na URL HTTPS de cadastro. Não existe código temporário de ativação. E-mail pode mudar sem alterar identidade/histórico. Os IDs na URL são contexto de cadastro, nunca credenciais ou prova de propriedade/premium. O serviço deve preservar vínculos existentes, sem sobrescrever dono silenciosamente.

Copie `firmware/LavCounter/CloudPrivate.example.h` para `CloudPrivate.h`, ignorado pelo Git. Informe futuramente SSID/senha, projeto/database, URL SaaS, endpoint premium e CA PEM confiável; clientId não é configuração manual. Sem configuração, o firmware mantém todas as funções locais. Não coloque senha de usuário SaaS nem chave privada de service account na placa.

`AuthProvider::acquire` fornece Firebase ID token com validade UTC e renovação. `PremiumProvider::check` consulta servidor autenticado, verifica vínculo projeto/database/clientId/deviceId/modo e retorna status/validade UTC. Os provedores padrão são indisponíveis; o arquivo privado poderá incluir implementações e usar `LAVCOUNTER_CLOUD_PROVIDERS(auth, premium)` para selecionar instâncias duradouras. Tokens e senhas não aparecem nas APIs/URL/logs. As interfaces são preparadas nesta mudança; cadastro Firebase Auth, pagamento e serviço SaaS reais ficam para integração futura.

Contrato obrigatório do servidor/regras: autenticar usuário/dispositivo; autorizar apenas o vínculo provisionado; conferir clientId/deviceId/sensorMode do payload com o caminho; exigir premium ativo e não expirado em cada escrita; impedir reassociação silenciosa e acesso de outro cliente. IDs ou API key sozinhos não autorizam escrita. Testes locais usam provedores/fakes, não substituem Security Rules ou validação em projeto real. Provisionamento e publicação de regras dependem dos dados de acesso futuros.

Caminho Firestore: `clients/{clientId}/devices/{deviceId}/modes/{sensorMode}/sessions/{sessionId}`. Validação e encerramento atualizam o mesmo documento. IDs de sessão permanecem strings e datas RTC permanecem strings locais, sem UTC inventado. Configuração remota não libera envio sem autorização premium válida. Ao ativar premium, o histórico persistido antes da ativação também entra na sincronização.

O core ESP32 fornece WiFiClientSecure/HTTPClient/FreeRTOS. A rede STA mantém o AP local; reintentos STA começam em 5 s e aumentam até 5 min. Worker exclusivo faz DNS/TLS/HTTP e consulta dos provedores; loop principal mantém sensor/armazenamento/APIs. Filas têm capacidade um e transportam sinais, com snapshot imutável em memória. Falha de criação do worker é visível e não troca para HTTP bloqueante.

As regras/backend devem validar também progressão: `syncStage` inteiro é 1 para validada, 2 para interrompida por reinício e 3 para encerrada. Proibir escrita com stage menor que o documento existente e validar correspondência de stage com type/interrupted. Uma requisição antiga atrasada não pode substituir encerramento por validação. Para stage igual, permitir apenas payload de sessão/revisão equivalente; nunca autorizar cliente a editar uma sessão terminal usando digest diferente. O emissor serializado e os documentos determinísticos limitam concorrência local, mas essa condição remota é necessária para requisições atrasadas no servidor.

TLS exige CA PEM e hora UTC de SNTP (`pool.ntp.org`/`time.google.com`); UTC do sistema não ajusta RTC DS3231, timestamps de sessões ou duração monotônica. Transporte tem conexão 5 s, handshake/leitura 8 s, espera inicial de hora até 10 s no worker. Requisição até 12 KiB, resposta até 16 KiB, snapshots até 4 KiB e nesting até oito níveis. 401 permite uma renovação de token, 403/400 param o ciclo com erro; timeout/5xx/429 usam backoff 5 s até 5 min com jitter e Retry-After numérico limitado a 300 s. Nunca usar setInsecure ou publicar respostas remotas/segredos em logs.

PremiumAccess é consultado ao conectar e periodicamente (60 s) e novamente pelo worker antes de cada upload. Active exige binding correspondente e expiresAt UTC futura; unknown/inactive/expired bloqueiam histórico. O servidor deve também recusar revogação/premium expirado em cada escrita. Cadastro, pagamento e implantação das regras reais permanecem dependentes da integração futura.

Indisponibilidade inicial do provedor de token usa retentativa espaçada sem enviar sessões; token devolvido sem validade ou renovação recusada após HTTP 401 interrompe o ciclo com erro de autorização. Assim uma falha de rede ao obter a credencial não exige reiniciar a placa para recuperar.

## Liberação temporária para integração Firebase

A integração de teste acrescenta `FirebaseRefreshAuth` e `TemporaryTestPremium`. Não exige compra nem endpoint premium/SaaS: o arquivo privado habilita explicitamente uma autorização de teste até **10/10/2026 às 21h de Brasília** (`2026-10-11T00:00:00Z`). A autorização exige token Firebase válido e corresponde exatamente a projeto, banco, clientId, deviceId e modo do aparelho. Depois do prazo, nenhum novo envio é permitido. O comportamento padrão, sem o arquivo privado e sem `testUntil`, continua exigindo premium remoto.

`scripts/prepare-firebase-test.ps1` cria uma identidade anônima dedicada uma única vez, verifica o projeto/UID e a renovação no Firebase real, e salva o refresh token em `firmware/LavCounter/CloudCredentials.h`, ignorado pelo Git. Reexecuções reutilizam essa identidade, inclusive após reinício da placa; não criam uma conta por boot. O provedor renova ID tokens no worker HTTPS, verifica UID/número do projeto/validade, e mantém tokens somente em sua memória privada. Nenhum token é devolvido pelo painel nem impresso pelo script. `GoogleRootCa.h` contém apenas certificados públicos GTS R1/R4 de `pki.goog`, sem fallback TLS inseguro.

A configuração privada do teste é destinada a **uma unidade de bancada**; não distribua o mesmo firmware com essa credencial para aparelhos de outros clientes. Para interromper o teste, desative a configuração privada (`enabled=false`) e remova a permissão nas regras. Excluir o usuário anônimo impede renovar tokens; o bloqueio imediato das escritas deve ser feito nas regras. As credenciais e autorizações definitivas de produção continuam sendo uma integração separada.

Preparação:

1. No projeto `lavcounter`, habilite e salve Authentication → Métodos de login → Anônimo. `ADMIN_ONLY_OPERATION` na preparação significa que o endpoint não autorizou criar a conta; corrija a configuração antes de prosseguir.
2. Execute `./scripts/prepare-firebase-test.ps1`. Isso verifica autenticação real, mas não publica regras nem escreve sessões.
3. Compile o perfil desejado. A placa gera clientId/deviceId automaticamente; não é necessário copiá-los. Preserve NVS e histórico ao atualizar.
4. Execute `./scripts/prepare-firebase-test.ps1` e publique `build/firestore-test.rules` em Firestore Database → Regras. O arquivo contém o UID dedicado; não exige IDs fixos. Esta autorização temporária permite apenas essa credencial de bancada, valida o formato dos IDs e sua correspondência com o payload, e permite consulta de sessões no caminho do cliente/aparelho. Não compartilhe a credencial entre clientes. O isolamento de produção exige vínculo autenticado por conta, além de filtros de consulta. Não publique o modelo com `REPLACE_AUTH_UID`.
5. Com a ESP32 conectada a uma rede externa com internet, confira o diagnóstico e solicite sincronização. Falta de regras, conexão, hora ou autenticação mantém o histórico local e não confirma o envio. O painel indica explicitamente o teste e a expiração; o botão continua exigindo token/autorização de teste válida.

`GET /api/cloud-sync` acrescenta `testMode`, `testUntil` (segundos UTC) e `authUid` (identificador público). Esses campos não são credenciais. A liberação temporária não altera o provisionamento de Wi-Fi; a rede criada pela própria ESP32 não fornece acesso externo por si só.

## Configuração de Wi-Fi pelo cliente

Não é necessário informar SSID/senha no arquivo privado: no painel local, clique em Sincronizar com a nuvem sem conexão externa e preencha a rede Wi-Fi de 2,4 GHz e a senha. Conectar e sincronizar salva no aparelho e tenta conexão mantendo o AP local. O botão Configurar Wi-Fi permite trocar/corrigir a rede. A senha não é devolvida por APIs nem guardada no navegador. As credenciais Firebase e regras continuam sendo configuradas pelo responsável pelo projeto.
