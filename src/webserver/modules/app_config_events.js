// ── Config Event Handlers ─────────────────────────────────────────────
// @web-module-requires: state, config_codec, config_post_api

function ensureSubpageRaw(slot) {
  if (!state.subpageRaw[slot]) {
    state.subpageRaw[slot] = { main: "", ext: "", ext2: "", ext3: "", ext4: "", ext5: "", ext6: "", ext7: "" };
  }
  return state.subpageRaw[slot];
}

function applyButtonConfigStateEvent(slot, val) {
  var b = state.buttons[slot - 1];
  var migrateConfig = buttonConfigNeedsMigration(val || "");
  var parsed = parseButtonConfig(val || "");
  b.entity = parsed.entity;
  b.label = parsed.label;
  b.icon = parsed.icon;
  b.icon_on = parsed.icon_on;
  b.sensor = parsed.sensor;
  b.unit = parsed.unit;
  b.type = parsed.type;
  b.precision = parsed.precision;
  b.options = parsed.options;
  if (migrateConfig) saveButtonConfig(slot);
  scheduleRender();
}

function applySubpageConfigStateEvent(slot, key, val) {
  ensureSubpageRaw(slot)[key] = val || "";
  applySubpageRaw(slot);
}

function configEventPatterns() {
  return [
    {
      re: /^text-button_(\d+)_config$/,
      fn: function (m, val) {
        var slot = parseInt(m[1], 10);
        if (slot < 1 || slot > TOTAL_SLOTS) return;
        applyButtonConfigStateEvent(slot, val);
      },
    },
    {
      re: /^text-subpage_(\d+)_config$/,
      fn: function (m, val) {
        var slot = parseInt(m[1], 10);
        if (slot < 1 || slot > TOTAL_SLOTS) return;
        applySubpageConfigStateEvent(slot, "main", val);
      },
    },
    {
      re: /^text-subpage_(\d+)_config_ext$/,
      fn: function (m, val) {
        var slot = parseInt(m[1], 10);
        if (slot < 1 || slot > TOTAL_SLOTS) return;
        applySubpageConfigStateEvent(slot, "ext", val);
      },
    },
    {
      re: /^text-subpage_(\d+)_config_ext_2$/,
      fn: function (m, val) {
        var slot = parseInt(m[1], 10);
        if (slot < 1 || slot > TOTAL_SLOTS) return;
        applySubpageConfigStateEvent(slot, "ext2", val);
      },
    },
    {
      re: /^text-subpage_(\d+)_config_ext_3$/,
      fn: function (m, val) {
        var slot = parseInt(m[1], 10);
        if (slot < 1 || slot > TOTAL_SLOTS) return;
        applySubpageConfigStateEvent(slot, "ext3", val);
      },
    },
    {
      re: /^text-subpage_(\d+)_config_ext_4$/,
      fn: function (m, val) {
        var slot = parseInt(m[1], 10);
        if (slot < 1 || slot > TOTAL_SLOTS) return;
        applySubpageConfigStateEvent(slot, "ext4", val);
      },
    },
    {
      re: /^text-subpage_(\d+)_config_ext_5$/,
      fn: function (m, val) {
        var slot = parseInt(m[1], 10);
        if (slot < 1 || slot > TOTAL_SLOTS) return;
        applySubpageConfigStateEvent(slot, "ext5", val);
      },
    },
    {
      re: /^text-subpage_(\d+)_config_ext_6$/,
      fn: function (m, val) {
        var slot = parseInt(m[1], 10);
        if (slot < 1 || slot > TOTAL_SLOTS) return;
        applySubpageConfigStateEvent(slot, "ext6", val);
      },
    },
    {
      re: /^text-subpage_(\d+)_config_ext_7$/,
      fn: function (m, val) {
        var slot = parseInt(m[1], 10);
        if (slot < 1 || slot > TOTAL_SLOTS) return;
        applySubpageConfigStateEvent(slot, "ext7", val);
      },
    },
  ];
}
