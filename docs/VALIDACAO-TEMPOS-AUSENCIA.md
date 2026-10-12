# Tempos de ausência independentes e opções fechadas — 12/10/2026

No amarelo (CANDIDATE), ausência contínua pelo `candidateAbsenceSeconds` encerra a sessão sem lavagem contabilizada e retorna ao vermelho. No verde (VALID_WASH), ausência contínua pelo `absenceMinSeconds` encerra a sessão já contabilizada e retorna ao vermelho, sem nova contagem. Status 2 admissível do VL53L1X continua sendo ausência para ambos. Falhas reais continuam Unknown e interrompem os intervalos; retorno breve exige nova permanência integral da candidata. Toda sessão mantém snapshot dos quatro parâmetros.

| Campo no painel | Opções | Padrão |
| --- | --- | --- |
| Distância máxima de presença (m) | 0,4 m (40 cm), 1 m, 1,5 m | 1,5 m |
| Permanência mínima para lavagem (min) | 1, 2, 3, 4, 5 min | 2 min |
| Ausência mínima para encerrar lavagem (min) | 0,5 min (30 segundos), 1 min, 2 min | 30 s |
| Ausência mínima para invalidar a lavagem (s) | 3 s, 5 s, 30 s, 1 min | 5 s |

Os quatro campos são seletores; o backend exige as mesmas opções, inclusive para requisições diretas. API e eventos continuam usando milímetros e segundos. O novo campo é `candidateAbsenceSeconds`; `absenceMinSeconds` passa a ser exclusivo da lavagem validada.

Configuração antiga de três campos: opções ainda válidas são preservadas, valores incompatíveis usam os padrões do respectivo campo, e o novo campo começa em 5 s. Um aviso pede conferência e salvamento. A adaptação acontece em RAM e não sobrescreve NVS até salvar explicitamente. Histórico, IDs e snapshots antigos são preservados.

## Resultados observados em testes

- `scripts/test-native.ps1`: sete executáveis aprovados. Inclui saída de candidata aos 3 s sem validar; saída de validada aos 30 s; limites exatos; snapshot mesmo após alteração da configuração; opções válidas e rejeição de intermediárias; configuração legada com aviso sem alterar NVS; persistência/reboot após Save; API rejeitando valor 600 mm e campo obrigatório ausente; quatro listas de opções na API; ciclo integrado, cores, uma contagem e status 2. Saída registrada em `build/validation-config-options.log`.
- `npm run test:web`: 15 testes aprovados. Verifica quatro seletores, textos e valores exatos, dois tempos distintos salvos e recarregados, cancelar/restaurar, falha de persistência, RTC, eventos e sincronização. Painel regenerado exclusivamente por `scripts/embed-web.cjs`. Captura: `build/web-config.png`.
- Compilação física: resultado registrado abaixo ao terminar. Nenhum upload executado.

## Bancada pendente

Salvar distância 1,5 m, permanência 1 min, invalidar candidata 3 s e encerrar validada 30 s. Aproximar objeto: amarelo. Retirar antes de 1 min: permanece amarelo até 3 s contínuos de ausência e volta ao vermelho sem contagem. Reintroduzir e manter por 1 min: verde/uma contagem. Retirar: mantém verde até 30 s contínuos de ausência e volta ao vermelho/mesmo ID, sem incremento. Repetir com retorno antes dos limites e falha I²C: nenhum intervalo interrompido deve ser creditado. Alterar tempos durante a sessão: só a próxima usa os novos valores. Verificar `status=2` como ausência em ambos os estados. Restaurar a configuração escolhida ao terminar.

Resultados físicos não foram observados; gravação e ensaio no aparelho continuam pendentes.

Compilação final aprovada em 12/10/2026: flash 1.303.868 / 1.310.720 bytes (99%, margem 6.852 bytes); RAM global 54.912 / 327.680 bytes (16%). Mesmo FQBN, partição e dependências do script. Binário build/firmware-physical/LavCounter.ino.bin, 1304016 bytes, gerado 12/10/2026 08:21:45; SHA-256 4B132FB8659A12E85717DCD0D9C653676DC97C272EC72A90482FE2F66510E1BD. Log build/compile-physical.log. Inclui GPIO25 para o LED vermelho e seletor nativo do RTC. Sem upload.

