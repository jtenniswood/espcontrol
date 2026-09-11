import { createBackupImportController } from "../../src/webserver/features/backup_import_controller";

function equal<T>(actual: T, expected: T, message: string): void {
  if (actual !== expected) throw new Error(`${message}: expected ${String(expected)}, received ${String(actual)}`);
}

export function runBackupImportControllerTests(): void {
  let gridCols = 3;
  const seenGridCols: number[] = [];
  const controller = createBackupImportController({
    normalizeBackup: () => ({ settings: { rotation: "90" } }),
    normalizeSettings: (settings) => settings as { rotation: string },
    gridColsForSettings: () => 5,
    getGridCols: () => gridCols,
    setGridCols: (value) => { gridCols = value; },
    planBackupImport: () => { seenGridCols.push(gridCols); return { buttons: 15 }; },
  });
  const planned = controller.plan({}, { device: "panel", slots: 15 });
  equal(planned.importedGridCols, 5, "restore planning uses the imported layout");
  equal(seenGridCols.join(","), "5", "layout is active only while the plan is built");
  equal(gridCols, 3, "restore planning returns the active editor layout");
}
