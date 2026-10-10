# Sensor real exclusivo — 07/10/2026

O firmware usa exclusivamente o VL53L0X físico, com o driver Pololu 1.3.1. Foram removidos o adaptador `MockPresenceSensor`, a seleção `LAVCOUNTER_SENSOR_MOCK`, o parâmetro `-MockSensor`, a rota `/api/mock-sensor`, o campo `mockSample`, o console de simulação e os controles de simulação do painel. O console serial permanece disponível para observar medições reais e testar os endereços I²C.

IDE e CLI compilam a mesma aquisição física. O sensor mantém orçamento de 50 ms e período de 100 ms, lê resultados disponíveis com validação de status/tamanho da resposta, rejeita distância zero/65535 e trata falha, desconexão ou amostra expirada como leitura desconhecida. Não existe substituição automática por ausência simulada. A máquina de estados continua validando uma lavagem por presença contínua e encerrando por ausência contínua válida.

Configuração física (`lavcounter`), histórico (`/presence-v2.jsonl`), confirmações de nuvem (`/cloud-v1.jsonl`), identidade e caminho remoto `modes/physical` foram preservados. Dados de simulação preexistentes não são apagados nem importados para os dados físicos. Documentos antigos de mock foram identificados como históricos. A compatibilidade dos utilitários de vínculo/caminho com metadados antigos não oferece um modo simulado no firmware.

Os testes de simulação de produto foram removidos. Fakes de hardware continuam restritos aos testes no computador, para verificar leituras válidas/inválidas, comunicação, temporização, recuperação, armazenamento e nuvem sem depender de uma placa física. O teste de firmware verifica também a ausência da rota/campo de simulação e a preservação de histórico antigo de teste. O teste web verifica distância real e ausência de controles de simulação.

As expectativas antigas dos testes web foram atualizadas para o comportamento existente: botão offline abre configuração Wi-Fi, datas inválidas aparecem como horário indisponível e o campo RTC usa `dd/MM/YYYY HH:mm`. Nenhuma lógica de Wi-Fi ou relógio foi alterada nesta remoção.

`scripts/test-native.ps1`: seis executáveis aprovados com os fontes finais, incluindo detecção física, console, armazenamento, identidade, autenticação e sincronização. Playwright: os 11 testes remanescentes passaram na suíte completa; o novo caso de sensor real passou em execução direcionada, totalizando 12 casos aprovados. O teste do perfil mock foi removido.

`git diff --check`: aprovado. `openspec validate --all --strict --no-interactive`: as quatro specs principais passaram; a verificação global apontou uma pendência preexistente na mudança `lavcounter-presenca-veicular`, que não contém delta specs. Essa mudança antiga não foi modificada nesta etapa.

A configuração local da Arduino IDE aponta o sketchbook para `C:\WashControl_MVP_HelloDisplay`. Como a biblioteca física não estava instalada nesse diretório, foi instalada Pololu VL53L0X 1.3.1 usando a configuração da IDE. A instalação já existente no sketchbook do CLI continua disponível.

Compilação final via `scripts/build.ps1`: **aprovada**. Flash: 1.295.688 / 1.310.720 bytes (98%), com 15.032 bytes livres. RAM estática: 54.816 / 327.680 bytes (16%), com 272.864 bytes livres. Binário em `build/firmware-physical/LavCounter.ino.bin`; log em `build/compile-physical.log`. A partição de aplicação tem pouca margem para novas funcionalidades. A RAM estática não comprova heap disponível durante TLS na placa.

Não houve upload nesta etapa. Para executar na unidade real, carregue o novo firmware sem apagar NVS/flash e envie `sensor on` no Monitor Serial a 115200 baud com Nova linha. A saída deve começar com `[VL53L0X]`, apresentar validade e distância em milímetros. A validação óptica, alcance e latência na placa continuam pendentes.
