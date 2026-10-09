These synthetic GIFs and their expected composited RGB565 frames were generated
with Pillow 12.3.0. They contain no external image assets.

- `disposal.gif`: four transparent frames, local palettes, disposal methods
  1/2/3, individual delays, and one repeat after the initial playthrough.
- `interlaced.gif`: seeded random pixels and a 256-colour palette, exercising
  interlace passes, LZW dictionary growth and dictionary resets.
- `static87.gif`: a small GIF87a image without animation extensions.

Each `.rgb565` file starts with three little-endian 16-bit values (width, height,
frame count), followed by the complete composited frames in little-endian RGB565.
The decoder is compared against Pillow's independently composited output.
