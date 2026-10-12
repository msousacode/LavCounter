# LavCounter

Firmware ESP32 que infere uma lavagem por permanência de veículo com VL53L0X ou VL53L1X, identificados automaticamente. Opera localmente, sem internet ou assinatura. Uma sessão valida após presença contínua e termina após ausência contínua; não mede consumo de água.

## Hardware e calibração

VL53L0X/VL53L1X e RTC DS3231 compartilham SDA GPIO21 / SCL GPIO22. Semáforo: vermelho GPIO25, amarelo GPIO18, verde GPIO19. Consulte [montagem e bancada](docs/BANCADA.md) antes de ligar o módulo. Baia vazia pode ser reconhecida por distância válida acima do limiar ou por status 2 admissível do VL53L1X (sinal insuficiente). Outros resultados inválidos permanecem como falha/desconhecido.

Padrões: distância máxima 1,5 m, permanência mínima 2 min, ausência de 30 s para encerrar lavagem validada e 5 s para invalidar candidata. O painel oferece apenas as opções permitidas, com os dois tempos independentes. Configure pelo painel; mudanças valem para novas sessões. Ao ligar ou reiniciar, começa em IDLE e detecta sem exigir baia vazia prévia. Consulte [máquina de estados](docs/DETECCAO.md).

## Compilar

Dependências fixadas: core esp32:esp32 3.3.10, ArduinoJson 7.4.2 e bibliotecas VL53L0X e VL53L1X de Pololu 1.3.1. Node.js incorpora o painel.

```powershell
arduino-cli core install esp32:esp32@3.3.10
arduino-cli lib install ArduinoJson@7.4.2 VL53L0X@1.3.1 VL53L1X@1.3.1
./scripts/build.ps1
```

O script aceita `-ArduinoCli` e registra `build/compile-physical.log`, com binários em `build/firmware-physical`. O perfil sketch.yaml fixa as dependências. Edite web/index.html e execute node scripts/embed-web.cjs; não edite WebPage.h manualmente.

### Testar o sensor real

O firmware identifica automaticamente o VL53L0X ou VL53L1X físico no boot e tenta novamente a cada 5 s se a inicialização falhar. A aquisição e detecção de presença começam sem comandos. Abra o Monitor Serial em 115200 baud para ver as leituras automáticas; `sensor off` silencia apenas o diagnóstico. Consulte o [console do sensor](docs/CONSOLE-SENSOR.md). A ausência ou falha do sensor bloqueia a detecção e aparece no diagnóstico.

### Compilar pela Arduino IDE

Abra `firmware/LavCounter/LavCounter.ino`, selecione a placa ESP32 e instale **ArduinoJson**, de Benoit Blanchon, versão **7.4.2**, pelo Gerenciador de Bibliotecas da IDE.

Se aparecer `ArduinoJson.h: No such file or directory`, confira a pasta do sketchbook nas preferências: a IDE pode usar uma pasta diferente do Arduino CLI. Neste computador, a IDE está configurada para `C:\WashControl_MVP_HelloDisplay`; portanto, procura a biblioteca em `C:\WashControl_MVP_HelloDisplay\libraries\ArduinoJson`. Uma instalação apenas em `Documents\Arduino\libraries` não atende essa configuração. Não é necessário alterar os includes do firmware.


Na IDE, instale também **VL53L0X** e **VL53L1X**, de Pololu, versão **1.3.1**, no mesmo sketchbook.

## Acesso local

Rede **LavCounter**, senha **lavcounter123**. Acesse http://192.168.4.1 (mDNS lavcounter.local quando disponível). HTTP Basic: usuário **lav123**, senha **lav123**. Todas as rotas exigem autenticação. O serviço é local e não usa TLS.

Painel: contagem observada, persistida, diária, sessão, distância, saúde do sensor, histórico e parametrização. Dados locais não dependem da futura camada SaaS; não há métricas financeiras nesta versão. RTC inválido deixa o diário indisponível, sem interromper a temporização monotônica.

Após ligar ou reiniciar, a detecção começa em IDLE, sem exigir área livre por um tempo inicial. Uma primeira leitura válida de presença inicia CANDIDATE e ainda precisa cumprir a permanência mínima para validar uma lavagem. Leituras inválidas não iniciam sessões. Uma reinicialização interrompe a sessão anterior; presença mantida após o reinício pode iniciar uma nova sessão.

## Nuvem e Premium

O aparelho gera `clientId` automaticamente na primeira inicialização e o mantém na NVS; `deviceId` usa o MAC de fábrica. A ação **Premium** abrirá o SaaS com esses IDs para associar a conta criada por e-mail/senha, sem código temporário. Sem autorização premium ativa, todas as sessões continuam apenas locais. Após ativar, o histórico persistido pode ser enviado ao Cloud Firestore automaticamente ou pelo botão **Sincronizar com a nuvem**.

A integração começa sem acesso configurado: nenhum upload ocorre. URL SaaS, destino Firebase, Wi-Fi, CA e provedores de autenticação/premium serão informados posteriormente em `CloudPrivate.h`, ignorado pelo Git. Consulte [configuração e contratos da nuvem](docs/NUVEM.md). O SaaS, pagamento e provisionamento remoto não fazem parte deste firmware.

## Histórico local

A configuração usa presenceConfig em NVS; histórico usa /presence-v2.jsonl. Dados antigos são preservados e não entram nas contagens. Validação e encerramento compartilham ID. Faça backup de firmware, filesystem e NVS antes de atualizar uma unidade; veja [API, migração e rollback](docs/API.md).

A fila comporta oito registros. Falhas e perdas são visíveis; o firmware nunca formata ou apaga histórico automaticamente. Energia interrompida pode perder sessão e gravações pendentes. No boot, o total é reconstruído do que foi persistido. Instalação inicial do SPIFFS exige procedimento explícito de acordo com a partição real.

## Verificação

```powershell
./scripts/test-native.ps1
npm ci
npm run test:web
```

Testes nativos usam Zig 0.14.1 e periféricos simulados; web usa Edge e API simulada. Veja [evidências e limites](docs/VALIDACAO.md). Inferir presença não comprova uma lavagem real. Calibração física permanece necessária.

## Origem

Veja [proveniência histórica](docs/PROVENIENCIA.md).


