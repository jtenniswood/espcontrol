// ── Environment State ──────────────────────────────────────────────────
// @web-module-requires: state

function voiceServicesSupported() {
  return !!(CFG.features && CFG.features.voiceServices);
}

function isHomeAssistantAutoTimezone(value) {
  return String(value || "") === AUTO_TIMEZONE_OPTION;
}

function effectiveTimezoneOptionForWeb(value) {
  if (!isHomeAssistantAutoTimezone(value)) return value;
  var active = String(state && state.activeTimezone || "").trim();
  return active && !isHomeAssistantAutoTimezone(active) ? active : FALLBACK_TIMEZONE_OPTION;
}

function timezoneOptionsWithFallback(options, selected, preserveSelectedAuto) {
  var list = Array.isArray(options) && options.length ? options.slice() : defaultTimezoneOptions();
  var supportsAuto = list.indexOf(AUTO_TIMEZONE_OPTION) !== -1;
  if (selected && list.indexOf(selected) === -1 &&
      (!isHomeAssistantAutoTimezone(selected) || supportsAuto || preserveSelectedAuto)) {
    list.unshift(selected);
  }
  return list;
}

function monthNameForIndex(index) {
  var monthIndex = parseInt(index, 10);
  if (!isFinite(monthIndex) || monthIndex < 0 || monthIndex > 11) return "Date";
  try {
    return new Intl.DateTimeFormat(normalizeLanguage(state.language), { month: "long" })
      .format(new Date(Date.UTC(2000, monthIndex, 1)));
  } catch (_) {
    return new Intl.DateTimeFormat("en", { month: "long" })
      .format(new Date(Date.UTC(2000, monthIndex, 1)));
  }
}
