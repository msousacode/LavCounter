$ErrorActionPreference='Stop'
[Net.ServicePointManager]::SecurityProtocol=[Net.SecurityProtocolType]::Tls12
$projectRoot=Split-Path $PSScriptRoot -Parent
$privatePath=Join-Path $projectRoot 'firmware/LavCounter/CloudPrivate.h'
$credentialsPath=Join-Path $projectRoot 'firmware/LavCounter/CloudCredentials.h'
$utf8=New-Object Text.UTF8Encoding($false)
function Call-Firebase([string]$Uri,[string]$ContentType,[string]$Body) {
 try { return Invoke-RestMethod -Method Post -Uri $Uri -ContentType $ContentType -Body $Body -TimeoutSec 20 }
 catch {
  # Never echo HTTP response bodies, tokens, URLs or request bodies.
  $code='NETWORK_ERROR';$failure=$_
  try {
   $remote=$failure.ErrorDetails.Message | ConvertFrom-Json
   $message=[string]$remote.error.message
   if($message -match '^([A-Z][A-Z0-9_]+)') { $code=$Matches[1] }
  } catch {}
  if($code -eq 'NETWORK_ERROR' -and $failure.Exception.Response) {
   try {
    $reader=New-Object IO.StreamReader($failure.Exception.Response.GetResponseStream())
    $remote=$reader.ReadToEnd() | ConvertFrom-Json;$reader.Close()
    if([string]$remote.error.message -match '^([A-Z][A-Z0-9_]+)') { $code=$Matches[1] }
   } catch {}
  }
  throw "Firebase recusou a preparação: $code. Confira login anônimo e restrições da API key."
 }
}
function Extract-Define([string]$Text,[string]$Name) {
 $m=[regex]::Match($Text,'(?m)^#define '+[regex]::Escape($Name)+' "([^"\r\n]+)"')
 if(-not $m.Success) { throw "Configuração ausente: $Name" }
 return $m.Groups[1].Value
}
function Assert-TestToken([string]$Token,[string]$User) {
 $parts=$Token.Split('.')
 if($parts.Length -ne 3) { throw 'Token Firebase inválido.' }
 $payload=$parts[1].Replace('-','+').Replace('_','/')
 while($payload.Length % 4) { $payload+='=' }
 $claims=[Text.Encoding]::UTF8.GetString([Convert]::FromBase64String($payload)) | ConvertFrom-Json
 if($claims.aud -ne 'lavcounter' -or $claims.sub -ne $User -or $claims.iss -ne 'https://securetoken.google.com/lavcounter' -or $claims.firebase.sign_in_provider -ne 'anonymous') { throw 'Token devolvido pelo endpoint TLS pertence a outro projeto/usuário ou não é anônimo.' }
}
function Save-Credential([string]$Refresh,[string]$User) {
 if($Refresh -notmatch '^[A-Za-z0-9_.~+/=-]{1,4096}$' -or $User -notmatch '^[A-Za-z0-9_-]{1,128}$') { throw 'Credencial com formato inesperado.' }
 $header=@"
#pragma once
// PRIVATE: dedicated Firebase anonymous test identity. Never commit or print this file.
#define LAVCOUNTER_FIREBASE_AUTH_UID "$User"
#define LAVCOUNTER_FIREBASE_PROJECT_NUMBER "957282795652"
#define LAVCOUNTER_FIREBASE_REFRESH_TOKEN "$Refresh"
"@
 [IO.File]::WriteAllText($credentialsPath,$header+"`n",$utf8)
}
$private=[IO.File]::ReadAllText($privatePath)
$apiKey=Extract-Define $private 'LAVCOUNTER_FIREBASE_API_KEY'
if(Test-Path -LiteralPath $credentialsPath) {
 $existing=[IO.File]::ReadAllText($credentialsPath)
 $refresh=Extract-Define $existing 'LAVCOUNTER_FIREBASE_REFRESH_TOKEN'
 $uid=Extract-Define $existing 'LAVCOUNTER_FIREBASE_AUTH_UID'
} else {
 $account=Call-Firebase ('https://identitytoolkit.googleapis.com/v1/accounts:signUp?key='+[Uri]::EscapeDataString($apiKey)) 'application/json' '{"returnSecureToken":true}'
 $refresh=[string]$account.refreshToken;$uid=[string]$account.localId
 if(-not $refresh -or -not $uid) { throw 'Firebase não devolveu identidade válida.' }
 Assert-TestToken ([string]$account.idToken) $uid
 # Persist the seed before a second request: a network failure must not create
 # another anonymous account on the next run.
 Save-Credential $refresh $uid
}
$session=Call-Firebase ('https://securetoken.googleapis.com/v1/token?key='+[Uri]::EscapeDataString($apiKey)) 'application/x-www-form-urlencoded' ('grant_type=refresh_token&refresh_token='+[Uri]::EscapeDataString($refresh))
if($session.user_id -ne $uid -or [string]$session.project_id -ne '957282795652') { throw 'UID/projeto Firebase diferente do esperado.' }
Assert-TestToken ([string]$session.id_token) $uid
$refresh=[string]$session.refresh_token
Save-Credential $refresh $uid

$rules=[IO.File]::ReadAllText((Join-Path $projectRoot 'docs/firestore-test.rules'))
$rules=$rules.Replace('REPLACE_AUTH_UID',$uid)
New-Item -ItemType Directory -Force (Join-Path $projectRoot 'build') | Out-Null
$rulesPath=Join-Path $projectRoot 'build/firestore-test.rules'
[IO.File]::WriteAllText($rulesPath,$rules,$utf8)
Write-Output 'Autenticação Firebase real e renovação verificadas; credencial salva no arquivo privado ignorado pelo Git.'
Write-Output "UID público de teste: $uid"
Write-Output "Regras preparadas: $rulesPath"
Write-Output 'Regras prontas sem cópia de IDs. Publique no console Firebase; este script não publica regras.'
