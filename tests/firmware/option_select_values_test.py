"""Check production option parsing and exact outgoing values."""
from pathlib import Path
import os
import json
import re
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]
text=(ROOT / 'components/espcontrol/button_grid_option_select.h').read_text()
def definition(name):
    matches=re.findall(r"^inline [^\n]*\b"+name+r"\([^;{]*\)\s*\{.*?^}",text,re.M|re.S)
    assert len(matches)==1,name
    return matches[0]+"\n"
source='#include <cassert>\n#include <string>\n#include <vector>\n#include <algorithm>\n#include <cctype>\n#include <utility>\nconstexpr int OPTION_SELECT_MAX_OPTIONS=64;\nconstexpr size_t OPTION_SELECT_OPTION_MAX_LEN=96;\nstd::string trim_display_unit(const std::string&v){auto a=v.find_first_not_of(" \\t\\r\\n");return a==std::string::npos?"":v.substr(a,v.find_last_not_of(" \\t\\r\\n")-a+1);}\n'
for name in ['option_select_trim', 'option_select_add_option', 'option_select_parse_delimited_options', 'option_select_parse_list_options', 'option_select_parse_options']:source+=definition(name)
python_options = ["NBSP\xa0room", "Zero\u200bwidth", "Private\ue000use", "Supplement\U000e0001tag", "Bell\aBack\bForm\fVertical\v", "Line\nTab\t", "Literal\\u200b", "Kid's \"room\""]
source += "int main(){\nauto decoded=option_select_parse_options(" + json.dumps(repr(python_options), ensure_ascii=False) + ");\n"
source += "const std::vector<std::string> python_expected={" + ",".join(json.dumps(v, ensure_ascii=False) for v in python_options) + "};assert(decoded==python_expected);\n"
source += r'''
 auto values=option_select_parse_options("['(Guest)', '[Auto]', '<Night>', '\\'Quoted\\'', '  Guest  ', 'Été', 'Kid\\'s', 'Sleep, Quiet', 'ECO', 'eco']");
 const std::vector<std::string> expected={"(Guest)","[Auto]","<Night>","'Quoted'","  Guest  ","Été","Kid's","Sleep, Quiet","ECO","eco"};assert(values==expected);
 const std::string cap_prefix="['first', 'second', ";
 std::string capped=cap_prefix;
 for(int i=0;i<65;++i){if(i)capped+=", ";capped+="'"+std::string(96,'a')+"'";}capped+="]";
 assert(option_select_parse_options(capped.substr(0,64*(96+4))).empty());
 for(const char*bad:{"['first', 'seco", "('first', 'second'", "[", "("})assert(option_select_parse_options(bad).empty());
 assert(option_select_parse_options(R"(["\uD83D\uDE00"])")[0]=="😀");
 for(const char*bad:{R"(['\xQ0'])",R"(['\u200'])",R"(['\U00110000'])",R"(['\uD800'])",R"(['\uDC00'])",R"(['\u0000'])",R"(['\q'])"})assert(option_select_parse_options(bad).empty());
 assert(option_select_parse_options("[]").empty());
 assert(option_select_parse_options("['unfinished]").empty());
 assert((option_select_parse_options("Home | Away")==std::vector<std::string>{"Home","Away"}));
 std::string long_option(97,'a');assert(option_select_parse_options("['"+long_option+"']").empty());
 assert((option_select_parse_options("['(Guest)', 'Guest']")==std::vector<std::string>{"(Guest)","Guest"}));
}'''
with tempfile.TemporaryDirectory() as temp:
    cpp=Path(temp)/"test.cpp";exe=Path(temp)/"test";cpp.write_text(source)
    subprocess.run([os.environ.get("CXX","c++"),"-std=c++17","-Wall","-Wextra","-Werror",str(cpp),"-o",str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print("option_select_values regression passed.")
