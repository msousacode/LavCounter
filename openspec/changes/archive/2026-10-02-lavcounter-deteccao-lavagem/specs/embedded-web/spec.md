# Spec Delta

## Purpose

Oferecer uma interface local simples para observar e ajustar a detecção de eventos do LavCounter sem dependência de internet.

## ADDED Requirements

### Requirement: Identidade e interface embarcada
O firmware SHALL compilar para ESP32 e exibir LavCounter na interface, inicialização e documentação do produto. A interface SHALL reutilizar e simplificar o frontend existente, funcionar offline pela rede do dispositivo e mostrar estado atual, métricas, histórico com categorias e motivos, qualidade do RTC e falhas de armazenamento. SHALL oferecer configuração de detecção e ajuste do RTC; tags e conta de água não integram esta interface.

#### Scenario: Teste local sem internet
- **WHEN** o usuário acessa o ESP32 pela rede local do dispositivo
- **THEN** a interface LavCounter carrega sem recursos externos e permite consultar eventos e estado de detecção

### Requirement: Autenticação da aplicação
A página e todas as APIs SHALL exigir usuário lavcounter e senha lav123, inclusive consultas, configuração e ajuste do RTC. Credenciais de conexão Wi-Fi SHALL ser documentadas separadamente das credenciais HTTP.

#### Scenario: Credenciais corretas
- **WHEN** o usuário autentica com lavcounter e lav123
- **THEN** obtém acesso à página e às APIs

#### Scenario: Acesso direto não autenticado
- **WHEN** uma página ou API é solicitada sem credenciais válidas
- **THEN** retorna desafio de autenticação sem revelar dados nem executar alterações

### Requirement: Operação sem LCD
O firmware SHALL inicializar e operar sem LCD conectado e sem dependência da biblioteca de display. A visualização de estado, métricas e diagnósticos SHALL estar disponível pela web, mantendo o RTC funcional.

#### Scenario: Dispositivo sem display
- **WHEN** o ESP32 inicia com sensor e RTC, sem LCD
- **THEN** a detecção e a interface web funcionam sem aguardar ou tentar inicializar display

### Requirement: Tela Parametrização
A interface SHALL oferecer formulário autenticado com valores atuais, unidades, limites aceitos e explicação de cada campo. SHALL permitir editar timeout de encerramento por inatividade, intervalo para reconhecer pausa, mínimos de duração/tempo com fluxo/volume para descarte e lavagem, faixa de vazão, critérios de pausas e retomadas, proporção mínima de fluxo, pontuação exigida e calibração do sensor. Parâmetros de filtro de pulsos e intervalo de medição SHALL ficar em seção avançada editável. SHALL incluir Salvar, Cancelar alterações e Restaurar padrões, com confirmação antes de persistir a restauração. Salvar SHALL informar sucesso ou erros por campo, e que mudanças valem para próximas sessões. Valores SHALL sobreviver à atualização da página e reinício do ESP32.

#### Scenario: Calibração pelo usuário
- **WHEN** o usuário altera timeout e volume mínimo e salva valores válidos
- **THEN** a página confirma persistência, exibe os valores salvos após recarregar e o firmware os usa nas próximas sessões sem recompilação

#### Scenario: Erro de preenchimento ou persistência
- **WHEN** há valor inválido ou falha ao persistir
- **THEN** a tela informa o erro sem apresentar sucesso nem substituir a configuração válida anterior

#### Scenario: Cancelamento e restauração
- **WHEN** o usuário cancela edições não salvas ou cancela a confirmação de restauração
- **THEN** os valores persistidos permanecem inalterados
