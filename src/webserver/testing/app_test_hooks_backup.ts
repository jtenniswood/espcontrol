import { state } from "../state/app_instance";
import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installAppTestHooksBackup(): GlobalDescriptors {
    if (typeof globalThis !== "undefined" && globalThis.__ESPCONTROL_TEST_HOOKS__) {
        registerEspControlTestHookGroup("backup", {
            BACKUP_CONFIG_VERSION: BACKUP_CONFIG_VERSION,
            BACKUP_FORMAT: BACKUP_FORMAT,
            createBackupConfig: createBackupConfig,
            normalizeBackupConfig: normalizeBackupConfig,
            planBackupImport: planBackupImport,
            backupImportGridColsFor: function (this: any, settings?: any, currentRotation?: any) {
                var oldRotation: any = state.screenRotation;
                state.screenRotation = currentRotation;
                try {
                    return gridColsForImportedSettings(normalizeImportedPanelSettings(settings));
                }
                finally {
                    state.screenRotation = oldRotation;
                }
            },
            planBackupImportForGridCols: function (this: any, data?: any, targetDevice?: any, gridCols?: any) {
                var oldGridCols: any = GRID_COLS;
                GRID_COLS = gridCols;
                try {
                    return planBackupImport(data, targetDevice);
                }
                finally {
                    GRID_COLS = oldGridCols;
                }
            },
            backupExportFileName: backupExportFileName,
            backupRestoreArchivedImages: backupRestoreArchivedImages,
        });
    }
    return {};
}
