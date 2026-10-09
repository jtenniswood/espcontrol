#!/usr/bin/env python3
"""Regenerate the GIF fixtures with Pillow 12.3.0 (not a test dependency)."""
from pathlib import Path
import random
import struct

from PIL import Image

ROOT = Path(__file__).resolve().parent
random.seed(2175)
palette = [0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255] + [0, 0, 0] * 252
frames = []
for index in range(4):
    image = Image.new("P", (8, 8), 0)
    image.putpalette(palette)
    for y in range(2):
        for x in range(3):
            image.putpixel((x + index, y + index), index % 3 + 1)
    frames.append(image)
frames[0].save(
    ROOT / "disposal.gif",
    save_all=True,
    append_images=frames[1:],
    duration=[100, 200, 300, 400],
    loop=1,
    disposal=[1, 2, 3, 1],
    transparency=0,
    optimize=False,
)

# A full palette and random pixels exercise all LZW code widths and clears.
image = Image.new("P", (96, 64))
image.putpalette([value for index in range(256)
                  for value in (index, (index * 3) % 256, (index * 7) % 256)])
image.putdata([random.randrange(256) for _ in range(96 * 64)])
image.save(ROOT / "interlaced.gif", interlace=True)
image = Image.new("P", (2, 2), 1)
image.putpalette(palette)
image.save(ROOT / "static87.gif")

for path in sorted(ROOT.glob("*.gif")):
    with Image.open(path) as image:
        output = bytearray(struct.pack("<HHH", image.width, image.height, image.n_frames))
        for frame in range(image.n_frames):
            image.seek(frame)
            for red, green, blue in image.convert("RGB").get_flattened_data():
                pixel = ((red & 248) << 8) | ((green & 252) << 3) | (blue >> 3)
                output.extend(struct.pack("<H", pixel))
    path.with_suffix(".rgb565").write_bytes(output)
    print(path.name, len(path.read_bytes()), len(output))
