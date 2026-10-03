# LavCounter

Firmware ESP32 que infere uma lavagem por permanência de veículo com VL53L1X. Opera localmente, sem internet ou assinatura. Uma sessão valida após presença contínua e termina após ausência contínua; não mede consumo de água.

## Hardware e calibração

VL53L1X e RTC DS3231 compartilham SDA GPIO21 / SCL GPIO22. Semáforo: vermelho GPIO25, amarelo GPIO18, verde GPIO19. Consulte [montagem e bancada](docs/BANCADA.md) antes de ligar o módulo. O box vazio precisa produzir leitura válida acima do limiar. Leitura inválida é falha, não ausência.

Padrões: distância máxima 1500 mm, permanência mínima 120 s, ausência mínima 10 s. Configure pelo painel; mudanças valem para novas sessões. Ao ligar, aguarda box vazio para evitar recontagem. Consulte [máquina de estados](docs/DETECCAO.md).

## Compilar

Dependências fixadas: core esp32:esp32 3.3.10, ArduinoJson 7.4.2 e VL53L1X de Pololu 1.3.1. Node.js incorpora o painel.

```powershell
arduino-cli core install esp32:esp32@3.3.10
arduino-cli lib install ArduinoJson@7.4.2 VL53L1X@1.3.1
./scripts/build.ps1
```

O script aceita `-ArduinoCli` e registra `build/compile-physical.log`, com binários em `build/firmware-physical`. O perfil sketch.yaml fixa as dependências. Edite web/index.html e execute node scripts/embed-web.cjs; não edite WebPage.h manualmente.

### Testar sem o sensor

Compile com `./scripts/build.ps1 -MockSensor` ou habilite `LAVCOUNTER_SENSOR_MOCK` em `BuildConfig.h` pela IDE. Após carregar esse firmware, o painel oferece **Veículo presente**, **Box vazio** e **Falha do sensor**, respeitando os tempos configurados. Dados de teste ficam separados dos físicos. Consulte [ativação, API e roteiro do mock](docs/MOCK-SENSOR.md).

### Compilar pela Arduino IDE

Abra `firmware/LavCounter/LavCounter.ino`, selecione a placa ESP32 e instale **ArduinoJson**, de Benoit Blanchon, versão **7.4.2**, pelo Gerenciador de Bibliotecas da IDE.

Se aparecer `ArduinoJson.h: No such file or directory`, confira a pasta do sketchbook nas preferências: a IDE pode usar uma pasta diferente do Arduino CLI. Neste computador, a IDE está configurada para `C:\WashControl_MVP_HelloDisplay`; portanto, procura a biblioteca em `C:\WashControl_MVP_HelloDisplay\libraries\ArduinoJson`. Uma instalação apenas em `Documents\Arduino\libraries` não atende essa configuração. Não é necessário alterar os includes do firmware.


Na IDE, instale também **VL53L1X**, de Pololu, versão **1.3.1**, no mesmo sketchbook.

## Acesso local

Rede **LavCounter**, senha **lavcounter123**. Acesse http://192.168.4.1 (mDNS lavcounter.local quando disponível). HTTP Basic: usuário **lavcounter**, senha **lav123**. Todas as rotas exigem autenticação. O serviço é local e não usa TLS.

Painel: contagem observada, persistida, diária, sessão, distância, saúde do sensor, histórico e parametrização. Dados locais não dependem da futura camada SaaS; não há métricas financeiras nesta versão. RTC inválido deixa o diário indisponível, sem interromper a temporização monotônica.

## Dados e atualização

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
