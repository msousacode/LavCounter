# Tasks

## 1. Núcleo de presença e configuração

- [x] 1.1 Substituir classificação hidráulica por estados WAIT_CLEAR, IDLE, CANDIDATE e VALID_WASH com presença/ausência/desconhecido; verificar testes de limites exatos, ausência breve, amostra vencida, falha, permanência prolongada e duas sessões consecutivas.
- [x] 1.2 Implementar os três parâmetros do design, snapshot por sessão e persistência independente em NVS; verificar valores inválidos, falha de escrita, revisão concorrente, reinício e preservação da configuração antiga.
- [x] 1.3 Atualizar docs/DETECCAO.md com transições, padrões, intervalos contínuos e limitação da inferência; conferir correspondência com os cenários de vehicle-presence.

## 2. Aquisição e semáforo

- [x] 2.1 Selecionar e fixar driver VL53L1X após consultar documentação oficial, registrando versão, interface elétrica e configuração de aquisição em README.md e sketch.yaml; verificar instalação e compilação do adaptador para ESP32.
- [x] 2.2 Integrar aquisição com tempo limitado no barramento do RTC e remover ISR, fila e cálculo de pulsos; verificar simulações de sensor ausente, timeout, leitura inválida e recuperação, mantendo serviço web e RTC responsivos.
- [x] 2.3 Implementar semáforo fixo e piscante conforme estado/saúde; verificar saídas simuladas para chegada, validação, tolerância de saída, falha e espera inicial.
- [x] 2.4 Atualizar docs/BANCADA.md com ligação, calibração, falhas e sequência física esperada; verificar que cada cenário de aquisição e LED possui procedimento e resultado observável, sem registrar execução física não realizada.

## 3. Histórico, RTC e agregados

- [x] 3.1 Implementar eventos versionados de validação e encerramento com mesmo ID, snapshot e qualidade temporal; verificar serialização sem métricas hidráulicas, RTC inválido, ajuste e sessão não validada.
- [x] 3.2 Implementar arquivo presence-v2.jsonl e deduplicação por ID/tipo, preservando events-v1.jsonl; verificar gravação parcial, reenvio, fila cheia, falta de espaço e ausência de formatação automática com testes de armazenamento.
- [x] 3.3 Consolidar sessões e calcular total observado, persistido, diário e sem data sem carregar histórico inteiro em RAM; verificar validação ainda ativa, encerramento, duplicatas e mudança de dia.
- [x] 3.4 Reconstruir agregados no boot e apresentar sessões interrompidas sem saída inventada; verificar reinício com veículo presente e liberação somente após ausência confirmada, inclusive falha do sensor durante espera.
- [x] 3.5 Documentar formato, pendências, perdas, backup e rollback em docs/API.md e README.md; conferir exemplos com registros produzidos pelos testes e explicitar limites de recuperação após corte de energia.

## 4. API e painel local

- [x] 4.1 Atualizar status, configuração e sessões para contrato v2 com paginação consolidada e rejeição de filtros hidráulicos; verificar handlers autenticados, entradas inválidas, falhas de persistência e ausência de campos antigos nas respostas.
- [x] 4.2 Substituir cartões, histórico e parametrização web por presença, contagens, RTC e saúde; atualizar testes web para sessão candidata/validada, falha do sensor, RTC inválido e pendências sem duplicação.
- [x] 4.3 Separar agregados locais do transporte e indicar origem local sem dependência SaaS; verificar operação offline e ausência de cálculos financeiros ou funcionalidades premium fictícias.
- [x] 4.4 Atualizar exemplos de API e instruções de uso e regenerar WebPage.h com node scripts/embed-web.cjs; verificar paridade do HTML incorporado e passagem dos testes web.

## 5. Verificação do conjunto

- [x] 5.1 Executar scripts/test-native.ps1 e npm run test:web com testes adaptados; registrar comandos, resultados e eventuais limitações em docs/VALIDACAO.md, sem manter falhas ocultas do contrato antigo.
- [x] 5.2 Executar scripts/build.ps1 para ESP32 com dependências fixadas; registrar evidência de compilação e revisar remoção de fluxo do código operacional, preservando documentos históricos e arquivos legados.
- [x] 5.3 Conferir rastreabilidade entre todos os cenários das três specs e testes ou roteiro físico, executar openspec validate reestruturar-lavcounter-vl53l1x --strict e registrar separadamente qualquer bancada ainda não executada.
