# Simulação de sensor

## Purpose

Permitir validar na ESP32 o ciclo completo de lavagem por comandos locais de presença, ausência e falha, sem conectar o VL53L1X.

## ADDED Requirements

### Requirement: Ativação explícita do mock
O firmware SHALL usar aquisição física por padrão e permitir selecionar mock explicitamente na compilação. A falta do sensor físico SHALL não ativar simulação automaticamente. O modo SHALL permanecer fixo durante a execução.

#### Scenario: Firmware físico sem sensor
- **WHEN** o firmware padrão inicia sem VL53L1X conectado
- **THEN** informa falha de aquisição e não disponibiliza comandos de simulação

#### Scenario: Inicialização do mock
- **WHEN** o firmware compilado para mock inicia
- **THEN** independe do VL53L1X, assume box vazio e aguarda ausência pelo tempo configurado antes de habilitar novas sessões

### Requirement: Comandos autenticados de entrada simulada
O sistema SHALL aceitar somente presença, ausência ou falha por API local autenticada no modo mock. O comando SHALL alterar a entrada do sensor, sem escrever diretamente estado da lavagem, contagens ou registros. A entrada SHALL permanecer até novo comando ou reinício, independentemente do navegador. Comandos repetidos SHALL não reiniciar temporizadores nem duplicar sessões. Entradas inválidas SHALL retornar erro sem alteração.

#### Scenario: Comando mantido e repetido
- **WHEN** presença é selecionada, reenviada e o navegador é fechado
- **THEN** a ESP32 continua produzindo presença e mantém a mesma sequência temporal

#### Scenario: Comando inválido ou não autorizado
- **WHEN** um cliente envia valor inválido ou não apresenta credenciais válidas
- **THEN** a API rejeita a solicitação e preserva a entrada anterior

### Requirement: Ciclo simulado com tempo real
O mock SHALL produzir amostras periódicas para a mesma lógica temporal da aquisição física, respeitando parâmetros da sessão, validação única, encerramento por ausência, LEDs e persistência. Falha SHALL produzir leitura desconhecida. O mock SHALL não acelerar relógios nem simular RTC.

#### Scenario: Lavagem completa
- **WHEN** após habilitação inicial o operador seleciona presença pelo mínimo de validação e depois ausência pelo mínimo de saída
- **THEN** ocorre uma única lavagem, os LEDs passam por amarelo, verde e vermelho e o histórico consolida validação e encerramento na mesma sessão

#### Scenario: Falha durante presença
- **WHEN** falha é selecionada antes da validação e depois presença é restaurada
- **THEN** a continuidade anterior é interrompida e uma nova sequência completa é necessária para validar

#### Scenario: Falha após validação
- **WHEN** falha é selecionada em sessão validada
- **THEN** o diagnóstico informa falha simulada, o verde pisca e nenhuma contagem ou saída adicional é produzida

### Requirement: Configuração de simulação isolada
O modo mock SHALL persistir parâmetros e sequência de identificadores de teste separadamente dos físicos. Reinício SHALL restaurar a configuração de teste, reiniciar a entrada como ausência e exigir ausência confirmada, sem inventar encerramento de sessão interrompida.

#### Scenario: Retorno ao firmware físico
- **WHEN** parâmetros são editados no mock e posteriormente o firmware físico é carregado sem apagar armazenamento
- **THEN** a configuração física anterior permanece disponível e não incorpora os parâmetros de teste
