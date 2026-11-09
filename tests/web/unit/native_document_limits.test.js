const test = require('node:test');
const assert = require('node:assert/strict');
const path = require('node:path');
const {loadTypeScriptModule} = require('../../../scripts/load_typescript_module');
const clientModule = loadTypeScriptModule(path.resolve('src/webserver/features/native_panel_config.ts'));
const flat = loadTypeScriptModule(path.resolve('src/webserver/model/panel_config.ts'));
const graph = loadTypeScriptModule(path.resolve('src/webserver/model/panel_config_conversion.ts'));
const document = {deviceProfile:'panel-a',buttons:{1:'light.test;Test'},subpages:{},settings:{button_order:'1'}};
const reply = (status,body=new Uint8Array(),version=1) => ({ok:status>=200&&status<300,status,
 headers:{get:name=>name==='ETag'?`"${version===2?'v2-':''}7"`:name==='X-Panel-Config-Version'?String(version):null},
 arrayBuffer:async()=>new Uint8Array(body).buffer,json:async()=>({})});
function fixture(representation,durable,limit,versions=representation===1?[1]:[1,2]) {
 const bytes=representation===1?flat.encodePanelConfig(document):graph.encodePanelPageDocument(document);
 let puts=0;
 const client=clientModule.createNativePanelConfigClient(async(path,request)=>{
  if(path.endsWith('capabilities'))return {...reply(200),json:async()=>({configuration:{read:true,write:true,document_versions:versions,
   durable_document_version:durable,max_navigation_depth:2,writable_navigation_depth:durable===2?2:1,
   max_record_body_bytes:4096,max_v2_document_bytes:524288,...(limit===undefined?{}:{max_durable_document_bytes:limit})}})};
  if(request?.method==='PUT'){puts++;return reply(204);}
  const selected=Number(request?.headers?.['X-Panel-Config-Version']||1);
  return reply(200,selected===1?flat.encodePanelConfig(document):bytes,selected);
 });
 return {client,bytes,puts:()=>puts};
}
test('native saves reject limits below every lossless durable form before PUT',async()=>{
 for(const version of [1,2]){
  const wire=version===1?flat.encodePanelConfig(document):graph.encodePanelPageDocument(document);
  const minimum=Math.min(wire.length,flat.encodePanelConfig(document).length);
  for(const limit of new Set([0,minimum-1,minimum,wire.length-1,wire.length])){
   const f=fixture(version,version,limit);
   assert.equal(await f.client.save(value=>value),limit<minimum?'too-large':'saved');
   assert.equal(f.puts(),limit<minimum?0:1);
  }
 }
});
test('larger converted reads remain valid and backups never apply a write-byte ceiling',async()=>{
 const limit=flat.encodePanelConfig(document).length;
 const f=fixture(2,1,limit,[2]);assert(f.bytes.length>limit);
 assert(await f.client.readSnapshot('panel-a'));
 assert.deepEqual(await f.client.readForBackup('panel-a'),f.bytes);
 assert.equal(f.puts(),0);
});
test('a v2 request converted to durable v1 is not rejected using its larger transfer size',async()=>{
 const limit=flat.encodePanelConfig(document).length;
 const f=fixture(2,1,limit);assert(f.bytes.length>limit);
 assert.equal(await f.client.save(value=>value),'saved');assert.equal(f.puts(),1);
});
test('older firmware without a durable byte capability retains its existing save behavior',async()=>{
 for(const version of [1,2]){
  const f=fixture(version,version,undefined);assert.equal(await f.client.save(value=>value),'saved');assert.equal(f.puts(),1);
 }
});
