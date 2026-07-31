import { state } from "../state/app_instance";
import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installLanguageStateModule(): GlobalDescriptors {
    // ── Language State ─────────────────────────────────────────────────────
    function languageLabel(this: any, value?: any) {
        value = normalizeLanguage(value);
        return LANGUAGE_LABELS[value] || value;
    }
    function languageOptionsWithFallback(this: any, options?: any, selected?: any) {
        var list: any = uniqueOptions((options && options.length ? options : ["en"]).map(normalizeLanguage));
        selected = normalizeLanguage(selected);
        if (list.indexOf(selected) === -1)
            list.unshift(selected);
        return list;
    }
    function appendLanguageOption(this: any, select?: any, opt?: any) {
        var o: any = document.createElement("option");
        o.value = normalizeLanguage(opt);
        o.textContent = languageLabel(opt);
        select.appendChild(o);
    }
    function syncLanguageSelect(this: any) {
        if (!els.setLanguage)
            return;
        state.languageOptions = languageOptionsWithFallback(state.languageOptions, state.language);
        els.setLanguage.innerHTML = "";
        state.languageOptions.forEach(function (this: any, opt?: any) {
            appendLanguageOption(els.setLanguage, opt);
        });
        els.setLanguage.value = normalizeLanguage(state.language);
    }
    return {
        "languageLabel": staticGlobal(languageLabel),
        "languageOptionsWithFallback": staticGlobal(languageOptionsWithFallback),
        "appendLanguageOption": staticGlobal(appendLanguageOption),
        "syncLanguageSelect": staticGlobal(syncLanguageSelect),
    };
}
