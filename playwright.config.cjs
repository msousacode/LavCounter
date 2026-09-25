const {defineConfig}=require('@playwright/test');
module.exports=defineConfig({testDir:'./tests/web',workers:1,reporter:'list',use:{browserName:'chromium',channel:'msedge',headless:true,viewport:{width:1100,height:800}}});
