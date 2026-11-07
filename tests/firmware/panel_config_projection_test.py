"""Independent expected v1 records, malformed wires and private-buffer guards."""
import argparse,json,struct,subprocess
from pathlib import Path
from panel_config_document_v2_cases import canonical,cases,presence,wire
p=argparse.ArgumentParser();p.add_argument('projector',type=Path);args=p.parse_args()
def run(data,mode=None):
 return subprocess.run([str(args.projector),*([mode] if mode else [])],input=data,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
def parse_v1(data):
 assert data[:8]==b'EPCF\1\0\x10\0'
 length,count,reserved=struct.unpack_from('<IHH',data,8)
 assert length==len(data)-16 and not reserved
 output={'profile':None,'buttons':{},'subpages':{},'settings':{}};offset=16
 for _ in range(count):
  kind,size=struct.unpack_from('<BH',data,offset);offset+=3;body=data[offset:offset+size];offset+=size
  if kind==1:output['profile']=body.decode()
  elif kind in (2,3):output['buttons' if kind==2 else 'subpages'][body[0]]=body[1:].decode()
  elif kind==4:output['settings'][body[1:1+body[0]].decode()]=body[1+body[0]:].decode()
  else:assert False
 assert offset==len(data)
 return output
result=run(wire(presence));assert result.returncode==0,result.stderr
assert parse_v1(result.stdout)=={'profile':'a','buttons':{1:'~,,,,,,subpage'},'subpages':{2:''},'settings':{}}
result=run(canonical);assert result.returncode==0,result.stderr
assert parse_v1(result.stdout)=={
 'profile':'guition-esp32-s3-4848s040',
 'buttons':{1:'~,Room ☀,,,,,subpage',32:'sensor.indoor_temperature;Room ☀;Auto;Auto;;;sensor;;'},
 'subpages':{1:'~B=Up:FF8C00,1|S,sensor.indoor_temperature,Temperature °C,,,sensor.indoor_humidity,°C,1,large=true',32:''},
 'settings':{'future_setting':'Café|☀\0value','button_order':'1,32'}}
rejected=0
for name,data,valid,mode in cases:
 if valid or mode:continue
 result=run(data);assert result.returncode==2,(name,result.returncode,result.stderr)
 rejected+=1
for mode in ['short','unaligned','small-output','input-output-alias','input-workspace-alias','output-workspace-alias','result-input-alias','result-output-alias','result-workspace-alias','null-result']:
 result=run(canonical,mode);assert result.returncode==2,(mode,result.returncode,result.stderr)
print('Projection: two independent v1 semantic goldens,',rejected,'invalid wires and 10 workspace/output/alias cases pass; allocation forbidden.')
