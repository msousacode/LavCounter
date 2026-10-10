# Detecção por presença

Ao ligar ou reiniciar: IDLE (vermelho), pronto para detectar. Uma presença válida inicia CANDIDATE (amarelo). Presença contínua pelo tempo mínimo valida uma lavagem e muda para VALID_WASH (verde). Ausência contínua pelo mínimo encerra a sessão e retorna a IDLE/vermelho. WAIT_CLEAR permanece apenas como estado legado, fora do fluxo normal.

| Parâmetro | Padrão | Limites |
| --- | --- | --- |
| presenceMaxMm | 1500 mm (1,5 m) | 100–4000 mm |
| washMinSeconds | 120 s | 1–86400 s |
| absenceMinSeconds | 10 s | 1–3600 s |

Parâmetros salvos são respeitados; cada sessão usa o snapshot capturado ao iniciar. Distância válida menor ou igual ao limiar é presença (1500 mm inclusive); acima é ausência. No VL53L1X, medição completa e recente com status 2 também é ausência operacional, sem distância confiável. Ela pode se alternar com distância válida acima do limiar sem reiniciar a ausência. Essa regra foi escolhida para baia sem fundo mensurável; sinal insuficiente também pode ocorrer com um veículo pouco refletivo e não comprova fisicamente baia vazia.

Status 7 e demais resultados não aceitos, erro I²C, confirmação falha, timeout ou amostra expirada são desconhecidos. Desconhecido reinicia os dois intervalos contínuos, sem encerrar nem desvalidar a sessão. Ausência interrompe a permanência de uma candidata; retorno antes do mínimo de ausência conserva a sessão, mas exige uma nova permanência integral. Lacunas maiores que 500 ms não são creditadas. Limites de validação/saída são inclusivos e medidos pelo relógio monotônico, independente do RTC.

Cada sessão valida e soma exatamente uma vez. O encerramento usa o mesmo ID e não soma novamente. Uma candidata que sai antes de validar é encerrada sem lavagem. Durante tolerância de saída mantém-se a cor da sessão. Status 2 admissível não provoca piscar de falha; desconhecido pisca a cor correspondente a 1 Hz.

Histórico confirmado sobrevive ao reinício; uma sessão anterior em aberto aparece interrompida, sem saída inventada. Um veículo ainda presente pode iniciar outro ID após o reinício e ser contado novamente depois da nova permanência mínima. Não há deduplicação física entre reinícios.

Presença é uma inferência de lavagem. Dois veículos sem ausência mínima formam uma sessão. Calibre limiar, retorno óptico e tempos no local; o alcance configurável não garante alcance físico. Não há reconhecimento de placa ou dependência de internet.
