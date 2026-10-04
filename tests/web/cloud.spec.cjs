const {test,expect}=require('@playwright/test');
const html=require('fs').readFileSync('web/index.html','utf8');
let cloud,posts,link,hold;
test.beforeEach(async({page})=>{
 cloud={configured:false,clientId:'00000000-0000-4000-8000-000000000001',deviceId:'esp32-010203040506',state:'not_configured',premiumStatus:'unknown',wifiConnected:false,pendingVersions:null,confirmedVersions:null,countsComplete:false,lastError:'',lastCompletedAt:null};posts=0;link=null;hold=null;
 await page.route('**/*',async route=>{
  const req=route.request(),url=new URL(req.url());
  if(url.pathname==='/')return route.fulfill({contentType:'text/html',body:html});
  let data={},status=200;
  if(url.pathname==='/api/status')data={sensorMode:'physical',state:'IDLE',observedWashes:3,persistedWashes:3,todayWashes:3,queuedEvents:0,undatedWashes:0,storageUsed:100,storageCapacity:1000};
  if(url.pathname==='/api/cloud-sync'){
   if(req.method()==='POST'){posts++;expect(req.postDataJSON()).toEqual({});if(hold)await hold;cloud.state='syncing';status=202;}data={...cloud};
  }
  if(url.pathname==='/api/premium-link'){if(link)data={url:link};else{status=503;data={error:'Cadastro premium indisponível: URL SaaS não configurada.'};}}
  return route.fulfill({status,contentType:'application/json',body:JSON.stringify(data)});
 });await page.goto('http://lavcounter.test/');
});
test('identidade automática e nuvem não configurada preservam métricas locais',async({page})=>{
 await expect(page.locator('#cloud-state')).toHaveText('Nuvem não configurada');await expect(page.locator('#cloud-identity')).toContainText(cloud.clientId);await expect(page.locator('#cloud-sync')).toBeDisabled();await expect(page.locator('#premium')).toBeDisabled();await expect(page.locator('#premium-reason')).toContainText('URL SaaS não configurada');await expect(page.locator('#observed')).toHaveText('3');expect(posts).toBe(0);
});
test('premium inativo, desconhecido, expirado e offline bloqueiam envio',async({page})=>{
 cloud.configured=true;cloud.wifiConnected=true;
 for(const [premiumStatus,state,text] of [['inactive','premium_required','Premium necessário'],['unknown','premium_unknown','ainda não confirmado'],['expired','premium_expired','expirada']]){cloud.premiumStatus=premiumStatus;cloud.state=state;await page.reload();await expect(page.locator('#cloud-state')).toContainText(text);await expect(page.locator('#cloud-sync')).toBeDisabled();}
 cloud.premiumStatus='active';cloud.wifiConnected=false;cloud.state='offline';await page.reload();await expect(page.locator('#cloud-state')).toHaveText('Wi-Fi desconectado');await expect(page.locator('#cloud-sync')).toBeDisabled();expect(posts).toBe(0);
 cloud.wifiConnected=true;cloud.state='premium_unknown';await page.reload();await expect(page.locator('#cloud-sync')).toBeDisabled();
});
test('Premium abre URL com IDs estáveis sem código nem segredo',async({page})=>{
 link='https://saas.example/cadastro?clientId='+cloud.clientId+'&deviceId='+cloud.deviceId;
 await page.addInitScript(()=>{window.open=(url,target,features)=>{window.opened={url,target,features};};});await page.reload();await expect(page.locator('#premium')).toBeEnabled();await page.locator('#premium').click();let opened=await page.evaluate(()=>window.opened);expect(opened.url).toBe(link);expect(opened.features).toBe('noopener,noreferrer');expect(new URL(opened.url).searchParams.size).toBe(2);await page.reload();await page.locator('#premium').click();expect((await page.evaluate(()=>window.opened)).url).toBe(opened.url);await expect(page.locator('#cloud-sync')).toBeDisabled();
});
test('aceite não é confirmação, clique concorrente é bloqueado e falha parcial permanece visível',async({page})=>{
 cloud={...cloud,configured:true,wifiConnected:true,premiumStatus:'active',state:'idle',countsComplete:true,pendingVersions:3,confirmedVersions:0};await page.reload();let release;hold=new Promise(r=>release=r);await expect(page.locator('#cloud-sync')).toBeEnabled();await page.locator('#cloud-sync').click();await expect(page.locator('#cloud-sync')).toBeDisabled();expect(posts).toBe(1);release();hold=null;await expect(page.locator('#cloud-message')).toContainText('Solicitação aceita');await expect(page.locator('#cloud-state')).toContainText('em andamento');
 cloud.state='backoff';cloud.pendingVersions=2;cloud.confirmedVersions=1;cloud.lastError='Envio não confirmado';await expect(page.locator('#cloud-error')).toHaveText('Envio não confirmado');await expect(page.locator('#cloud-counts')).toContainText('Confirmadas: 1');await expect(page.locator('#observed')).toHaveText('3');
 cloud.state='idle';cloud.lastError='';cloud.pendingVersions=0;cloud.confirmedVersions=3;cloud.lastCompletedAt='2026-10-03T12:00:00';await expect(page.locator('#cloud-message')).toHaveText('Sincronização confirmada.');
});
test('layout móvel e endereço inválido do cadastro',async({page})=>{
 link='http://saas.example/cadastro';await page.setViewportSize({width:390,height:844});await page.reload();await expect(page.locator('#premium')).toBeDisabled();await expect(page.locator('#premium-reason')).toContainText('inválido');await expect(page.locator('body')).toHaveJSProperty('scrollWidth',390);await page.screenshot({path:'build/web-cloud-mobile.png',fullPage:true});
});
test('teste Firebase libera sincronização sem compra e identifica expiração',async({page})=>{
 cloud={...cloud,configured:true,wifiConnected:true,premiumStatus:'active',state:'idle',testMode:true,testUntil:1791676800};
 await page.setViewportSize({width:390,height:844});await page.reload();
 await expect(page.locator('#cloud-test-notice')).toBeVisible();await expect(page.locator('#cloud-test-notice')).toContainText('10/10/2026');
 await expect(page.locator('#cloud-description')).toContainText('sem compra de Premium');
 await expect(page.locator('#cloud-state')).toHaveText('Firebase disponível para teste');await expect(page.locator('#cloud-sync')).toBeEnabled();
 await expect(page.locator('#premium')).toBeDisabled();await expect(page.locator('body')).toHaveJSProperty('scrollWidth',390);
 cloud.premiumStatus='expired';cloud.state='premium_expired';await page.reload();await expect(page.locator('#cloud-state')).toHaveText('Prazo do teste Firebase encerrado');await expect(page.locator('#cloud-sync')).toBeDisabled();
 await page.screenshot({path:'build/web-firebase-test-mobile.png',fullPage:true});
});
