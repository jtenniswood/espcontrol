import { createColorSwatches } from "../components/color_swatches";
import type { ControlsFieldsFeature } from "./controls_fields";

const MAGIC = 0x31505745;
const MAX_SIDE = 480;
const JPEG_MAGIC = 0x32505745;
export interface WallpaperLimits { jpeg: boolean; maxSide: number; maxPixels: number; maxBytes: number }
const LEGACY_LIMITS: WallpaperLimits = { jpeg: false, maxSide: 480, maxPixels: 480 * 480, maxBytes: 16 + 480 * 480 * 2 };
export function wallpaperLimits(capability: unknown): WallpaperLimits {
  const value = capability as Record<string, unknown> | null;
  if (value?.jpeg !== true) return LEGACY_LIMITS;
  const maxSide = value.max_side, maxPixels = value.max_pixels, maxBytes = value.max_bytes;
  if (typeof maxSide !== "number" || !Number.isInteger(maxSide) || maxSide < 1 || maxSide > 1280 ||
      typeof maxPixels !== "number" || !Number.isInteger(maxPixels) || maxPixels < 1 || maxPixels > 1280 * 800 ||
      typeof maxBytes !== "number" || !Number.isInteger(maxBytes) || maxBytes <= 16 || maxBytes > 512 * 1024 - 16)
    throw new Error("Invalid wallpaper capabilities. Reload the page to try again.");
  return { jpeg: true, maxSide, maxPixels, maxBytes };
}
const HEADER_BYTES = 16;
const MEDIA_TYPE = "application/vnd.espcontrol.wallpaper";

export function wallpaperDimensions(width: number, height: number, limits: WallpaperLimits = LEGACY_LIMITS): { width: number; height: number } {
  if (!Number.isFinite(width) || !Number.isFinite(height) || width <= 0 || height <= 0)
    throw new Error("This image has invalid dimensions.");
  const scale = Math.min(1, limits.maxSide / Math.max(width, height), Math.sqrt(limits.maxPixels / (width * height)));
  return { width: Math.max(1, Math.floor(width * scale)), height: Math.max(1, Math.floor(height * scale)) };
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

export function encodeJpegWallpaper(width: number, height: number, jpeg: ArrayBuffer): ArrayBuffer {
  if (!Number.isInteger(width) || !Number.isInteger(height) || width < 1 || height < 1 ||
      width > 1280 || height > 1280 || width * height > 1280 * 800 || jpeg.byteLength < 4 ||
      jpeg.byteLength > 512 * 1024 - 32) throw new Error("Invalid wallpaper image.");
  const buffer = new ArrayBuffer(HEADER_BYTES + jpeg.byteLength);
  const view = new DataView(buffer);
  [JPEG_MAGIC, width, height, jpeg.byteLength].forEach((value, index) => view.setUint32(index * 4, value, true));
  new Uint8Array(buffer, HEADER_BYTES).set(new Uint8Array(jpeg));
  return buffer;
}
function wallpaperPreview(document: Document, buffer: ArrayBuffer): string {
  if (buffer.byteLength >= HEADER_BYTES && new DataView(buffer).getUint32(0, true) === JPEG_MAGIC) {
    const view = new DataView(buffer);
    const width = view.getUint32(4, true), height = view.getUint32(8, true);
    if (width < 1 || height < 1 || width > 1280 || height > 1280 || width * height > 1280 * 800 ||
        view.getUint32(12, true) !== buffer.byteLength - HEADER_BYTES || buffer.byteLength > 512 * 1024 - 16)
      throw new Error("Invalid wallpaper response.");
    // A data URL remains stable across reloads, without retaining object URLs.
    const bytes = new Uint8Array(buffer, HEADER_BYTES);
    let binary = "";
    for (let offset = 0; offset < bytes.length; offset += 8192)
      binary += String.fromCharCode(...bytes.subarray(offset, offset + 8192));
    return "data:image/jpeg;base64," + btoa(binary);
  }
  return wallpaperCanvas(document, buffer).toDataURL("image/png");
}

export function wallpaperCardColor(hex: string, enabled: boolean, opacityPercent = 80): string {
  const percent = Number.isFinite(opacityPercent) ? Math.max(0, Math.min(100, opacityPercent)) : 80;
  const alpha = Math.round(percent * 255 / 100).toString(16).padStart(2, "0").toUpperCase();
  return "#" + hex + (enabled ? alpha : "");
}

export function parseWallpaperOpacity(value: unknown): number {
  if (typeof value !== "number" || !Number.isInteger(value) || value < 0 || value > 100)
    throw new Error("Invalid card transparency response.");
  return value;
}

export function parseWallpaperOpacities(value: unknown, separate: boolean): { off: number; active: number } {
  const settings = value as Record<string, unknown> | null;
  const off = parseWallpaperOpacity(settings?.opacity);
  return { off, active: separate ? parseWallpaperOpacity(settings?.active_opacity) : off };
}

export interface BackgroundSettings { mode: "wallpaper" | "colour"; colour: string }
export function parseBackgroundSettings(value: unknown): BackgroundSettings {
  const settings = value as Record<string, unknown> | null;
  if ((settings?.mode !== "wallpaper" && settings?.mode !== "colour") ||
      typeof settings.colour !== "string" || !/^[0-9a-f]{6}$/i.test(settings.colour))
    throw new Error("Invalid background response. Reload the page to try again.");
  return { mode: settings.mode, colour: settings.colour.toUpperCase() };
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

async function prepareWallpaper(document: Document, file: File, limits: WallpaperLimits): Promise<ArrayBuffer> {
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
    let { width, height } = wallpaperDimensions(image.naturalWidth, image.naturalHeight, limits);
    const canvas = document.createElement("canvas");
    const context = canvas.getContext("2d");
    if (!context) throw new Error("Your browser cannot prepare a wallpaper.");
    for (;;) {
      canvas.width = width; canvas.height = height;
      context.imageSmoothingEnabled = true;
      context.imageSmoothingQuality = "high";
      context.fillStyle = "#000"; // Flatten transparent source images.
      context.fillRect(0, 0, width, height);
      context.drawImage(image, 0, 0, width, height);
      if (!limits.jpeg) return encodeWallpaper(width, height, context.getImageData(0, 0, width, height).data);
      // Keep resolution first, using smaller JPEGs for typical photos.
      // Reduce dimensions only after stronger compression cannot fit.
      for (const quality of [0.75, 0.7, 0.65, 0.6]) {
        const jpeg = await new Promise<Blob>((resolve, reject) => canvas.toBlob(
          blob => blob ? resolve(blob) : reject(new Error("Your browser cannot prepare a wallpaper.")), "image/jpeg", quality));
        if (jpeg.type !== "image/jpeg") throw new Error("Your browser cannot prepare a JPEG wallpaper.");
        if (jpeg.size + HEADER_BYTES <= limits.maxBytes)
          return encodeJpegWallpaper(width, height, await jpeg.arrayBuffer());
      }
      if (width === 1 && height === 1) throw new Error("This image cannot fit on the display.");
      width = Math.max(1, Math.floor(width * 0.85)); height = Math.max(1, Math.floor(height * 0.85));
    }
  } finally { URL.revokeObjectURL(url); }
}

export function buildWallpaperSettings(
  document: Document, request: typeof fetch, onChange: (url: string, opacity: number, activeOpacity: number, colour: string) => void,
  fields: Pick<ControlsFieldsFeature, "createRangeSlider" | "segmentControl">,
): HTMLElement {
  const body = document.createElement("div");
  body.className = "sp-field";
  let background: BackgroundSettings = { mode: "wallpaper", colour: "212121" };
  let backgroundSupported = false;
  const tabs = fields.segmentControl([["wallpaper", "Wallpaper"], ["colour", "Colour"]], "wallpaper", (mode: string) => {
    void saveBackground({ mode: mode as BackgroundSettings["mode"], colour: background.colour });
  });
  tabs.segment.id = "sp-set-background-mode";
  tabs.segment.setAttribute("role", "tablist");
  tabs.segment.setAttribute("aria-label", "Background type");
  for (const mode of ["wallpaper", "colour"]) {
    tabs.buttons[mode].id = "sp-background-tab-" + mode;
    tabs.buttons[mode].setAttribute("role", "tab");
    tabs.buttons[mode].setAttribute("aria-controls", "sp-background-" + mode);
  }
  const colourField = document.createElement("div");
  colourField.id = "sp-background-colour";
  colourField.setAttribute("role", "tabpanel");
  colourField.setAttribute("aria-labelledby", "sp-background-tab-colour");
  const swatches = createColorSwatches(() => background.colour, hex => {
    void saveBackground({ mode: "colour", colour: hex });
  }, "Background colour presets");
  colourField.appendChild(swatches);
  const backgroundStatus = document.createElement("div");
  backgroundStatus.className = "sp-field-hint";
  backgroundStatus.setAttribute("role", "status");
  const imageField = document.createElement("div");
  imageField.className = "sp-field sp-card-bg-field";
  imageField.id = "sp-background-wallpaper";
  imageField.setAttribute("role", "tabpanel");
  imageField.setAttribute("aria-labelledby", "sp-background-tab-wallpaper");
  const upload = document.createElement("input");
  upload.id = "sp-set-wallpaper";
  upload.type = "file";
  upload.setAttribute("aria-label", "Upload Image");
  upload.accept = "image/jpeg,image/png,image/webp";
  upload.className = "sp-card-bg-file";
  upload.hidden = true;
  const preview = document.createElement("div");
  preview.id = "sp-wallpaper-preview";
  preview.className = "sp-wallpaper-preview";
  preview.setAttribute("role", "img");
  preview.setAttribute("aria-label", "No wallpaper uploaded");
  const previewGrid = document.createElement("div");
  previewGrid.className = "sp-wallpaper-preview-grid";
  previewGrid.setAttribute("aria-hidden", "true");
  preview.appendChild(previewGrid);
  const actions = document.createElement("div");
  actions.className = "sp-card-bg-actions";
  const uploadButton = document.createElement("button");
  uploadButton.type = "button";
  uploadButton.className = "sp-action-btn";
  uploadButton.innerHTML = '<span class="mdi mdi-upload" aria-hidden="true"></span><span>Upload Image</span>';
  uploadButton.addEventListener("click", () => upload.click());
  const remove = document.createElement("button");
  remove.type = "button";
  remove.className = "sp-action-btn sp-wallpaper-delete";
  remove.innerHTML = '<span class="mdi mdi-delete-outline" aria-hidden="true"></span>';
  remove.setAttribute("aria-label", "Delete wallpaper");
  remove.title = "Delete wallpaper";
  actions.append(upload, uploadButton, remove);
  const status = document.createElement("div");
  status.className = "sp-field-hint";
  status.setAttribute("role", "status");
  status.textContent = "Loading wallpaper…";
  const control = fields.createRangeSlider("Standard Card Transparency", 20, () => { void saveOpacity(); });
  const activeControl = fields.createRangeSlider("Active Card Transparency", 2, () => { void saveOpacity(); });
  const activeRange = activeControl.range as HTMLInputElement;
  activeRange.id = "sp-set-active-card-transparency";
  activeRange.min = "0"; activeRange.max = "100"; activeRange.step = "1";
  activeControl.wrap.querySelector("label").htmlFor = activeRange.id;
  activeControl.wrap.hidden = true;
  const range = control.range as HTMLInputElement;
  range.id = "sp-set-card-transparency";
  range.min = "0"; range.max = "100"; range.step = "1";
  control.wrap.querySelector("label").htmlFor = range.id;
  const hint = document.createElement("div");
  hint.className = "sp-info-panel sp-wallpaper-info";
  hint.setAttribute("role", "note");
  const hintIcon = document.createElement("span");
  hintIcon.className = "mdi mdi-information-outline";
  hintIcon.setAttribute("aria-hidden", "true");
  const hintText = document.createElement("span");
  hintText.textContent = "0% is solid; 100% is clear. Text and icons stay visible.";
  hint.append(hintIcon, hintText);
  imageField.append(actions, status);
  body.append(tabs.segment, preview, imageField, colourField, backgroundStatus, hint, control.wrap, activeControl.wrap);
  let hasWallpaper = false, supported = false, busy = false, opacitySupported = false;
  let opacity = 80, activeOpacity = 98, savedOpacity = 80, savedActiveOpacity = 98, previewUrl = "";
  let wallpaperLabel = "No wallpaper uploaded";
  let separateOpacity = false;
  let limits = LEGACY_LIMITS;
  function sync() {
    upload.disabled = !supported || busy;
    uploadButton.disabled = upload.disabled;
    imageField.setAttribute("aria-busy", String(busy));
    remove.disabled = !supported || busy || !hasWallpaper;
    const hasBackground = background.mode === "colour" || hasWallpaper;
    imageField.hidden = background.mode !== "wallpaper";
    colourField.hidden = background.mode !== "colour";
    for (const mode of ["wallpaper", "colour"]) {
      tabs.buttons[mode].disabled = busy || (mode === "colour" && !backgroundSupported);
      tabs.buttons[mode].classList.toggle("active", background.mode === mode);
      tabs.buttons[mode].setAttribute("aria-selected", String(background.mode === mode));
      tabs.buttons[mode].tabIndex = background.mode === mode ? 0 : -1;
    }
    for (const input of Array.from(swatches.querySelectorAll<HTMLButtonElement | HTMLInputElement>("button,input")))
      input.disabled = !backgroundSupported || busy;
    swatches._syncColor();
    hint.hidden = !hasBackground;
    control.wrap.hidden = !hasBackground;
    activeControl.wrap.hidden = !hasBackground || !separateOpacity;
    range.disabled = !opacitySupported || busy || !hasBackground;
    activeRange.disabled = !separateOpacity || !opacitySupported || busy || !hasBackground;
    activeRange.value = String(hasBackground ? 100 - activeOpacity : 0);
    activeControl.val.textContent = activeRange.value + "%";
    range.value = String(hasBackground ? 100 - opacity : 0);
    control.val.textContent = range.value + "%";
  }
  range.addEventListener("input", () => {
    opacity = 100 - Number(range.value);
    if (!separateOpacity) activeOpacity = opacity;
    updatePreview();
  });
  activeRange.addEventListener("input", () => {
    activeOpacity = 100 - Number(activeRange.value);
    updatePreview();
  });
  function updatePreview() {
    const colourMode = background.mode === "colour";
    const enabled = colourMode || hasWallpaper;
    preview.style.backgroundImage = colourMode ? "none" : previewUrl ? `url("${previewUrl}")` : "";
    preview.style.backgroundColor = colourMode ? "#" + background.colour : "";
    preview.setAttribute("aria-label", colourMode ? "Background colour #" + background.colour : wallpaperLabel);
    onChange(colourMode ? "" : previewUrl, enabled ? opacity : 100, enabled ? activeOpacity : 100,
      colourMode ? background.colour : "");
  }
  function apply(buffer: ArrayBuffer | null) {
    previewUrl = buffer ? wallpaperPreview(document, buffer) : "";
    hasWallpaper = !!buffer;
    preview.style.backgroundImage = previewUrl ? `url("${previewUrl}")` : "";
    wallpaperLabel = buffer
      ? `Saved wallpaper, ${new DataView(buffer).getUint32(4, true)} by ${new DataView(buffer).getUint32(8, true)} pixels`
      : "No wallpaper uploaded";
    updatePreview();
    status.textContent = buffer ? `Wallpaper saved (${new DataView(buffer).getUint32(4, true)} × ${new DataView(buffer).getUint32(8, true)}). Changes apply immediately.`
      : "";
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
      if (capabilities?.background?.version === 1) {
        const settings = await request("/api/v1/background", { credentials: "include", cache: "no-store" });
        if (!settings.ok) throw new Error("Could not load background. Reload the page to try again.");
        background = parseBackgroundSettings(await settings.json());
        backgroundSupported = true;
        updatePreview();
      }
      if (capabilities?.wallpaper?.version !== 1) {
        status.textContent = "Wallpaper requires firmware with wallpaper support and image storage.";
        return;
      }
      limits = wallpaperLimits(capabilities.wallpaper);
      separateOpacity = capabilities.wallpaper.state_opacity === true;
      if (!separateOpacity) control.wrap.querySelector("label").textContent = "Card transparency";
      if (capabilities.wallpaper.opacity === true) {
        const settings = await request("/api/v1/wallpaper-opacity", { credentials: "include", cache: "no-store" });
        if (!settings.ok) throw new Error("Could not load card transparency. Reload the page to try again.");
        const values = parseWallpaperOpacities(await settings.json(), separateOpacity);
        opacity = savedOpacity = values.off;
        activeOpacity = savedActiveOpacity = values.active;
        opacitySupported = true;
      } else {
        hintText.textContent = "Install updated firmware to adjust card transparency.";
        activeOpacity = opacity;
      }
      if (capabilities.wallpaper.available !== true) {
        status.textContent = "Wallpaper requires image storage on this display.";
        updatePreview();
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
    } catch (error) { backgroundStatus.textContent = error instanceof Error ? error.message : "Could not load background."; }
    finally { sync(); }
  }
  async function saveBackground(next: BackgroundSettings) {
    if (!backgroundSupported || busy) { sync(); return; }
    const previous = background;
    background = next;
    busy = true; sync(); updatePreview();
    backgroundStatus.textContent = "Saving background…";
    try {
      const response = await request("/api/v1/background", {
        method: "PUT", credentials: "include", cache: "no-store",
        headers: { "Content-Type": "application/json" }, body: JSON.stringify(next),
      });
      if (!response.ok) throw new Error("Could not save background. Check the connection and try again.");
      background = parseBackgroundSettings(await response.json());
      backgroundStatus.textContent = "";
    } catch (error) {
      background = previous;
      backgroundStatus.textContent = error instanceof Error ? error.message : "Could not save background.";
    } finally { busy = false; sync(); updatePreview(); }
  }
  tabs.segment.addEventListener("keydown", (event: KeyboardEvent) => {
    if (!["ArrowLeft", "ArrowRight", "Home", "End"].includes(event.key)) return;
    const mode = event.key === "Home" ? "wallpaper" : event.key === "End" ? "colour"
      : background.mode === "wallpaper" ? "colour" : "wallpaper";
    const button = tabs.buttons[mode] as HTMLButtonElement;
    if (button.disabled) return;
    event.preventDefault(); button.focus(); button.click();
  });
  async function save(file: File | null) {
    let submitted = false;
    busy = true; sync(); status.textContent = file ? "Uploading wallpaper…" : "Removing wallpaper…";
    try {
      const buffer = file ? await prepareWallpaper(document, file, limits) : null;
      const payload = buffer ?? new ArrayBuffer(HEADER_BYTES);
      if (!buffer) new DataView(payload).setUint32(0, MAGIC, true);
      submitted = true;
      const response = await request("/api/v1/wallpaper", {
        method: "PUT", credentials: "include", cache: "no-store",
        headers: { "Content-Type": MEDIA_TYPE }, body: payload,
      });
      if (!response.ok) throw new Error("Could not save wallpaper. Check the connection and try again.");
      apply(buffer);
    } catch (error) {
      // A single-slot write can remove the previous image before failing.
      // Reload only after a submitted request; invalid files never touch flash.
      if (submitted) {
        try {
          const response = await request("/api/v1/wallpaper", { credentials: "include", cache: "no-store" });
          if (response.ok) apply(response.status === 204 ? null : await response.arrayBuffer());
        } catch { /* Keep the last known preview if the display cannot be reached. */ }
      }
      status.textContent = error instanceof Error ? error.message : "Could not save wallpaper.";
    }
    finally { busy = false; upload.value = ""; sync(); }
  }
  async function saveOpacity() {
    busy = true; sync(); backgroundStatus.textContent = "Saving card transparency…";
    try {
      const response = await request("/api/v1/wallpaper-opacity", {
        method: "PUT", credentials: "include", cache: "no-store",
        headers: { "Content-Type": "application/json" }, body: JSON.stringify(separateOpacity ? { opacity, active_opacity: activeOpacity } : { opacity }),
      });
      if (!response.ok) throw new Error("Could not save card transparency. Check the connection and try again.");
      const values = parseWallpaperOpacities(await response.json(), separateOpacity);
      opacity = savedOpacity = values.off;
      activeOpacity = savedActiveOpacity = values.active;
      updatePreview();
      backgroundStatus.textContent = "";
    } catch (error) {
      opacity = savedOpacity;
      activeOpacity = savedActiveOpacity;
      updatePreview();
      backgroundStatus.textContent = error instanceof Error ? error.message : "Could not save card transparency.";
    } finally { busy = false; sync(); }
  }
  upload.addEventListener("change", () => { const file = upload.files?.[0]; if (file) void save(file); });
  remove.addEventListener("click", () => void save(null));
  sync(); void load();
  return body;
}
