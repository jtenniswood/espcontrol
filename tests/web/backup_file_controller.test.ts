import {
  createBackupFileController,
  createStoredZip,
} from "../../src/webserver/features/backup_file_controller";

function equal<T>(actual: T, expected: T, message: string): void {
  if (actual !== expected) throw new Error(`${message}: expected ${String(expected)}, received ${String(actual)}`);
}

export function runBackupFileControllerTests(): void {
  const events: string[] = [];
  let selected: ((name: string, bytes: Uint8Array) => void) | undefined;
  let failed: (() => void) | undefined;
  const controller = createBackupFileController({
    transport: {
      download: (content, filename) => events.push(
        `download:${filename}:${typeof content === "string" ? content : content.length}`),
      chooseFile: (onFile, onError) => { selected = onFile; failed = onError; },
    },
    showBanner: (message, kind) => events.push(`${kind}:${message}`),
  });

  controller.download({ version: 2 }, "espcontrol.json");
  equal(events[0], 'download:espcontrol.json:{\n  "version": 2\n}', "downloads readable JSON");

  let restored: unknown;
  controller.import((data) => { restored = data; });
  selected?.("backup.json", new TextEncoder().encode('{"version":2}'));
  equal((restored as { version: number }).version, 2, "passes parsed data to restore journey");

  controller.import(() => { throw new Error("must not restore invalid JSON"); });
  selected?.("backup.json", new TextEncoder().encode("not-json"));
  equal(events.at(-1), "error:Invalid file — could not parse JSON", "reports parse failures clearly");

  controller.import(() => { throw new Error("must not restore unreadable files"); });
  failed?.();
  equal(events.at(-1), "error:Invalid file — could not read backup", "reports read failures clearly");

  let archivedImage = 0;
  controller.import((data, entries) => {
    restored = data;
    archivedImage = entries?.["images/test.jpg"]?.length || 0;
  });
  selected?.("backup.zip", createStoredZip([
    { name: "backup.json", bytes: new TextEncoder().encode('{"version":3}') },
    { name: "images/test.jpg", bytes: new Uint8Array([1, 2, 3]) },
  ]));
  equal((restored as { version: number }).version, 3, "reads configuration from ZIP backups");
  equal(archivedImage, 3, "passes archived assets to the restore journey");

  const corrupted = createStoredZip([
    { name: "backup.json", bytes: new TextEncoder().encode('{"version":4}') },
  ]);
  const corruptedByte = 31 + "backup.json".length;
  corrupted[corruptedByte] = (corrupted[corruptedByte] || 0) ^ 0x01;
  controller.import(() => { throw new Error("must not restore a corrupted ZIP"); });
  selected?.("backup.zip", corrupted);
  equal(events.at(-1), "error:ZIP backup contains a corrupted entry.",
    "rejects ZIP entries whose contents do not match their CRC");
}
