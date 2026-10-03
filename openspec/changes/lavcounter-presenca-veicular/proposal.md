# Proposal

## Why

O LavCounter passa a contar lavagens inferidas pela permanência de veículos para apoiar a conciliação de caixa de lava-rápidos. Medir consumo de água deixou de atender ao objetivo do produto.

## What Changes

- **BREAKING**: substituir aquisição, sessões e classificação por água por presença com VL53L1X, ESP32, RTC e semáforo obrigatório.
- Validar uma única lavagem por sessão após presença mínima; encerrar após ausência mínima, com configuração persistente.
- **BREAKING**: substituir contrato de eventos, configuração e métricas por sessões `VALID_WASH`, duração e distâncias em milímetros.
- Oferecer contagens locais, últimas sessões, diagnóstico do sensor e semáforo, com identificação explícita de dados locais.
- Separar métricas locais da futura camada premium de conciliação, sem nuvem, cobrança ou cálculos financeiros nesta entrega.
- Remover lógica, interface e documentação operacional de pulsos, litros, volume, vazão e custo por m³.
- Não criar testes de integração; planejar testes unitários, compilação e conferência manual de bancada.

## Capabilities

### New Capabilities

- `vehicle-presence`: aquisição, configuração, estados e contagem única por permanência.
- `wash-history`: contrato, relógio, persistência e contagens de sessões válidas.
- `local-dashboard`: painel e APIs locais com separação de capacidades premium futuras.

### Modified Capabilities

Nenhuma especificação principal está publicada. A mudança em andamento `lavcounter-deteccao-lavagem` contém o escopo antigo: suas regras de fluxo e classificação são substituídas por esta proposta, sem arquivá-la como concluída nem alterar suas evidências históricas.

## Impact

Afeta `firmware/LavCounter/{Detection.h,ConfigStore.h,Event.h,Storage.h,LavCounter.ino}`, integração com `RtcClock.h`, `web/index.html`, cabeçalho gerado `WebPage.h`, dependências de compilação, testes legados e documentação. Reutiliza servidor local autenticado, SoftAP, RTC DS3231 e SPIFFS; adiciona adaptador VL53L1X. Dados antigos permanecem preservados, separados do novo histórico, sem conversão automática em lavagens por presença. Preços de R$ 455,00 e R$ 34,90/mês são contexto comercial, não requisitos do firmware.
