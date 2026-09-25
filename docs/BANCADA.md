# Validação de bancada

**Estado: pendente de dispositivo, sensor e RTC reais.** Testes com memória, transporte e I2C simulados não comprovam funcionamento elétrico, comunicação Wi-Fi real ou precisão de calibração.

## Preparação

1. Registrar modelo da placa, flash, porta serial, sensor e RTC; conferir pinagem e níveis de tensão no README.
2. Compilar pelo script, guardar log e obter backup de qualquer firmware/dado existente.
3. Inicializar SPIFFS explicitamente se necessário, sem apagar dados de um dispositivo já utilizado. Gravar firmware somente na placa correta.
4. Conectar ao AP LavCounter, autenticar, verificar Status e ajustar hora local do RTC.
5. Registrar os parâmetros e revisão antes de cada cenário. Salvar ajustes pela web, sem recompilar.

## Casos a executar

| Caso | Ação | Resultado esperado |
| --- | --- | --- |
| Sem LCD | Inicializar somente ESP32, RTC e sensor | Inicialização e web funcionam sem display |
| Pulsos conhecidos | Injetar 225 pulsos válidos | 0,5 L com 450 pulsos/L |
| Lavagem contínua | Simular 180 s de fluxo, 30 L | Lavagem com padrões iniciais |
| Lavagem intermitente | 180 s totais, 90 s fluxo, 15 L e pausa de 90 s | Uma sessão, uma pausa/retomada, lavagem |
| Timeout | Parar até atingir timeout | Encerramento único; espera terminal fora da duração |
| Retomada | Retomar antes do timeout e repetir no limite/após | Mesma sessão antes; nova sessão no limite/após |
| Uso duvidoso | Uso relevante sem evidências combinadas | Uso não identificado, histórico preservado |
| Evento mínimo | Pulso isolado abaixo dos três mínimos | Evento descartado auditável |
| Recalibração | Alterar todos os grupos de campos pela web | Próximas sessões usam a revisão salva; sessão ativa e histórico mantêm anteriores |
| Reinício | Reiniciar após esvaziar fila de gravação | Configuração e eventos persistem; IDs não repetem |
| RTC | Ajustar durante sessão e repetir com RTC desconectado | Durações corretas; ajuste/falha explicitados |
| Web concorrente | Consultar histórico repetidamente durante pulsos conhecidos | Contagem correspondente; investigar qualquer perda sinalizada |
| Autenticação | Acessar cada rota sem senha, com senha errada e correta | 401 nos dois primeiros casos, acesso autorizado no terceiro |
| Falhas de flash | Usar imagem de teste cheia/incompleta após backup | Falha explícita, nenhuma formatação automática; registros íntegros acessíveis |

## Evidências a registrar

Para cada caso: data, operador, versão do firmware, revisão da configuração, pulsos esperados/observados, volume real/medido, horários do RTC, métricas registradas, categoria/motivos, resultado e capturas da web/serial. Para calibração real, medir volume com recipiente de referência e comparar usos rotulados do estabelecimento.

Não marcar tarefas 7.2 e 7.3 como concluídas sem estes resultados. A aprovação em computador cobre lógica e contratos de software; o fluxo elétrico e a precisão real exigem bancada.
