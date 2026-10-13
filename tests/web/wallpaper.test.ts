import { wallpaperDimensions, encodeWallpaper, decodeWallpaper, wallpaperCardColor, parseWallpaperOpacity } from "../../src/webserver/application/wallpaper";

function equal(actual: unknown, expected: unknown): void {
  if (JSON.stringify(actual) !== JSON.stringify(expected)) throw new Error(`Expected ${JSON.stringify(expected)}, got ${JSON.stringify(actual)}`);
}
function rejects(action: () => unknown): void {
  try { action(); } catch { return; }
  throw new Error("Expected invalid wallpaper to be rejected");
}
export function runWallpaperTests(): void {
  equal(wallpaperDimensions(1200, 800), { width: 480, height: 320 });
  equal(wallpaperDimensions(480, 800), { width: 288, height: 480 });
  equal(wallpaperDimensions(32, 16), { width: 32, height: 16 });
  rejects(() => wallpaperDimensions(0, 4));
  rejects(() => wallpaperDimensions(Infinity, 4));
  const pixels = new Uint8ClampedArray([255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255]);
  const encoded = encodeWallpaper(3, 1, pixels);
  equal(Array.from(new Uint8Array(encoded).slice(16)), [0, 248, 224, 7, 31, 0]);
  const decoded = decodeWallpaper(encoded);
  equal(decoded.width, 3); equal(decoded.height, 1); equal(Array.from(decoded.rgba), Array.from(pixels));
  rejects(() => decodeWallpaper(encoded.slice(0, -1)));
  rejects(() => decodeWallpaper(new ArrayBuffer(16)));
  const corrupt = encoded.slice(0);
  new DataView(corrupt).setUint32(4, 481, true);
  rejects(() => decodeWallpaper(corrupt));
  rejects(() => encodeWallpaper(3, 2, pixels));
  rejects(() => encodeWallpaper(481, 1, new Uint8ClampedArray(481 * 4)));
  equal(wallpaperCardColor("313131", true), "#313131CC");
  equal(wallpaperCardColor("FFFFFF", false), "#FFFFFF");
  equal(wallpaperCardColor("313131", true, 0), "#31313100");
  equal(wallpaperCardColor("313131", true, 50), "#31313180");
  equal(wallpaperCardColor("313131", true, 100), "#313131FF");
  equal(wallpaperCardColor("313131", false, 0), "#313131");
  for (const opacity of [0, 50, 80, 100]) equal(parseWallpaperOpacity(opacity), opacity);
  for (const opacity of [-1, 101, 20.5, "80", false, null]) rejects(() => parseWallpaperOpacity(opacity));
}
