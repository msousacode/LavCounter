# Design

## Context

Ver proposal.md para motivação. Origem confirmada: C:\WashControl_MVP_HelloDisplay; código em sua subpasta WashControl_MVP_HelloDisplay. O destino contém somente estrutura OpenSpec. Não há testes ou manifesto de compilação no código inspecionado.

O sketch de origem usa Arduino ESP32, WebServer, SoftAP, SPIFFS, LCD 16x2 (0x27), fluxo em GPIO27 com interrupção FALLING e calibração 450 pulsos/L, RTC identificado no código como DS3231 (0x68), I2C em GPIO21/22. StateMachine exige tag OneWire e habilita o sensor apenas na sessão; volume é truncado para inteiro e encerramento depende de reapresentação de tag. O frontend inline já oferece histórico e ajuste de RTC; não há autenticação HTTP. A documentação descreve simulação antiga, divergindo do código atual.

Na leitura RTC, o código atribui o registrador de dia da semana ao dia do mês e desloca os campos seguintes; a conversão civil também merece testes de calendário. Storage usa SPIFFS.begin(true), lê o arquivo todo e compara retorno unsigned de print com zero negativo, sem detectar escrita parcial.

## Goals / Non-Goals

**Goals:** módulos pequenos, coleta contínua, regras determinísticas testáveis sem ESP32, configuração persistente e resultado explicável. Manter os drivers e o estilo visual úteis.

**Non-Goals:** RTOS adicional, inferência estatística, identificação de veículo, controle de válvula, sincronização em nuvem, recuperação exata de sessão ativa após corte de energia ou importação automática do histórico antigo.

## Decisions

### 1. Estrutura e aquisição

Copiar somente fontes e dependências necessárias para firmware/LavCounter, sem .git nem build da origem. Manter sketch como composição; separar FlowMeter, RtcClock, SessionDetector/StateMachine, EventClassifier, DetectionConfig, Storage e handlers web. Usar funções puras para classificar e clock/entrada injetáveis nos testes, sem criar framework de plugins. Tags e alertas por limite de veículo saem do ciclo obrigatório; a nova versão não tem LCD: remover biblioteca LiquidCrystal_I2C, objeto, inicialização e chamadas de display, mantendo I2C para o RTC. Estado e volume aparecem na web; LEDs indicam estado, buzzer permanece inativo sem regra de alerta definida.

ISR permanece curta: contagem e timestamps monotônicos protegidos por seção crítica. Monitorar desde a inicialização; não zerar contador global a cada sessão. Aplicar filtro de intervalo mínimo configurável, inicialmente 1 ms, documentando limite de vazão resultante. Consumir deltas em janelas de 1 s, usando tempo real decorrido para vazão. Pulsos rejeitados pelo filtro elétrico são diagnóstico; eventos compostos de pulsos válidos podem resultar em descarte registrado. Não usar polling do pino nem conversão para litros inteiros.

### 2. Estados e semântica temporal

IDLE -> FLOWING no primeiro pulso válido; FLOWING -> WAITING_RESUME após gap de 3 s; retomada antes de 180 s desde o último pulso volta a FLOWING; timeout finaliza e retorna a IDLE sem bloqueio de cinco segundos. Defaults são experimentais. Na fronteira, comparar timestamp do pulso com o prazo: pulso no prazo ou posterior pertence ao próximo evento, mesmo que o loop o processe atrasado.

Guardar primeiro e último pulso, instante de fechamento e segmentos de fluxo. Tempo efetivo é soma dos segmentos do primeiro ao último pulso de cada trecho; gap >= pauseGapMs separa trechos. Um pulso isolado tem tempo efetivo zero. Pausa interna é intervalo entre último pulso do trecho anterior e primeiro da retomada. Contar pausa apenas quando retomada confirma intervalo interno; espera terminal não incrementa pausas nem duração. Assim, durationMs = flowMs + pauseMs; avgFlowLpm = volumeL * 60000 / flowMs quando flowMs > 0, senão nulo. Medição contínua exibida por janelas não deve alterar essas definições. Buffer limitado de transições preserva fronteiras; overflow gera quality flag e impede classificação automática como lavagem.

Registrar startedAt, finishedAt (último fluxo) e closedAt (timeout), além de duração monotônica. Capturar calendário próximo aos pulsos; qualquer projeção do relógio RTC para timestamp de pulso usa âncora RTC/monotônica e qualidade explícita. Corrigir leitura/escrita dos sete registradores, validação BCD/calendário, modo de horas e indicador de perda de oscilação. Manter RTC em horário local do estabelecimento, gravando texto ISO local sem sufixo UTC e timeBasis=local-rtc. Ajustes durante sessão geram clockAdjusted; leituras inválidas geram null, nunca epoch zero como data válida. Usar relógio monotônico de 64 bits ou tratamento explícito de rollover. Alternativa descartada: usar diferença do RTC para timeout, pois ajustes alteram o resultado.

### 3. Classificador simples e auditável

EventClassifier recebe evento encerrado e snapshot de configuração. Primeiro descartar somente se duration < 5 s E volume < 0,2 L E flow < 2 s. Igualdade ao limite passa o mínimo. Depois calcular seis evidências booleanas de peso 1:

| Evidência | Valor inicial |
| --- | --- |
| Duração | >= 120 s |
| Tempo com fluxo | >= 60 s |
| Volume | >= 10 L |
| Vazão durante fluxo | 2 a 25 L/min, inclusive |
| Pausas | ao menos 1 e maior pausa interna <= 120 s |
| Retomadas | ao menos 1 e proporção flow/duration >= 0,20 |

Lavagem exige evidências de volume, tempo com fluxo e vazão, mais pontuação >= 4 de 6. Assim um uso contínuo pode ser lavagem por duração, e pausas/retomadas contribuem sem serem obrigatórias. Caso contrário: uso não identificado. Qualidade de aquisição degradada força uso não identificado salvo descarte mínimo; RTC inválido sozinho não invalida métricas. Não chamar pontuação de probabilidade. Registrar cada evidência, motivos de falha, score, classifierVersion e snapshot da configuração. Isso atende múltiplos critérios sem modelo treinado; não garante distinguir usos fisicamente semelhantes.

Exemplos iniciais: 180 s de fluxo contínuo, 30 L, 10 L/min => lavagem; 300 s totais com apenas 10 s de fluxo e 1 L => uso não identificado; pulso isolado abaixo dos três mínimos => descartado; 180 s totais, 90 s de fluxo, 15 L, pausa de 90 s e retomada => lavagem. Testes devem variar fronteiras e reduzir timeout para execução de bancada.

### 4. Configuração, persistência e consulta

Usar Preferences/NVS para configuração validada com revisão e valores padrão. Snapshot no primeiro pulso: mudanças de calibração, filtros, gaps e classificação só entram após encerrar sessão. Validar valores finitos positivos, score entre 4 e 6, limites ordenados, pauseGap < inactivityTimeout e intervalos operacionais documentados. Todos os limiares são editáveis pela tela Parametrização sem recompilar. UI permite restaurar defaults mediante ação explícita. Atualização atômica: sucesso somente após persistência; falha mantém a configuração anterior. Não reclassificar histórico retroativamente.

Manter JSONL em novo /events-v1.jsonl. Registro inclui schemaVersion, firmwareVersion, id monotônico persistente reservado antes do uso, timestamps/qualidade, pulses, volumeL, durationMs, flowMs, pauseMs, pauseCount, resumeCount, maxPauseMs, avgFlowLpm, category, score, reasons e configuração. Categorias de API: washing, unidentified, discarded; tradução explícita no frontend. Persistir também descartes para auditoria. Sem importar /sessions.jsonl automaticamente.

Confirmar quantidade total de bytes escritos, fechar arquivo e expor erro. Não formatar automaticamente; manter evento pendente para nova tentativa com mesmo id e diagnóstico quando fila limitada esgotar. Deduplicar reenvio por id e tolerar última linha incompleta. Preservar registros existentes quando cheio; exibir aviso de capacidade. Sem rotação silenciosa nesta versão. Leitura incremental e limite de 50 registros por resposta; um índice simples por offsets pode evitar varreduras repetidas sem banco de dados.

### 5. Web local e identidade

Tela Parametrização com quatro grupos de entradas numéricas:

| Grupo | Campos editáveis e valores iniciais |
| --- | --- |
| Sessão e pausas | Reconhecer pausa: 3 s; encerrar por inatividade: 180 s |
| Eventos mínimos | Duração: 5 s; tempo com fluxo: 2 s; volume: 0,2 L (descarte somente abaixo dos três) |
| Identificação de lavagem | Duração: 120 s; fluxo: 60 s; volume: 10 L; vazão: 2 a 25 L/min; mínimo de pausas: 1; maior pausa interna: 120 s; mínimo de retomadas: 1; proporção de fluxo: 20%; pontuação: 4 |
| Sensor e ajustes avançados | Calibração: 450 pulsos/L; intervalo mínimo entre pulsos: 1 ms; janela de vazão: 1 s |

Usar rótulos claros, ajuda curta, unidades visíveis e limites publicados pelo GET /api/config, para manter validação do formulário e servidor coerentes. Campos de contagem são inteiros >= 1; proporção > 0 e <= 100%; pontuação inteira de 4 a 6. Os demais limites devem respeitar capacidade numérica e relação entre tempos, sem bloquear calibrações úteis por estabelecimento. Converter segundos/percentuais da tela para as unidades internas explicitamente.

O timeout significa tempo sem água para encerrar e avaliar o evento. Não equivale a declarar que não houve lavagem. Tempo mínimo de lavagem é outro campo, combinado com volume, fluxo e demais evidências. Mostrar essa distinção na ajuda do formulário.

Rodapé com Salvar, Cancelar alterações e Restaurar padrões. Cancelar recarrega valores persistidos; restaurar exige confirmação e passa pela mesma validação/persistência de Salvar. GET retorna valores, revisão, defaults e limites; POST valida o conjunto completo e retorna revisão confirmada ou erros por campo. A tela mostra revisão salva e aviso de aplicação na próxima sessão; não expõe nomes internos da API ao usuário.

Reutilizar CSS/HTML existente com seções Status, Eventos, Parametrização e Relógio. HTML/CSS/JS locais, sem CDN. GET /api/status, GET /api/sessions?category=&cursor=&limit=, GET/POST /api/config e GET/POST /api/rtc. Remover registro das rotas de tags e exclusão em massa legadas. Retornar JSON estruturado, filtros validados, erros visíveis e campos textuais escapados; status atual pode ser atualizado a cada 2 s. Paginação deve ceder processamento à aquisição.

Centralizar HTTP Basic Auth em todos os handlers: lavcounter / lav123. SSID LavCounter, mDNS lavcounter. Premissa de implantação: senha do AP lavcounter123, separada da senha web solicitada lav123; documentar ambas. A autenticação web não depende de cadastrar usuário no Wi-Fi. Reutilizar WebServer reduz dependências; não introduzir login com tokens nesta versão.

## Risks / Trade-offs

- Uso não relacionado com mesmas características de lavagem → regras auditáveis, categoria duvidosa e calibração com dados rotulados; nenhuma promessa de acurácia em campo.
- Pausa maior que timeout divide lavagem; timeout excessivo une clientes → configurar por estabelecimento e testar ambas as fronteiras.
- Ruído e vazão muito baixa → filtro elétrico, volume fracionário e descarte combinado; validar pulsos reais em bancada.
- Hardware presumido pelo código pode diferir da bancada → documentar DS3231/YF-S201 e pinagem como base, confirmar antes de ligar hardware.
- Memória/flash limitadas e queda de energia → paginação, limites explícitos, escrita verificada e aviso de falha; recuperação exata de sessão ativa fica fora da versão.
- HTTP Basic sobre AP local não fornece TLS → limitar o desenho ao uso local solicitado e documentar credenciais; não expor serviço pela internet.

## Migration Plan

1. Criar firmware/LavCounter a partir da origem preservada e registrar proveniência em documentação técnica; nomes legados só aparecem nessa referência histórica.
2. Fixar versão do core ESP32 e bibliotecas na implementação e fornecer comando Arduino CLI com FQBN esp32:esp32:esp32 como alvo inicial, conforme plataforma Arduino observada. Registrar opções de flash/partição efetivamente compiladas.
3. Executar testes de lógica, compilar e registrar tamanho de flash/RAM; só então validar RTC, fluxo e web em ESP32 de bancada.
4. Exportar/backupear dados de dispositivo legado antes de gravar nova imagem; não migrar dados antigos implicitamente. Rollback usa firmware e backup antigos, mantendo a origem intacta.

## Open Questions

- Os valores de detecção e a calibração devem ser refinados com amostras reais do estabelecimento; isso altera parâmetros, não a arquitetura.
- A placa exata e capacidade de flash precisam ser conferidas antes do upload físico; a compilação inicial usa ESP32 genérico e pinagem da origem.
