import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installAppTestHooksBackup(): GlobalDescriptors {
    if (typeof globalThis !== "undefined" && globalThis.__ESPCONTROL_TEST_HOOKS__) {
        registerEspControlTestHookGroup("backup", {
            BACKUP_CONFIG_VERSION: BACKUP_CONFIG_VERSION,
            BACKUP_FORMAT: BACKUP_FORMAT,
            createBackupConfig: createBackupConfig,
            normalizeBackupConfig: normalizeBackupConfig,
            planBackupImport: planBackupImport,
            backupExportFileName: backupExportFileName,
            backupRestoreArchivedImages: backupRestoreArchivedImages,
        });
    }
    return {};
}
