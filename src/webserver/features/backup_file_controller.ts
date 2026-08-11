export type BackupFileBannerKind = "error" | "success" | "warning";

export interface BackupFileTransport {
  download(content: string, filename: string): void;
  chooseJsonFile(onText: (text: string) => void, onError: () => void): void;
}

export interface BackupFileControllerOptions {
  readonly transport: BackupFileTransport;
  readonly showBanner: (message: string, kind: BackupFileBannerKind) => void;
}

/**
 * Owns the browser-independent part of backup file transfer. The application
 * adapter supplies the DOM picker and download implementation while this
 * controller keeps validation and user-facing errors consistent.
 */
export function createBackupFileController(options: BackupFileControllerOptions) {
  return {
    download(data: unknown, filename: string): void {
      options.transport.download(JSON.stringify(data, null, 2), filename);
    },

    import(onBackup: (data: unknown) => void): void {
      options.transport.chooseJsonFile((text) => {
        let data: unknown;
        try {
          data = JSON.parse(text);
        } catch (_) {
          options.showBanner("Invalid file — could not parse JSON", "error");
          return;
        }
        onBackup(data);
      }, () => {
        options.showBanner("Invalid file — could not read backup", "error");
      });
    },
  };
}
