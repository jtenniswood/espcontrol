const MAGIC = 0x31505745;
const MAX_SIDE = 480;
const HEADER_BYTES = 16;
const MEDIA_TYPE = "application/vnd.espcontrol.wallpaper";

export function wallpaperDimensions(width: number, height: number): { width: number; height: number } {
  if (!Number.isFinite(width) || !Number.isFinite(height) || width <= 0 || height <= 0)
    throw new Error("This image has invalid dimensions.");
  const scale = Math.min(1, MAX_SIDE / Math.max(width, height));
  return { width: Math.max(1, Math.round(width * scale)), height: Math.max(1, Math.round(height * scale)) };
}

export function encodeWallpaper(width: number, height: number, rgba: Uint8ClampedArray): ArrayBuffer {
  if (!Number.isInteger(width) || !Number.isInteger(height) || width < 1 || height < 1 ||
      width > MAX_SIDE || height > MAX_SIDE || rgba.length !== width * height * 4)
    throw new Error("Invalid wallpaper pixels.");
  const buffer = new ArrayBuffer(HEADER_BYTES + width * height * 2);
  const view = new DataView(buffer);
  [MAGIC, width, height, width * height * 2].forEach((value, index) => view.setUint32(index * 4, value, true));
  for (let i = 0; i < width * height; i++) {
    const offset = i * 4;
    const rgb565 = ((rgba[offset]! >> 3) << 11) | ((rgba[offset + 1]! >> 2) << 5) | (rgba[offset + 2]! >> 3);
    view.setUint16(HEADER_BYTES + i * 2, rgb565, true);
  }
  return buffer;
}

export function decodeWallpaper(buffer: ArrayBuffer): { width: number; height: number; rgba: Uint8ClampedArray } {
  if (buffer.byteLength < HEADER_BYTES) throw new Error("Invalid wallpaper response.");
  const view = new DataView(buffer);
  const width = view.getUint32(4, true), height = view.getUint32(8, true);
  if (view.getUint32(0, true) !== MAGIC || width < 1 || height < 1 || width > MAX_SIDE || height > MAX_SIDE ||
      view.getUint32(12, true) !== width * height * 2 || buffer.byteLength !== HEADER_BYTES + width * height * 2)
    throw new Error("Invalid wallpaper response.");
  const rgba = new Uint8ClampedArray(width * height * 4);
  for (let i = 0; i < width * height; i++) {
    const rgb = view.getUint16(HEADER_BYTES + i * 2, true);
    rgba[i * 4] = Math.round(((rgb >> 11) & 31) * 255 / 31);
    rgba[i * 4 + 1] = Math.round(((rgb >> 5) & 63) * 255 / 63);
    rgba[i * 4 + 2] = Math.round((rgb & 31) * 255 / 31);
    rgba[i * 4 + 3] = 255;
  }
  return { width, height, rgba };
}

export function wallpaperCardColor(hex: string, enabled: boolean): string {
  return "#" + hex + (enabled ? "CC" : ""); // 80% fill, matching LVGL's 204/255.
}

function wallpaperCanvas(document: Document, buffer: ArrayBuffer): HTMLCanvasElement {
  const { width, height, rgba } = decodeWallpaper(buffer);
  const canvas = document.createElement("canvas");
  canvas.width = width; canvas.height = height;
  const context = canvas.getContext("2d");
  if (!context) throw new Error("Your browser cannot prepare a wallpaper.");
  const image = context.createImageData(width, height);
  image.data.set(rgba);
  context.putImageData(image, 0, 0);
  return canvas;
}

async function prepareWallpaper(document: Document, file: File): Promise<ArrayBuffer> {
  if (!["image/jpeg", "image/png", "image/webp"].includes(file.type))
    throw new Error("Choose a JPEG, PNG or WebP image.");
  if (file.size > 20 * 1024 * 1024) throw new Error("Choose an image smaller than 20 MB.");
  const url = URL.createObjectURL(file);
  const image = document.createElement("img");
  try {
    await new Promise<void>((resolve, reject) => {
      image.onload = () => resolve();
      image.onerror = () => reject(new Error("This image could not be opened. Try another file."));
      image.src = url;
    });
    const { width, height } = wallpaperDimensions(image.naturalWidth, image.naturalHeight);
    const canvas = document.createElement("canvas");
    canvas.width = width; canvas.height = height;
    const context = canvas.getContext("2d");
    if (!context) throw new Error("Your browser cannot prepare a wallpaper.");
    context.fillStyle = "#000"; // Flatten transparent source images before RGB565 conversion.
    context.fillRect(0, 0, width, height);
    context.drawImage(image, 0, 0, width, height);
    return encodeWallpaper(width, height, context.getImageData(0, 0, width, height).data);
  } finally { URL.revokeObjectURL(url); }
}

export function buildWallpaperSettings(
  document: Document, request: typeof fetch, onChange: (url: string) => void,
): HTMLElement {
  const body = document.createElement("div");
  body.className = "sp-field";
  const label = document.createElement("label");
  label.className = "sp-field-label";
  label.htmlFor = "sp-set-wallpaper";
  label.textContent = "Wallpaper";
  const upload = document.createElement("input");
  upload.id = "sp-set-wallpaper";
  upload.type = "file";
  upload.accept = "image/jpeg,image/png,image/webp";
  const remove = document.createElement("button");
  remove.type = "button";
  remove.className = "sp-fw-btn";
  remove.textContent = "Remove wallpaper";
  const status = document.createElement("div");
  status.className = "sp-field-hint";
  status.setAttribute("role", "status");
  status.textContent = "Loading wallpaper…";
  body.append(label, upload, remove, status);
  let hasWallpaper = false, supported = false, busy = false;
  function sync() { upload.disabled = !supported || busy; remove.disabled = !supported || busy || !hasWallpaper; }
  function apply(buffer: ArrayBuffer | null) {
    const url = buffer ? wallpaperCanvas(document, buffer).toDataURL("image/png") : "";
    hasWallpaper = !!buffer;
    onChange(url);
    status.textContent = buffer ? "Wallpaper saved. Cards use an 80% fill. Changes apply immediately."
      : "JPEG, PNG or WebP. Saved on this display; images are resized automatically. Changes apply immediately.";
  }
  async function load() {
    try {
      let capabilityResponse: Response | undefined;
      for (let attempt = 0; attempt < 4; attempt++) {
        capabilityResponse = await request("/api/v1/capabilities", { credentials: "include", cache: "no-store" });
        if ((capabilityResponse.status === 404 || capabilityResponse.status === 503) && attempt < 3) {
          await new Promise(resolve => setTimeout(resolve, 2000));
          continue;
        }
        break;
      }
      const capabilities = capabilityResponse?.ok ? await capabilityResponse.json() : null;
      if (capabilities?.wallpaper?.version !== 1 || capabilities.wallpaper.available !== true) {
        status.textContent = "Wallpaper requires firmware with wallpaper support and image storage.";
        return;
      }
      const response = await request("/api/v1/wallpaper", { credentials: "include", cache: "no-store" });
      if (response.status === 404 || response.status === 501) {
        status.textContent = "Wallpaper requires firmware with wallpaper support and image storage.";
        return;
      }
      if (!response.ok) throw new Error("Could not load wallpaper. Reload the page to try again.");
      apply(response.status === 204 ? null : await response.arrayBuffer());
      supported = true;
    } catch (error) { status.textContent = error instanceof Error ? error.message : "Could not load wallpaper."; }
    finally { sync(); }
  }
  async function save(file: File | null) {
    busy = true; sync(); status.textContent = file ? "Uploading wallpaper…" : "Removing wallpaper…";
    try {
      const buffer = file ? await prepareWallpaper(document, file) : null;
      const payload = buffer ?? new ArrayBuffer(HEADER_BYTES);
      if (!buffer) new DataView(payload).setUint32(0, MAGIC, true);
      const response = await request("/api/v1/wallpaper", {
        method: "PUT", credentials: "include", cache: "no-store",
        headers: { "Content-Type": MEDIA_TYPE }, body: payload,
      });
      if (!response.ok) throw new Error("Could not save wallpaper. Check the connection and try again.");
      apply(buffer);
    } catch (error) { status.textContent = error instanceof Error ? error.message : "Could not save wallpaper."; }
    finally { busy = false; upload.value = ""; sync(); }
  }
  upload.addEventListener("change", () => { const file = upload.files?.[0]; if (file) void save(file); });
  remove.addEventListener("click", () => void save(null));
  sync(); void load();
  return body;
}
