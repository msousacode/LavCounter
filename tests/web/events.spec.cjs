const {test,expect}=require('@playwright/test');
const fs=require('fs');
const html=fs.readFileSync('web/index.html','utf8');
const session=(id,date,validated=true)=>({sessionId:String(id),startedAt:date,validated,validatedAt:null,closedAt:date,durationMs:120000,type:'closed'});
let rows,clock,requests,fail;
test.beforeEach(async({page})=>{
 rows=[session(1,'2026-10-12T09:00:00'),session(2,'2026-10-06T23:59:59'),session(3,'2026-10-05T23:59:59'),session(4,'2026-09-28T00:00:00'),session(5,'2026-09-27T23:59:59'),session(6,'2026-10-01T00:00:00'),session(7,'2026-10-12T10:00:00',false),session(8,'2026-10-13T00:00:00'),session(9,null)];
 clock={valid:true,dateTime:'2026-10-12T12:00:00'};requests=[];fail=false;
 await page.route('**/*',async route=>{
  const u=new URL(route.request().url());
  if(u.pathname==='/')return route.fulfill({contentType:'text/html',body:html});
  let data={};
  if(u.pathname==='/api/cloud-sync')data={configured:false,state:'not_configured',premiumStatus:'unknown',wifiConnected:false,pendingVersions:null,confirmedVersions:null,countsComplete:false,lastError:''};
  if(u.pathname==='/api/premium-link')return route.fulfill({status:503,contentType:'application/json',body:JSON.stringify({error:'Cadastro premium indisponível: URL SaaS não configurada.'})});
  if(u.pathname==='/api/rtc')data=clock;
  if(u.pathname==='/api/sessions'){
   requests.push(u.searchParams.get('cursor'));
   if(fail)return route.fulfill({status:503,contentType:'application/json',body:JSON.stringify({error:'Histórico indisponível'})});
   const offset=Number(u.searchParams.get('cursor')),events=rows.slice(offset,offset+5);
   data={events,hasMore:offset+5<rows.length,nextCursor:String(offset+5),degraded:false};
  }
  await route.fulfill({contentType:'application/json',body:JSON.stringify(data)});
 });
 await page.goto('http://lavcounter.test/');
 await page.getByRole('tab',{name:'Eventos',exact:true}).click();
 await expect(page.locator('#reload-events')).toBeEnabled();
});
const ids=page=>page.locator('#event-rows tr td:first-child').allTextContents();
test('padrão válidas em sete dias, histórico completo e data decrescente',async({page})=>{
 await expect(page.getByRole('radio',{name:'Válidas',exact:true})).toBeChecked();
 await expect(page.locator('#event-period')).toHaveValue('7');
 expect(await ids(page)).toEqual(['#1 · 12/10/2026 09:00','#2 · 06/10/2026 23:59']);
 expect(requests).toEqual(['0','5']);
 await expect(page.locator('#events-message')).toContainText('1 sem data confiável');
 await page.screenshot({path:'build/web-events-filters.png',fullPage:true});
});
test('resultados e períodos respeitam limites do calendário sem novas consultas',async({page})=>{
 await page.getByRole('radio',{name:'Não Válidas',exact:true}).check();
 await expect(page.locator('#event-rows tr')).toHaveCount(1);
 await expect(page.locator('#event-rows')).toContainText('#7');
 await page.getByRole('radio',{name:'Ambos',exact:true}).check();
 await expect(page.locator('#event-rows tr')).toHaveCount(3);
 expect((await ids(page))[0]).toContain('#7');
 await page.locator('#event-period').selectOption('today');
 await expect(page.locator('#event-rows tr')).toHaveCount(2);
 await page.locator('#event-period').selectOption('15');
 await expect(page.locator('#event-rows tr')).toHaveCount(6);
 await expect(page.locator('#event-rows')).toContainText('#4');
 await expect(page.locator('#event-rows')).not.toContainText('#5');
 await page.locator('#event-period').selectOption('month');
 await expect(page.locator('#event-rows tr')).toHaveCount(5);
 await expect(page.locator('#event-rows')).not.toContainText('#8');
 expect(requests).toHaveLength(2);
});
test('paginação visual mantém ordem global, IDs grandes e datas alternativas',async({page})=>{
 rows=Array.from({length:23},(_,i)=>session(String(100+i),'2026-10-12T09:00:00'));
 rows.push({...session('9007199254740993',null),validatedAt:'2026-10-12T11:00:00'});
 rows.push({...session('9007199254740992',null),closedAt:'2026-10-12T11:00:00'});
 await page.locator('#reload-events').click();
 await expect(page.locator('#event-rows tr')).toHaveCount(20);
 expect((await ids(page)).slice(0,2).map(x=>x.split(' · ')[0])).toEqual(['#9007199254740993','#9007199254740992']);
 await page.locator('#more-events').click();
 await expect(page.locator('#event-rows tr')).toHaveCount(25);
 await expect(page.locator('#more-events')).toBeHidden();
});
test('relógio inválido, vazio e erro de consulta são explícitos',async({page})=>{
 rows=[];await page.locator('#reload-events').click();
 await expect(page.locator('#events-message')).toContainText('Nenhuma sessão encontrada');
 clock={valid:false,dateTime:null};await page.locator('#reload-events').click();
 await expect(page.locator('#events-message')).toContainText('Ajuste o relógio');
 fail=true;await page.locator('#reload-events').click();
 await expect(page.locator('#events-message')).toContainText('Histórico indisponível');
});
test('filtros móveis sem transbordar a página',async({page})=>{
 await page.setViewportSize({width:390,height:844});
 await expect(page.locator('body')).toHaveJSProperty('scrollWidth',390);
 await page.getByRole('radio',{name:'Ambos',exact:true}).check();
 await page.screenshot({path:'build/web-events-filters-mobile.png',fullPage:true});
});
