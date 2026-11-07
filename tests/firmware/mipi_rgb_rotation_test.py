#!/usr/bin/env python3
"""Run the production RGB write/rotation path against a framebuffer API model."""
from pathlib import Path
import argparse
import os
import re
import shlex
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument("--idf-source-root", type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
production = (root / "components/mipi_rgb/mipi_rgb.cpp").read_text()
methods = "\n".join(re.search(r"^" + signature + r".*?^\}", production, re.M | re.S)[0] for signature in (
    r"bool MipiRgb::apply_panel_rotation_\(", r"void MipiRgb::write_to_display_\("))
metadata = (root / "components/mipi_rgb/display.py").read_text()
assert metadata.count("has_hardware_rotation=True") == 2
source = r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>
namespace display {
enum DisplayRotation { DISPLAY_ROTATION_0_DEGREES=0, DISPLAY_ROTATION_90_DEGREES=90,
 DISPLAY_ROTATION_180_DEGREES=180, DISPLAY_ROTATION_270_DEGREES=270 };
}
using esp_err_t=int;
constexpr int ESP_OK=0;
#define ESP_LOGE(...) ((void)0)
struct Panel {
 int width=7,height=5,swaps=0,mirrors=0,draws=0,fail=0;
 bool swap=false,mx=false,my=false;
 std::vector<uint16_t> pixels=std::vector<uint16_t>(35,0xaaaa);
};
int esp_lcd_panel_swap_xy(Panel *p,bool swap) {
 ++p->swaps;if(p->fail==1)return -1;p->swap=swap;return 0;
}
int esp_lcd_panel_mirror(Panel *p,bool mx,bool my) {
 ++p->mirrors;if(p->fail==2)return -1;p->mx=mx;p->my=my;return 0;
}
int esp_lcd_panel_draw_bitmap(Panel *p,int x1,int y1,int x2,int y2,const uint8_t *data) {
 ++p->draws;
 // Model IDF's documented swap/mirror framebuffer-copy contract.
 for(int y=y1;y<y2;++y)for(int x=x1;x<x2;++x) {
  int px=p->swap?y:x,py=p->swap?x:y;
  if(p->mx)px=p->width-1-px;
  if(p->my)py=p->height-1-py;
  uint16_t value;std::memcpy(&value,data+2*((y-y1)*(x2-x1)+x-x1),2);
  assert(px>=0&&px<p->width&&py>=0&&py<p->height);
  p->pixels.at(py*p->width+px)=value;
 }
 return 0;
}
struct MipiRgb {
 Panel *handle_;
 display::DisplayRotation rotation_{display::DISPLAY_ROTATION_0_DEGREES};
 display::DisplayRotation panel_rotation_{display::DISPLAY_ROTATION_0_DEGREES};
 bool panel_rotation_valid_{false};
 bool apply_panel_rotation_(display::DisplayRotation);
 void write_to_display_(int,int,int,int,const uint8_t*,int,int,int,bool=false);
};
''' + methods + r'''
void rectangle(display::DisplayRotation rotation,bool padded) {
 Panel panel;MipiRgb driver{&panel};driver.rotation_=rotation;
 const int w=3,h=2,x1=1,y1=1,xoff=padded?2:0,yoff=padded?1:0,pad=padded?3:0;
 const int stride=xoff+w+pad;
 std::vector<uint16_t> input(stride*(h+yoff)+8);
 for(size_t i=0;i<input.size();++i)input[i]=uint16_t(1000+i);
 std::vector<uint16_t> expected(35,0xaaaa);
 for(int y=0;y<h;++y)for(int x=0;x<w;++x) {
  int px=x1+x,py=y1+y,tx=px,ty=py;
  // Independent clockwise coordinate equations used by the previous LVGL path.
  switch(rotation) {
   case display::DISPLAY_ROTATION_90_DEGREES:tx=panel.width-1-py;ty=px;break;
   case display::DISPLAY_ROTATION_180_DEGREES:tx=panel.width-1-px;ty=panel.height-1-py;break;
   case display::DISPLAY_ROTATION_270_DEGREES:tx=py;ty=panel.height-1-px;break;
   default:break;
  }
  expected.at(ty*panel.width+tx)=input.at((y+yoff)*stride+x+xoff);
 }
 driver.write_to_display_(x1,y1,w,h,reinterpret_cast<const uint8_t*>(input.data()),xoff,yoff,pad);
 assert(panel.pixels==expected&&panel.swaps==1&&panel.mirrors==1);
 assert(panel.draws==(padded?h:1));
 driver.write_to_display_(x1,y1,w,h,reinterpret_cast<const uint8_t*>(input.data()),xoff,yoff,pad);
 assert(panel.pixels==expected&&panel.swaps==1&&panel.mirrors==1);
}
void fallback_and_failures() {
 Panel panel;MipiRgb driver{&panel};driver.rotation_=display::DISPLAY_ROTATION_180_DEGREES;
 uint16_t pixel=0x1234;
 driver.write_to_display_(1,1,1,1,reinterpret_cast<const uint8_t*>(&pixel),0,0,0);
 assert(panel.pixels[3*7+5]==pixel);
 // The generic pixel fallback has already transformed its backing buffer.
 pixel=0x5678;driver.write_to_display_(1,1,1,1,reinterpret_cast<const uint8_t*>(&pixel),0,0,0,true);
 assert(panel.pixels[1*7+1]==pixel&&!panel.swap&&!panel.mx&&!panel.my);
 driver.write_to_display_(1,1,1,1,reinterpret_cast<const uint8_t*>(&pixel),0,0,0);
 assert(panel.mx&&panel.my&&panel.swaps==3&&panel.mirrors==3);
 for(int failure:{1,2}) {
  panel.fail=failure;driver.rotation_=display::DISPLAY_ROTATION_90_DEGREES;
  driver.panel_rotation_valid_=false;int draws=panel.draws;
  driver.write_to_display_(1,1,1,1,reinterpret_cast<const uint8_t*>(&pixel),0,0,0);
  assert(panel.draws==draws&&!driver.panel_rotation_valid_);
  panel.fail=0;
  driver.write_to_display_(1,1,1,1,reinterpret_cast<const uint8_t*>(&pixel),0,0,0);
  assert(panel.draws==draws+1&&driver.panel_rotation_valid_&&panel.swap&&panel.mx&&!panel.my);
 }
}
int main() {
 for(auto r:{display::DISPLAY_ROTATION_0_DEGREES,display::DISPLAY_ROTATION_90_DEGREES,
             display::DISPLAY_ROTATION_180_DEGREES,display::DISPLAY_ROTATION_270_DEGREES})
  for(bool padded:{false,true})rectangle(r,padded);
 fallback_and_failures();
}
'''
if args.idf_source_root:
    header = args.idf_source_root / "components/esp_lcd/rgb/rgb_lcd_rotation_sw.h"
    assert header.is_file(), header
    source = source.replace("struct Panel {", "#include \"rgb_lcd_rotation_sw.h\"\nstruct Panel {\n uint32_t rotate_mask=0;")
    start = source.index("  int px=p->swap?y:x")
    begin = source.rfind(" // Model IDF", 0, start)
    end = source.index(" return 0;\n}", start)
    source = source[:begin] + r"""
 p->rotate_mask=(p->swap?ROTATE_MASK_SWAP_XY:0)|(p->mx?ROTATE_MASK_MIRROR_X:0)|(p->my?ROTATE_MASK_MIRROR_Y:0);
 auto *rgb_panel=p;int x_start=x1,y_start=y1,x_end=x2,y_end=y2;
 int h_res=p->width,v_res=p->height,bytes_per_pixel=2;
 uint32_t bytes_per_line=h_res*2,copy_bytes_per_line=(x_end-x_start)*2;
 auto *fb=reinterpret_cast<uint8_t*>(p->pixels.data());auto *to=fb;
 const uint8_t *from=data;size_t offset=y_start*copy_bytes_per_line+x_start*2;
 size_t bytes_to_flush=0;uint8_t *flush_ptr=nullptr;
 // Match IDF's warning policy for its mixed int/size_t loop indices.
 #pragma GCC diagnostic push
 #pragma GCC diagnostic ignored "-Wsign-compare"
 COPY_PIXEL_CODE_BLOCK(16)
 #pragma GCC diagnostic pop
 (void)bytes_to_flush;(void)flush_ptr;
""" + source[end:]

with tempfile.TemporaryDirectory(prefix="rgb-rotation-") as directory:
    temp = Path(directory)
    (temp / "test.cpp").write_text(source)
    extra = []
    if args.idf_source_root:
        (temp / "rgb_lcd_rotation_sw.h").write_bytes(header.read_bytes())
        (temp / "esp_bit_defs.h").write_text("#pragma once\n#define BIT(n) (1U << (n))\n")
        extra = ["-isystem", str(temp)]
    executable = temp / "test"
    subprocess.run(shlex.split(os.environ.get("CXX", "c++")) + [
        "-std=c++17", "-Wall", "-Wextra", "-Werror", *extra, str(temp / "test.cpp"), "-o", str(executable)
    ], check=True)
    subprocess.run([str(executable)], check=True)
print(("Pinned IDF pixel-copy macros: " if args.idf_source_root else "API model: ") + "RGB writes preserve all four rotations, padded subrectangles, fallback coordinates and failed-transform recovery")
