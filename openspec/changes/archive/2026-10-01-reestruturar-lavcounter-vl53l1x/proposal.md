# Reestruturar LavCounter para detecção por presença

## Why

O LavCounter deixou de ser um medidor baseado em fluxo de água e passou a ser um contador inteligente de lavagens por presença de veículo.

A mudança reduz complexidade de instalação, elimina dependência hidráulica e posiciona o produto como ferramenta de conciliação de caixa para lava-rápidos.

## What Changes

- **BREAKING**: remover suporte ao sensor de fluxo de água e substituir os contratos de configuração, status e sessão por presença veicular.
- Remover métricas de pulsos, litros, volume, vazão e custo por m³.
- Usar o sensor VL53L1X para detectar presença de veículo.
- Validar lavagem quando o veículo permanecer detectado por tempo mínimo configurável.
- Computar apenas uma lavagem por sessão.
- Encerrar sessão quando o veículo deixar de ser detectado por tempo mínimo configurável.
- Usar RTC para registrar data e hora.
- Usar semáforo de LEDs:
  - vermelho: nenhum veículo detectado;
  - amarelo: veículo detectado, mas ainda não validado;
  - verde: lavagem validada/computada.
- Manter painel local com métricas básicas.
- Preparar separação futura para métricas premium SaaS.

## Capabilities

### New Capabilities

- `vehicle-presence`: aquisição VL53L1X, parâmetros persistentes, sessão por presença e semáforo.
- `wash-history`: contagem única, registros com RTC, persistência e separação do histórico hidráulico.
- `local-dashboard`: API autenticada e painel offline com métricas básicas e limites explícitos da futura oferta SaaS.

### Modified Capabilities

Nenhuma: não existem especificações principais publicadas. As mudanças abertas `lavcounter-deteccao-lavagem` e `lavcounter-presenca-veicular` são contexto; esta mudança concentra a reestruturação, sem marcar aquelas como implementadas ou arquivá-las.

## Impact

- Firmware da ESP32.
- Máquina de estados de detecção.
- Modelo de sessão.
- API/local web server.
- Painel local.
- Testes ou simulações de detecção.
- Configuração e histórico versionados; preservar dados hidráulicos existentes sem convertê-los em lavagens por presença.
- Dependência do driver VL53L1X e documentação de montagem/calibração. Reutilizar RTC DS3231, SPIFFS, SoftAP e autenticação existentes.
