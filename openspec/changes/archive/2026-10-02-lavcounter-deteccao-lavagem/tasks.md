# Tasks

## 1. Base LavCounter e compilação

- [x] 1.1 Copiar fontes necessárias da origem confirmada para firmware/LavCounter, preservando a origem e atribuições de bibliotecas; verificar inventário sem .git/build legados e registrar proveniência.
- [x] 1.2 Configurar compilação Arduino CLI para ESP32 com versões fixadas, bibliotecas e partição documentadas; executar build inicial e salvar comando, resultado e uso de memória.
- [x] 1.3 Centralizar identidade LavCounter e remover biblioteca, objeto, inicialização e chamadas do LCD; manter I2C do RTC e verificar compilação sem LiquidCrystal_I2C, inicialização sem display e mensagens com a nova identidade.

## 2. Aquisição e RTC

- [x] 2.1 Implementar coleta contínua segura de pulsos com filtro configurável e timestamps monotônicos; testar deltas, rejeição de ruído, fronteiras com loop atrasado e overflow sinalizado.
- [x] 2.2 Implementar volume fracionário e vazão por intervalo real; testar 225 pulsos = 0,5 L a 450 pulsos/L e intervalos distintos com vazão equivalente.
- [x] 2.3 Corrigir driver DS3231 e tratamento de qualidade; testar sequência de registradores, datas inválidas, ano bissexto, virada de mês/ano, falha I2C e perda de oscilação.
- [x] 2.4 Documentar pinagem, calibração, resolução e significado dos horários; conferir exemplos contra saídas dos testes e ausência de UTC fictício.

## 3. Configuração e sessões

- [x] 3.1 Criar configuração persistente validada para todos os campos da tela Parametrização, com atualização atômica e snapshot por sessão; testar calibração zero, limites invertidos, timeout inválido, falha ao salvar, reinício e alteração durante sessão sem afetar histórico.
- [x] 3.2 Substituir dependência de tags por IDLE/FLOWING/WAITING_RESUME; testar início automático, pausa longa, retomada, timeout e pulso exatamente na fronteira sem duplicação nem perda.
- [x] 3.3 Acumular tempos, pausas, retomadas e timestamps conforme design; testar duração = fluxo + pausas, exclusão de espera terminal, pulso isolado e relógio ajustado/rollover.
- [x] 3.4 Documentar estados e parâmetros iniciais; validar uma sequência de pulsos conhecida comparando todos os campos esperados.

## 4. Classificação

- [x] 4.1 Implementar descarte combinado e classificador puro com seis evidências, condições obrigatórias e motivos; testar exemplos do design e todas as fronteiras dos limites.
- [x] 4.2 Testar uso acima de quatro minutos sem evidências suficientes, lavagem contínua, lavagem com pausas, volume relevante duvidoso e aquisição degradada; verificar categoria e motivos de cada caso.
- [x] 4.3 Documentar regras, configuração e calibração local, incluindo limites de inferência; verificar que exemplos publicados correspondem aos testes e não apresentam score como probabilidade.

## 5. Histórico

- [x] 5.1 Implementar esquema JSONL de eventos, IDs persistentes e gravação verificada das três categorias; testar reinício, unicidade, metadados e round-trip de todos os campos.
- [x] 5.2 Implementar diagnóstico e tentativas limitadas em falha, sem formatação automática; simular escrita parcial, falta de espaço e reenvio, verificando preservação de histórico e ausência de duplicatas.
- [x] 5.3 Implementar leitura incremental paginada e filtrada, tolerando linha incompleta; testar mais de 50 registros, cursor, filtros e indicação de degradação.
- [x] 5.4 Documentar esquema, limites, backup e perda possível de sessão ativa em corte de energia; conferir exemplos JSON e mensagens de falha com saídas reais.

## 6. Web embarcada

- [x] 6.1 Adaptar frontend existente para Status, Eventos, Parametrização e Relógio com identidade LavCounter; verificar offline, categorias, motivos, unidades, estado vazio e estados de erro.
- [x] 6.2 Implementar APIs de status, sessões, configuração e RTC com validação e autenticação comum lavcounter/lav123; testar credenciais corretas/incorretas e acesso direto a cada rota sem autenticação, sem efeitos colaterais.
- [ ] 6.3 Configurar SSID LavCounter e mDNS lavcounter, documentando senha do AP lavcounter123 separadamente da senha HTTP; verificar acesso local e formulários de configuração/RTC.
- [x] 6.4 Remover navegação e rotas legadas de tags, conta de água e exclusão em massa; verificar ausência de funções antigas acessíveis e documentação coerente.

- [x] 6.5 Criar formulário Parametrização com todos os campos e grupos do design, unidades, ajuda, valores atuais, Salvar, Cancelar e restauração confirmada; verificar preenchimento inicial, validação por campo, falha de persistência e recarga dos valores salvos.
- [x] 6.6 Validar calibração integral pela web: alterar timeout, mínimos, vazão, pausas/retomadas e pontuação sem recompilar; verificar efeito nas próximas sessões, preservação da sessão ativa/histórico e persistência após reinício, documentando o procedimento na ajuda.

## 7. Integração e aceitação

- [x] 7.1 Executar suíte completa e compilar firmware final para ESP32 com comando documentado; registrar versões, resultado e ocupação de flash/RAM, sem tratar build como prova de funcionamento físico.
- [ ] 7.2 Executar roteiro de bancada sem LCD, com RTC e sensor, incluindo pausas abaixo/acima do timeout, leituras web simultâneas, reinício após gravação e cada categoria; registrar pulsos esperados/observados, volume, horários e classificações.
- [ ] 7.3 Revisar os critérios de aceitação contra evidências e registrar limitações de calibração e hardware; considerar concluída a validação física somente com resultados de bancada disponíveis.
