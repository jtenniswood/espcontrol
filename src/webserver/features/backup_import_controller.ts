export interface BackupImportControllerOptions<Normalized, Settings, Plan, Target> {
  readonly normalizeBackup: (data: unknown) => Normalized;
  readonly normalizeSettings: (settings: unknown) => Settings | null;
  readonly gridColsForSettings: (settings: Settings | null) => number;
  readonly getGridCols: () => number;
  readonly setGridCols: (gridCols: number) => void;
  readonly planBackupImport: (data: unknown, target: Target) => Plan;
}

export interface PlannedBackupImport<Settings, Plan> {
  readonly importedSettings: Settings | null;
  readonly importedGridCols: number;
  readonly backupPlan: Plan;
}

export interface BackupImportController<Settings, Plan, Target> {
  plan(data: unknown, target: Target): PlannedBackupImport<Settings, Plan>;
}

/** Builds a restore plan using the layout that the backup will activate. */
export function createBackupImportController<Normalized extends { settings?: unknown }, Settings, Plan, Target>(
  options: BackupImportControllerOptions<Normalized, Settings, Plan, Target>,
): BackupImportController<Settings, Plan, Target> {
  return {
    plan(data: unknown, target: Target): PlannedBackupImport<Settings, Plan> {
      const normalizedBackup = options.normalizeBackup(data);
      const importedSettings = options.normalizeSettings(normalizedBackup.settings);
      const importedGridCols = options.gridColsForSettings(importedSettings);
      const previousGridCols = options.getGridCols();
      options.setGridCols(importedGridCols);
      try {
        return { importedSettings, importedGridCols, backupPlan: options.planBackupImport(data, target) };
      } finally {
        options.setGridCols(previousGridCols);
      }
    },
  };
}
