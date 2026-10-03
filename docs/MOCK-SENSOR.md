# Simular lavagens sem VL53L1X

O mock roda na ESP32 e é controlado pelo painel local. Presença, ausência e falha alimentam a mesma máquina de estados, LEDs e gravação usados na operação física. Os tempos são reais; não existe botão para computar uma lavagem imediatamente.

## Compilar e ativar

Na Arduino IDE, abra `firmware/LavCounter/LavCounter.ino`. Em `BuildConfig.h`, altere o valor padrão de `LAVCOUNTER_SENSOR_MOCK` de `0` para `1` e compile/carregue normalmente. Não habilite apagar toda a flash: os registros e a configuração devem ser preservados. Para retornar ao VL53L1X, restaure `0` e carregue novamente.

Pelo PowerShell na raiz do projeto:

```powershell
.\scripts\build.ps1 -MockSensor
.\scripts\build.ps1
```

O primeiro comando gera `build/firmware-mock`, o segundo `build/firmware-physical`. Logs ficam em `build/compile-mock.log` e `build/compile-physical.log`. O script define explicitamente o modo e prevalece sobre o padrão do cabeçalho; ele somente compila, não faz upload. Dependências: core ESP32 3.3.10 e ArduinoJson 7.4.2; apenas o build físico exige Pololu VL53L1X 1.3.1. O mock não inclui nem inicializa esse driver. Ausência do sensor no firmware físico continua sendo falha, nunca ativa o mock.

## Usar o painel

Conecte-se à rede `LavCounter` (senha `lavcounter123`), abra `http://192.168.4.1` e autentique com usuário `lavcounter` e senha `lav123`. A faixa **Modo de simulação** confirma o firmware carregado. Sem essa faixa, confira o build e o upload.

- **Veículo presente:** inicia/continua a presença; aguarde a permanência mínima para validar.
- **Box vazio:** inicia/continua ausência; aguarde o mínimo para encerrar a sessão.
- **Falha do sensor:** simula leitura desconhecida, interrompe continuidade dos temporizadores e faz piscar a cor da sessão, sem validar nem encerrar.

Repetir um comando não reinicia o tempo. Fechar ou recarregar a página não altera a entrada. Reiniciar a ESP32 restaura a entrada para box vazio e exige ausência confirmada antes de iniciar sessões. Alterações em Parametrização valem para novas sessões; use tempos menores para ensaios rápidos.

## API local

Todas as rotas exigem a autenticação existente. Exemplo PowerShell:

```powershell
$headers = @{ Authorization = 'Basic ' + [Convert]::ToBase64String([Text.Encoding]::ASCII.GetBytes('lavcounter:lav123')) }
Invoke-RestMethod -Uri http://192.168.4.1/api/mock-sensor -Method Post -Headers $headers -ContentType 'application/json' -Body '{"sample":"present"}'
curl.exe -u lavcounter:lav123 http://192.168.4.1/api/status
```

`POST /api/mock-sensor` aceita somente um campo `sample`, com `present`, `absent` ou `fault`. Resposta 200: `{"sensorMode":"mock","mockSample":"present"}`. Corpo inválido retorna 400 sem trocar a entrada, falta de autenticação retorna 401 e firmware físico retorna 404. A confirmação não significa lavagem validada. Em erro de rede, o painel consulta o status novamente, pois o comando pode ter chegado ao dispositivo.

`/api/status` adiciona `sensorMode` (`mock`/`physical`) e `mockSample` (entrada ou `null` no físico). Preserva `source: local`; distância é `null` no mock. Sessões e itens de `/api/sessions` incluem `sensorMode`. Identifique uma sessão pelo par modo/ID; IDs podem coincidir entre os modos.

## Dados e relógio

| Dado | Físico | Mock |
|---|---|---|
| Journal SPIFFS | `/presence-v2.jsonl` | `/presence-mock-v2.jsonl` |
| Namespace NVS (configuração e IDs) | `lavcounter` | `lavcounterMock` |

Cada modo consulta apenas seus registros e contagens. O primeiro uso do mock carrega os padrões, sem copiar parâmetros físicos. Registros físicos antigos sem origem são lidos como físicos. Não há apagamento automático. Os arquivos compartilham a capacidade SPIFFS; falta de espaço continua sendo informada.

O RTC é real e compartilhado: ajustá-lo altera o relógio do dispositivo em ambos os modos. Sem RTC válido, a simulação continua contando por tempo monotônico, com horários indisponíveis e total sem data. Os LEDs também são reais, nos GPIOs já configurados.

## Roteiro de bancada

1. Carregue o mock sem sensor e sem apagar flash. Confirme faixa, entrada Box vazio e vermelho piscando durante a espera inicial; após a ausência mínima, vermelho fixo e nenhuma sessão.
2. Em Parametrização, salve permanência de 3 s e ausência de 2 s. Registre os totais iniciais, pois ensaios anteriores são preservados.
3. Selecione Veículo presente: amarelo e sessão candidata. Antes de 3 s não deve contar. Após 3 s contínuos, verde e aumento de exatamente uma lavagem. Aguarde mais 5 s e repita o comando: a contagem não aumenta.
4. Selecione Falha do sensor: verde pisca, diagnóstico de falha simulada e mesma contagem. Box vazio por menos de 2 s seguido de presença mantém a sessão validada.
5. Selecione Box vazio e aguarde pelo menos 2 s: vermelho e sessão encerrada. Em Eventos, atualize e confirme um único registro consolidado, marcado Simulada, com validação e encerramento (ou horários indisponíveis sem RTC).
6. Inicie nova presença; antes de 3 s selecione falha. Restaure presença e confirme que exige outros 3 s completos. Recarregue/feche a página: o processamento continua na ESP32.
7. Aguarde persistência sem fila, reinicie e confira totais preservados, box vazio inicial e ausência obrigatória; sessão interrompida não recebe saída inventada.
8. Recarregue firmware físico sem apagar flash e confira configuração/histórico físico anteriores, ausência de controles mock e falha de aquisição se VL53L1X continuar desconectado.

Os testes automatizados não comprovam alcance óptico, I2C ou conexões elétricas reais. Registre separadamente execução em bancada e testes em host.
