# Tasks

## 1. Aquisição simulada e seleção de firmware

- [x] 1.1 Adicionar seleção `LAVCOUNTER_SENSOR_MOCK` com padrão físico e adaptador mock periódico; verificar em testes nativos presença, ausência, falha, repetição e ausência inicial obrigatória sem acessar o driver físico.
- [x] 1.2 Integrar adaptador a `servicePresence()` sem duplicar a máquina de estados; testar limite de validação, permanência prolongada, ausência breve, saída confirmada, falha antes/depois da validação e LEDs correspondentes.
- [x] 1.3 Expor seleção no script de build e documentar ativação pela Arduino IDE, dependências e retorno ao modo físico; verificar comandos e garantir que o modo mock compile sem incluir a biblioteca VL53L1X.

## 2. Isolamento de dados

- [x] 2.1 Parametrizar journal e namespace de configuração conforme design, preservando padrões físicos; testar configurações e IDs independentes, reabertura e preservação dos arquivos físicos.
- [x] 2.2 Adicionar origem às sessões e histórico, mantendo leitura de registros físicos antigos; testar consolidação validação/encerramento, paginação, totais isolados, reinício, RTC inválido e falhas de armazenamento.
- [x] 2.3 Documentar caminhos, identidade por modo, espaço compartilhado e RTC real compartilhado; conferir documentação com os valores implementados e testes de isolamento.

## 3. API autenticada do mock

- [x] 3.1 Implementar `POST /api/mock-sensor` e campos de status previstos no design; testar respostas 200/400/401/404, corpo inválido sem mutação, repetição idempotente e rejeição no firmware físico.
- [x] 3.2 Documentar contrato e exemplos de comandos com tempos reais; verificar os exemplos em testes de integração do sketch e confirmar preservação de `source: local` e distância nula no mock.

## 4. Controles do painel

- [x] 4.1 Adicionar indicação persistente de mock, entrada atual e três botões em `web/index.html`, com confirmação, tratamento de erros e reconciliação; testar em Playwright cliques, comandos enviados, falha de rede, recarregamento, uso offline e ausência de controles no modo físico.
- [x] 4.2 Identificar métricas e histórico simulados, orientar tempos configurados e regenerar `WebPage.h` usando `scripts/embed-web.cjs`; verificar consistência do arquivo gerado e apresentação móvel nos testes de UI.
- [x] 4.3 Documentar roteiro na ESP32 sem VL53L1X: aguardar box vazio inicial, ajustar tempos de teste, simular presença até validar, falha, ausência até encerrar, nova sessão e reinício; conferir que cada passo descreve estado, LED, contagem e registro esperados.

## 5. Verificação integrada

- [x] 5.1 Executar suíte nativa, suíte web e compilações ESP32 física/mock; registrar resultados e verificar regressões da operação física e ciclo completo por API no mock.
- [x] 5.2 Executar roteiro de bancada se a ESP32 estiver disponível, sem apagar dados existentes, e registrar evidências; se indisponível, registrar explicitamente a pendência física sem apresentá-la como teste aprovado.
