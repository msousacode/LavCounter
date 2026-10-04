# Tasks

## 1. Configuração, identidade e contrato de acesso

- [x] 1.1 Criar configuração opcional privada e exemplo sem segredos para destino, STA e URLs SaaS/premium, sem clientId manual, e manter padrão desabilitado; verificar testes de configuração ausente, parcial, URL inválida e segmentos inválidos, e build sem arquivo privado.
- [x] 1.2 Implementar deviceId pelos seis bytes do MAC eFuse, clientId UUID v4 automático persistido em NVS comum aos perfis e vínculo persistido projeto/database/cliente/placa/modo; verificar primeiro boot offline, UUID válido/distinto com fonte aleatória falsa, reuso após reinício/upgrade, falha/corrupção NVS sem regeneração silenciosa, separação mock e bloqueio de reassociação.
- [x] 1.3 Criar interface CloudAuth para token válido/renovação e provedor padrão indisponível; verificar testes de token ausente, expirado e renovação recusada sem vazamento em logs/status.
- [x] 1.4 Documentar configuração futura, identidade automática, associação clientId/deviceId ao UID por cadastro e-mail/senha sem código temporário, contrato de autorização premium/regras e reprovisionamento após reset; verificar exemplos sem segredos e teste local que recusa vínculo/payload de outro cliente, registrando que SaaS/pagamento/enforcement real ficam para integração futura.
- [x] 1.5 Criar PremiumAccess com provedor padrão unknown e contrato autenticado de vínculo/status/validade; verificar fakes para inactive, unknown, active, expired, revogação e resposta divergente, sem flag local que libere acesso nem armazenamento da senha do usuário.

## 2. Fonte durável e confirmações

- [x] 2.1 Expor iteração incremental de snapshots consolidados persistidos em Storage.h, capturando limite do journal e excluindo fila RAM; verificar tests/storage.cpp para histórico anterior, validação/encerramento, linhas truncadas, IDs grandes e serviço da presença entre passos.
- [x] 2.2 Implementar canonicalização/digest de revisão e journal separado de confirmações com integridade e vínculo por modo; verificar testes de gravação parcial, corrupção, falta de espaço, reboot e ausência de alteração nos arquivos de histórico/legado.
- [x] 2.3 Implementar seleção de revisões pendentes e contadores atuais por sessão; verificar validação confirmada que encerra depois, encerramento durante envio, interrupted no boot, digest divergente e marcadores antigos sem falsa conclusão.
- [x] 2.4 Documentar formato, capacidade, confirmação durável e recuperação em docs/API.md; verificar contrato contra fixtures de reinício e resposta perdida, incluindo reenvio conservador sem limpeza automática.

## 3. Transporte Firestore e conectividade

- [x] 3.1 Implementar caminho REST determinístico e conversão fields Firestore para todos os campos v2 mais identidade/revisão; verificar testes com transporte falso para null, mapas, inteiros exatos, horários locais e upsert de validação/encerramento no mesmo documento.
- [x] 3.2 Implementar cliente HTTPS com CA/bundle validado, hora UTC independente do RTC, token via CloudAuth, limites e classificação de resposta; verificar TLS/hora ausentes, 2xx, 400, 401 com uma renovação, 403, 429/Retry-After, 5xx e timeout sem confirmação indevida.
- [x] 3.3 Adicionar AP+STA e reconexão espaçada sem bloquear setup/loop; verificar fakes de conexão/reconexão/roteador sem internet e funcionamento do AP com configuração ausente, e compilar contra core ESP32 3.3.10.
- [x] 3.4 Integrar worker FreeRTOS com filas limitadas a um snapshot/resultado e acesso exclusivo de storage/NVS pelo coordenador; verificar testes do coordenador com worker falso lento, falha de memória e ausência de HTTP nos handlers/loop de detecção.
- [x] 3.5 Documentar autenticação substituível, TLS, limites, tratamento de erros e STA privada; verificar roteiro entregue de conectividade e indicar ensaios reais adiados até receber os dados de acesso.

## 4. Coordenação e operação local

- [x] 4.1 Integrar disparos de conexão/persistência/ativação premium, gate de autorização antes de ciclo/requisição, ciclo limitado, confirmação após 2xx e marcador, backoff e retomada; verificar testes nativos sem upload quando premium inativo/desconhecido/expirado, suspensão por revogação, ativação com histórico prévio, falha parcial, reboot, resposta perdida, novos registros durante envio e concorrência.
- [x] 4.2 Adicionar GET/POST /api/cloud-sync e GET /api/premium-link com autenticação, respostas 200/202/403/503, estado premium sanitizado e contadores desconhecidos como null; verificar tests/firmware.cpp e mock-firmware.cpp para acesso negado, gate premium, configuração ausente, URL HTTPS codificada com IDs persistidos e sem segredos, ocupado e separação de modos.
- [x] 4.3 Adicionar ação Premium, botão de sincronização e diagnóstico em web/index.html, acompanhamento sem confundir cadastro/aceite com ativação/sucesso e geração de WebPage.h; verificar Playwright para URL com IDs estáveis sem código, URL ausente, premium inativo/desconhecido/expirado/ativo, não configurado/offline, conclusão, falha parcial, duplo clique e permanência de métricas locais.
- [x] 4.4 Atualizar README.md e docs/API.md com cadastro por URL, IDs automáticos, contrato premium futuro, comandos, códigos/status e comportamento offline; verificar exemplos contra testes de API e mensagens efetivamente exibidas no painel.

## 5. Validação de integração

- [x] 5.1 Executar scripts/test-native.ps1 e npm run test:web após adicionar os novos testes/fakes nos grupos anteriores; verificar todas as regressões de sensor, armazenamento e interface, sem acessar projeto Firebase real.
- [x] 5.2 Compilar perfis físico e mock com scripts/build.ps1 e scripts/build.ps1 -MockSensor; verificar compatibilidade eFuse/TLS/FreeRTOS no core fixado e registrar uso de flash/RAM e logs.
- [x] 5.3 Entregar roteiro integrado de bancada em docs/VALIDACAO.md para rede lenta, AP+STA, coleta durante envio, corte de energia e isolamento remoto; verificar cenários automatizáveis com fakes e registrar claramente ensaios de hardware/Firebase ainda não executados por falta de placa/acesso, sem marcar validação real como concluída.


