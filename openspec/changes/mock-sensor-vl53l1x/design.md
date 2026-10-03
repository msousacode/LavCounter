# Design

## Context

Ver motivação em `proposal.md`. A branch atual é `mock-sensor`. `PresenceSensor.h` converte leituras Pololu em `lav::Sample`; `servicePresence()` alimenta `Detector` e gera eventos, contagens e LEDs. A aquisição roda aproximadamente a cada 100 ms e a detecção interrompe continuidade para intervalos maiores que 500 ms. O painel fonte é `web/index.html`, incorporado por `scripts/embed-web.cjs`. O fake em `tests/fakes/VL53L1X.h` atende testes nativos, não a ESP32 em execução.

O journal físico é `/presence-v2.jsonl`; configuração e IDs usam Preferences no namespace `lavcounter`. O campo existente `source: local` identifica operação local, não o tipo de sensor. As APIs já usam autenticação e o RTC inválido não impede contagem monotônica.

## Goals / Non-Goals

**Goals:** substituir somente a entrada de aquisição, mantendo o caminho real de detecção e gravação; tornar testes repetíveis e separar dados de teste dos físicos.

**Non-Goals:** forçar estados internos, acelerar tempo, simular RTC ou GPIO, reproduzir precisão óptica/distâncias, criar SaaS ou um simulador que funcione sem ESP32. Não haverá exclusão automática de registros nem alternância de modo durante uma sessão.

## Decisions

### 1. Seleção na compilação

Introduzir `LAVCOUNTER_SENSOR_MOCK`, com padrão 0 em cabeçalho de configuração e possibilidade de override na compilação. Documentar a seleção pela Arduino IDE e expor opção equivalente em `scripts/build.ps1`. Isolar inclusão/instanciação do driver físico para que a compilação mock não dependa de `VL53L1X.h`; o build físico mantém a biblioteca e suas verificações. A alternativa de fallback por falha foi descartada porque esconderia defeitos; troca em runtime criaria sessões com origens mistas.

### 2. Adaptador de amostras

Criar adaptador mock com interface equivalente a `begin/poll`, saúde e erro, selecionado na integração do sketch. Emitir `Present`, `Absent` ou `Unknown` a cada 100 ms usando o relógio monotônico existente. Estado inicial `Absent`; manter `WaitClear` e a ausência inicial obrigatória. Entrada mantida em RAM até comando ou reboot, sem depender de conexão do browser. Reenvio da mesma entrada não reinicia nada. Preservar `Detector`, geração de eventos e atualização dos LEDs. Distância mock é `null`, pois não existe medição em milímetros. Simular valores numéricos de distância adicionaria escopo não necessário à escolha de três entradas.

### 3. Contrato de controle

Adicionar `POST /api/mock-sensor` com JSON estrito `{"sample":"present"}`, aceitando somente `present`, `absent`, `fault`. Autenticar antes de validar ou aplicar. Responder 200 com entrada aceita e modo; 400 para corpo inválido, 401 conforme autenticação existente e 404 no firmware físico. O comando não valida lavagem diretamente: o processamento periódico aplica a nova entrada. Dois clientes seguem a ordem de recebimento pelo servidor; último comando aceito prevalece.

Adicionar a `/api/status` `sensorMode: physical|mock` e `mockSample: present|absent|fault|null`. Preservar `source: local` e campos existentes. Em mock, saúde é verdadeira para presença/ausência e falsa para falha, com mensagem explícita de falha simulada. Novos campos são aditivos; registros anteriores sem origem são físicos. Mostrar confirmação de comando separada do estado da sessão, atualizado pelo polling existente.

### 4. Isolamento com as mesmas classes de persistência

Parametrizar caminho do journal e namespace Preferences, com os valores físicos atuais como padrão. Mock usa `/presence-mock-v2.jsonl` e namespace `lavcounterMock`; configuração e IDs permanecem independentes. Usar padrões existentes na primeira inicialização, sem copiar configuração física. O mesmo `EventStorage` calcula totais, fila e paginação apenas do journal selecionado. Adicionar `sensorMode` à serialização da sessão e às respostas do histórico; atribuir origem física aos registros antigos sem campo. Identidade de sessão é relativa ao modo, pois IDs podem coincidir entre os journals. Não alterar algoritmos de deduplicação nem qualidade temporal.

Um journal único com filtros foi descartado pelo risco de contaminação de totais e maior complexidade de recuperação. Os arquivos ainda compartilham espaço SPIFFS; não existe promessa de capacidade física isolada. RTC é compartilhado e real: ajustes manuais de relógio continuam afetando o dispositivo e devem ser explicados no roteiro.

### 5. Painel e validação

Na aba de status, mostrar faixa persistente de simulação, entrada atual, três botões e aviso de temporização real. Desabilitar envios concorrentes até concluir o comando; em erro apresentar mensagem e reconciliar com status, pois uma resposta perdida não prova que o comando falhou na ESP32. Recarregar página apenas consulta o estado. Identificar totais e eventos como simulados; esconder controles no modo físico. Regenerar `WebPage.h` pelo script existente.

Ampliar testes nativos da integração em ambos os modos, incluindo autenticação, valores inválidos, repetições, boot, interrupção por falha, validação única, encerramento e isolamento/reabertura de armazenamento. Playwright cobre controles, feedback, recarregamento, erro e modo físico. Compilar ambos os firmwares e documentar ensaio na ESP32 sem sensor usando mínimos de teste pela configuração normal do mock.

## Risks / Trade-offs

- Espera dos tempos reais pode parecer inatividade → explicar os tempos e permitir reduzi-los na configuração isolada de teste.
- Mock não comprova desempenho óptico/I2C → manter testes físicos existentes e registrar essa limitação na validação.
- Firmware mock pode permanecer instalado por engano → indicação persistente e padrão físico na compilação.
- Falta de espaço compartilhado pode afetar a gravação → preservar tratamento de falhas existente e não apagar dados automaticamente.

## Migration Plan

Implementar na branch atual, executar testes e builds físico/mock, carregar mock sem apagar SPIFFS/NVS e executar roteiro de lavagem. Para retornar à operação, compilar com mock desabilitado e carregar preservando armazenamento; o firmware volta aos caminhos físicos originais. Dados de teste permanecem separados para consultas futuras no modo mock. Não migrar journals existentes nem fabricar saída de sessões interrompidas.
