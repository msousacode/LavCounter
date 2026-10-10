# Design

## Context

Ver proposal.md para motivação. O projeto usa Arduino ESP32, Pololu VL53L0X/VL53L1X 1.3.1, amostragem de 100 ms, orçamento de 50 ms e expiração em 500 ms. `PresenceSensor.h` converte apenas distâncias válidas em Present/Absent; status 2 e 7 viram Unknown. `Detection.h` já temporiza presença/ausência, preserva uma contagem por sessão e inicia em Idle. `LavCounter.ino` captura configuração por sessão, persiste validação/encerramento e aciona LEDs pela saúde do sensor. `sensor.healthy` também controla a disponibilidade de `distanceMm` na API, e o console chama essa mesma variável de `valido`: esse acoplamento precisa ser desfeito.

O arquivo arquivado `2026-10-01-reestruturar-lavcounter-vl53l1x/design.md` escolheu Unknown para todo erro óptico e WAIT_CLEAR no boot para evitar recontagem. Seu requisito de recuperação ainda existe em `wash-history`, enquanto o código e a solicitação anterior já adotam IDLE. A mudança hidráulica arquivada de 02/10 é contexto de origem; não se restauram fluxo, tags ou categorias. A mudança de sincronização arquivada de 03/10 exige o mesmo ID na validação/saída; o novo encerramento deve continuar usando esse caminho. README e DETECCAO ainda contêm descrições contraditórias de boot e baia vazia.

## Goals / Non-Goals

**Goals:** classificar ocupação sem exigir fundo mensurável no VL53L1X; preservar continuidade temporal e contagem única; representar ausência operacional com distância inválida sem sinalizar pane; reconciliar contrato, testes e documentação com boot em IDLE.

**Non-Goals:** inferir ausência de qualquer erro; validar distâncias de status 2; mudar modo óptico, orçamento ou período; alterar pinagem, NVS, esquema de eventos ou regras Firebase; recuperar uma sessão RAM após falta de energia; reintroduzir aquisição simulada no produto.

## Decisions

### 1. Classificação no adaptador com precedência explícita de falhas

Manter o núcleo recebendo `lav::Sample` Present/Absent/Unknown; a interpretação dos status pertence ao adaptador e a uma pequena função determinística testável, sem framework adicional.

| Entrada recente | Classificação | Distância válida | Saúde operacional |
| --- | --- | --- | --- |
| VL53L1X status 0 ou VL53L0X status bruto 11, distância > 0 e != 65535, até o limiar | Present | sim | disponível |
| Mesmas condições, distância acima do limiar | Absent | sim | disponível |
| VL53L1X status 2, aquisição completa, confirmação bem-sucedida e sem timeout | Absent | não | disponível |
| VL53L1X status 7 ou outro status não aceito; VL53L0X inválido | Unknown | não | indisponível para detecção |
| Falha I²C, resposta curta, erro na confirmação, timeout, sem amostra ou expirada | Unknown | não | indisponível |

Verificar transporte, integridade, confirmação e timeout antes de permitir a exceção de status 2. O número bruto não decide presença nem ausência nessa exceção: não exigir `bruto_mm > 1500` nem tratar um bruto pequeno como presença. Status 0 com zero/65535 permanece Unknown. Não reinterpretar status bruto 2 do VL53L0X usando a enumeração do VL53L1X.

Alternativas rejeitadas: conservar toda leitura inválida como Unknown mantém a sessão presa no cenário observado; aceitar todo erro como ausência cria saídas com desconexão; usar a distância bruta inválida como limite inventa confiabilidade que o sensor não forneceu. A documentação Pololu define status 2 como sinal abaixo do limiar, não como garantia física de ausência: https://github.com/pololu/vl53l1x-arduino/blob/1.3.1/VL53L1X.h . A interpretação como ausência operacional foi confirmada pelo operador.

### 2. Separar dados de medição e ocupação

Adicionar ao adaptador `distanceValid`, última classificação e uma razão curta. Manter `healthy` como disponibilidade operacional para determinar ocupação, incluindo status 2 admissível. `lastSampleUs` indica chegada de medição completa; a validade da classificação deve expirar também para sequências anteriormente classificadas como ausência. Falha ou expiração substitui imediatamente a classificação pública por Unknown e invalida a distância, sem reutilizar a classificação anterior.

Na API v2 adicionar `presenceSample` (`present`, `absent`, `unknown`), `presenceReason`, `distanceValid` e `sensorRangeStatus` (status do modelo identificado). Preservar `sensorModel`, `sensorHealthy` e `sensorError`; `distanceMm` dependerá de `distanceValid`, não apenas de `healthy`. Status 2 admissível deixa `sensorError` vazio e usa razão `signal_insufficient`. Outras razões podem ser `within_threshold`, `beyond_threshold`, `invalid_range`, `i2c_error`, `timeout`, `stale` ou `initializing`. O painel traduz essas razões em linguagem do operador; nomes técnicos ficam na API/console.

O console mantém a saída automática e seus limites de UART; `valido` passa a representar a validade da distância e acrescenta ocupação/razão/saúde. O painel mostra ausência por sinal insuficiente sem número de distância nem mensagem genérica de sensor quebrado. Evitar inferir classificação somente a partir de `distanceMm=null`, pois esse valor também ocorre em falhas reais.

Alternativa de apenas marcar `healthy=true` foi rejeitada: publicaria a distância inválida como confiável e o console diria `valido=sim`.

### 3. Reutilizar temporização e fluxo de sessão

Manter `presenceMaxMm` configurável, padrão 1500 mm, com limite inclusivo; não sobrescrever configuração salva. Snapshot da sessão determina limiar e tempos até o encerramento. `washMinSeconds` padrão 120 e `absenceMinSeconds` padrão 10 permanecem configuráveis. O aceite de status 2 foi confirmado após o tempo configurado; portanto não haverá retorno imediato ao vermelho a cada amostra ausente.

Fluxo: IDLE/vermelho → primeira presença válida → CANDIDATE/amarelo → presença contínua pelo mínimo → VALID_WASH/verde, contabilizando uma vez. Absent por distância válida ou status 2 inicia/continua o mesmo intervalo de saída, mantendo a cor da sessão durante a tolerância; no limite exato encerra e volta a IDLE/vermelho. Ausência antes de validar reinicia a permanência; se completar o mínimo de ausência, encerra sem contagem. Unknown reinicia ambos os intervalos, preservando a identidade e validação já obtida. Lacunas acima de 500 ms não são creditadas.

O encerramento usa a transição existente e o mesmo ID da validação em `Event`/`EventStorage`; não criar um caminho paralelo para saída por status 2. Nada novo será somado ao sair. A sincronização observa a versão encerrada pelo fluxo durável existente.

### 4. Boot em IDLE e reconciliação documental

Conservar o estado inicial Idle já implementado e coberto em testes; remover a exigência contraditória de WAIT_CLEAR das specs por este delta e atualizar documentação durante apply. WAIT_CLEAR pode permanecer como valor legado por compatibilidade de exibição, sem entrar no fluxo normal; não é necessário apagar o enum nesta mudança. Preservar sessões antigas como interrompidas e registros confirmados, sem inventar saída. Um veículo presente após reiniciar poderá iniciar novo ID e ser contado após outra permanência mínima. Não se promete deduplicação física entre reinícios.

Alternativa de restaurar WAIT_CLEAR contraria a solicitação explícita do operador e não resolve a classificação de baia vazia.

## Risks / Trade-offs

- Sinal insuficiente também pode ocorrer com veículo pouco refletivo → regra operacional explicitamente aceita, tolerância de ausência mantida, bancada com veículo real e registro de status/motivo; não declarar prova física de baia vazia.
- Outros status continuam impedindo saída → diagnóstico específico; ampliar a lista exige nova decisão, não uma conversão genérica de erro em ausência.
- Reinício com veículo presente pode contabilizar novamente → documentar e testar novo ID/permanência, preservando histórico anterior.
- Saúde muda de significado para clientes que a usavam como validade da distância → campo explícito `distanceValid`, `distanceMm=null` em status 2 e documentação de compatibilidade.
- Partição física tinha apenas 10.232 bytes livres na compilação registrada → evitar dependências, compilar a fonte final pelo script e medir flash/RAM; não mudar partição ou remover funcionalidades silenciosamente.

## Migration Plan

Implementar somente após apply explícito. Atualizar adaptador, diagnósticos, painel e testes, mantendo chaves NVS, IDs e arquivos históricos. Regenerar WebPage.h pelo script. Executar testes nativos, verificações web pertinentes e compilação ESP32 final; preservar evidências novas separadas dos arquivos arquivados. Validar fisicamente alvo próximo, saída sem fundo, falha real e novo veículo, com tempos configurados conhecidos. Gravação/bancada deve ser descrita como pendente até realmente executada. Rollback para firmware anterior não exige migração de dados; volta a tratar sinal insuficiente como Unknown. Arquivos arquivados e specs principais não são reescritos durante esta proposta.
