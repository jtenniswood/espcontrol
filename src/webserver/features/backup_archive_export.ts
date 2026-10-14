import type { BackupArchiveEntry } from "./backup";
import { CardImageStorageUnavailableError } from "./card_images";

export type BackupExportFailureChoice = "retry" | "configuration-only" | "cancel";

export interface BackupArchiveExportOptions {
  createImageEntries(): Promise<BackupArchiveEntry[]>;
  download(entries: readonly BackupArchiveEntry[]): void;
  chooseAfterFailure(message: string): Promise<BackupExportFailureChoice>;
  showBanner(message: string, kind: "warning" | "error" | "success"): void;
}

export async function exportBackupArchive(options: BackupArchiveExportOptions): Promise<boolean> {
  for (;;) {
    let entries: BackupArchiveEntry[];
    try {
      entries = await options.createImageEntries();
    } catch (error) {
      if (error instanceof CardImageStorageUnavailableError) {
        options.download([]);
        options.showBanner("Configuration backup exported. This display does not have image storage.", "warning");
        return true;
      }
      const message = error instanceof Error ? error.message : "Could not read the image library.";
      options.showBanner(`Backup was not exported. ${message}`, "error");
      const choice = await options.chooseAfterFailure(message);
      if (choice === "retry") continue;
      if (choice === "cancel") return false;
      options.download([]);
      options.showBanner("Configuration-only backup exported without images.", "warning");
      return true;
    }
    // Download errors are not image-storage errors and must not trigger a
    // second, incomplete archive download.
    options.download(entries);
    const count = Math.max(0, entries.length - 1);
    options.showBanner(`Backup exported with ${count} image${count === 1 ? "" : "s"}.`, "success");
    return true;
  }
}
