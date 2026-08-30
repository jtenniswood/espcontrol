import { createBackupFileController } from "../../src/webserver/features/backup_file_controller";

function equal<T>(actual: T, expected: T, message: string): void {
  if (actual !== expected) throw new Error(`${message}: expected ${String(expected)}, received ${String(actual)}`);
}

export function runBackupFileControllerTests(): void {
  const events: string[] = [];
  let selected: ((text: string) => void) | undefined;
  let failed: (() => void) | undefined;
  const controller = createBackupFileController({
    transport: {
      download: (content, filename) => events.push(`download:${filename}:${content}`),
      chooseJsonFile: (onText, onError) => { selected = onText; failed = onError; },
    },
    showBanner: (message, kind) => events.push(`${kind}:${message}`),
  });

  controller.download({ version: 2 }, "espcontrol.json");
  equal(events[0], 'download:espcontrol.json:{\n  "version": 2\n}', "downloads readable JSON");

  let restored: unknown;
  controller.import((data) => { restored = data; });
  selected?.('{"version":2}');
  equal((restored as { version: number }).version, 2, "passes parsed data to restore journey");

  controller.import(() => { throw new Error("must not restore invalid JSON"); });
  selected?.("not-json");
  equal(events.at(-1), "error:Invalid file — could not parse JSON", "reports parse failures clearly");

  controller.import(() => { throw new Error("must not restore unreadable files"); });
  failed?.();
  equal(events.at(-1), "error:Invalid file — could not read backup", "reports read failures clearly");
}
