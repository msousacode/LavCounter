# Proposal

## Why

O firmware base mede pulsos, mas depende de tags para iniciar sessões e não encerra por inatividade. O LavCounter precisa registrar usos reais da água e distinguir lavagens de usos duvidosos por múltiplas características, permitindo ajuste por estabelecimento.

## What Changes

- Criar a nova versão em C:\LavCounter a partir do código confirmado em C:\WashControl_MVP_HelloDisplay, preservando a origem.
- Adotar LavCounter no firmware, interface, rede e documentação.
- Remover a dependência do LCD: esta versão opera sem display físico, com visualização pela web.
- Oferecer tela Parametrização para o usuário editar e salvar timeouts e critérios sem alterar código ou recompilar firmware.
- **BREAKING**: substituir o ciclo obrigatório de tags por sessões automáticas de fluxo, com espera por retomada e encerramento por inatividade.
- Medir pulsos, volume fracionário, vazão, duração, tempo efetivo com fluxo, pausas e retomadas; registrar início e fim pelo RTC.
- Classificar eventos encerrados como lavagem, uso não identificado ou evento descartado, com critérios combinados, parâmetros configuráveis e motivos auditáveis.
- Persistir todas as categorias e sinalizar falhas de relógio ou armazenamento.
- Simplificar o frontend embarcado existente para status, histórico, configuração de detecção e ajuste do RTC; autenticar página e APIs com lavcounter / lav123.
- Documentar compilação ESP32 e validar a lógica com sequências reproduzíveis de pulsos e testes de bancada.

## Capabilities

### New Capabilities

- `flow-sessions`: aquisição de fluxo, sessão automática, métricas e RTC.
- `event-classification`: critérios mínimos, classificação combinada e configuração por estabelecimento.
- `event-history`: persistência e consulta de eventos com motivos e qualidade dos dados.
- `embedded-web`: interface local autenticada e identidade LavCounter.

### Modified Capabilities

Nenhuma: ainda não existem especificações principais.

## Impact

Afeta a cópia do sketch Arduino, StateMachine, SystemState, Storage e frontend inline. Reutiliza ESP32 Arduino, WebServer, SoftAP, SPIFFS, sensor configurado como YF-S201 e RTC configurado como DS3231. A implementação deverá corrigir a leitura dos registradores de calendário e a validação da gravação, sem perpetuar a documentação antiga que ainda descreve fluxo simulado. Tags, limites por veículo e conta de água deixam de compor a interface desta versão; migração de dados legados, nuvem, aprendizado de máquina e controle de válvulas ficam fora do escopo.
