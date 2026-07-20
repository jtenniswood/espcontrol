import type { CardConfig } from "../contracts/types";
import type {
  BackupArchiveEntries,
  BackupArchiveEntry,
  BackupAssetProvider,
} from "./backup";

export interface CardImageItem {
  readonly id: string;
  readonly name: string;
  readonly size: number;
  readonly width?: number;
  readonly height?: number;
}

export interface CardImageLibraryInfo {
  readonly available: boolean;
  readonly requiresUsbFlash: boolean;
  readonly formatVersion: number;
  readonly maxActiveBackgrounds: number;
  readonly storageBytes: number;
  readonly usedBytes: number;
  readonly freeBytes: number;
  readonly maxBytes: number;
}

export interface CardImageHttpResponse {
  readonly ok: boolean;
  json(): Promise<unknown>;
  text(): Promise<string>;
  arrayBuffer(): Promise<ArrayBuffer>;
}

export interface CardImageHttpRequest {
  readonly method?: string;
  readonly headers?: Readonly<Record<string, string>>;
  readonly body?: unknown;
}

export interface CardImagesFeatureDependencies {
  readonly maxActiveBackgrounds: number;
  fetch(url: string, request?: CardImageHttpRequest): Promise<CardImageHttpResponse>;
  normalizeId(value: unknown): string;
  imageUrl(id: string): string;
  targetSize(): number;
  uploadMaxBytes(): number;
  minimumQuality(): number;
}

export interface OptimizedCardImage extends Blob {
  readonly optimizedWidth: number;
  readonly optimizedHeight: number;
  readonly optimizedQuality: number;
}

export interface CardImagesFeature {
  list(force?: boolean): Promise<CardImageItem[]>;
  info(): CardImageLibraryInfo;
  cachedImages(): CardImageItem[];
  replaceCachedImages(images: readonly CardImageItem[]): void;
  invalidate(): void;
  resize(file: Blob): Promise<OptimizedCardImage>;
  upload(file: Blob): Promise<CardImageItem>;
  uploadBytes(bytes: Uint8Array, failureMessage?: string): Promise<CardImageItem>;
  rename(id: unknown, name: unknown): Promise<CardImageItem>;
  delete(id: unknown): Promise<void>;
  readBytes(id: unknown): Promise<Uint8Array>;
}

export interface CardImageReferencePlan {
  readonly buttons?: readonly CardConfig[];
  readonly subpages?: Readonly<Record<string, { readonly buttons?: readonly CardConfig[] } | null | undefined>>;
}

export interface CardImageBackupDependencies {
  normalizeId(value: unknown): string;
  imageId(button: CardConfig): string;
  setImageId(button: CardConfig, id: string): void;
}

interface CardImageListPayload {
  readonly available?: unknown;
  readonly requires_usb_flash?: unknown;
  readonly format_version?: unknown;
  readonly max_active_backgrounds?: unknown;
  readonly storage_bytes?: unknown;
  readonly used_bytes?: unknown;
  readonly free_bytes?: unknown;
  readonly max_bytes?: unknown;
  readonly images?: unknown;
}

interface CardImageManifestItem {
  readonly id: string;
  readonly name: string;
  readonly file: string;
}

function integer(value: unknown): number {
  return parseInt(String(value), 10) || 0;
}

function imageItem(value: unknown, normalizeId: (value: unknown) => string): CardImageItem | null {
  if (!value || typeof value !== "object") return null;
  const raw = value as Record<string, unknown>;
  const id = normalizeId(raw.id);
  if (!id) return null;
  const width = integer(raw.width);
  const height = integer(raw.height);
  return {
    id,
    name: String(raw.name || id),
    size: Math.max(0, integer(raw.size)),
    ...(width > 0 ? { width } : {}),
    ...(height > 0 ? { height } : {}),
  };
}

export function createCardImagesFeature(dependencies: CardImagesFeatureDependencies): CardImagesFeature {
  let loaded = false;
  let images: CardImageItem[] = [];
  let libraryInfo: CardImageLibraryInfo = {
    available: false,
    requiresUsbFlash: false,
    formatVersion: 0,
    maxActiveBackgrounds: dependencies.maxActiveBackgrounds,
    storageBytes: 0,
    usedBytes: 0,
    freeBytes: 0,
    maxBytes: 0,
  };

  const invalidate = (): void => {
    loaded = false;
    images = [];
  };

  const list = async (force = false): Promise<CardImageItem[]> => {
    if (!force && loaded) return images.slice();
    const response = await dependencies.fetch("/api/card-images");
    if (!response.ok) throw new Error("Could not load images.");
    const payload = await response.json() as CardImageListPayload;
    const rawImages = Array.isArray(payload?.images) ? payload.images : [];
    images = rawImages
      .map((item) => imageItem(item, dependencies.normalizeId))
      .filter((item): item is CardImageItem => item !== null);
    libraryInfo = {
      available: !!payload?.available,
      requiresUsbFlash: !!payload?.requires_usb_flash,
      formatVersion: integer(payload?.format_version),
      maxActiveBackgrounds: integer(payload?.max_active_backgrounds) || dependencies.maxActiveBackgrounds,
      storageBytes: integer(payload?.storage_bytes),
      usedBytes: integer(payload?.used_bytes),
      freeBytes: integer(payload?.free_bytes),
      maxBytes: integer(payload?.max_bytes),
    };
    loaded = true;
    return images.slice();
  };

  const decodeUploadResponse = async (response: CardImageHttpResponse, fallback: string): Promise<CardImageItem> => {
    if (!response.ok) {
      const message = await response.text();
      throw new Error(message || fallback);
    }
    const item = imageItem(await response.json(), dependencies.normalizeId);
    if (!item) throw new Error(fallback);
    invalidate();
    return item;
  };

  const uploadBytes = (bytes: Uint8Array, failureMessage = "Could not upload image."): Promise<CardImageItem> =>
    dependencies.fetch("/api/card-images", {
      method: "POST",
      headers: { "Content-Type": "image/jpeg" },
      body: bytes,
    }).then((response) => decodeUploadResponse(response, failureMessage));

  const resize = (file: Blob): Promise<OptimizedCardImage> => new Promise((resolve, reject) => {
    if (!file?.type || !file.type.startsWith("image/")) {
      reject(new Error("Choose an image file."));
      return;
    }
    const targetSize = dependencies.targetSize();
    const maxBytes = dependencies.uploadMaxBytes();
    const minQuality = dependencies.minimumQuality();
    const source = new Image();
    const objectUrl = URL.createObjectURL(file);
    source.onload = (): void => {
      URL.revokeObjectURL(objectUrl);
      const canvas = document.createElement("canvas");
      canvas.width = targetSize;
      canvas.height = targetSize;
      const context = canvas.getContext("2d");
      if (!context) {
        reject(new Error("Could not optimize that image."));
        return;
      }
      context.imageSmoothingEnabled = true;
      context.imageSmoothingQuality = "high";
      const scale = Math.max(targetSize / source.naturalWidth, targetSize / source.naturalHeight);
      const width = source.naturalWidth * scale;
      const height = source.naturalHeight * scale;
      context.drawImage(source, (targetSize - width) / 2, (targetSize - height) / 2, width, height);
      const finish = (blob: Blob | null, quality: number): void => {
        if (!blob || blob.size > maxBytes) {
          reject(new Error("Image is still too large after browser optimization."));
          return;
        }
        resolve(Object.assign(blob, {
          optimizedWidth: targetSize,
          optimizedHeight: targetSize,
          optimizedQuality: quality,
        }));
      };
      const encode = (quality: number): void => {
        if (canvas.toBlob) {
          canvas.toBlob((blob) => {
            if (blob && blob.size > maxBytes && quality > minQuality) {
              encode(Math.max(minQuality, quality - 0.08));
            } else {
              finish(blob, quality);
            }
          }, "image/jpeg", quality);
          return;
        }
        const data = canvas.toDataURL("image/jpeg", quality);
        const raw = atob(data.substring(data.indexOf(",") + 1));
        const bytes = new Uint8Array(raw.length);
        for (let index = 0; index < raw.length; index++) bytes[index] = raw.charCodeAt(index);
        const blob = new Blob([bytes], { type: "image/jpeg" });
        if (blob.size > maxBytes && quality > minQuality) {
          encode(Math.max(minQuality, quality - 0.08));
        } else {
          finish(blob, quality);
        }
      };
      encode(0.78);
    };
    source.onerror = (): void => {
      URL.revokeObjectURL(objectUrl);
      reject(new Error("Could not read that image."));
    };
    source.src = objectUrl;
  });

  return {
    list,
    info: () => ({ ...libraryInfo }),
    cachedImages: () => images.slice(),
    replaceCachedImages(value) {
      images = value.slice();
      loaded = images.length > 0;
    },
    invalidate,
    resize,
    async upload(file) {
      const optimized = await resize(file);
      return decodeUploadResponse(await dependencies.fetch("/api/card-images", {
        method: "POST",
        headers: { "Content-Type": "image/jpeg" },
        body: optimized,
      }), "Could not upload image.");
    },
    uploadBytes,
    async rename(value, name) {
      const id = dependencies.normalizeId(value);
      if (!id) throw new Error("Could not rename image.");
      return decodeUploadResponse(await dependencies.fetch(`/api/card-images/${id}/rename`, {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded" },
        body: `name=${encodeURIComponent(String(name || ""))}`,
      }), "Could not rename image.");
    },
    async delete(value) {
      const id = dependencies.normalizeId(value);
      if (!id) return;
      const response = await dependencies.fetch(`/api/card-images/${id}`, { method: "DELETE" });
      if (!response.ok) throw new Error("Could not delete image.");
      invalidate();
    },
    async readBytes(value) {
      const id = dependencies.normalizeId(value);
      if (!id) throw new Error("Could not read image.");
      const response = await dependencies.fetch(dependencies.imageUrl(id));
      if (!response.ok) throw new Error(`Could not read image ${id}`);
      return new Uint8Array(await response.arrayBuffer());
    },
  };
}

export function createCardImageBackupAssetProvider(
  cardImages: CardImagesFeature,
  dependencies: CardImageBackupDependencies,
): BackupAssetProvider<CardImageReferencePlan> {
  const createArchiveEntries = async (): Promise<BackupArchiveEntry[]> => {
    const manifest = { format: "espcontrol.card-images", version: 1, images: [] as CardImageManifestItem[] };
    const entries: BackupArchiveEntry[] = [];
    for (const image of await cardImages.list(true)) {
      const file = `images/${image.id}.jpg`;
      manifest.images.push({ id: image.id, name: image.name || image.id, file });
      try {
        entries.push({ name: file, bytes: await cardImages.readBytes(image.id) });
      } catch {
        throw new Error(`Could not read image ${image.name || image.id}`);
      }
    }
    entries.unshift({
      name: "images.json",
      bytes: new TextEncoder().encode(JSON.stringify(manifest, null, 2)),
    });
    return entries;
  };

  const restoreArchiveEntries = async (
    entries?: BackupArchiveEntries | null,
  ): Promise<Record<string, string>> => {
    if (!entries?.["images.json"]) return {};
    let manifest: unknown;
    try {
      manifest = JSON.parse(new TextDecoder().decode(entries["images.json"]));
    } catch {
      throw new Error("ZIP backup has an invalid image manifest.");
    }
    if (!manifest || typeof manifest !== "object") {
      throw new Error("ZIP backup has an unsupported image manifest.");
    }
    const rawManifest = manifest as Record<string, unknown>;
    if (rawManifest.format !== "espcontrol.card-images" || rawManifest.version !== 1 ||
        !Array.isArray(rawManifest.images)) {
      throw new Error("ZIP backup has an unsupported image manifest.");
    }

    const archived: Array<{ item: CardImageManifestItem; bytes: Uint8Array }> = [];
    const seen = new Set<string>();
    for (const value of rawManifest.images) {
      if (!value || typeof value !== "object") {
        throw new Error("ZIP backup has an invalid or missing archived card image.");
      }
      const raw = value as Record<string, unknown>;
      const id = dependencies.normalizeId(raw.id);
      const file = String(raw.file || "");
      const bytes = entries[file];
      if (!id || seen.has(id) || file !== `images/${id}.jpg` || !bytes) {
        throw new Error("ZIP backup has an invalid or missing archived card image.");
      }
      seen.add(id);
      archived.push({
        item: { id, name: String(raw.name || id).trim() || id, file },
        bytes,
      });
    }
    if (archived.length === 0) return {};

    const references: Record<string, string> = {};
    const createdIds: string[] = [];
    try {
      const existing = await cardImages.list(true);
      const info = cardImages.info();
      if (!info.available) throw new Error("Card image storage is unavailable on this display.");
      const existingIds = new Set(existing.map((image) => image.id));
      for (const image of archived) {
        if (existingIds.has(image.item.id)) {
          references[image.item.id] = image.item.id;
          continue;
        }
        const restored = await cardImages.uploadBytes(image.bytes, "Could not restore an archived card image.");
        createdIds.push(restored.id);
        references[image.item.id] = restored.id;
        if (image.item.name && image.item.name !== restored.id) {
          await cardImages.rename(restored.id, image.item.name);
        }
      }
      cardImages.invalidate();
      return references;
    } catch (error) {
      for (const id of createdIds) {
        try {
          await cardImages.delete(id);
        } catch {
          // Best-effort rollback keeps the original restore failure visible.
        }
      }
      cardImages.invalidate();
      throw error;
    }
  };

  const remapImportedReferences = (
    plan: CardImageReferencePlan,
    references: Readonly<Record<string, string>>,
  ): void => {
    const remap = (buttons: readonly CardConfig[] = []): void => {
      for (const button of buttons) {
        const oldId = dependencies.imageId(button);
        const newId = oldId ? references[oldId] : undefined;
        if (newId) dependencies.setImageId(button, newId);
      }
    };
    remap(plan.buttons);
    for (const subpage of Object.values(plan.subpages || {})) remap(subpage?.buttons);
  };

  return { createArchiveEntries, restoreArchiveEntries, remapImportedReferences };
}
