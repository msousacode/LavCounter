# Detecção e calendário

## Sessões

`IDLE` inicia ao primeiro pulso válido. Ausência de pulsos por 3 s entra em `WAITING_RESUME`. Retomada antes de 180 s desde o último pulso mantém a sessão; no limite ou após, encerra a anterior antes de iniciar outra. Esses valores são editáveis pela web. Não há período bloqueante entre sessões.

Tempos usam microssegundos monotônicos de 64 bits. Duração é do primeiro ao último pulso; a espera terminal não é contabilizada. Um intervalo >= limite de pausa separa trechos: conta pausa e retomada quando chega o próximo pulso. Tempo com fluxo é a soma dos intervalos menores que o limite; tempo pausado é a soma dos demais intervalos internos. `duração = fluxo + pausas`. Um pulso isolado tem duração e tempo com fluxo zero.

Exemplo: pulsos em 1, 2, 3, 93 e 94 segundos → cinco pulsos, duração 93 s, fluxo 3 s, pausa 90 s, uma pausa e uma retomada. Encerramento em 274 s com timeout 180 s. O intervalo terminal de 180 s não altera essas métricas.

Volume = pulsos / calibração. Com 450 pulsos/L, 225 pulsos são 0,5 L. Vazão média = volume × 60 / segundos com fluxo; sem tempo efetivo, o resultado é indisponível, não infinito. Vazão instantânea usa pulsos e tempo real na janela configurada. Contagem é contínua, inclusive durante requisições web. A ISR armazena timestamps em fila de 4096 posições (4095 úteis); o loop filtra por intervalo mínimo usando a configuração da sessão.

Filtro inicial 1 ms rejeita intervalos menores, limitando a aceitação a 1000 pulsos/s (133,33 L/min com 450 pulsos/L). Ajustá-lo exige conhecer o sensor. Pulsos rejeitados são diagnóstico; eventos mínimos formados por pulsos aceitos são registrados como descartados.

## Classificação

Primeiro: descartar somente se duração < 5 s **e** fluxo < 2 s **e** volume < 0,2 L. Igualdade passa o respectivo mínimo. Depois, seis evidências:

| Evidência | Valor inicial |
| --- | --- |
| Duração | >= 120 s |
| Fluxo efetivo | >= 60 s |
| Volume | >= 10 L |
| Vazão durante fluxo | 2 a 25 L/min inclusive |
| Pausas | >= 1, maior pausa <= 120 s |
| Retomadas | >= 1, proporção de fluxo >= 20% |

Lavagem exige volume, fluxo e vazão aprovados **e** pelo menos quatro evidências. Uma lavagem contínua pode pontuar por duração; pausas não são obrigatórias com limiar quatro. Aumentar para cinco/seis favorece padrões intermitentes. Aquisição degradada impede lavagem automática. Demais usos são não identificados, com motivos; RTC inválido sozinho não invalida métricas.

Todos os limites são ajustáveis na tela. A pontuação é contagem de evidências, não probabilidade nem garantia de acurácia. Exemplos: 180 s de fluxo, 30 L e 10 L/min → lavagem; 300 s totais, 10 s de fluxo e 1 L → uso não identificado. Compare previsões com usos reais antes de operar como contador comercial.

## RTC

DS3231 em horário **local do estabelecimento**: textos ISO locais sem `Z` nem conversão UTC. A leitura respeita dia da semana, dia do mês, mês e ano, modos 12/24 h, BCD e flag de oscilação. Ajuste pela tela Relógio valida calendário entre 2000 e 2099.

`startedAt` corresponde ao primeiro pulso, `finishedAt` ao último e `closedAt` ao timeout. São projetados a partir de âncora RTC/monotônica, com precisão do calendário de um segundo. Falha ou horário inválido gera null e qualidade indisponível. Ajustes conhecidos ou saltos detectados >2 s durante a sessão geram `clockAdjusted`; duração e timeout continuam monotônicos. Valores do histórico não são corrigidos retroativamente.
