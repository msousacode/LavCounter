# Painel local

## Purpose

Permitir operar o contador e consultar suas métricas básicas pela rede local, sem depender de internet ou serviços premium.

## ADDED Requirements

### Requirement: Operação local autenticada
O sistema SHALL manter painel e APIs autenticados funcionando sem internet, permitindo consultar estado, distância e saúde do sensor, semáforo, configuração, RTC e histórico paginado. Entradas inválidas SHALL retornar erro claro sem modificar dados.

#### Scenario: Operação offline
- **WHEN** o operador autenticado acessa o dispositivo sem internet
- **THEN** ele pode visualizar estado, editar parâmetros, ajustar RTC e consultar sessões

#### Scenario: Acesso sem autenticação
- **WHEN** um cliente consulta o painel ou uma API sem credenciais válidas
- **THEN** o servidor exige autenticação antes de expor os dados

### Requirement: Métricas básicas de presença
O painel SHALL mostrar total de lavagens observadas, total persistido, lavagens do dia quando o RTC estiver válido, sessão atual e histórico de sessões com duração e horários. SHALL indicar dados locais, contagens sem data, fila, perdas e falhas. SHALL remover campos e cálculos de pulsos, litros, volume, vazão, pontuação hidráulica e custo por m³ da operação e dos novos contratos.

#### Scenario: Sessão validada pendente de gravação
- **WHEN** a sessão é validada e aguarda persistência
- **THEN** o painel a inclui na contagem observada e distingue a pendência do total persistido, sem duplicá-la após confirmação

#### Scenario: RTC indisponível
- **WHEN** não há data atual confiável
- **THEN** o painel indica contagem diária indisponível e mantém total e diagnóstico acessíveis

### Requirement: Separação da futura oferta SaaS
O sistema SHALL manter coleta e métricas locais independentes da futura camada premium. Esta versão SHALL não exigir assinatura, conexão com nuvem ou cálculos financeiros para operar; qualquer indicação de funcionalidade futura SHALL ser explicitamente indisponível.

#### Scenario: Uso sem serviço premium
- **WHEN** o dispositivo opera sem conta SaaS
- **THEN** todas as funções básicas locais permanecem disponíveis e nenhuma métrica financeira simulada é apresentada
