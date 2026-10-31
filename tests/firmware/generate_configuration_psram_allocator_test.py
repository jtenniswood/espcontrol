"""Compile the real firmware configuration allocator against an ESP32 heap stub."""
import argparse
from pathlib import Path
parser=argparse.ArgumentParser();parser.add_argument('output',type=Path);args=parser.parse_args()
root=Path(__file__).resolve().parents[2];source=(root/'components/espcontrol/espcontrol_app.cpp').read_text()
def function(marker):
 start=source.index(marker);opening=source.index('{',start);depth=1;cursor=opening+1
 while depth:
  depth+=(source[cursor]=='{')-(source[cursor]=='}');cursor+=1
 return source[start:cursor]
text=Path(__file__).with_name('configuration_psram_allocator_test.cpp.in').read_text()
for marker,token in [('static uint8_t *allocate_configuration_slot(', '@ALLOCATE@'),('static void release_configuration_slot(', '@RELEASE@')]:text=text.replace(token,function(marker))
args.output.write_text(text)
