// ── Backup contract compatibility bridge ──────────────────────────────
// @web-module-requires: state, grid, config_codec

var _backupFeature = createBackupFeature({
  deviceId: DEVICE_ID,
  gridCols: GRID_COLS,
  numSlots: NUM_SLOTS,
  normalizeButtonConfig: normalizeButtonConfig,
  parseSubpageConfig: parseSubpageConfig,
  serializeSubpageConfig: serializeSubpageConfig,
  buildSubpageGrid: buildSubpageGrid,
});

var BACKUP_CONFIG_VERSION = _backupFeature.BACKUP_CONFIG_VERSION;
var BACKUP_FORMAT = _backupFeature.BACKUP_FORMAT;

function backupEmptyButtonConfig() {
  return _backupFeature.emptyButtonConfig();
}

function backupNormalizeButtonConfig(button) {
  return _backupFeature.normalizeButtonConfig(button);
}

function createBackupConfig(snapshot) {
  return _backupFeature.createBackupConfig(snapshot);
}

function normalizeBackupConfig(data) {
  return _backupFeature.normalizeBackupConfig(data);
}

function planBackupImport(data, targetDevice) {
  return _backupFeature.planBackupImport(data, targetDevice);
}
