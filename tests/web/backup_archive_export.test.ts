import { exportBackupArchive, type BackupExportFailureChoice } from "../../src/webserver/features/backup_archive_export";
import { CardImageStorageUnavailableError, createCardImageBackupAssetProvider, createCardImagesFeature } from "../../src/webserver/features/card_images";
import type { BackupArchiveEntry } from "../../src/webserver/features/backup";

function equal<T>(actual: T, expected: T, message: string): void {
  if (actual !== expected) throw new Error(`${message}: expected ${String(expected)}, received ${String(actual)}`);
}

export async function runBackupArchiveExportTests(): Promise<void> {
  const full = [{ name: "images.json", bytes: new Uint8Array() }, { name: "images/a.jpg", bytes: new Uint8Array([1]) }];
  for (const choice of ["cancel", "configuration-only", "retry"] as const) {
    const downloads: Array<readonly BackupArchiveEntry[]> = [];
    const banners: string[] = [];
    let reads = 0;
    let choices = 0;
    const result = await exportBackupArchive({
      async createImageEntries() {
        if (reads++ === 0) throw new Error("Could not read image Kitchen");
        return full;
      },
      download(entries) { downloads.push(entries); },
      async chooseAfterFailure(message) {
        equal(downloads.length, 0, "failed read does not download an incomplete backup before a choice");
        equal(message, "Could not read image Kitchen", "retain the failed image name");
        choices++;
        return choice;
      },
      showBanner(message) { banners.push(message); },
    });
    equal(choices, 1, "failed image read offers explicit recovery");
    equal(result, choice !== "cancel", "cancel does not claim an export");
    equal(downloads.length, choice === "cancel" ? 0 : 1, "download only the chosen backup");
    if (choice === "retry") equal(downloads[0]?.length, 2, "retry preserves the full image library");
    if (choice === "configuration-only") equal(downloads[0]?.length, 0, "configuration-only export requires explicit choice");
    equal(banners.some((text) => text.includes("storage is unavailable")), false, "do not misdiagnose failed reads");
  }

  let configurationOnly = false;
  equal(await exportBackupArchive({
    createImageEntries: async () => { throw new CardImageStorageUnavailableError(); },
    download: (entries) => { configurationOnly = entries.length === 0; },
    chooseAfterFailure: async (): Promise<BackupExportFailureChoice> => { throw new Error("no retry needed on unsupported firmware"); },
    showBanner: () => undefined,
  }), true, "panels without image storage still export settings");
  equal(configurationOnly, true, "unsupported panel gets a configuration backup");

  let readFailed = false;
  const feature = createCardImagesFeature({
    maxActiveBackgrounds: 9,
    fetch: async (url) => ({
      ok: url === "/api/card-images",
      json: async () => ({ available: true, images: [{ id: "a", name: "Kitchen", size: 1 }] }),
      text: async () => "connection failed",
      arrayBuffer: async () => new ArrayBuffer(0),
    }),
    normalizeId: (id) => String(id || ""), imageUrl: (id) => `/card-images/${id}.jpg`,
    targetSize: () => 200, uploadMaxBytes: () => 45000, minimumQuality: () => 0.42,
  });
  const provider = createCardImageBackupAssetProvider(feature, {
    normalizeId: (id) => String(id || ""), imageId: () => "", setImageId: () => undefined,
  });
  try { await provider.createArchiveEntries(); } catch (error) {
    readFailed = error instanceof Error && !(error instanceof CardImageStorageUnavailableError);
  }
  equal(readFailed, true, "provider distinguishes an image-read failure from unsupported storage");
}
