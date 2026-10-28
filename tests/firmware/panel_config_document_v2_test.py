"""Run the shared hostile corpus against the real allocation-free C++ reader."""
import argparse,subprocess
from pathlib import Path
from panel_config_document_v2_cases import cases,canonical,fixture,records,wire
parser=argparse.ArgumentParser();parser.add_argument('reader',type=Path);args=parser.parse_args()
for name,data,valid,mode in cases:
 result=subprocess.run([str(args.reader),*([mode] if mode else [])],input=data,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
 assert result.returncode==(0 if valid else 2),(name,result.returncode,result.stdout.decode(),result.stderr.decode())
 if valid:assert result.stdout.startswith(b'0 '),(name,result.stdout)
written=subprocess.run([str(args.reader),'writer-roundtrip'],input=canonical,check=True,stdout=subprocess.PIPE).stdout
assert written==wire(sorted(records,key=lambda r:(r['kind'],r.get('id',0))))
# Compare parsed fields/text with independent semantic data, not a writer roundtrip.
result=subprocess.run([str(args.reader)],input=canonical,check=True,stdout=subprocess.PIPE).stdout.decode().splitlines()
assert result[0]=='0 '+str(8*len(records))
parsed=[]
for line in result[1:]:
 parts=line.split(' ',17);numeric=list(map(int,parts[:17]));text=bytes.fromhex(parts[17]).decode() if len(parts)>17 else ''
 parsed.append((numeric,text))
for (numeric,text),expected in zip(parsed,sorted(records,key=lambda r:(r['kind'],r.get('id',0)))):
 values=[expected['kind'],expected.get('id',0),expected.get('first',0),expected.get('second',0),expected.get('third',0),expected.get('field_mask',0),expected.get('flags',0),expected.get('raw_source',0)]+[expected.get('fields',{}).get(str(i),0) for i in range(9)]
 assert numeric==values,(numeric,values)
 assert text==expected.get('text',''),(text,expected)
print('Native v2 structural reader:',len(cases),'golden/truncation/graph/bounds/workspace cases pass; service and durable format unchanged.')
