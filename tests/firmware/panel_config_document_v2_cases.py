"""Independent binary golden and hostile graph cases for the real v2 reader."""
import copy,json,struct
from pathlib import Path
fixture=json.loads((Path(__file__).resolve().parents[2]/'compatibility/fixtures/panel_config_v2.json').read_text())
records=fixture['records'];cases=[]
def wire(rows,version=2):
 payload=b''.join(bytes([r['kind']])+struct.pack('<H',len(bytes.fromhex(r['body_hex'])))+bytes.fromhex(r['body_hex']) for r in rows)
 return b'EPCF'+struct.pack('<HHIHH',version,16,len(payload),len(rows),0)+payload
def change(rows,kind,ident,offset,value):
 row=next(r for r in rows if r['kind']==kind and r.get('id',0)==ident)
 body=bytearray.fromhex(row['body_hex']);body[offset:offset+2]=struct.pack('<H',value);row['body_hex']=body.hex()
def reject(name,data,mode=''):cases.append((name,data,False,mode))
def accept(name,data,mode=''):cases.append((name,data,True,mode))
canonical=bytes.fromhex(fixture['wire_hex']);assert wire(records)==canonical
accept('independent golden',canonical);accept('record order is not identity',wire(list(reversed(records))))
accept('empty dashboard still has explicit root/profile',wire([{'kind':1,'body_hex':b'a'.hex()},{'kind':4,'body_hex':struct.pack('<HHHHB',0,65535,65535,0,0).hex()}]))
accept('validation can invalidate a formerly valid reader',canonical,'revalidate')
for mode in ['short','unaligned','overlap']:reject('workspace '+mode,canonical,mode)
reject('unsupported version',wire(records,3));reject('v1 is not silently accepted as v2',wire(records,1))
for length in range(16):reject('short header '+str(length),canonical[:length])
for end in range(16,len(canonical)):reject('truncation '+str(end),canonical[:end])
for end in range(16,len(canonical)):
 b=bytearray(canonical[:end]);struct.pack_into('<I',b,8,len(b)-16);reject('record body truncation '+str(end),bytes(b))
for field in [6,8,12,14]:
 b=bytearray(canonical);b[field]^=1;reject('header corruption '+str(field),bytes(b))
for kind,ident in [(1,0),(2,1),(3,20),(4,50),(5,10),(6,10),(7,8)]:
 rows=copy.deepcopy(records);rows.append(copy.deepcopy(next(r for r in rows if r['kind']==kind and r.get('id',0)==ident)));reject('duplicate identity '+str(kind),wire(rows))
for kind,ident,offset,value,name in [
 (3,20,2,600,'dangling entity string'),(4,50,2,600,'dangling parent'),(4,50,4,600,'dangling owner'),
 (4,50,6,600,'dangling order'),(5,90,2,600,'dangling card page'),(5,90,4,0,'zero card slot'),
 (5,90,9,600,'dangling card entity'),(5,90,17,600,'dangling card sensor'),(5,11,19,600,'dangling raw import'),
 (6,10,2,0,'navigation targets root'),(6,10,2,51,'navigation owner mismatch'),
 (7,8,2,600,'dangling setting value'),(4,51,4,10,'one card cannot own two pages'),
 (5,11,4,1,'duplicate page slot'),(4,0,2,50,'root has a parent'),
 (2,1,0,0,'zero string id'),(3,20,0,65535,'reserved entity id'),(4,51,0,65535,'reserved page id')]:
 rows=copy.deepcopy(records);change(rows,kind,ident,offset,value);reject(name,wire(rows))
rows=copy.deepcopy(records);rows.append({'kind':2,'id':19,'body_hex':(struct.pack('<H',19)+b'future_setting').hex()});rows.append({'kind':7,'id':19,'body_hex':struct.pack('<HH',19,9).hex()});reject('setting text aliases cannot override an earlier key',wire(rows))
rows=copy.deepcopy(records);change(rows,5,90,21,2) # sensor -> subpage
rows.extend([{'kind':4,'id':60,'body_hex':(struct.pack('<HHHHB',60,50,90,7,1)).hex()}, {'kind':6,'id':90,'body_hex':struct.pack('<HH',90,60).hex()}, {'kind':5,'id':150,'body_hex':(struct.pack('<HHHHBH',150,60,1,64,0,3)).hex()}]);accept('depth two is understood by the wire reader only',wire(rows))
deep=copy.deepcopy(rows);change(deep,5,150,9,2);deep.extend([{'kind':4,'id':61,'body_hex':struct.pack('<HHHHB',61,60,150,0,0).hex()},{'kind':6,'id':150,'body_hex':struct.pack('<HH',150,61).hex()}]);reject('third level',wire(deep))
cycle=copy.deepcopy(rows);cycle=[r for r in cycle if r['kind']!=6];change(cycle,4,50,2,60);change(cycle,4,50,4,150);reject('parent cycle with otherwise matching owners',wire(cycle))
for size,valid in [(4095,True),(4096,False)]:
 rows=copy.deepcopy(records);rows.append({'kind':2,'id':100,'body_hex':(struct.pack('<H',100)+b'x'*size).hex()});cases.append(('deployed string boundary '+str(size),wire(rows),valid,''))
for label,value in [('overlong',b'\xc0\x80'),('surrogate',b'\xed\xa0\x80'),('truncated',b'\xf0\x9f\x98'),('out of range',b'\xf4\x90\x80\x80')]:
 rows=copy.deepcopy(records);rows.append({'kind':2,'id':100,'body_hex':(struct.pack('<H',100)+value).hex()});reject('UTF8 '+label,wire(rows))
rows=copy.deepcopy(records);rows.append({'kind':99,'id':100,'body_hex':'0000'});reject('unknown record kind',wire(rows))
for mode in ['writer-dangling','writer-duplicate','writer-field-mask','writer-small-output','writer-short-index','writer-hidden-raw','writer-reserved-field']:
 reject(mode,canonical,mode)
# Every record's reserved shape bits/IDs are exercised with a correct envelope.
for kind,ident,offset,value,name in [
 (4,50,8,4,'unknown page flags'),(5,90,8,4,'unknown card flags'),
 (4,50,8,3,'conflicting page provenance'),(5,90,8,3,'conflicting card provenance'),
 (4,50,8,2,'absent page cannot have an order'),(5,90,8,2,'absent card cannot have fields'),
 (5,90,6,0x8000,'unknown card field'),(3,20,2,0,'empty entity reference'),
 (4,50,6,65535,'reserved order reference'),(5,90,11,20,'wrong field reference kind')]:
 rows=copy.deepcopy(records);row=next(r for r in rows if r['kind']==kind and r.get('id',0)==ident)
 body=bytearray.fromhex(row['body_hex'])
 if kind in (4,5) and offset==8:body[offset]=value
 else:struct.pack_into('<H',body,offset,value)
 row['body_hex']=body.hex();reject(name,wire(rows))
for missing in [1,4]:
 rows=[r for r in records if not(r['kind']==missing and r.get('id',0)==0)];reject('missing required root/profile '+str(missing),wire(rows))
rows=copy.deepcopy(records);rows.append({'kind':2,'id':100,'body_hex':(struct.pack('<H',100)+b'k'*64).hex()});rows.append({'kind':7,'id':100,'body_hex':struct.pack('<HH',100,0).hex()});reject('setting key boundary',wire(rows))
bulk=copy.deepcopy(records)
for ident in range(100,1100):bulk.append({'kind':2,'id':ident,'body_hex':(struct.pack('<H',ident)+b'x').hex()})
accept('large dictionary uses a supplied index',wire(bulk))
change(bulk,5,90,21,65534);reject('late broken reference in a large dictionary',wire(bulk))
sparse=copy.deepcopy(records);change(sparse,5,11,4,65535);accept('slot values are positions, not reserved identities',wire(sparse))
rootflags=copy.deepcopy(records);row=next(r for r in rootflags if r['kind']==4 and r['id']==0);body=bytearray.fromhex(row['body_hex']);body[8]=1;row['body_hex']=body.hex();reject('root cannot claim child import spelling',wire(rootflags))
presence=[{'kind':1,'body_hex':b'a'.hex()},
 {'kind':2,'id':1,'body_hex':(struct.pack('<H',1)+b'subpage').hex()},
 {'kind':4,'id':0,'body_hex':struct.pack('<HHHHB',0,65535,65535,0,0).hex()},
 {'kind':5,'id':10,'body_hex':struct.pack('<HHHHBH',10,0,1,64,0,1).hex()},
 {'kind':4,'id':50,'body_hex':struct.pack('<HHHHB',50,0,10,0,2).hex()},
 {'kind':6,'id':10,'body_hex':struct.pack('<HH',10,50).hex()},
 {'kind':5,'id':11,'body_hex':struct.pack('<HHHHB',11,0,2,0,2).hex()},
 {'kind':4,'id':51,'body_hex':struct.pack('<HHHHB',51,0,11,0,0).hex()}]
accept('missing stored owner and implicit empty target have distinct provenance',wire(presence))

if __name__ == '__main__':
 print(json.dumps([{'name':n,'hex':d.hex(),'valid':v} for n,d,v,m in cases if not m]))
