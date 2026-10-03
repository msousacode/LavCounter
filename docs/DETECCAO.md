# Detecção por presença

Estados: WAIT_CLEAR → IDLE → CANDIDATE → VALID_WASH → IDLE. No boot, somente ausência contínua habilita o contador; um veículo já estacionado não é contado novamente.

| Parâmetro | Padrão | Limites |
| --- | --- | --- |
| presenceMaxMm | 1500 mm | 100–4000 mm |
| washMinSeconds | 120 s | 1–86400 s |
| absenceMinSeconds | 10 s | 1–3600 s |

Distância válida menor ou igual ao limiar significa presença. As faixas são limites de configuração, não garantia de alcance físico. Calibre no box.

Cada sessão usa snapshot da configuração. Presença contínua valida uma vez no limite exato do tempo mínimo. Ausência breve mantém a sessão, mas reinicia o tempo de presença de uma candidata. Ausência contínua encerra no limite mínimo. Leitura inválida reinicia as duas sequências contínuas, sem encerrar nem desvalidar a sessão. Intervalos superiores a 500 ms entre amostras não são creditados.

O RTC fornece datas; tempo monotônico fornece duração. Ajustar o relógio não muda os limiares. Vermelho indica espera, amarelo candidata e verde validada. Durante tolerância de saída, a cor é mantida. Falha do sensor pisca a cor a 1 Hz; WAIT_CLEAR pisca vermelho.

Presença é uma inferência: um veículo parado pode não estar sendo lavado. Dois veículos sem ausência mínima formam uma única sessão. Não há reconhecimento de placa nem dependência de nuvem.
