import { state } from "../state/app_instance";
import { LANGUAGE_LABELS } from "../state/app_state";
import { normalizeLanguage } from "../model/settings";
import { uniqueOptions } from "./ui_primitives";
import type { UiRuntimeState } from "./state";
export function languageLabel(value?: any) {
        value = normalizeLanguage(value);
        return LANGUAGE_LABELS[value] || value;
}
export function languageOptionsWithFallback(options?: any, selected?: any) {
        var list: any = uniqueOptions((options && options.length ? options : ["en"]).map(normalizeLanguage));
        selected = normalizeLanguage(selected);
        if (list.indexOf(selected) === -1)
            list.unshift(selected);
        return list;
}
export function appendLanguageOption(select?: any, opt?: any) {
        var o: any = document.createElement("option");
        o.value = normalizeLanguage(opt);
        o.textContent = languageLabel(opt);
        select.appendChild(o);
}
export function syncLanguageSelect(runtime: UiRuntimeState) {
        const els = runtime.els;
        if (!els.setLanguage)
            return;
        state.languageOptions = languageOptionsWithFallback(state.languageOptions, state.language);
        els.setLanguage.innerHTML = "";
        state.languageOptions.forEach(function (this: any, opt?: any) {
            appendLanguageOption(els.setLanguage, opt);
        });
        els.setLanguage.value = normalizeLanguage(state.language);
}
