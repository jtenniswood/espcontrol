import { state } from "../state/app_instance";
import { BACKUP_CONFIG_VERSION, BACKUP_FORMAT } from "../model/backup";
import type { AppTestHookRegistrar } from "./app_test_hooks";
import type { ApplicationLayoutState } from "../application/application_context";
import type { BackupContractFeature } from "../application/backup_contract";
import type { AppBackupFeature } from "../application/app_backup";
export function installAppTestHooksBackup(layout: ApplicationLayoutState, backup: BackupContractFeature, application: AppBackupFeature, registerEspControlTestHookGroup: AppTestHookRegistrar): void {
    if (typeof globalThis !== "undefined" && globalThis.__ESPCONTROL_TEST_HOOKS__) {
        registerEspControlTestHookGroup("backup", {
            BACKUP_CONFIG_VERSION: BACKUP_CONFIG_VERSION,
            BACKUP_FORMAT: BACKUP_FORMAT,
            createBackupConfig: backup.createBackupConfig,
            normalizeBackupConfig: backup.normalizeBackupConfig,
            planBackupImport: backup.planBackupImport,
            backupImportGridColsFor: function (this: any, settings?: any, currentRotation?: any) {
                var oldRotation: any = state.screenRotation;
                state.screenRotation = currentRotation;
                try {
                    return application.gridColsForImportedSettings(application.normalizeImportedPanelSettings(settings));
                }
                finally {
                    state.screenRotation = oldRotation;
                }
            },
            planBackupImportForGridCols: function (this: any, data?: any, targetDevice?: any, gridCols?: any) {
                var oldGridCols: any = layout.gridCols;
                layout.gridCols = gridCols;
                try {
                    return backup.planBackupImport(data, targetDevice);
                }
                finally {
                    layout.gridCols = oldGridCols;
                }
            },
            backupExportFileName: application.backupExportFileName,
        });
    }
}
