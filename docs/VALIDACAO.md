# Validação por presença — 01/10/2026

## Software

- `./scripts/test-native.ps1`: núcleo, armazenamento e sketch/handlers reais com periféricos simulados; execução aprovada. Inclui calendário/RTC (1.200 meses), preservado da suíte anterior.
- `npm run test:web`: cinco testes aprovados no Edge headless. Cobrem uso offline, sessão candidata/validada, contagem observada/persistida, texto seguro, salvar/cancelar/restaurar, falha de persistência, sensor/RTC indisponível e layout móvel.
- Capturas: `build/web-config.png`, `build/web-events.png`, `build/web-mobile.png`. Parametrização e layout móvel inspecionados visualmente.
- `openspec validate reestruturar-lavcounter-vl53l1x --strict`: validação estrita aprovada.
- Cabeçalho web gerado por `node scripts/embed-web.cjs`; fonte operacional sem métricas hidráulicas.

## Rastreabilidade

| Spec / cenários | Evidência |
| --- | --- |
| vehicle-presence: distância no limiar, falha de aquisição | tests/firmware.cpp: igualdade em 1500 mm, ausência em 1501 mm, inválida, timeout, expiração, desconexão e recuperação |
| vehicle-presence: limite exato, permanência prolongada, presença interrompida | tests/core.cpp: sequência a 100 ms, reinício da continuidade, validação única |
| vehicle-presence: saída e próximo veículo, ausência breve/falha | tests/core.cpp e tests/firmware.cpp: tolerância, desconhecido durante ausência, encerramento e nova sessão |
| vehicle-presence: edição durante sessão, entradas inválidas/falha | tests/storage.cpp e tests/firmware.cpp: snapshot, revisão concorrente, limites, fração, falha de gravação e boot |
| vehicle-presence: semáforo normal e falha após validação | tests/core.cpp: cor/fase; tests/firmware.cpp: GPIO verde; bancada itens 1–5 para sinais físicos |
| wash-history: lavagem ainda presente e reenvio/encerramento | tests/storage.cpp: validação em fila/disco, duplicatas, fechamento sob mesmo ID, linha sem newline e reenvio |
| wash-history: RTC inválido/ajustado | tests/firmware.cpp: data inicial preservada e clockAdjusted; tests/storage.cpp: sem data e mudança de dia |
| wash-history: reinício com veículo | tests/core.cpp: WAIT_CLEAR; tests/storage.cpp: contagem reconstruída, interrupted sem saída inventada; bancada item 8 |
| wash-history: armazenamento indisponível e migração | tests/storage.cpp: montagem falha, espaço insuficiente, fila cheia, legacyDisk e chave config preservados, IDs sincronizados |
| local-dashboard: offline e autenticação | tests/web/ui.spec.cjs: nenhuma requisição externa; tests/firmware.cpp: todas as rotas autenticadas |
| local-dashboard: validação pendente e RTC indisponível | testes web e firmware: totais separados, diário indisponível, distância inválida |
| local-dashboard: uso sem premium | painel autocontido, agregados em Storage.h independentes de HTTP, teste web restringe rede à origem local |

## Compilação ESP32

Resultado final: **sucesso**, com os fontes revisados. Flash: **1.112.384 / 1.310.720 bytes (84%)**. RAM estática: **51.424 / 327.680 bytes (15%)**, restando 276.256 bytes. Log: `build/compile.log`; binário: `build/firmware/LavCounter.ino.bin`. Dependências instaladas e fixadas: esp32:esp32 3.3.10, ArduinoJson 7.4.2 e Pololu VL53L1X 1.3.1. Comando: `./scripts/build.ps1`. FQBN: `esp32:esp32:esp32:PartitionScheme=default,FlashSize=4M`.

## Limites e bancada pendente

Nenhum upload ou ensaio físico foi feito. Testes simulados não comprovam alcance óptico, condições elétricas, latência real com flash ocupada, enlace Wi-Fi nem durabilidade de gravação sob corte de energia. Execute [BANCADA.md](BANCADA.md) na unidade real. A contagem infere lavagem pela permanência; a montagem deve produzir uma leitura válida do box vazio.

As evidências anteriores de fluxo pertencem à mudança lavcounter-deteccao-lavagem e ao histórico Git. Seus itens de bancada não foram marcados como concluídos nem a mudança foi arquivada.

## Progresso OpenSpec

19/19 tarefas de implementação e verificação de software concluídas. Mudança pronta para arquivamento; bancada física pendente conforme roteiro, sem upload realizado.

## Semáforo — 02/10/2026

Pinagem atual: vermelho GPIO25, amarelo GPIO18 e verde GPIO19, com saída ativa em nível alto. GPIO5 deixou de ser acionado. Testes nativos aprovados para todas as combinações de estado/saúde, limites de 499/500/999/1000 ms, exclusividade das cores e transições reais de vermelho para amarelo e verde. A indicação inicial é atualizada já no setup. Ligação física pressupõe indicadores compatíveis com 3,3 V e baixa corrente; conferir a tensão nominal dos LEDs com resistor embutido antes de conectar.

Compilação da revisão dos LEDs: **aprovada** via `./scripts/build.ps1`, com 1.112.484 bytes de flash (84%) e 51.424 bytes de RAM estática (15%). Nenhum upload ou teste físico realizado.
