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

# Sincronização Firebase — 03/10/2026

As evidências abaixo referem-se à mudança `sync-sessions-firebase`; os registros históricos anteriores continuam referentes às revisões anteriores.

## Software da nuvem

- `scripts/test-native.ps1`: testes reais dos módulos de configuração, identidade, digest/journal, conversão REST, coordenador e APIs com fakes de hardware/rede. Cobrem UUID estável, falha/corrupção NVS, isolamento de vínculo, token expirado/renovação, todos os estados premium, MAC completo, SHA-256 com vetores conhecidos, exclusão de RAM, limite capturado do arquivo, fechamento durante envio, reinício com/sem confirmação, falha parcial de marcador, 400/401/403/429/5xx, resposta incompleta, TLS/hora UTC ausentes, AP+STA e regressões locais.
- `npm run test:web`: 11 testes aprovados no Edge headless. Cinco novos casos verificam URL Premium com IDs estáveis sem código/segredo, ausência de configuração, premium inativo/desconhecido/expirado, offline, clique concorrente, aceite distinto de confirmação, falha parcial, métricas locais e layout móvel. Captura nova: `build/web-cloud-mobile.png`.
- `openspec validate sync-sessions-firebase --strict`: aprovado.

## Compilação final da nuvem

Os dois perfis compilaram com sucesso com os fontes finais, usando `scripts/build.ps1` e `scripts/build.ps1 -MockSensor`.

| Perfil | Flash (limite 1.310.720 bytes) | RAM estática (limite 327.680 bytes) | Log |
| --- | --- | --- | --- |
| Sensor físico | 1.278.444 bytes (97%); livres 32.276 bytes | 54.208 bytes (16%); livres 273.472 bytes | `build/compile-physical.log` |
| Sensor simulado | 1.274.948 bytes (97%); livres 35.772 bytes | 54.144 bytes (16%); livres 273.536 bytes | `build/compile-mock.log` |

Binários em `build/firmware-physical/LavCounter.ino.bin` e `build/firmware-mock/LavCounter.ino.bin`. A partição de aplicação tem pouca margem para os provedores futuros; sua integração deve reavaliar o tamanho do firmware. RAM estática não comprova disponibilidade de heap durante TLS/worker; essa medição permanece no roteiro de bancada.

## Roteiro de bancada e integração futura

1. Ligar sem configuração privada: verificar UUID persistido, Premium/nuvem indisponíveis com motivo, coleta e histórico local funcionais. Reiniciar e alternar físico/mock sem apagar flash: IDs devem permanecer iguais e os históricos separados.
2. Informar Wi-Fi/URL SaaS/destino/CA e provedores em integração futura. Abrir Premium e verificar IDs na URL, sem senha/token/código. Cadastrar conta por e-mail/senha no SaaS e verificar vínculo com UID sem sobrescrever outro cliente. Ainda sem assinatura, nenhum documento de sessão deve ser escrito.
3. Com servidor autorizando premium ativo, verificar upload do histórico anterior. Manter veículo presente até validar: contar uma sessão remota; retirar veículo e verificar atualização desse mesmo documento. Alterar e-mail da conta: clientId e caminho não devem mudar.
4. Na unidade real, medir chamadas/amostras do sensor durante DNS indisponível, handshake TLS lento, servidor sem resposta e arquivo cheio; exigir processamento no intervalo configurado de 100 ms. Fakes verificam o loop independente do worker, mas não comprovam tempo real/heap do ESP32. Confirmar acesso AP+STA, inclusive mudança de canal e reconexão do navegador.
5. Cortar energia antes do envio, depois de escrita remota e antes/depois do marcador local. Verificar reenvio sem duplicação, preservação de NVS/journal, interrupted sem closedAt inventado e retomada do encerramento quando persistido.
6. Revogar/expirar premium no servidor: novas escritas devem ser recusadas e o painel deve bloquear envio mantendo sessões locais. Testar dois clientes e duas placas com sessionId igual, origem física/mock e tentativa autenticada de escrever fora do próprio vínculo; validar regras/backend reais, não apenas caminho diferente.
7. Inspecionar firmware privado, logs, respostas locais e URL: não conter service-account key, senha SaaS ou tokens. Remover CA/hora de rede e verificar que não há fallback TLS inseguro. Validar falta de espaço para marcador sem apagar histórico.

Não houve upload na ESP32, ensaio físico, cadastro SaaS, cobrança ou escrita em Firebase real nesta mudança. A validação desses cenários requer aparelho e os dados de acesso futuros. As verificações automatizadas usam interfaces/provedores falsos e não comprovam autorização remota em produção ou latência real.

## Liberação de teste Firebase — 03/10/2026

Implementado provedor REST de renovação Firebase Auth e autorização local de bancada com prazo fixo até 10/10/2026 às 21h de Brasília, sem compra de premium. O teste continua exigindo token real, identidade persistida, TLS/UTC e regras remotas restritas. O painel distingue esse modo da autorização premium de produção. Nenhuma alteração foi feita no provisionamento de Wi-Fi.

`scripts/test-native.ps1`: sete executáveis aprovados, incluindo a nova suíte `tests/firebase.cpp` para renovação/caching/rotação, UID/projeto incompatíveis, respostas incompletas ou malformadas, token sem validade, vínculo exato e limite temporal da autorização. `tests/firebase-firmware.cpp` seleciona os provedores de teste no sketch real com credenciais fictícias e verifica APIs autenticadas, ausência de segredos, recusa de flags de liberação e operação local. `npm run test:web`: 12 testes aprovados, incluindo liberação/expiração de bancada e layout móvel. Captura `build/web-firebase-test-mobile.png` inspecionada. Arquivos privados são ignorados pelo Git; preparação PowerShell passou pela validação sintática.

Compilações finais dos dois perfis aprovadas contra o core ESP32 fixado:

| Perfil | Flash (limite 1.310.720 bytes) | RAM estática (limite 327.680 bytes) | Log |
| --- | --- | --- | --- |
| Físico | 1.279.692 bytes (97%); livres 31.028 bytes | 54.280 bytes (16%); livres 273.400 bytes | `build/compile-physical.log` |
| Simulado | 1.276.160 bytes (97%); livres 34.560 bytes | 54.216 bytes (16%); livres 273.464 bytes | `build/compile-mock.log` |

Estes binários foram gerados **sem credencial Firebase real**, mantendo o envio desativado. A seleção dos provedores de teste foi validada no sketch nativo com fixtures; após provisionar a credencial real, é necessário recompilar e conferir novamente o tamanho do firmware antes do upload. O heap/TLS e o envio na placa ainda não foram medidos. A margem de flash permanece pequena.

A primeira tentativa real de criar o usuário anônimo no projeto `lavcounter` foi recusada com `ADMIN_ONLY_OPERATION`. Após o usuário habilitar o provedor anônimo, `scripts/prepare-firebase-test.ps1` criou a conta dedicada e confirmou a renovação real do ID token, incluindo projeto/UID esperados. A credencial foi salva em `CloudCredentials.h`, ignorado pelo Git. UID público da unidade de teste: `dtW8bQiOVBXebvokOWKjZmwSaqY2`.

Nenhuma regra foi publicada e nenhuma sessão foi escrita no Firestore. `build/firestore-test.rules` já contém o UID autenticado, mas ainda precisa de clientId/deviceId da unidade real e de publicação pelo proprietário do projeto. O painel local não estava acessível desta máquina em `192.168.4.1`; os IDs foram solicitados ao usuário. A ESP32 foi detectada na COM4; nenhum upload foi feito nesta etapa.

Após o provisionamento, os dois perfis foram recompilados com sucesso **com a credencial Firebase real e os provedores de teste habilitados**:

| Perfil | Flash (limite 1.310.720 bytes) | RAM estática (limite 327.680 bytes) | Log |
| --- | --- | --- | --- |
| Físico | 1.285.676 bytes (98%); livres 25.044 bytes | 54.368 bytes (16%); livres 273.312 bytes | `build/compile-physical.log` |
| Simulado | 1.282.244 bytes (97%); livres 28.476 bytes | 54.304 bytes (16%); livres 273.376 bytes | `build/compile-mock.log` |

Binários atualizados em `build/firmware-physical/LavCounter.ino.bin` e `build/firmware-mock/LavCounter.ino.bin`. A autenticação e renovação reais foram verificadas pelo script no computador; envio de sessões e consumo de heap durante TLS na ESP32 permanecem pendentes de bancada. A partição de aplicação do perfil físico tem 25.044 bytes livres.

## Teste sem cópia manual de IDs — 04/10/2026
Regras de bancada passam a aceitar clientId/deviceId gerados pela placa, mantendo UID dedicado, prazo, validação de caminho/payload e progressão de etapas. Consulta de sessões permitida no caminho específico. Script gera regras completas sem parâmetros de IDs. Autenticação e renovação reais aprovadas novamente. Publicação das regras e upload de sessões ainda não confirmados. Este teste de uma unidade não implementa vínculo/isolamento de contas de produção.
Verificação remota em 04/10/2026 após publicação informada pelo usuário: GET autenticado de documento inexistente com IDs válidos não provisionados retornou HTTP 404, em vez de recusa de autorização. Nenhuma sessão foi escrita por essa sondagem. Wi-Fi STA não preenchido no arquivo privado bloqueia o teste de envio pela ESP32.

## Wi-Fi pelo painel — 04/10/2026

Implementados cadastro autenticado de rede/senha, persistência em registro NVS único comum aos modos, reconexão com AP local ativo e fluxo Conectar e sincronizar aguardando Wi-Fi/autorização Firebase. Senha ausente das respostas. Reconexão volta a verificar autorização após falha anterior. Nenhum novo teste automatizado escrito ou executado nesta etapa, conforme solicitação do usuário. Sintaxe JavaScript e correspondência do painel incorporado verificadas.

Compilação final simulada aprovada: 1.289.984 / 1.310.720 bytes de flash (98%), RAM estática 54.312 / 327.680 bytes (16%). Firmware gravado com sucesso na ESP32-D0WD-V3 da COM4; esptool verificou hashes e reiniciou a placa. Upload usou a partição existente, sem apagar NVS nem SPIFFS. Envio de sessões, persistência de Wi-Fi após reinício e comportamento AP+STA aguardam configuração da rede pelo painel e validação na unidade real. Perfil físico não recompilado nesta etapa.

## Formatos e ordem dos eventos — 04/10/2026

Usuário confirmou que a integração Firebase funcionou na unidade após configurar a rede pelo painel. Atualização seguinte padroniza datas/horas exibidas em dd/MM/YYYY HH:mm, durações em hh:mm:ss e ordena eventos como Sessão / Início, Encerramento, Duração, Resultado, Validação. Inclui relógio/edição, última sincronização e expiração do teste; APIs e registros permanecem no formato original. Nenhum novo teste automatizado criado. Sintaxe JavaScript conferida; compilação simulada aprovada com 1.291.488 bytes de flash (98%) e 54.312 bytes de RAM estática (16%). Upload na COM4 concluído com hash verificado e reinício, sem apagar NVS/SPIFFS. Esptool fez fallback da gravação incremental para a imagem inteira antes da verificação final bem-sucedida. Visualização final no painel da unidade aguarda recarga pelo usuário.

## Cores dos resultados — 04/10/2026

Eventos com validated=true recebem fundo verde suave (#edf7f0); todos os demais recebem vermelho suave (#fff1f0). Texto de resultado usa verde/vermelho mais escuro e continua explícito. Sintaxe JavaScript conferida, sem novos testes automatizados. Perfil simulado compilado com 1.291.792 bytes de flash (98%) e 54.312 bytes de RAM estática (16%). Gravado na COM4, com hash final verificado e reinício. NVS e SPIFFS preservados; visualização final aguarda recarga do painel.
