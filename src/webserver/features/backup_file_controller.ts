export type BackupFileBannerKind = "error" | "success" | "warning";

export interface BackupFileTransport {
  download(content: string | Uint8Array, filename: string, contentType: string): void;
  chooseFile(onFile: (name: string, bytes: Uint8Array) => void, onError: () => void): void;
}

export interface BackupFileControllerOptions {
  readonly transport: BackupFileTransport;
  readonly showBanner: (message: string, kind: BackupFileBannerKind) => void;
}

export interface BackupFileController {
  download(data: unknown, filename: string): void;
  downloadArchive(data: unknown, entries: readonly BackupArchiveEntry[], filename: string): void;
  import(onBackup: (data: unknown, entries?: BackupArchiveEntries) => void): void;
}

export interface BackupArchiveEntry {
  readonly name: string;
  readonly bytes: Uint8Array;
}

export type BackupArchiveEntries = Readonly<Record<string, Uint8Array>>;

function u16(value: number): Uint8Array {
  return new Uint8Array([value & 255, (value >>> 8) & 255]);
}

function u32(value: number): Uint8Array {
  return new Uint8Array([
    value & 255, (value >>> 8) & 255, (value >>> 16) & 255, (value >>> 24) & 255,
  ]);
}

function crc32(bytes: Uint8Array): number {
  let crc = 0xFFFFFFFF;
  for (const byte of bytes) {
    crc ^= byte;
    for (let bit = 0; bit < 8; bit += 1) crc = (crc >>> 1) ^ ((crc & 1) ? 0xEDB88320 : 0);
  }
  return (crc ^ 0xFFFFFFFF) >>> 0;
}

function join(chunks: readonly Uint8Array[]): Uint8Array {
  const output = new Uint8Array(chunks.reduce((size, chunk) => size + chunk.length, 0));
  let offset = 0;
  for (const chunk of chunks) {
    output.set(chunk, offset);
    offset += chunk.length;
  }
  return output;
}

export function createStoredZip(entries: readonly BackupArchiveEntry[]): Uint8Array {
  const chunks: Uint8Array[] = [];
  const central: Uint8Array[] = [];
  let offset = 0;
  for (const entry of entries) {
    const name = new TextEncoder().encode(entry.name);
    const body = entry.bytes;
    const checksum = crc32(body);
    const local = join([
      u32(0x04034b50), u16(20), u16(0), u16(0), u16(0), u16(0), u32(checksum),
      u32(body.length), u32(body.length), u16(name.length), u16(0), name, body,
    ]);
    chunks.push(local);
    central.push(join([
      u32(0x02014b50), u16(20), u16(20), u16(0), u16(0), u16(0), u16(0),
      u32(checksum), u32(body.length), u32(body.length), u16(name.length), u16(0),
      u16(0), u16(0), u16(0), u32(0), u32(offset), name,
    ]));
    offset += local.length;
  }
  const centralBytes = join(central);
  chunks.push(centralBytes, join([
    u32(0x06054b50), u16(0), u16(0), u16(entries.length), u16(entries.length),
    u32(centralBytes.length), u32(offset), u16(0),
  ]));
  return join(chunks);
}

export function readStoredZip(bytes: Uint8Array): BackupArchiveEntries {
  const entries: Record<string, Uint8Array> = {};
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  let offset = 0;
  while (offset + 4 <= bytes.length && view.getUint32(offset, true) === 0x04034b50) {
    if (offset + 30 > bytes.length) throw new Error("Invalid ZIP backup.");
    const flags = view.getUint16(offset + 6, true);
    const compression = view.getUint16(offset + 8, true);
    const expectedChecksum = view.getUint32(offset + 14, true);
    const size = view.getUint32(offset + 18, true);
    const uncompressedSize = view.getUint32(offset + 22, true);
    const nameLength = view.getUint16(offset + 26, true);
    const extraLength = view.getUint16(offset + 28, true);
    if ((flags & 8) || compression !== 0) throw new Error("Unsupported ZIP backup format.");
    if (size !== uncompressedSize) throw new Error("Invalid ZIP backup.");
    const nameStart = offset + 30;
    const dataStart = nameStart + nameLength + extraLength;
    const dataEnd = dataStart + size;
    if (dataEnd > bytes.length) throw new Error("Invalid ZIP backup.");
    const name = new TextDecoder().decode(bytes.slice(nameStart, nameStart + nameLength));
    if (!name || Object.prototype.hasOwnProperty.call(entries, name)) {
      throw new Error("Invalid ZIP backup.");
    }
    const body = bytes.slice(dataStart, dataEnd);
    if (crc32(body) !== expectedChecksum) throw new Error("ZIP backup contains a corrupted entry.");
    entries[name] = body;
    offset = dataEnd;
  }
  if (!entries["backup.json"]) throw new Error("ZIP backup is missing backup.json.");
  return entries;
}

/**
 * Owns the browser-independent part of backup file transfer. The application
 * adapter supplies the DOM picker and download implementation while this
 * controller keeps validation and user-facing errors consistent.
 */
export function createBackupFileController(
  options: BackupFileControllerOptions,
): BackupFileController {
  return {
    download(data: unknown, filename: string): void {
      options.transport.download(JSON.stringify(data, null, 2), filename, "application/json");
    },

    downloadArchive(data: unknown, entries: readonly BackupArchiveEntry[], filename: string): void {
      const backup = new TextEncoder().encode(JSON.stringify(data, null, 2));
      options.transport.download(
        createStoredZip([{ name: "backup.json", bytes: backup }, ...entries]),
        filename,
        "application/zip",
      );
    },

    import(onBackup: (data: unknown, entries?: BackupArchiveEntries) => void): void {
      options.transport.chooseFile((name, bytes) => {
        let entries: BackupArchiveEntries | undefined;
        let text: string;
        try {
          if (name.toLowerCase().endsWith(".zip")) {
            entries = readStoredZip(bytes);
            text = new TextDecoder().decode(entries["backup.json"]);
          } else {
            text = new TextDecoder().decode(bytes);
          }
        } catch (error) {
          options.showBanner((error as Error).message || "Invalid ZIP backup.", "error");
          return;
        }
        let data: unknown;
        try {
          data = JSON.parse(text);
        } catch (_) {
          options.showBanner("Invalid file — could not parse JSON", "error");
          return;
        }
        onBackup(data, entries);
      }, () => {
        options.showBanner("Invalid file — could not read backup", "error");
      });
    },
  };
}
