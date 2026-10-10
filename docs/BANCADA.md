# Bancada de presença — execução física pendente

Este roteiro não constitui evidência de testes físicos. Confirme a placa e o módulo usados antes da montagem.

## Montagem

Para verificar comunicação e leituras pelo USB, consulte [console serial do sensor](CONSOLE-SENSOR.md).

ESP32: SDA GPIO21, SCL GPIO22; RTC DS3231 no barramento existente. VL53L0X ou VL53L1X, reconhecido automaticamente, no endereço padrão 0x29, RTC 0x68. Para a placa adaptadora Pololu compatível, alimente VIN com 3,3 V e use GND comum. Outros módulos exigem conferir regulador, conversão de nível e pinagem próprios; não ligar o sensor nu como se fosse a placa adaptadora. Pull-ups do barramento devem usar nível compatível com 3,3 V. LEDs com resistores: vermelho GPIO23, amarelo GPIO18 e verde GPIO19; saída alta acende. Confirme restrições de boot da placa real. Buzzer GPIO26 permanece inativo.

Drivers Pololu VL53L0X/VL53L1X 1.3.1: orçamento 50 ms, período contínuo 100 ms, timeout de inicialização 50 ms e Wire 20 ms por transação. A aquisição consulta a interrupção de resultado antes de ler os registradores, sem aguardar uma nova medição. Expiração de amostras em 500 ms. No VL53L0X, status bruto 11 com distância maior que zero e diferente de 65535 indica distância válida. No VL53L1X, status traduzido 0 indica distância válida e status 2 é ausência operacional sem distância válida. Em ambos, transporte completo e confirmação são obrigatórios; falhas prevalecem sobre a classificação. Falhas de leitura ou de confirmação da interrupção são desconhecidas e permitem reinicialização do sensor.

O VL53L0X não usa a opção `Long` do VL53L1X. A biblioteca Pololu não calcula o teste de sigma do driver completo da ST; o firmware verifica o status de hardware, mas a precisão e o alcance na montagem real precisam ser medidos. No VL53L0X, o box vazio precisa produzir distância válida acima do limiar; ausência de retorno óptico permanece desconhecida. No VL53L1X, status 2 íntegro também inicia ausência operacional, sem comprovar fisicamente baia vazia. Não assuma que a distância útil do sensor anterior é mantida.

Fontes: [driver](https://github.com/pololu/vl53l1x-arduino/tree/1.3.1), [placa adaptadora](https://www.pololu.com/product/3415). Alcance depende da superfície, luz, geometria e instalação; 4000 mm é apenas o teto configurável.

## Procedimentos e resultados esperados

1. Ligar: inicia em IDLE/vermelho. Com veículo presente, inicia candidata e pode validar após a permanência mínima, inclusive depois de reinício.
2. Aproximar veículo dentro de 1500 mm: amarelo. Manter por 120 s: verde, uma contagem observada; após gravação, total persistido também sobe. Permanecer outros 120 s: nenhum incremento adicional.
3. Antes de validar, afastar brevemente e retornar: permanência mínima reinicia. Após validar, ausência menor que 10 s conserva verde e a contagem.
4. Manter distância acima do limiar por 10 s: vermelho e encerramento sob o mesmo ID. Repetir chegada: novo ID elegível a uma lavagem. Saída antes de validar não aumenta total.
5. Interromper comunicação/sinal: cor atual pisca, painel mostra falha, nenhuma validação ou saída fictícia. Restaurar: sequência de presença/ausência começa novamente; sessão já validada não é contada de novo.
6. Alterar parâmetros durante sessão: apenas a próxima usa novos valores. Reiniciar e conferir valores. Verificar limite de distância e ajustar conforme o box, incluindo superfícies escuras e luz ambiente.
7. Remover/invalidar RTC: total continua, diário fica indisponível e horários desconhecidos são null. Ajustar RTC durante sessão: duração monotônica preservada e clockAdjusted no registro.
8. Em dispositivo de teste com backup, interromper energia após validação persistida: histórico recuperado, sessão interrompida, sem horário de saída inventado. Veículo presente não é recontado até liberar o box.
9. Simular flash cheia/indisponível preferencialmente pelos testes nativos: nenhuma formatação, falha e fila visíveis. Ao restaurar escrita, um único registro consolidado por sessão. Não provocar perda no equipamento de produção.
10. Acessar painel pelo celular sem internet, conferir cores, configuração, relógio, histórico e tentativas sem autenticação. Consultar histórico longo enquanto sensor opera e medir latência real de aquisição, incluindo falhas I2C.

Registrar placa, módulo, posição, ambiente, parâmetros, horários, leituras e resultado de cada item. Nenhum upload ou ensaio físico foi executado automaticamente nesta mudança.

## LEDs com resistor embutido — pinagem atualizada em 02/10/2026

Indicadores compatíveis com 3,3 V e baixa corrente: positivo no GPIO23 (vermelho), GPIO18 (amarelo) ou GPIO19 (verde); negativo no GND comum. O resistor interno substitui o resistor externo somente quando dimensionado para essa alimentação. Indicadores de 5 V/12 V ou módulos de três terminais exigem conferir o circuito antes de ligar; o firmware usa saída ativa em nível alto. GPIO5 não é mais usado pelo semáforo.

Sem veículo: vermelho fixo. Presença candidata: amarelo fixo. Lavagem validada: verde fixo até a saída confirmada. Falha do sensor pisca a cor da sessão, 500 ms acesa e 500 ms apagada, sem bloquear a aquisição. Testar as três cores, ausência breve, saída confirmada e falha após validação.

