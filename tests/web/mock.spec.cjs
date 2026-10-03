const {test,expect}=require('@playwright/test');
const html=require('fs').readFileSync('web/index.html','utf8');
test('mock mantém entrada no servidor, confirma comandos e reconcilia resposta perdida',async({page})=>{
 let sample='absent',mode='mock',loseResponse=false;const commands=[];
 await page.route('**/*',async route=>{
  const req=route.request(),url=new URL(req.url());expect(url.hostname).toBe('lavcounter.test');
  if(url.pathname==='/')return route.fulfill({contentType:'text/html',body:html});
  let data={};
  if(url.pathname==='/api/mock-sensor'){
   expect(req.method()).toBe('POST');sample=req.postDataJSON().sample;commands.push(sample);
   if(loseResponse)return route.abort();
   data={sensorMode:mode,mockSample:sample};
  }
  if(url.pathname==='/api/status')data={sensorMode:mode,mockSample:sample,state:'CANDIDATE',observedWashes:0,persistedWashes:0,todayWashes:null,distanceMm:null,ledColor:'yellow',ledBlinking:sample==='fault',sensorError:sample==='fault'?'Falha simulada do sensor':'',queuedEvents:0,undatedWashes:0,storageUsed:0,storageCapacity:1000};
  if(url.pathname==='/api/sessions')data={events:[{sessionId:'1',sensorMode:'mock',validated:true,type:'closed',durationMs:1000}],nextCursor:'1',hasMore:false};
  await route.fulfill({contentType:'application/json',body:JSON.stringify(data)});
 });
 await page.setViewportSize({width:390,height:844});await page.goto('http://lavcounter.test/');
 await expect(page.locator('#mock-banner')).toBeVisible();await expect(page.locator('#mock-current')).toHaveText('Box vazio');
 await page.getByRole('button',{name:'Veículo presente',exact:true}).click();await expect(page.locator('#mock-message')).toContainText('Entrada aceita');await expect(page.locator('#mock-current')).toHaveText('Veículo presente');await expect(page.locator('#observed')).toHaveText('0');
 await page.reload();await expect(page.locator('#mock-current')).toHaveText('Veículo presente');expect(commands).toEqual(['present']);
 loseResponse=true;await page.getByRole('button',{name:'Falha do sensor',exact:true}).click();await expect(page.locator('#mock-message')).toContainText('Não foi possível confirmar');await expect(page.locator('#mock-current')).toHaveText('Falha do sensor');await expect(page.locator('#health')).toContainText('Falha simulada');
 loseResponse=false;await page.getByRole('button',{name:'Box vazio',exact:true}).click();await expect(page.locator('#mock-current')).toHaveText('Box vazio');expect(commands).toEqual(['present','fault','absent']);
 await expect(page.locator('body')).toHaveJSProperty('scrollWidth',390);await page.screenshot({path:'build/web-mock-mobile.png',fullPage:true});
 await page.getByRole('tab',{name:'Eventos',exact:true}).click();await expect(page.locator('#mock-banner')).toBeVisible();await expect(page.locator('#event-rows')).toContainText('Simulada');
 mode='physical';await page.reload();await expect(page.locator('#mock-banner')).toBeHidden();await expect(page.locator('#mock-controls')).toBeHidden();
});
