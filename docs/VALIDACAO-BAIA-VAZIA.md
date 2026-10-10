# Validação: reconhecimento de baia vazia pelo limiar

Mudança `reconhecer-baia-vazia-por-limiar`, aplicada em 09/10/2026. Os arquivos arquivados foram considerados no design da mudança: o fluxo mantém identidade e histórico durável, substitui a exigência antiga de WAIT_CLEAR e aceita exclusivamente status 2 íntegro do VL53L1X como ausência operacional.

## Evidências automatizadas

- `scripts/test-native.ps1` (saída em `build/validation-baia-native.log`): sete executáveis (núcleo, sensor físico, nuvem, Firebase, integração Firebase, armazenamento e sketch). Cobrem limite inclusivo nos dois modelos, status 2 independentemente da distância bruta, demais status inválidos, todas as transferências de aquisição/calibração/DSS/confirmação, resposta curta, timeout, expiração e recuperação. O sketch verifica LEDs, uma contagem, mesmo ID na saída, sessão consolidada, próximo ID, API autenticada e console automático limitado. Núcleo e armazenamento cobrem continuidade, snapshot, reinício e histórico interrompido sem saída fabricada.
- `npm run test:web`: 14 testes passaram no Microsoft Edge, incluindo ausência por sinal insuficiente sem distância/pane, falha real, retomada e IDLE após encerramento; configuração, RTC, histórico e nuvem também passaram.
- `scripts/build.ps1`: resultado e uso de memória registrados abaixo após término. `WebPage.h` é gerado por `scripts/embed-web.cjs`; partição e dependências permanecem as exigidas pelo script.

O VL53L1X utiliza calibração de baixo consumo, ajuste DSS e ganho compatíveis com Pololu 1.3.1. Cada transferência de aquisição é verificada individualmente, pois o status final do driver poderia ocultar falha anterior com uma confirmação bem-sucedida.

## Roteiro físico reproduzível — pendente

Nenhum upload ou ensaio físico foi realizado nesta aplicação. Resultados abaixo são **esperados**, não observados em hardware. Preservar histórico/configuração antes de gravar o firmware; não apagar NVS ou filesystem.

1. Conectar VL53L1X em SDA 21, SCL 22, alimentação adequada à placa e GND comum. LEDs: vermelho 23, amarelo 18, verde 19, com resistores. Abrir serial em 115200; os diagnósticos começam sem `sensor on`.
2. Pelo painel, salvar `presenceMaxMm=1500`, `washMinSeconds=120`, `absenceMinSeconds=10`. Anotar revisão, contagem inicial e IDs existentes. Para ensaio rápido, usar permanência 5 s e ausência 2 s, registrando a alteração e restaurando os valores ao terminar.
3. Reiniciar com baia vazia: estado IDLE/vermelho. Registrar modelo/ID, status, ocupação, motivo e saúde. Distância válida acima de 1500 ou status 2 íntegro significa ausência; status 7 significa desconhecido. Status 2 não comprova fisicamente ausência de veículo.
4. Colocar alvo até 1500 mm (inclusive): presença válida inicia CANDIDATE/amarelo. Retirar brevemente antes da permanência mínima e retornar: a permanência reinicia, mantendo a sessão durante a tolerância de ausência.
5. Manter presença contínua pelo mínimo: VALID_WASH/verde e incremento de uma unidade. Continuar presente: nenhuma contagem adicional. Anotar ID.
6. Retirar alvo. Confirmar status 2 ou distância válida acima do limite. Antes do mínimo de ausência, mantém verde/mesmo ID; ao atingir o mínimo, encerra e volta a IDLE/vermelho sem novo incremento. Conferir um evento consolidado, validado e encerrado no mesmo ID. Alternar status 2 e distância válida ausente deve continuar o mesmo intervalo.
7. Reintroduzir alvo: novo ID, candidata; só conta após nova permanência. Reiniciar com alvo presente também inicia nova candidata/ID; após permanência pode contar novamente. Sessão persistida sem saída anterior permanece interrompida, com horário de saída nulo.
8. Durante uma sessão, desligar a alimentação do sensor mantendo GND comum, ou desconectar SDA com alimentação desligada para preparar o ensaio. Falha real deve produzir Unknown, distância nula e piscar a cor da sessão, sem fabricar saída/contagem. Reconectar: recuperação automática e novos intervalos contínuos. Não usar desconexão de GND como critério de ausência.
9. Executar `sensor off`: console periódico para, mas LEDs, detecção e painel continuam. Registrar também veículo real pouco refletivo, pois status 2 pode ocorrer mesmo com objeto presente.

Registrar data, revisão, montagem, veículo/alvo, logs e resultados reais em nova evidência após executar. Upload e aprovação física continuam pendentes.

Correção adicional solicitada: entrada de RTC agora aceita formatos brasileiros sem preenchimento de zeros, segundos opcionais e ISO local, normalizando antes do envio. O limite antigo de 16 caracteres truncava datas coladas com segundos; foi ampliado para 24. Teste web verifica os corpos POST exatos e rejeição de 31/02. A causa observada na unidade ainda depende da mensagem do operador; falha física I²C não é corrigida por formatação.



## Build final observado

Compilação final com RTC e sensor concluída com sucesso em 09/10/2026 às 22:44 (hora local), ESP32 core 3.3.10, Pololu VL53L0X/VL53L1X 1.3.1, ArduinoJson 7.4.2, partição default de 4 MB. Flash: 1.302.124 / 1.310.720 bytes (99%), margem 8.596 bytes. RAM global: 54.888 / 327.680 bytes (16%), restando 272.792 bytes. Log: build/compile-physical.log.

Binário: build/firmware-physical/LavCounter.ino.bin, 1302272 bytes, gerado 09/10/2026 22:44:11. SHA-256: C52808F3D155E45FA33F81A7501C79A92CF164D7D77347AFAF02EBBB34BB1D7A. O tamanho do arquivo inclui cabeçalhos/alinhamento e difere do uso de flash reportado pelo compilador. Nenhum upload foi executado.

## Revisão de transições — 10/10/2026

Operador confirmou permanência configurada de 120 s. A revisão não reproduziu salto direto IDLE → VALID_WASH: a chegada retorna imediatamente como CANDIDATE e a validação exige presença contínua. Acrescentada guarda explícita `state_ == Candidate` na validação e regressão com uptime de um dia, baia ausente antes da chegada, 119,9 s ainda candidata, validação aos 120 s, ausência por 10 s e nova candidata. Os sete executáveis nativos passaram, incluindo status 2 como ausência e LEDs exclusivos. A causa do comportamento observado na placa continua sem confirmação por logs ou ensaio físico; a guarda reforça a regra e não comprova a causa. O binário desta revisão precisa ser gravado para testar no aparelho.

Build da revisão de 10/10/2026 concluído com sucesso: flash 1.302.128 / 1.310.720 bytes, RAM global 54.888 / 327.680 bytes. Binário build/firmware-physical/LavCounter.ino.bin, 1302272 bytes, gerado 10/10/2026 07:29:49; SHA-256 BB6609957CC391C3FC840F0DBE9F7AACD1B65D51A6FD31E7EDA4E48830713B63. Mesma partição e dependências, sem upload. Esses dados substituem a identificação do binário de 09/10 para esta revisão.

