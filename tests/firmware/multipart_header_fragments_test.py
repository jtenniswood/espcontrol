"""Exercise production multipart header callbacks at every fragment boundary."""
from pathlib import Path
import os
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]
stubs={
"esphome/core/defines.h":"#define USE_ESP32\n#define USE_WEBSERVER_OTA\n",
"esphome/core/log.h":"#define ESP_LOGV(...)\n#define ESP_LOGW(...)\n#define ESP_LOGE(...)\n",
"esphome/core/helpers.h":"#pragma once\n#include <optional>\nnamespace esphome {using std::optional;}\n",
"esp_http_server.h":"#pragma once\nstruct httpd_req_t {};\n",
"multipart_parser.h":r'''#pragma once
#include <cstddef>
#include <cstdint>
struct multipart_parser;
using data_cb=int(*)(multipart_parser*,const char*,size_t);
using notify_cb=int(*)(multipart_parser*);
// The dependency is a C POD: production zeroes this struct with memset.
struct multipart_parser_settings {
  data_cb on_header_field, on_header_value, on_part_data;
  notify_cb on_part_data_begin, on_headers_complete, on_part_data_end, on_body_end;
};
struct multipart_parser{multipart_parser_settings settings;void*data{};};
extern multipart_parser*active_parser;
inline multipart_parser*multipart_parser_init(const char*,multipart_parser_settings*s){return active_parser=new multipart_parser{*s,nullptr};}
inline void multipart_parser_free(multipart_parser*p){delete p;}
inline void multipart_parser_set_data(multipart_parser*p,void*d){p->data=d;}
inline void*multipart_parser_get_data(multipart_parser*p){return p->data;}
inline size_t multipart_parser_execute(multipart_parser*,const char*,size_t n){return n;}
'''}
source=r'''
#include "multipart.cpp"
#include <cassert>
#include <iostream>
multipart_parser*active_parser=nullptr;
namespace esphome::web_server_idf {
bool str_ncmp_ci(const char*a,const char*b,size_t n){for(size_t i=0;i<n;++i)if(tolower(a[i])!=tolower(b[i]))return false;return true;}
const char*strcasestr_n(const char*a,size_t n,const char*b){size_t m=strlen(b);for(size_t i=0;i+m<=n;++i)if(str_ncmp_ci(a+i,b,m))return a+i;return nullptr;}
}
using namespace esphome::web_server_idf;
void fragments(data_cb callback,const std::string&text,size_t split){assert(callback(active_parser,text.data(),split)==0);assert(callback(active_parser,text.data()+split,text.size()-split)==0);}
int main(){
 const std::string field="Content-Disposition",value="form-data; name=\"update\"; filename=\"firmware.bin\"";
 for(size_t f=0;f<=field.size();++f)for(size_t v=0;v<=value.size();++v){
  MultipartReader reader("--boundary");auto&s=active_parser->settings;std::string data;reader.set_data_callback([&](const uint8_t*p,size_t n){data.append((const char*)p,n);});
  fragments(s.on_header_field,field,f);fragments(s.on_header_value,value,v);
  // Start a new header; the previous one must be finalized only now.
  fragments(s.on_header_field,"Content-Type",4);fragments(s.on_header_value,"application/octet-stream",8);
  s.on_headers_complete(active_parser);assert(reader.has_file()&&reader.get_current_part().filename=="firmware.bin");
  s.on_part_data(active_parser,"FIRMWARE",8);s.on_part_data_end(active_parser);assert(!reader.is_complete());
  s.on_body_end(active_parser);assert(reader.is_complete()&&data=="FIRMWARE");
 }
 // One byte fragments, final header without Content-Type, and a non-file part.
 {MultipartReader reader("--boundary");auto&s=active_parser->settings;
  for(char c:field)assert(s.on_header_field(active_parser,&c,1)==0);
  for(char c:value)assert(s.on_header_value(active_parser,&c,1)==0);
  s.on_headers_complete(active_parser);assert(reader.has_file());
  s.on_part_data_end(active_parser);s.on_body_end(active_parser);assert(!reader.is_complete()); // Empty file
 }
 {MultipartReader reader("--boundary");auto&s=active_parser->settings;
  fragments(s.on_header_field,field,5);fragments(s.on_header_value,"form-data; name=\"token\"",7);s.on_headers_complete(active_parser);s.on_part_data_end(active_parser);
  fragments(s.on_header_field,field,6);fragments(s.on_header_value,value,19);s.on_headers_complete(active_parser);assert(reader.has_file());s.on_part_data(active_parser,"123",3);s.on_part_data_end(active_parser);s.on_body_end(active_parser);assert(reader.is_complete());
 }
 {MultipartReader reader("--boundary");auto&s=active_parser->settings;std::string huge(4097,'a');assert(s.on_header_value(active_parser,huge.data(),huge.size())!=0);}
 {MultipartReader reader("--boundary");active_parser->settings.on_body_end(active_parser);assert(!reader.is_complete());}
}
'''
with tempfile.TemporaryDirectory() as temp:
    temp=Path(temp)
    for name,text in stubs.items():
        p=temp/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(text)
    cpp=temp/"test.cpp";exe=temp/"test";cpp.write_text(source)
    subprocess.run([os.environ.get("CXX","c++"),"-std=c++17","-Wall","-Wextra","-Werror","-Wno-unused-const-variable","-I"+str(temp),"-I"+str(ROOT/"components/web_server_idf"),str(cpp),"-o",str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
server=(ROOT/"components/web_server_idf/web_server_idf.cpp").read_text()
assert "if (!reader->is_complete())" in server
print("Multipart header fragmentation regression passed.")
