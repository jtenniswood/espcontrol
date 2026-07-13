// ── Language State ─────────────────────────────────────────────────────
// @web-module-requires: state

function languageLabel(value) {
  value = normalizeLanguage(value);
  return LANGUAGE_LABELS[value] || value;
}

function languageOptionsWithFallback(options, selected) {
  var list = uniqueOptions((options && options.length ? options : ["en"]).map(normalizeLanguage));
  selected = normalizeLanguage(selected);
  if (list.indexOf(selected) === -1) list.unshift(selected);
  return list;
}

function appendLanguageOption(select, opt) {
  var o = document.createElement("option");
  o.value = normalizeLanguage(opt);
  o.textContent = languageLabel(opt);
  select.appendChild(o);
}

function syncLanguageSelect() {
  if (!els.setLanguage) return;
  state.languageOptions = languageOptionsWithFallback(state.languageOptions, state.language);
  els.setLanguage.innerHTML = "";
  state.languageOptions.forEach(function (opt) {
    appendLanguageOption(els.setLanguage, opt);
  });
  els.setLanguage.value = normalizeLanguage(state.language);
}
