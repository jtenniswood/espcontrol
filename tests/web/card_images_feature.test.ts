import type { CardConfig } from "../../src/webserver/contracts/types";
import {
  createCardImageBackupAssetProvider,
  createCardImagesFeature,
  type CardImageHttpRequest,
  type CardImageHttpResponse,
} from "../../src/webserver/features/card_images";

function equal<T>(actual: T, expected: T, message: string): void {
  if (actual !== expected) throw new Error(`${message}: expected ${String(expected)}, received ${String(actual)}`);
}

function deepEqual(actual: unknown, expected: unknown, message: string): void {
  const actualText = JSON.stringify(actual);
  const expectedText = JSON.stringify(expected);
  if (actualText !== expectedText) throw new Error(`${message}: expected ${expectedText}, received ${actualText}`);
}

function response(options: {
  readonly ok?: boolean;
  readonly json?: unknown;
  readonly text?: string;
  readonly bytes?: Uint8Array;
} = {}): CardImageHttpResponse {
  return {
    ok: options.ok !== false,
    json: async () => options.json || {},
    text: async () => options.text || "",
    arrayBuffer: async () => (options.bytes || new Uint8Array()).slice().buffer,
  };
}

function normalizeId(value: unknown): string {
  const id = String(value || "").trim().toLowerCase();
  return /^[a-z0-9-]{1,40}$/.test(id) ? id : "";
}

function manifestEntries(images: ReadonlyArray<{ id: string; name: string }>): Record<string, Uint8Array> {
  const manifestImages = images.map((image) => ({
    ...image,
    file: `images/${image.id}.jpg`,
  }));
  const entries: Record<string, Uint8Array> = {
    "images.json": new TextEncoder().encode(JSON.stringify({
      format: "espcontrol.card-images",
      version: 1,
      images: manifestImages,
    })),
  };
  for (const image of manifestImages) entries[image.file] = new Uint8Array([0xFF, 0xD8, 0xFF, 0xD9]);
  return entries;
}

function featureFor(
  fetchImpl: (url: string, request?: CardImageHttpRequest) => Promise<CardImageHttpResponse>,
) {
  return createCardImagesFeature({
    maxActiveBackgrounds: 9,
    fetch: fetchImpl,
    normalizeId,
    imageUrl: (id) => `/card-images/${id}.jpg`,
    targetSize: () => 200,
    uploadMaxBytes: () => 45 * 1024,
    minimumQuality: () => 0.42,
  });
}

function providerFor(
  fetchImpl: (url: string, request?: CardImageHttpRequest) => Promise<CardImageHttpResponse>,
) {
  const feature = featureFor(fetchImpl);
  const provider = createCardImageBackupAssetProvider(feature, {
    normalizeId,
    imageId: (button) => String(button.options || "").replace(/^bg_image=/, ""),
    setImageId(button, id) { button.options = `bg_image=${id}`; },
  });
  return { feature, provider };
}

export async function runCardImagesFeatureTests(): Promise<void> {
  let listRequests = 0;
  const listed = featureFor(async (url) => {
    equal(url, "/api/card-images", "library uses the card-image API");
    listRequests++;
    return response({ json: {
      available: true,
      format_version: 2,
      max_active_backgrounds: 6,
      storage_bytes: 8192,
      used_bytes: 4096,
      free_bytes: 4096,
      max_bytes: 45056,
      images: [{ id: "image-one", name: "Kitchen", size: 1234, width: 200, height: 200 }],
    } });
  });
  deepEqual(await listed.list(), [{
    id: "image-one", name: "Kitchen", size: 1234, width: 200, height: 200,
  }], "library response crosses the typed card-image boundary");
  await listed.list();
  equal(listRequests, 1, "loaded library results are cached");
  deepEqual(listed.info(), {
    available: true,
    requiresUsbFlash: false,
    formatVersion: 2,
    maxActiveBackgrounds: 6,
    storageBytes: 8192,
    usedBytes: 4096,
    freeBytes: 4096,
    maxBytes: 45056,
  }, "storage metadata is normalized once");

  let emptyRequests = 0;
  const emptyProvider = providerFor(async () => {
    emptyRequests++;
    throw new Error("empty restore must not access the display");
  }).provider;
  deepEqual(await emptyProvider.restoreArchiveEntries(manifestEntries([])), {}, "image-free restore is a no-op");
  equal(emptyRequests, 0, "image-free restore does not query storage");

  const existingRequests: string[] = [];
  const existingProvider = providerFor(async (url, request) => {
    existingRequests.push(`${request?.method || "GET"} ${url}`);
    return response({ json: {
      available: true,
      free_bytes: 0,
      images: [{ id: "existing-1", name: "Existing", size: 4 }],
    } });
  }).provider;
  deepEqual(
    await existingProvider.restoreArchiveEntries(manifestEntries([{ id: "existing-1", name: "Existing" }])),
    { "existing-1": "existing-1" },
    "restore reuses an image that is already present",
  );
  deepEqual(existingRequests, ["GET /api/card-images"], "existing images require no upload or rename");

  const capacityRequests: string[] = [];
  const capacityProvider = providerFor(async (url, request) => {
    capacityRequests.push(`${request?.method || "GET"} ${url}`);
    if (!request?.method || request.method === "GET") {
      return response({ json: { available: true, free_bytes: 0, images: [] } });
    }
    if (request.method === "POST" && url === "/api/card-images") {
      return response({ json: { id: "restored-1", name: "restored-1", size: 4 } });
    }
    throw new Error(`unexpected request ${request.method} ${url}`);
  }).provider;
  deepEqual(
    await capacityProvider.restoreArchiveEntries(manifestEntries([{ id: "too-large", name: "restored-1" }])),
    { "too-large": "restored-1" },
    "restore lets firmware reclaim disposable cache space",
  );
  deepEqual(capacityRequests, ["GET /api/card-images", "POST /api/card-images"],
    "low reported free space still reaches the cache-evicting upload endpoint");

  const rollbackRequests: string[] = [];
  let uploadCount = 0;
  const rollbackProvider = providerFor(async (url, request) => {
    const method = request?.method || "GET";
    rollbackRequests.push(`${method} ${url}`);
    if (method === "GET") return response({ json: { available: true, free_bytes: 16384, images: [] } });
    if (method === "POST" && url === "/api/card-images") {
      uploadCount++;
      return uploadCount === 1
        ? response({ json: { id: "created-1", name: "created-1", size: 4 } })
        : response({ ok: false, text: "restore failed" });
    }
    if (method === "POST" && url.endsWith("/rename")) {
      return response({ json: { id: "created-1", name: "First", size: 4 } });
    }
    if (method === "DELETE") return response();
    throw new Error(`unexpected request ${method} ${url}`);
  }).provider;
  let restoreFailure = "";
  try {
    await rollbackProvider.restoreArchiveEntries(manifestEntries([
      { id: "old-1", name: "First" },
      { id: "old-2", name: "Second" },
    ]));
  } catch (error) {
    restoreFailure = (error as Error).message;
  }
  equal(restoreFailure, "restore failed", "restore retains the display failure reason");
  equal(rollbackRequests.at(-1), "DELETE /api/card-images/created-1", "partial restore is rolled back");

  const buttons: CardConfig[] = [
    { entity: "", label: "", icon: "", icon_on: "", sensor: "", unit: "", type: "", precision: "", options: "bg_image=old-1" },
  ];
  existingProvider.remapImportedReferences({ buttons }, { "old-1": "new-1" });
  equal(buttons[0]?.options, "bg_image=new-1", "restored IDs are remapped through the asset provider");
}
