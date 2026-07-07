// ── State defaults ─────────────────────────────────────────────────────
// @web-module-requires: style_tokens

var AUTO_TIMEZONE_OPTION = "Auto (Home Assistant)";
var FALLBACK_TIMEZONE_OPTION = "UTC (GMT+0)";
var NTP_SERVER_DEFAULTS = ["0.pool.ntp.org", "1.pool.ntp.org", "2.pool.ntp.org"];
var LANGUAGE_LABELS = {
  cs: "Čeština (Czech)",
  da: "Dansk (Danish)",
  de: "Deutsch (German)",
  en: "English",
  es: "Español (Spanish)",
  fi: "Suomi (Finnish)",
  fr: "Français (French)",
  hu: "Magyar (Hungarian)",
  it: "Italiano (Italian)",
  nb: "Norsk bokmål (Norwegian Bokmål)",
  nl: "Nederlands (Dutch)",
  pl: "Polski (Polish)",
  pt: "Português (Portuguese)",
  "pt-br": "Português (Brasil) (Brazilian Portuguese)",
  ro: "Română (Romanian)",
  sk: "Slovenčina (Slovak)",
  sl: "Slovenščina (Slovenian)",
  sv: "Svenska (Swedish)",
  tr: "Türkçe (Turkish)",
  uk: "Українська (Ukrainian)"
};
var THEME_PRESETS = {
  Light: { on: "0073FF" },
  Dark: { on: WEB_UI_COLORS.primary },
};
var DEFAULT_COLOR_PRESET = THEME_PRESETS[defaultTheme()];

function defaultTheme() {
  return "Dark";
}

function defaultTimezoneOptions() {
  var options = (CFG && Array.isArray(CFG.timezoneOptions)) ? CFG.timezoneOptions.slice() : [];
  return options;
}
