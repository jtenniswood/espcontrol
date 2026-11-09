from pathlib import Path
import argparse
import re
parser=argparse.ArgumentParser();parser.add_argument("output");args=parser.parse_args()
root=Path(__file__).resolve().parents[2]
def function(text,marker):
 start=text.index(marker);opening=text.index("{",start);depth=1;cursor=opening+1
 while depth:
  if text[cursor]=="{":depth+=1
  elif text[cursor]=="}":depth-=1
  cursor+=1
 return text[start:cursor]
app=(root/"components/espcontrol/espcontrol_app.h").read_text()
cpp=(root/"components/espcontrol/espcontrol_app.cpp").read_text()
parts={"@APP_TEMPLATE@":function(app,"  template <typename Text, typename... Subpages>"),"@APP_SETTER@":function(cpp,"void EspControlApp::set_panel_config_button(").replace("EspControlApp::","MiniApp::"),"@APP_EMPTY@":function(cpp,"void EspControlApp::set_panel_config_text_empty_default(").replace("EspControlApp::","MiniApp::"),"@ALLOCATION_OPERATORS@":re.search(r"(static\s+void\s*\*\s*operator\s+new\s*\(.*?)(?=^\s*NativeConfigurationRuntime\s*\()",cpp,re.M|re.S).group(1)}
result=(root/"tests/firmware/panel_config_text_storage_test.cpp.in").read_text()
for token,value in parts.items():
 assert result.count(token)==1,token
 result=result.replace(token,value)
Path(args.output).write_text(result)
