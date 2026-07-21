import type { CardConfig } from "../../src/webserver/contracts/types";
import {
  CARD_IMAGE_SAVE_FAILURE_MESSAGE,
  createCardImageBackupAssetProvider,
  createCardImagesFeature,
  deleteCardImageConfigurationFirst,
  deleteCardImageWithDeviceTransaction,
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
      reference_transactions: true,
      restore_transactions: true,
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
    referenceTransactions: true,
    restoreTransactions: true,
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

  const stagedRequests: string[] = [];
  const staged = providerFor(async (url, request) => {
    const method = request?.method || "GET";
    stagedRequests.push(`${method} ${url}`);
    if (method === "GET") {
      return response({ json: {
        available: true,
        restore_transactions: true,
        images: [],
      } });
    }
    if (url === "/api/card-images/restore/begin") {
      return response({ json: { session: "0123456789abcdef" } });
    }
    if (url === "/api/card-images?restore=0123456789abcdef") {
      return response({ json: { id: "created-stage", name: "created-stage", size: 4 } });
    }
    if (url === "/api/card-images/created-stage/rename") {
      return response({ json: { id: "created-stage", name: "Stage", size: 4 } });
    }
    if (url === "/api/card-images/restore/0123456789abcdef/commit") return response();
    throw new Error(`unexpected request ${method} ${url}`);
  });
  deepEqual(
    await staged.provider.restoreArchiveEntries(manifestEntries([{ id: "old-stage", name: "Stage" }])),
    { "old-stage": "created-stage" },
    "transactional restore returns the staged ID mapping",
  );
  equal(stagedRequests.includes("POST /api/card-images/restore/0123456789abcdef/commit"), false,
    "staged assets are not committed before configuration posts complete");
  await staged.provider.commitRestore?.();
  equal(stagedRequests.at(-1), "POST /api/card-images/restore/0123456789abcdef/commit",
    "successful configuration persistence commits the staged assets");

  const stagedRollbackRequests: string[] = [];
  let stagedUploadCount = 0;
  const stagedRollback = providerFor(async (url, request) => {
    const method = request?.method || "GET";
    stagedRollbackRequests.push(`${method} ${url}`);
    if (method === "GET") {
      return response({ json: { available: true, restore_transactions: true, images: [] } });
    }
    if (url === "/api/card-images/restore/begin") {
      return response({ json: { session: "fedcba9876543210" } });
    }
    if (url === "/api/card-images?restore=fedcba9876543210") {
      stagedUploadCount++;
      return stagedUploadCount === 1
        ? response({ json: { id: "staged-one", name: "staged-one", size: 4 } })
        : response({ ok: false, text: "staged upload failed" });
    }
    if (url === "/api/card-images/staged-one/rename") {
      return response({ json: { id: "staged-one", name: "One", size: 4 } });
    }
    if (url === "/api/card-images/restore/fedcba9876543210/rollback") return response();
    throw new Error(`unexpected request ${method} ${url}`);
  }).provider;
  let stagedRestoreFailure = "";
  try {
    await stagedRollback.restoreArchiveEntries(manifestEntries([
      { id: "stage-old-1", name: "One" },
      { id: "stage-old-2", name: "Two" },
    ]));
  } catch (error) {
    stagedRestoreFailure = (error as Error).message;
  }
  equal(stagedRestoreFailure, "staged upload failed", "staged restore keeps the upload failure reason");
  equal(stagedRollbackRequests.at(-1),
    "POST /api/card-images/restore/fedcba9876543210/rollback",
    "failed staged restore uses the durable device rollback instead of separate deletes");

  const buttons: CardConfig[] = [
    { entity: "", label: "", icon: "", icon_on: "", sensor: "", unit: "", type: "", precision: "", options: "bg_image=old-1" },
  ];
  existingProvider.remapImportedReferences({ buttons }, { "old-1": "new-1" });
  equal(buttons[0]?.options, "bg_image=new-1", "restored IDs are remapped through the asset provider");

  async function deletionScenario(options: {
    readonly references: string[];
    readonly postFailure?: boolean;
    readonly deleteFailure?: boolean;
  }): Promise<{
    deleted: boolean;
    rerenders: number;
    error: string;
    references: string[];
    persistedReferences: string[];
    restoredPosts: number;
  }> {
    let deleted = false;
    let rerenders = 0;
    let waits = 0;
    let postError = false;
    let restoredPosts = 0;
    let error = "";
    const references = options.references.slice();
    let persistedReferences = references.slice();
    try {
      await deleteCardImageConfigurationFirst({
        async waitForPendingPosts() {
          waits++;
          if (waits !== 2) return;
          postError = !!options.postFailure;
          persistedReferences = postError && references.length > 1
            ? [references[0] || "", ...options.references.slice(1)]
            : references.slice();
        },
        resetPostError() { postError = false; },
        clearReferences() {
          const before = references.slice();
          references.fill("");
          return {
            changed: before.filter(Boolean).length,
            restore() { references.splice(0, references.length, ...before); },
            persist() {
              restoredPosts++;
              persistedReferences = before.slice();
            },
          };
        },
        postsHadError() { return postError; },
        async deleteImage() {
          if (options.deleteFailure) throw new Error("Image could not be deleted.");
          deleted = true;
        },
        rerender() { rerenders++; },
      });
    } catch (caught) {
      error = (caught as Error).message;
    }
    return { deleted, rerenders, error, references, persistedReferences, restoredPosts };
  }

  deepEqual(await deletionScenario({ references: [] }), {
    deleted: true, rerenders: 0, error: "", references: [], persistedReferences: [], restoredPosts: 0,
  }, "unreferenced images delete immediately after the queue boundary");
  deepEqual(await deletionScenario({ references: ["main"] }), {
    deleted: true, rerenders: 0, error: "", references: [""], persistedReferences: [""], restoredPosts: 0,
  }, "a main-card reference is persisted before deletion");
  deepEqual(await deletionScenario({ references: ["main", "subpage"] }), {
    deleted: true, rerenders: 0, error: "", references: ["", ""],
    persistedReferences: ["", ""], restoredPosts: 0,
  }, "shared main and subpage references are cleared together");
  deepEqual(await deletionScenario({ references: ["main", "subpage"], postFailure: true }), {
    deleted: false, rerenders: 1, error: CARD_IMAGE_SAVE_FAILURE_MESSAGE,
    references: ["main", "subpage"], persistedReferences: ["main", "subpage"], restoredPosts: 1,
  }, "failed configuration posts retain the image and restore every local and persisted reference");
  deepEqual(await deletionScenario({ references: ["main"], deleteFailure: true }), {
    deleted: false, rerenders: 1, error: "Image could not be deleted.", references: [""],
    persistedReferences: [""], restoredPosts: 0,
  }, "a failed image delete keeps successfully persisted reference clearing");

  let releasePendingPost: (() => void) | undefined;
  const pendingPost = new Promise<void>((resolve) => { releasePendingPost = resolve; });
  const deviceDeleteEvents: string[] = [];
  const deviceDelete = deleteCardImageWithDeviceTransaction({
    waitForPendingPosts() {
      deviceDeleteEvents.push("wait");
      return pendingPost;
    },
    resetPostError() { deviceDeleteEvents.push("reset"); },
    async deleteImage() { deviceDeleteEvents.push("delete"); },
    clearLocalReferences() { deviceDeleteEvents.push("clear-local"); },
    rerender() { deviceDeleteEvents.push("rerender"); },
  });
  await Promise.resolve();
  deepEqual(deviceDeleteEvents, ["wait"],
    "device-owned deletion waits for an already queued card save");
  releasePendingPost?.();
  await deviceDelete;
  deepEqual(deviceDeleteEvents, ["wait", "reset", "delete", "clear-local", "rerender"],
    "device-owned deletion drains the queue before deleting and updating local state");
}
