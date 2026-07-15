import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installBackupContractModule(): GlobalDescriptors {
    // ── Backup contract compatibility bridge ──────────────────────────────
    var _backupFeature: any = createBackupFeature({
        deviceId: DEVICE_ID,
        gridCols: GRID_COLS,
        numSlots: NUM_SLOTS,
        normalizeButtonConfig: normalizeButtonConfig,
        parseSubpageConfig: parseSubpageConfig,
        serializeSubpageConfig: serializeSubpageConfig,
        buildSubpageGrid: buildSubpageGrid,
    });
    var BACKUP_CONFIG_VERSION: any = _backupFeature.BACKUP_CONFIG_VERSION;
    var BACKUP_FORMAT: any = _backupFeature.BACKUP_FORMAT;
    function backupEmptyButtonConfig(this: any) {
        return _backupFeature.emptyButtonConfig();
    }
    function backupNormalizeButtonConfig(this: any, button?: any) {
        return _backupFeature.normalizeButtonConfig(button);
    }
    function createBackupConfig(this: any, snapshot?: any) {
        return _backupFeature.createBackupConfig(snapshot);
    }
    function normalizeBackupConfig(this: any, data?: any) {
        return _backupFeature.normalizeBackupConfig(data);
    }
    function planBackupImport(this: any, data?: any, targetDevice?: any) {
        return _backupFeature.planBackupImport(data, targetDevice);
    }
    return {
        "_backupFeature": liveGlobal(() => _backupFeature, (value?: any) => { _backupFeature = value; }),
        "BACKUP_CONFIG_VERSION": liveGlobal(() => BACKUP_CONFIG_VERSION, (value?: any) => { BACKUP_CONFIG_VERSION = value; }),
        "BACKUP_FORMAT": liveGlobal(() => BACKUP_FORMAT, (value?: any) => { BACKUP_FORMAT = value; }),
        "backupEmptyButtonConfig": staticGlobal(backupEmptyButtonConfig),
        "backupNormalizeButtonConfig": staticGlobal(backupNormalizeButtonConfig),
        "createBackupConfig": staticGlobal(createBackupConfig),
        "normalizeBackupConfig": staticGlobal(normalizeBackupConfig),
        "planBackupImport": staticGlobal(planBackupImport),
    };
}
