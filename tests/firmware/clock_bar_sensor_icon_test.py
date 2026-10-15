#!/usr/bin/env python3
"""Exercise production sensor icon sizing with different text and icon fonts."""
import os
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
header = (root / 'components/espcontrol/editable_clock_bar.h').read_text()
helpers = header[header.index('inline int text_width('):header.index('inline void refresh(')]
program = r'''
#include <algorithm>
#include <cassert>
#include <string>
struct lv_font_t { int width, height; };
struct lv_point_t { int x, y; };
struct lv_obj_t {
 const lv_font_t *font;
 int color = 0, width = 0, height = 0, align = 0, x = 0;
 std::string text{};
};
constexpr int LV_PART_MAIN = 0, LV_COORD_MAX = 32767, LV_TEXT_FLAG_NONE = 0;
constexpr int LV_ALIGN_LEFT_MID = 1, LV_ALIGN_CENTER = 2;
const lv_font_t *lv_obj_get_style_text_font(lv_obj_t *o, int) { return o->font; }
int lv_obj_get_style_text_letter_space(lv_obj_t *, int) { return 0; }
int lv_obj_get_style_text_color(lv_obj_t *o, int) { return o->color; }
int lv_font_get_line_height(const lv_font_t *f) { return f->height; }
void lv_obj_set_style_text_font(lv_obj_t *o, const lv_font_t *f, int) { o->font = f; }
void lv_obj_set_style_text_color(lv_obj_t *o, int color, int) { o->color = color; }
void lv_text_get_size(lv_point_t *p, const char *, const lv_font_t *f, int, int, int, int) { p->x = f->width; p->y = f->height; }
void lv_obj_set_size(lv_obj_t *o, int width, int height) { o->width = width; o->height = height; }
void lv_obj_set_width(lv_obj_t *o, int width) { o->width = width; }
void lv_obj_align(lv_obj_t *o, int align, int x, int) { o->align = align; o->x = x; }
const char *lv_label_get_text(lv_obj_t *o) { return o->text.c_str(); }
struct Widget { lv_obj_t *label, *sensor_icon; int sensor_icon_width = 0; };
'''
program += helpers + r'''
int main() {
 const lv_font_t text_font{65, 33}, icon_font{26, 36};
 lv_obj_t text{&text_font}, icon{&text_font}, prototype{&icon_font};
 text.color = 0x333333;
 Widget widget{&text, &icon};
 assert(sensor_icon_prefix(widget, &prototype, 30) == 30);
 assert(icon.font == &icon_font && text.font == &text_font);
 assert(icon.color == text.color && icon.height == 30 && icon.width == 26);
 assert(icon.x == 0 && icon.align == LV_ALIGN_LEFT_MID);
 size_content(widget, 100);
 assert(text.width == 70 && text.x == 30 && text.align == LV_ALIGN_LEFT_MID);
 assert(icon.x + icon.width < text.x && text.x + text.width == 100);
 size_content(widget, 20); assert(text.width == 0);
 text.color = 0xffffff;
 assert(sensor_icon_prefix(widget, &prototype, 50) == 30);
 assert(icon.color == text.color && icon.height == 36);
 Widget plain{&text, nullptr};
 assert(sensor_icon_prefix(plain, &prototype, 30) == 0);
 size_content(plain, 100);
 assert(text.width == 100 && text.x == 0 && text.align == LV_ALIGN_CENTER);
}
'''
with tempfile.TemporaryDirectory(prefix='clock-bar-icons-') as directory:
    source = Path(directory) / 'test.cpp'
    binary = Path(directory) / 'test'
    source.write_text(program)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror', str(source), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('Sensor icons use the small icon font and theme color, reserve space left of text, and bound height.')
