# Saída de lavagem com status 7 intercalado — 12/10/2026

O operador relatou lavagem validada que não encerrava após a saída. O trecho fornecido contém uma amostra VL53L1X status 7, desconhecida, seguida de distâncias válidas de aproximadamente 2,2 m classificadas como ausência. O limiar da sessão no log é 1000 mm. Dez linhas periódicas representam aproximadamente nove segundos e, isoladamente, não comprovam uma sequência de 30 s; porém revelam o caso que reiniciava a contagem: qualquer Unknown apagava a ausência confirmada. Como o console imprime a cada segundo e a aquisição ocorre a cada 100 ms, podem existir outras amostras inválidas entre as linhas.

## Correção

Somente após VALID_WASH, uma leitura óptica status 7 do VL53L1X, íntegra e com razão `invalid_range`, pausa o encerramento. O tempo de ausência confirmado anteriormente é preservado. Status 7 continua Unknown, não inicia ausência nem acrescenta tempo. A contagem soma apenas intervalos delimitados por duas amostras ausentes consecutivas; a retomada depois da pausa exige nova confirmação. Assim, 30 s configurados podem levar mais de 30 s de relógio quando houver pausas.

Presença válida dentro do limiar zera a saída. Falha I²C, confirmação falha, timeout, amostra expirada, lacuna maior que 500 ms e outros status inválidos continuam zerando a contagem. Um sensor permanentemente inválido não pode encerrar sozinho. Distância inválida de status 7 não é reinterpretada como ausência. Status 2 íntegro e distância válida acima do limiar continuam sendo ausência. A candidata permanece exigindo continuidade integral para validar e mantém seu tempo separado de invalidação.

## Evidências

`scripts/test-native.ps1`: sete executáveis passaram. O núcleo verifica pausa prolongada sem crédito de tempo desconhecido, várias pausas intercaladas, retomada exata, retorno do veículo zerando saída, falha de transporte e lacuna zerando saída e candidata sem crédito de tempo inválido. O sketch real com fakes verifica presença/validação, distância ausente, status 7, retomada por status 2 e encerramento após 30 s confirmados com mesmo ID, uma contagem e LED vermelho GPIO25. Os testes existentes de API, configuração, RTC, persistência e sincronização também passaram. Log: `build/validation-exit-optical-pause.log`.

Nenhuma mudança no painel; `WebPage.h` é regenerado pelo script de build. Compilação e identificação do binário registradas abaixo após término.

## Ensaio físico pendente

Gravar o novo firmware sem apagar NVS/histórico. Antes de iniciar outra sessão, salvar limiar desejado (o log anterior usa 1 m), permanência e saída de 30 s. Validar com veículo presente, retirar e registrar status/ocupação/estado até encerrar. Confirmar distância válida acima do limite ou status 2 como ausência. Inserir erros ópticos status 7: não reiniciam o tempo já confirmado, mas não somam tempo. Reintroduzir veículo antes de terminar: saída zera. Desconectar o sensor: transporte falho não pode confirmar saída. Ao encerrar, conferir vermelho, mesmo ID e contagem única.

Upload e teste físico não foram executados nesta revisão.

Compilação final aprovada em 12/10/2026: flash 1.304.048 / 1.310.720 bytes (99%, margem 6.672 bytes); RAM global 54.920 / 327.680 bytes (16%). Mesma partição e dependências. Binário build/firmware-physical/LavCounter.ino.bin, 1304192 bytes, gerado 12/10/2026 09:42:26; SHA-256 B2CEA725260623F5C4F63EB7B31B1A068FFDA606268861FFDBB2849CB64D4B99. Log build/compile-physical.log. Inclui login lav123/lav123. Nenhum upload executado.

