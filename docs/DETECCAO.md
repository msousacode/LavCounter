# Detecção por presença

Ao ligar ou reiniciar: IDLE (vermelho), pronto para detectar. Uma presença válida inicia CANDIDATE (amarelo). Presença contínua pelo tempo mínimo valida uma lavagem e muda para VALID_WASH (verde). Ausência contínua pelo mínimo encerra a sessão e retorna a IDLE/vermelho. WAIT_CLEAR permanece apenas como estado legado, fora do fluxo normal.

| Parâmetro | Padrão | Opções permitidas |
| --- | --- | --- |
| presenceMaxMm | 1500 mm (1,5 m) | 400, 1000, 1500 mm (painel: 0,4 / 1 / 1,5 m) |
| washMinSeconds | 120 s | 60, 120, 180, 240, 300 s (painel: 1–5 min) |
| absenceMinSeconds | 30 s | 30, 60, 120 s (painel: 0,5 / 1 / 2 min) |
| candidateAbsenceSeconds | 5 s | 3, 5, 30, 60 s (painel: 3 s / 5 s / 30 s / 1 min) |

No amarelo, `candidateAbsenceSeconds` encerra a candidata sem contabilizar. No verde, `absenceMinSeconds` encerra a lavagem já validada sem acrescentar outra contagem. Ambos retornam a IDLE/vermelho. O tempo de ausência é contínuo: retorno do objeto ou falha interrompe o intervalo. Os dois tempos, limiar e permanência são capturados no snapshot da sessão.

O painel usa seletores fechados, e a API rejeita valores intermediários e campos ausentes. Na leitura da antiga configuração de três campos, valores ainda permitidos são preservados; incompatíveis usam o padrão do campo, e o novo tempo de candidata usa 5 s. O painel mostra um aviso para conferir e salvar. A NVS antiga só é substituída ao salvar; histórico e IDs não são alterados.

Parâmetros salvos são respeitados; cada sessão usa o snapshot capturado ao iniciar. Distância válida menor ou igual ao limiar é presença (1500 mm inclusive); acima é ausência. No VL53L1X, medição completa e recente com status 2 também é ausência operacional, sem distância confiável. Ela pode se alternar com distância válida acima do limiar sem reiniciar a ausência. Essa regra foi escolhida para baia sem fundo mensurável; sinal insuficiente também pode ocorrer com um veículo pouco refletivo e não comprova fisicamente baia vazia.

Status 7 e demais resultados não aceitos, erro I²C, confirmação falha, timeout ou amostra expirada são desconhecidos. Na candidata, desconhecido reinicia os intervalos. No verde, somente status 7 óptico do VL53L1X, com aquisição íntegra e razão invalid_range, pausa o encerramento preservando a ausência já confirmada. Esse status não inicia ausência nem acrescenta tempo; a contagem retoma com novas amostras ausentes. O encerramento requer o tempo configurado de ausência confirmada, podendo levar mais tempo de relógio devido às pausas. Retorno de presença válida, demais falhas ou lacunas maiores que 500 ms zeram a contagem de saída, sem desvalidar a lavagem. Não se usa a distância bruta inválida para decidir ausência. Ausência interrompe a permanência de uma candidata; retorno antes do mínimo de ausência conserva a sessão, mas exige uma nova permanência integral. Limites de validação/saída são inclusivos e medidos pelo relógio monotônico, independente do RTC.

Cada sessão valida e soma exatamente uma vez. O encerramento usa o mesmo ID e não soma novamente. Uma candidata que sai antes de validar é encerrada sem lavagem. Durante tolerância de saída mantém-se a cor da sessão. Status 2 admissível não provoca piscar de falha; desconhecido pisca a cor correspondente a 1 Hz.

Histórico confirmado sobrevive ao reinício; uma sessão anterior em aberto aparece interrompida, sem saída inventada. Um veículo ainda presente pode iniciar outro ID após o reinício e ser contado novamente depois da nova permanência mínima. Não há deduplicação física entre reinícios.

Presença é uma inferência de lavagem. Dois veículos sem ausência mínima formam uma sessão. Calibre limiar, retorno óptico e tempos no local; o alcance configurável não garante alcance físico. Não há reconhecimento de placa ou dependência de internet.
