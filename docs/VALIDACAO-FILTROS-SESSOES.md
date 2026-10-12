# Filtros de sessões registradas

Implementação em 12/10/2026. O painel combina resultado (Válidas, Não Válidas, Ambos) e período (Dia atual, Últimos 7 dias, Últimos 15 dias, Mês atual). O padrão é Válidas / Últimos 7 dias, com as sessões mais recentes primeiro.

A consulta percorre todas as páginas da API antes de aplicar filtros e ordenação no navegador. Os períodos usam o calendário do RTC e incluem hoje. A data de início é a referência, com validação e encerramento como alternativas. Ausência de data confiável é sinalizada. A lista apresenta 20 sessões por vez; mudanças nos filtros usam o histórico já consultado. Atualizar histórico mantém os filtros.

## Verificações

- `npm run test:web`: 20 testes aprovados, incluindo cinco cenários dos filtros.
- `scripts/test-native.ps1`: aprovado; presença, temporizadores, contagem, configuração, armazenamento, autenticação, nuvem e integração preservados.
- Filtros combinados, limites de sete/quinze dias e mês, exclusão de datas futuras, histórico com várias páginas, IDs acima de 2^53, paginação visual, datas alternativas, RTC inválido, lista vazia e erro HTTP verificados.
- Layout conferido em desktop e celular (390 px), sem transbordamento da página; tabela com rolagem horizontal.
- `scripts/build.ps1`: compilação ESP32 aprovada; flash 1.307.648 / 1.310.720 bytes, RAM 54.920 / 327.680 bytes. Página incorporada em `firmware/LavCounter/WebPage.h`; binário em `build/firmware-physical/LavCounter.ino.bin`.

Capturas: `build/web-events-filters.png` e `build/web-events-filters-mobile.png`.

As regras do sensor, máquina de estados, contagem e API não foram alteradas. A instalação e o teste do painel em uma ESP32 física ainda precisam ser realizados.
