# Tasks

## 1. Classificação e integridade de aquisição

- [x] 1.1 Separar classificação Present/Absent/Unknown, validade da distância e saúde operacional no adaptador físico; verificar testes de 1499/1500/1501 mm, zero/65535 com status válido e limiar configurado diferente do padrão nos dois modelos.
- [x] 1.2 Implementar exclusivamente para VL53L1X status 2 a ausência operacional, com precedência de timeout, integridade I²C e confirmação; verificar no fake medições com valores brutos pequenos, grandes, zero e 65535, status 7 e demais status, resposta curta, falha de leitura e confirmação, timeout e desconexão, sem aceitar erro de transporte como ausência. Manter todos os status brutos inválidos do VL53L0X como Unknown.
- [x] 1.3 Garantir que classificação/distância públicas expirem em 500 ms e mudem para Unknown na falha, e que a recuperação automática recomece com dados novos; verificar sequência status 2 → sem amostra → expiração → recuperação e ausência de reutilização de distância/classificação anterior.
- [x] 1.4 Atualizar BANCADA e console quanto à tabela de classificação por modelo e à diferença entre sinal insuficiente e falha; verificar exemplos coerentes com os testes e registrar que a regra operacional não comprova fisicamente baia vazia.

## 2. Fluxo temporal, LEDs e identidade de sessão

- [x] 2.1 Integrar a ausência operacional ao fluxo normal, preservando snapshot de limiar/tempos e boot em IDLE; verificar testes de presença contínua até o limite de validação, interrupções por ausência/desconhecido, candidata encerrada sem contagem, alternância de ausência válida/status 2, retorno breve e encerramento exatamente no mínimo de ausência.
- [x] 2.2 Verificar no sketch real com fakes o ciclo IDLE/vermelho → CANDIDATE/amarelo → VALID_WASH/verde → IDLE/vermelho com saída por status 2: uma contagem, validação/saída sob mesmo ID, próximo veículo com novo ID, cores mantidas na tolerância, status 2 sem piscar e falha real piscando. Corrigir somente os pontos necessários à integração e manter os testes como evidência.
- [x] 2.3 Reconciliar README e DETECCAO com boot em IDLE, permanência padrão 120 s e ausência padrão 10 s configuráveis, limiar inclusivo e possibilidade de nova contagem após reinício com veículo presente; verificar regressão de histórico preservado, sessão anterior interrompida sem saída fabricada e novo ID após reinício.

## 3. API, console e painel

- [x] 3.1 Acrescentar `presenceSample`, `presenceReason`, `distanceValid` e `sensorRangeStatus` ao status autenticado, mantendo `distanceMm=null` em status 2 e falhas; atualizar docs/API e verificar handlers com status 0/2/7, timeout, expiração, falha I²C e acesso sem autenticação.
- [x] 3.2 Adaptar o console automático para validade da distância, classificação, razão e saúde operacional sem segunda aquisição; atualizar CONSOLE-SENSOR e verificar saída no boot sem comandos, exemplos para status 2/7, sensor off sem interromper detecção e saída limitada com receptor bloqueado.
- [x] 3.3 Mostrar no painel presença/ausência/desconhecido e motivo compreensível, distinguindo ausência por sinal insuficiente de falha; verificar casos Playwright de status 2 sem distância/alerta de pane, falha verdadeira, retomada e fim da sessão, e regenerar WebPage.h exclusivamente pelo script existente.

## 4. Verificação integrada e evidências

- [x] 4.1 Executar scripts/test-native.ps1 e testes web pertinentes, registrar resultados em documento de validação novo e manter eventuais falhas preexistentes identificadas; verificar uma sessão consolidada e uma contagem após validação/saída, sem regressão de RTC, configuração ou sincronização durável.
- [x] 4.2 Compilar a fonte final com scripts/build.ps1 e registrar flash/RAM e binário resultante; verificar que cabe na partição atual sem alterar partição/dependências silenciosamente e que o build usa os fontes finais.
- [x] 4.3 Entregar roteiro reproduzível de bancada com limiar/tempos conhecidos, baia vazia/status 2, alvo a até 1,5 m, permanência, saída, segundo veículo e desconexão real; verificar que o roteiro distingue resultados esperados de resultados observados e marca upload/ensaio físico como pendente até execução real.



