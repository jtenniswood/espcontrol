import { wallpaperDimensions, encodeWallpaper, decodeWallpaper, wallpaperCardColor, parseWallpaperOpacity, parseWallpaperOpacities, wallpaperLimits, encodeJpegWallpaper, parseBackgroundSettings } from "../../src/webserver/application/wallpaper";

function equal(actual: unknown, expected: unknown): void {
  if (JSON.stringify(actual) !== JSON.stringify(expected)) throw new Error(`Expected ${JSON.stringify(expected)}, got ${JSON.stringify(actual)}`);
}
function rejects(action: () => unknown): void {
  try { action(); } catch { return; }
  throw new Error("Expected invalid wallpaper to be rejected");
}
export function runWallpaperTests(): void {
  equal(parseBackgroundSettings({mode: "wallpaper", colour: "212121"}), {mode: "wallpaper", colour: "212121"});
  equal(parseBackgroundSettings({mode: "colour", colour: "abcdef"}), {mode: "colour", colour: "ABCDEF"});
  for (const value of [null, {}, {mode: "color", colour: "ABCDEF"}, {mode: "colour", colour: "#ABCDEF"}, {mode: "wallpaper", colour: "ABCDEX"}, {mode: "colour", colour: 123456}])
    rejects(() => parseBackgroundSettings(value));
  equal(wallpaperDimensions(1200, 800), { width: 480, height: 320 });
  equal(wallpaperDimensions(480, 800), { width: 288, height: 480 });
  equal(wallpaperDimensions(32, 16), { width: 32, height: 16 });
  rejects(() => wallpaperDimensions(0, 4));
  rejects(() => wallpaperDimensions(Infinity, 4));
  const limits = wallpaperLimits({ jpeg: true, max_side: 1280, max_pixels: 1280 * 800, max_bytes: 524272 });
  equal(wallpaperDimensions(2560, 1600, limits), { width: 1280, height: 800 });
  equal(wallpaperDimensions(1200, 800, limits), { width: 1200, height: 800 });
  equal(wallpaperDimensions(100, 50, limits), { width: 100, height: 50 });
  const square = wallpaperDimensions(2000, 2000, limits);
  if (square.width * square.height > limits.maxPixels) throw new Error("Pixel budget exceeded");
  equal(wallpaperLimits({}), wallpaperLimits(null));
  rejects(() => wallpaperLimits({ jpeg: true, max_side: 4000, max_pixels: 1024000, max_bytes: 524272 }));
  rejects(() => wallpaperLimits({ jpeg: true, max_side: 1280, max_pixels: 1024001, max_bytes: 524272 }));
  rejects(() => wallpaperLimits({ jpeg: true, max_side: 1280, max_pixels: 1024000, max_bytes: 524273 }));
  const jpeg = new Uint8Array([255, 216, 255, 217]).buffer;
  const compressed = encodeJpegWallpaper(1280, 800, jpeg);
  equal(new DataView(compressed).getUint32(0, true), 0x32505745);
  equal(new DataView(compressed).getUint32(12, true), 4);
  equal(Array.from(new Uint8Array(compressed, 16)), [255, 216, 255, 217]);
  rejects(() => encodeJpegWallpaper(1280, 801, jpeg));
  rejects(() => encodeJpegWallpaper(1281, 1, jpeg));
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
  equal(parseWallpaperOpacities({ opacity: 40, active_opacity: 90 }, true), { off: 40, active: 90 });
  equal(parseWallpaperOpacities({ opacity: 65 }, false), { off: 65, active: 65 });
  equal(parseWallpaperOpacities({ opacity: 0, active_opacity: 100 }, true), { off: 0, active: 100 });
  for (const value of [null, { opacity: 40 }, { opacity: 40, active_opacity: 101 }, { opacity: 40, active_opacity: "90" }])
    rejects(() => parseWallpaperOpacities(value, true));
  for (const opacity of [0, 50, 80, 100]) equal(parseWallpaperOpacity(opacity), opacity);
  for (const opacity of [-1, 101, 20.5, "80", false, null]) rejects(() => parseWallpaperOpacity(opacity));
}
