# Premium controlado pelo Firebase

Em 12/10/2026, a integração Firebase privada passou a manter Premium liberado localmente sem vencimento. O prazo anterior de teste não bloqueia mais a tentativa de sincronização; as regras do Firebase continuam autorizando cada escrita. A configuração remota relatada pelo usuário (até 31/12/2026) não foi consultada nem alterada.

`FirebaseManagedPremium` exige integração habilitada, token válido, identidade persistida e vínculo exato de projeto/banco/dispositivo/modo. O provedor padrão sem integração privada permanece indisponível. `testUntil=0`, `firebaseManagedPremium=true` e `testMode=true` são apresentados pela API autenticada; nenhum parâmetro HTTP libera o modo. O painel informa que a autorização dos envios é controlada pelo Firebase, sem apresentar vencimento local.

## Validação

- Testes nativos aprovados: liberação após o prazo antigo, token vencido bloqueado, vínculos incorretos bloqueados, configuração padrão bloqueada, montagem do sketch e diagnóstico público sem credenciais.
- Regressões nativas aprovadas: sensor, máquina de estados, configuração, armazenamento, renovação Firebase Auth, falhas HTTP, confirmação durável e sincronização.
- `npm run test:web`: 21 testes aprovados, incluindo o aviso de liberação sem prazo local, tratamento de erro remoto e regressões dos filtros de sessões.
- `scripts/build.ps1`: compilação ESP32 aprovada, flash 1.307.692 / 1.310.720 bytes e RAM 54.904 / 327.680 bytes. Firmware gerado em `build/firmware-physical/LavCounter.ino.bin`.

A expiração de tokens Firebase continua ativa e a renovação automática foi preservada. A permissão local não comprova sucesso remoto; uploads recusados não são confirmados. A gravação e o teste com uma ESP32 física e com as regras reais do Firebase ainda precisam ser realizados.
