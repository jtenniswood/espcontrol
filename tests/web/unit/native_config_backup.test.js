const test = require("node:test");
const assert = require("node:assert/strict");
const path = require("node:path");
const { loadTypeScriptModule } = require("../../../scripts/load_typescript_module");
const load = file => loadTypeScriptModule(path.resolve(__dirname, "../../../src/webserver/model", file));
const backup=load("native_config_backup.ts"),flat=load("panel_config.ts"),graph=load("panel_config_conversion.ts"),v2=load("panel_config_v2.ts");
const envelope=load("backup.ts");
const native=loadTypeScriptModule(path.resolve(__dirname,"../../../src/webserver/features/native_panel_config.ts"));
const exporter=loadTypeScriptModule(path.resolve(__dirname,"../../../src/webserver/features/backup_export_controller.ts"));
const document=()=>({deviceProfile:"panel-a",buttons:{1:";Root;;;;;subpage",32:"future.unknown;Hidden"},subpages:{1:"~B,1|G,,Child",33:"~B=Root:ABCDEF,1|,light.leaf,Leaf"},settings:{button_order:"1",opaque:"retained\0🌡️"}});
test("native backups retain exact graph bytes and existing v1 payloads",()=>{
  const source=document(), records=v2.decodeNativeV2(graph.encodePanelPageDocument(source));
  const wire=v2.encodeNativeV2(records.reverse());
  const payload=backup.createNativeConfigBackupPayload(wire);
  assert.equal(payload.document_version,2);
  assert.equal(payload.device_profile,source.deviceProfile);
  assert.deepEqual(backup.decodeNativeConfigBackupPayload(payload),wire);
  assert.deepEqual(JSON.parse(JSON.stringify(backup.decodeNativeConfigDocument(wire))),source);
  const old=flat.encodePanelConfig({...source,subpages:{1:"B,1|light.old:Old"}});
  assert.deepEqual(backup.createNativeConfigBackupPayload(old),flat.createPanelConfigBackupPayload(old));
  assert.deepEqual(backup.decodeNativeConfigBackupPayload(backup.createNativeConfigBackupPayload(old)),old);
});
test("native backup declarations, corruption and excessive graphs fail before decoding an editor snapshot",()=>{
  const wire=graph.encodePanelPageDocument(document()),payload=backup.createNativeConfigBackupPayload(wire);
  for(const malformed of [null,[],{}, {...payload,document_version:3},{...payload,document_version:"2"},
    {...payload,device_profile:"different"},{...payload,payload:"invalid base64"},{...payload,payload:"A".repeat(699056)}])
    assert.throws(()=>backup.decodeNativeConfigBackupPayload(malformed));
  const old=flat.encodePanelConfig({deviceProfile:"panel-a",buttons:{},subpages:{},settings:{}});
  assert.throws(()=>backup.decodeNativeConfigBackupPayload({...payload,payload:flat.encodeBase64(old)}),/version does not match/);
  const damaged=wire.slice();damaged[0]=0;
  assert.throws(()=>backup.createNativeConfigBackupPayload(damaged));
  const oversized=new Uint8Array(524289);oversized.set(wire.subarray(0,16));
  assert.throws(()=>backup.decodeNativeConfigDocument(oversized),/graph byte/);
});
test("graph backup exports use outer version three while flat exports retain version two",()=>{
  const wire=graph.encodePanelPageDocument(document()),payload=backup.createNativeConfigBackupPayload(wire);
  const created=envelope.createBackupEnvelope({native_config:payload},{buttons:[],subpages:{}});
  assert.equal(created.version,3);
  assert.deepEqual(created.native_config,payload);
  assert.doesNotThrow(()=>envelope.validateBackupEnvelope(created));
  assert.throws(()=>envelope.validateBackupEnvelope({...created,native_config:undefined}),/complete native/);
  assert.throws(()=>envelope.validateBackupEnvelope({...created,native_config:{...payload,document_version:3}}),/complete native/);
  const controller=exporter.createBackupExportController({serializeButtonConfig:()=>"",serializeSubpageConfig:()=>""});
  const output=controller.addNativeConfig({version:2},{deviceProfile:"panel-a",buttons:[],subpages:{},nativeDocumentBytes:wire});
  assert.equal(output.version,3);
  assert.deepEqual(backup.decodeNativeConfigBackupPayload(output.native_config),wire);
  const old=flat.encodePanelConfig({deviceProfile:"panel-a",buttons:{},subpages:{},settings:{}});
  const flatOutput=controller.addNativeConfig({version:2},{deviceProfile:"panel-a",buttons:[],subpages:{},nativeDocumentBytes:old});
  assert.equal(flatOutput.version,2);
  assert.deepEqual(flatOutput.native_config,flat.createPanelConfigBackupPayload(old));
});
const reply=(status,body=new Uint8Array(),version=null)=>({ok:status>=200&&status<300,status,
  headers:{get:name=>name==="X-Panel-Config-Version"?version:null},json:async()=>({}),arrayBuffer:async()=>new Uint8Array(body).buffer});
const capabilities=(extra={})=>({...reply(200),json:async()=>({configuration:{read:true,write:true,document_versions:[1,2],
  max_navigation_depth:2,writable_navigation_depth:1,durable_document_version:1,...extra}})});
test("backup negotiation keeps durable-v1 snapshots exact and reads authoritative graphs when required",async()=>{
  const wire=graph.encodePanelPageDocument(document());
  const old=flat.encodePanelConfig({deviceProfile:"panel-a",buttons:{},subpages:{},settings:{}});
  for(const [extra,flatAvailable,expected,bytes] of [
    [{},true,[1],old],[{},false,[1,2],wire],
    [{durable_document_version:2,writable_navigation_depth:2},true,[2],wire],
    [{durable_document_version:2,writable_navigation_depth:2,document_versions:[2]},false,[2],wire],
  ]){
    const requests=[];
    const client=native.createNativePanelConfigClient(async(path,request)=>{
      if(path.endsWith("capabilities"))return capabilities(extra);
      assert.notEqual(request?.method,"PUT");const version=Number(request.headers["X-Panel-Config-Version"]);requests.push(version);
      return version===1&&!flatAvailable?reply(406):reply(200,version===1?old:wire,String(version));
    });
    assert.deepEqual(await client.readForBackup("panel-a"),bytes);
    assert.deepEqual(requests,expected);
  }
});
test("backup read failures cannot fall back to partial or mismatched representations",async()=>{
  const wire=graph.encodePanelPageDocument(document());
  for(const response of [reply(503),reply(401),reply(200,wire,"1"),reply(200,wire),reply(200,new Uint8Array(),"2")]){
    let reads=0;
    const client=native.createNativePanelConfigClient(async path=>path.endsWith("capabilities")?
      capabilities({durable_document_version:2}):(reads++,response));
    await assert.rejects(()=>client.readForBackup("panel-a"));assert.equal(reads,1);
  }
  const disguised=native.createNativePanelConfigClient(async path=>path.endsWith("capabilities")?capabilities():reply(200,wire,"1"));
  await assert.rejects(()=>disguised.readForBackup("panel-a"),/negotiated version/);
  const limited=native.createNativePanelConfigClient(async path=>path.endsWith("capabilities")?
    capabilities({durable_document_version:2,max_v2_document_bytes:32}):reply(200,wire,"2"));
  await assert.rejects(()=>limited.readForBackup("panel-a"),/advertised byte limit/);
});
test("inactive-record checks use owner paths rather than numeric nested page IDs",()=>{
  const active={...document(),buttons:{1:document().buttons[1]}};
  assert.equal(native.nativeBackupHasInactiveRecords(active,9),false);
  assert.equal(native.nativeBackupHasInactiveRecords({...active,subpages:{...active.subpages,42:"B"}},9),true);
  assert.equal(native.nativeBackupHasInactiveRecords({...active,subpages:{...active.subpages,321:"B"}},9),true);
  assert.equal(native.nativeBackupHasInactiveRecords(document(),9),true);
});
