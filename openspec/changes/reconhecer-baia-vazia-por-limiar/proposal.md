# Proposal

## Why

O VL53L1X reconhece objetos próximos, mas o fundo da baia vazia frequentemente retorna sinal insuficiente (status 2), atualmente convertido em desconhecido; assim, a sessão permanece candidata ou validada sem confirmar saída. O operador confirmou que esse resultado deve permitir ausência pelo tempo configurado, preservando a permanência mínima para contar uma lavagem.

## What Changes

- Reconhecer presença com distância válida menor ou igual a `presenceMaxMm`, inicialmente 1500 mm (1,5 m); valores salvos continuam respeitados.
- **BREAKING comportamental:** permitir que uma medição recente e completa do VL53L1X com status 2 represente ausência, independentemente da distância bruta inválida. Distância válida acima do limiar também representa ausência. Essa é uma regra operacional, não comprovação física de baia vazia.
- Preservar status 7, demais status não aceitos, falhas I²C, timeout e falta de amostra como desconhecidos, sem validar nem encerrar sessões.
- Manter candidata amarela, validação e contagem única após permanência contínua, encerramento após ausência contínua configurada e retorno ao IDLE vermelho; padrões existentes: 120 s de permanência e 10 s de ausência.
- Formalizar boot em IDLE, conforme solicitação anterior e código atual, reconciliando a especificação de recuperação que ainda exige WAIT_CLEAR.
- Separar validade da distância, classificação de presença e saúde operacional no diagnóstico/API/painel. Ausência por status 2 não deve aparecer como distância válida nem como comunicação perdida.
- Preservar aquisição automática, identificação dos dois chips, configuração por sessão, histórico v2 e sincronização; não estender a exceção de status 2 ao status bruto do VL53L0X.

## Capabilities

### New Capabilities

Nenhuma.

### Modified Capabilities

- `vehicle-presence`: classificação presença/ausência/desconhecido, saída por sinal insuficiente VL53L1X, ciclo temporal e sinalização.
- `wash-history`: recuperação inicia em IDLE, preservando registros anteriores e explicitando possibilidade de nova sessão com veículo presente após reinício.
- `local-dashboard`: mostrar classificação e validade da distância separadas da saúde do sensor, distinguindo ausência operacional de falha.

## Impact

Implementação futura em `PresenceSensor.h`, `Detection.h`, `LavCounter.ino`, `SensorConsole.h`, `web/index.html` e geração de `WebPage.h`; testes nativos, fakes e Playwright; documentação README, DETECCAO, BANCADA, API e console. Sem novas dependências, alterações de pinagem, migração de NVS/SPIFFS ou alteração do contrato de documentos Firebase. O histórico arquivado permanece intacto. A imagem física já ocupa 99% da partição, exigindo compilação e conferência de tamanho durante a implementação.
