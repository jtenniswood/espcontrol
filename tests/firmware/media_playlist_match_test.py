#!/usr/bin/env python3
"""Compare allocation-free playlist matching with production legacy helpers."""
import os
import shlex
import subprocess
import tempfile
from pathlib import Path
from generate_climate_update_test import definition

root = Path(__file__).resolve().parents[2]
media = (root / 'components/espcontrol/button_grid_media.h').read_text()
parser = (root / 'components/espcontrol/button_grid_config_parser.h').read_text()
helpers = definition(parser, 'trim_display_unit') + '\n'
for name in ['media_playlist_trim_text', 'media_playlist_lower_text', 'media_playlist_without_url_suffix',
             'media_playlist_leaf_id', 'media_playlist_content_id_matches', 'media_playlist_content_type_matches']:
    helpers += definition(media, name) + '\n'
source = r'''
#include <cassert>
#include <cstdlib>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
#include "media_playlist_match.h"
static size_t allocations=0;
void *operator new(size_t size){++allocations;if(void *p=std::malloc(size?size:1))return p;std::abort();}
void *operator new[](size_t size){return ::operator new(size);}
void operator delete(void *p)noexcept{std::free(p);}
void operator delete[](void *p)noexcept{std::free(p);}
void operator delete(void *p, std::size_t) noexcept { std::free(p); }
void operator delete[](void *p, std::size_t) noexcept { std::free(p); }
''' + helpers + r'''
void check(const std::string &left,const std::string &right) {
  const auto id=media_playlist_content_id_matches(left,right),type=media_playlist_content_type_matches(left,right);
  const auto before=allocations;
  if(espcontrol::media::playlist_content_id_matches(left,right)!=id){std::fprintf(stderr,"ID mismatch left=[%s] right=[%s] expected=%d\n",left.c_str(),right.c_str(),id);std::abort();}
  assert(espcontrol::media::playlist_content_type_matches(left,right)==type);
  assert(allocations==before);
}
int main() {
  const std::vector<std::string> values={""," ","abc","abcd","ABCDEFGH","spotify:playlist:abcd",
    " spotify:playlist:abcd/?v=1#end ","media-source://library/playlist/ABCD","https://host/library/playlist/abcd",
    "https://host/ABCDEFGH/extra","abc /?query","playlist","music","audio","track","MUSIC","  Audio ",
    "a/b/c/", "a:b:c:", "a?b#c", "a#b?c", "media-source://", "MEDIA-SOURCE://abcde ", u8"Bibliothèque:été"};
  for(const auto &left:values)for(const auto &right:values)check(left,right);
  uint32_t random=19;
  auto next=[&](){random=random*1664525+1013904223;return random;};
  const std::string chars=" abcdefghABCDEFGH:/:?#\t\n0123456789";
  for(size_t i=0;i<10000;++i) {
    std::string left=values[next()%values.size()],right=values[next()%values.size()];
    for(size_t j=next()%30;j>0;--j)left+=chars[next()%chars.size()];
    if(i%3)right=left;
    for(size_t j=next()%10;j>0;--j)right+=chars[next()%chars.size()];
    check(left,right);
  }
}
'''
with tempfile.TemporaryDirectory(prefix='espcontrol-media-match-') as directory:
    unit = Path(directory) / 'test.cpp'
    executable = Path(directory) / 'test'
    unit.write_text(source)
    subprocess.run(shlex.split(os.environ.get('CXX', 'clang++')) + ['-std=c++17', '-Wall', '-Wextra', '-Werror',
        '-I' + str(root / 'components/espcontrol'), str(unit), '-o', str(executable)], check=True)
    subprocess.run([str(executable)], check=True)
print('Playlist matching matches production helpers without C++ allocations.')
