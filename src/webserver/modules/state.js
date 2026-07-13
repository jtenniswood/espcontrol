// ── State ──────────────────────────────────────────────────────────────
// @web-module-requires: firmware_metadata

function uniqueOptions(options) {
  var out = [];
  (options || []).forEach(function (opt) {
    opt = String(opt);
    if (out.indexOf(opt) < 0) out.push(opt);
  });
  return out;
}

function setSelectValue(select, value, label) {
  if (!select) return;
  value = String(value);
  var found = false;
  for (var i = 0; i < select.options.length; i++) {
    if (select.options[i].value === value) {
      found = true;
      break;
    }
  }
  if (!found) {
    var o = document.createElement("option");
    o.value = value;
    o.textContent = label || value;
    select.appendChild(o);
  }
  select.value = value;
}

var els = {};
var dragSrcPos = -1;
var didDrag = false;
var previewPlaceholder = null;
var previewDropIdx = -1;
var dragRafPending = CFG.dragAnimation ? false : null;
var dragSrcEl = CFG.dragAnimation ? null : null;
var dragIsSubpage = false;
var dragEnterCount = 0;
var orderReceived = false;
var migrationTimer = null;
var sliderMigrationTimer = null;
var pendingSliderSubpageMigrations = {};
var _eventSource = null;

// ── Utilities ──────────────────────────────────────────────────────────

function escHtml(s) {
  return String(s).replace(/&/g, "&amp;").replace(/</g, "&lt;")
    .replace(/>/g, "&gt;").replace(/"/g, "&quot;");
}

function mdiIcon(icon, className) {
  var iconName = String(icon || "cog").trim();
  var span = document.createElement("span");
  span.className = className || "mdi";
  span.classList.add("mdi-" + (/^[a-z0-9]+(?:-[a-z0-9]+)*$/.test(iconName) ? iconName : iconSlug(iconName)));
  return span;
}

function textSpan(text, className) {
  var span = document.createElement("span");
  if (className) span.className = className;
  span.textContent = text == null ? "" : String(text);
  return span;
}

function escAttr(s) {
  return String(s == null ? "" : s).replace(/&/g, "&amp;").replace(/"/g, "&quot;")
    .replace(/</g, "&lt;").replace(/>/g, "&gt;");
}

function isSettingsFocused() {
  var ae = document.activeElement;
  return ae && els.buttonSettings && els.buttonSettings.contains(ae);
}

function isSettingsOpen() {
  return !!(els.settingsOverlay && els.settingsOverlay.classList.contains("sp-visible"));
}
