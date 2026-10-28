"""Independent native-v1 sources and expected imported graph semantics."""
import argparse,struct,subprocess
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('importer',type=Path);args=p.parse_args()
def v1(rows):
 payload=b''.join(struct.pack('<BH',kind,len(body))+body for kind,body in rows)
 return b'EPCF'+struct.pack('<HHIHH',1,16,len(payload),len(rows),0)+payload
def slot(kind,position,value):return kind,bytes([position])+value.encode()
def setting(key,value):
 key=key.encode();return 4,bytes([len(key)])+key+value.encode()
def run(data,mode=None):return subprocess.run([str(args.importer),*([mode] if mode else [])],input=data,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
def graph(data):
 assert data[:8]==b'EPCF\2\0\x10\0'
 size,count,reserved=struct.unpack_from('<IHH',data,8);assert size==len(data)-16 and not reserved
 strings={0:''};entities={0:0};pages={};cards={};nav={};settings={};profile=None;offset=16
 for _ in range(count):
  kind,length=struct.unpack_from('<BH',data,offset);offset+=3;body=data[offset:offset+length];offset+=length
  if kind==1:profile=body.decode()
  elif kind==2:strings[struct.unpack_from('<H',body)[0]]=body[2:].decode()
  elif kind==3:
   ident,value=struct.unpack('<HH',body);entities[ident]=value
  elif kind==4:
   ident,parent,owner,order,flags=struct.unpack('<HHHHB',body);pages[ident]=(parent,owner,order,flags)
  elif kind==5:
   ident,page,position,mask,flags=struct.unpack_from('<HHHHB',body);fields=[0]*9;cursor=9
   for i in range(9):
    if mask&(1<<i):fields[i]=struct.unpack_from('<H',body,cursor)[0];cursor+=2
   raw=struct.unpack_from('<H',body,cursor)[0] if flags&1 else 0
   cards[ident]=(page,position,fields,flags,raw)
  elif kind==6:
   owner,target=struct.unpack('<HH',body);nav[owner]=target
  elif kind==7:
   key,value=struct.unpack('<HH',body);settings[key]=value
  else:assert False
 assert offset==len(data)
 assert len(strings)==len(set(strings.values()))
 assert len(entities)==len(set(entities.values()))
 output={}
 for ident,(parent,owner,order,flags) in pages.items():
  assert parent==(65535 if ident==0 else 0)
  key=0 if ident==0 else cards[owner][1]
  children=[]
  for page,position,fields,card_flags,raw in cards.values():
   if page!=ident:continue
   values=[strings[entities[ref] if i in (0,4) else ref] for i,ref in enumerate(fields)]
   children.append((position,values,card_flags,strings[raw]))
  output[key]={'order':strings[order],'flags':flags,'nav':ident==0 or owner in nav,'cards':sorted(children)}
 return {'profile':profile,'settings':{strings[k]:strings[v] for k,v in settings.items()},'pages':output}
folder=';Folder;;;;;subpage';sensor='sensor.same;Known;;;;;sensor'
child='S,sensor.same,%EF%BB%BFRoom,,,sensor.shared,°C,1,unknown=%FF%2C'
rows=[(1,b'panel-a'),slot(2,1,folder),slot(2,2,''),slot(2,3,sensor),slot(2,4,folder),
 slot(3,1,'~B=Back%3Aroom:ABCDEF,1w|'+child+'|'),slot(3,3,'B|sensor.hidden:Hidden'),slot(3,32,'~'),
 setting('button_order','4,3d,,1,2'),setting('future','\ufeffvalue\0😀'),setting('__proto__','opaque')]
empty=['']*9;folder_fields=['','Folder','','','','','subpage','','']
expected={'profile':'panel-a','settings':{'button_order':'4,3d,,1,2','future':'\ufeffvalue\0😀','__proto__':'opaque'},'pages':{
 0:{'order':'4,3d,,1,2','flags':0,'nav':True,'cards':[(1,folder_fields,1,folder),(2,empty,1,''),(3,['sensor.same','Known','','','','','sensor','',''],1,sensor),(4,folder_fields,1,folder),(32,empty,2,'')]},
 1:{'order':'B=Back%3Aroom:ABCDEF,1w','flags':1,'nav':True,'cards':[(1,['sensor.same','\ufeffRoom','Auto','Auto','sensor.shared','°C','sensor','1','unknown=%FF%2C'],1,child),(2,['','','Auto','Auto','','','','',''],1,'')]},
 3:{'order':'B','flags':0,'nav':False,'cards':[(1,['sensor.hidden','Hidden','Auto','Auto','','','','',''],1,'sensor.hidden:Hidden')]},
 4:{'order':'','flags':2,'nav':True,'cards':[]},32:{'order':'','flags':1,'nav':False,'cards':[]}}}
source=v1(rows)
for data in [source,v1(list(reversed(rows)))]:
 result=run(data);assert result.returncode==0,result.stderr
 assert graph(result.stdout)==expected
minimal=run(v1([(1,b'a')]));assert minimal.returncode==0,minimal.stderr
assert graph(minimal.stdout)=={'profile':'a','settings':{},'pages':{0:{'order':'','flags':0,'nav':True,'cards':[]}}}
collision=v1([(1,b'a'),slot(2,1,'^Ur8>KPrf[;One'),slot(2,2,'BJNQ*lxYGn;Two')])
result=run(collision);assert result.returncode==0,result.stderr
assert [row[1][0] for row in graph(result.stdout)['pages'][0]['cards']]==['^Ur8>KPrf[','BJNQ*lxYGn']
rejected=0
for end in range(len(source)):
 result=run(source[:end]);assert result.returncode==2,(end,result.stderr);rejected+=1
for data in [v1(rows+[rows[1]]),v1(rows+[setting('future','duplicate')]),v1(rows+[(1,b'duplicate')]),v1([(1,b'a'),(2,b'\0bad')]),v1([(1,b'a'),(2,b'!bad')]),v1([(1,b'a'),(99,b'unknown')]),v1([(1,b'a'),slot(2,1,'x'*4096)]),v1([(1,b'a'),(2,b'\1\xff')])]:
 result=run(data);assert result.returncode==2,result.stderr;rejected+=1
# Valid v1 envelopes with more cards than v2 identities can represent fail
# explicitly instead of clipping their stored definitions.
too_many=v1([(1,b'a')]+[slot(3,i,'|'*4095) for i in range(1,33)])
result=run(too_many);assert result.returncode==2 and result.stderr.strip()==b'3',result.stderr
for mode in ['short','table-full','unaligned','small-output','input-output-alias','input-workspace-alias','output-workspace-alias','result-input-alias','result-output-alias','result-workspace-alias','null-result','null-bounds','bounds-input-alias']:
 result=run(source,mode);assert result.returncode==2,(mode,result.stderr)
print('Import: independent graph, reordered source, empty graph and hash collision checks,',rejected,'malformed inputs, identity overflow and 13 alias/budget cases pass; allocation forbidden.')
