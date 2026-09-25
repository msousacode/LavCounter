# LavCounter

Firmware ESP32 para monitorar sessões de água, aguardar retomadas e classificar eventos como **lavagem**, **uso não identificado** ou **evento descartado**. Opera sem LCD e sem tag. A interface embarcada funciona sem internet.

## Hardware

| Componente | Conexão inicial |
| --- | --- |
| Sensor de fluxo (base: YF-S201) | Sinal em GPIO27, interrupção FALLING |
| RTC DS3231 | SDA GPIO21, SCL GPIO22, endereço 0x68 |
| LEDs opcionais | Vermelho GPIO5, amarelo GPIO18, verde GPIO19 |
| Buzzer legado | GPIO26 mantido inativo |

Use sinais compatíveis com 3,3 V no ESP32. Confira modelo, alimentação e interface elétrica do sensor antes de conectar. O RTC compartilha apenas o barramento I2C; não há biblioteca nem inicialização de LCD. Placa inicial de compilação: ESP32 genérico, flash 4 MB, partição `default`. Confira a placa real antes do upload.

## Compilar

Ferramentas usadas: Arduino CLI 1.5.1, core **esp32:esp32 3.3.10**, ArduinoJson **7.4.2** e Node.js para incorporar o HTML.

```powershell
# Se necessário, com Arduino CLI no PATH:
arduino-cli core install esp32:esp32@3.3.10
arduino-cli lib install ArduinoJson@7.4.2
./scripts/build.ps1
```

O script aceita `-ArduinoCli <caminho>` e gera `build/compile.log`. O perfil `firmware/LavCounter/sketch.yaml` também fixa plataforma e biblioteca. A fonte da web é `web/index.html`; `node scripts/embed-web.cjs` atualiza o cabeçalho embarcado. Não edite o cabeçalho gerado.

## Acesso local

1. Conecte à rede **LavCounter**, senha Wi-Fi **lavcounter123**.
2. Acesse `http://192.168.4.1` (ou o IP exibido na serial). mDNS: `lavcounter.local` quando suportado pelo cliente.
3. Autentique no navegador: usuário **lavcounter**, senha **lav123**.

As credenciais HTTP são diferentes da senha Wi-Fi. Página e APIs exigem autenticação. O serviço foi projetado para rede local; não possui TLS.

## Calibração sem alterar código

Abra **Parametrização**. A tela mostra valores salvos, unidades, limites e ajuda por campo. Edite timeout, mínimos de duração/fluxo/volume, faixa de vazão, pausas, retomadas, proporção de fluxo e pontuação. A seção avançada permite ajustar pulsos/L, filtro elétrico e janela de vazão.

- **Salvar** confirma somente após persistir no ESP32. Valores permanecem após reiniciar.
- **Cancelar alterações** recarrega os valores salvos.
- **Restaurar padrões** pede confirmação antes de salvar os valores iniciais.
- Alterações valem para **novas sessões**. A sessão ativa conserva sua parametrização; o histórico não é reclassificado.

O timeout encerra a sessão após inatividade e aciona a classificação. Ele **não determina sozinho** se houve lavagem. Consulte [detecção e calendário](docs/DETECCAO.md), [API e histórico](docs/API.md) e [roteiro de bancada](docs/BANCADA.md).

## Armazenamento e limitações

Eventos encerrados de todas as categorias são gravados em SPIFFS. Eventos ainda na fila não são confirmados como persistidos. O firmware não formata a partição automaticamente nem apaga eventos para liberar espaço. Em partição vazia/não formatada, deve ser gravada uma imagem SPIFFS inicial por procedimento explícito de instalação. Preserve um backup antes de substituir firmware ou filesystem.

Fila limitada a oito eventos; falha prolongada de armazenamento pode esgotá-la. A web informa fila, perda de eventos, espaço e falhas. Um corte de energia pode perder sessão ativa e eventos ainda na fila. A recuperação exata desses dados não faz parte desta versão. Ruído muito intenso pode esgotar o buffer de pulsos; a sessão é sinalizada como degradada e não vira lavagem automática.

Os limiares são iniciais para calibração. Usos de água com padrões semelhantes podem ser confundidos; valide com exemplos reais do estabelecimento. A compilação e testes de software não substituem os testes físicos.

## Origem

Base reaproveitada e preservada: veja [proveniência](docs/PROVENIENCIA.md).

## Testes e evidências

```powershell
./scripts/test-native.ps1
npm ci
npm run test:web
```

Os testes nativos usam Zig 0.14.1 (`-Zig` aceita outro caminho) e executam núcleo, persistência e handlers reais com periféricos simulados. Os testes web usam Microsoft Edge instalado e API simulada. Veja [resultados e limites da validação](docs/VALIDACAO.md); aprovação em computador não substitui a bancada.
