import { state } from "../state/app_instance";
import { normalizeBrightnessMode } from "../model/settings";
import type { UiRuntimeState } from "./state";
import type { CoreFeature } from "./core";
import type { ApplicationLayoutState } from "./application_context";
import type { EnvironmentStateFeature } from "./environment_state";
import type { ClockBarFeature } from "./clock_bar_state";

export interface AppStatusPreviewFeature {
    getTzId(timezone?: any): any;
    formatGmtOffset(minutes?: any): any;
    timezoneOffsetMinutes(timezoneId?: any, date?: any): any;
    formatTimezoneOption(option?: any): any;
    appendTimezoneOption(select?: any, option?: any): void;
    updateClockText(): void;
    updateClock(): void;
    clockBarTemperatureActive(): any;
    clockBarTemperatureItemId(index?: any): any;
    clockBarTemperatureItemIndex(item?: any): any;
    isClockBarTemperatureItem(item?: any): any;
    clockBarTemperatureItemIds(): any;
    clockBarItems(): any;
    clockBarDefaultSection(item?: any): any;
    clockBarItemActive(item?: any): any;
    clockBarItemElement(item?: any): any;
    clockBarItemLabel(item?: any): any;
    createClockBarItemElement(item?: any, section?: any): any;
    renderClockBarLayout(): void;
    syncClockBarItemElement(item?: any): void;
    updateClockBarItemUi(): void;
    syncInput(element?: any, value?: any): void;
    updateSunInfo(): void;
    updateTempPreview(): void;
    normalizeNetworkTransport(value?: any): any;
    normalizeWifiStrengthPercent(value?: any): any;
    networkPreviewIconSlug(transport?: any, strengthPercent?: any): any;
    updateNetworkPreview(): void;
    updateVoicePreview(): void;
}

export function createAppStatusPreviewFeature(runtime: UiRuntimeState, core: CoreFeature, layout: ApplicationLayoutState, environment: EnvironmentStateFeature, clockBar: ClockBarFeature): AppStatusPreviewFeature {
    const { now: webserverNow } = core;
    const { isHomeAssistantAutoTimezone, effectiveTimezoneOptionForWeb, voiceServicesUiState } = environment;
    const els = runtime.els;
    const {
        clockBarTemperatureUnitSymbol,
        temperatureVisible: clockBarTemperatureVisible,
        visibleInPreview: clockBarVisibleInPreview,
        primaryTemperatureEntity: primaryClockBarTemperatureEntity,
    } = clockBar;
    // ── Clock (minute-aligned) ─────────────────────────────────────────────
    function getTzId(this: any, tz?: any) {
        if (isHomeAssistantAutoTimezone(tz))
            return "UTC";
        var idx: any = tz.indexOf(" (");
        return idx > 0 ? tz.substring(0, idx) : tz;
    }
    function formatGmtOffset(this: any, minutes?: any) {
        var sign: any = minutes >= 0 ? "+" : "-";
        var abs: any = Math.abs(minutes);
        var h: any = Math.floor(abs / 60);
        var m: any = abs % 60;
        return "GMT" + sign + h + (m ? ":" + String(m).padStart(2, "0") : "");
    }
    function timezoneOffsetMinutes(this: any, tzId?: any, date?: any) {
        try {
            var parts: any = new Intl.DateTimeFormat("en-US", {
                timeZone: tzId,
                hourCycle: "h23",
                year: "numeric",
                month: "2-digit",
                day: "2-digit",
                hour: "2-digit",
                minute: "2-digit",
                second: "2-digit"
            }).formatToParts(date);
            var values: any = {};
            for (var i: any = 0; i < parts.length; i++) {
                if (parts[i].type !== "literal")
                    values[parts[i].type] = parts[i].value;
            }
            var localAsUtc: any = Date.UTC(Number(values.year), Number(values.month) - 1, Number(values.day), Number(values.hour), Number(values.minute), Number(values.second));
            return Math.round((localAsUtc - date.getTime()) / 60000);
        }
        catch (_) {
            return null;
        }
    }
    function formatTimezoneOption(this: any, opt?: any) {
        if (isHomeAssistantAutoTimezone(opt))
            return opt;
        var tzId: any = getTzId(opt);
        var offset: any = timezoneOffsetMinutes(tzId, webserverNow());
        if (offset == null || !isFinite(offset))
            return opt;
        return tzId + " (" + formatGmtOffset(offset) + ")";
    }
    function appendTimezoneOption(this: any, select?: any, opt?: any) {
        var o: any = document.createElement("option");
        o.value = opt;
        o.textContent = formatTimezoneOption(opt);
        select.appendChild(o);
    }
    function updateClockText(this: any) {
        if (!els.clock)
            return;
        var now: any = webserverNow();
        var tzId: any = getTzId(effectiveTimezoneOptionForWeb(state.timezone));
        try {
            var parts: any = new Intl.DateTimeFormat("en-US", {
                timeZone: tzId, hour: "numeric", minute: "2-digit",
                hour12: state.clockFormat === "12h"
            }).formatToParts(now);
            var h: any = "", m: any = "";
            for (var i: any = 0; i < parts.length; i++) {
                if (parts[i].type === "hour")
                    h = parts[i].value;
                else if (parts[i].type === "minute")
                    m = parts[i].value;
            }
            els.clock.textContent = (state.clockFormat === "24h"
                ? h.padStart(2, "0") : h) + ":" + m;
        }
        catch (_) {
            var hr: any = now.getUTCHours();
            var mn: any = String(now.getUTCMinutes()).padStart(2, "0");
            els.clock.textContent = String(hr).padStart(2, "0") + ":" + mn;
        }
    }
    function updateClock(this: any) {
        updateClockText();
        var now: any = webserverNow();
        var msToNext: any = (60 - now.getSeconds()) * 1000 - now.getMilliseconds();
        setTimeout(updateClock, msToNext + 50);
    }
    function clockBarTemperatureActive(this: any) {
        return clockBarTemperatureVisible();
    }
    function clockBarTemperatureItemId(this: any, index?: any) {
        return index === 0 ? "temperature" : "temperature_" + (index + 1);
    }
    function clockBarTemperatureItemIndex(this: any, item?: any) {
        if (item === "temperature")
            return 0;
        return -1;
    }
    function isClockBarTemperatureItem(this: any, item?: any) {
        return clockBarTemperatureItemIndex(item) >= 0;
    }
    function clockBarTemperatureItemIds(this: any) {
        return ["temperature"];
    }
    function clockBarItems(this: any) {
        var items: any = ["temperature", "time"];
        if (voiceServicesUiState().clockBarItemVisible)
            items.push("voice");
        items.push("network");
        return items;
    }
    function clockBarDefaultSection(this: any, item?: any) {
        if (isClockBarTemperatureItem(item))
            return "left";
        if (item === "time")
            return "middle";
        if (item === "voice")
            return "right";
        if (item === "network")
            return "right";
        return "left";
    }
    function clockBarItemActive(this: any, item?: any) {
        var tempIndex: any = clockBarTemperatureItemIndex(item);
        if (tempIndex >= 0)
            return clockBarTemperatureVisible();
        if (item === "time")
            return !!state.clockBarTimeOn;
        if (item === "voice")
            return voiceServicesUiState().iconVisible;
        if (item === "network")
            return !!state.networkStatusOn;
        return false;
    }
    function clockBarItemElement(this: any, item?: any) {
        return els.clockBarItems && els.clockBarItems[item] || null;
    }
    function clockBarItemLabel(this: any, item?: any) {
        if (isClockBarTemperatureItem(item))
            return "Temperature";
        if (item === "time")
            return "Clock";
        if (item === "voice")
            return "Voice Services";
        if (item === "network")
            return "Connectivity";
        return "Clock Bar";
    }
    function createClockBarItemElement(this: any, item?: any, section?: any) {
        var button: any = document.createElement("div");
        button.className = "sp-clockbar-item sp-clockbar-" + (isClockBarTemperatureItem(item) ? "temperature" : item);
        button.setAttribute("data-clockbar-item", item);
        button.setAttribute("data-clockbar-section", section);
        button.setAttribute("aria-label", clockBarItemLabel(item));
        button.setAttribute("role", "button");
        button.setAttribute("tabindex", "0");
        if (isClockBarTemperatureItem(item)) {
            var temp: any = document.createElement("span");
            temp.className = "sp-temp";
            temp.textContent = "--";
            button.appendChild(temp);
            if (!els.temps)
                els.temps = {};
            els.temps[item] = temp;
        }
        else if (item === "time") {
            var clock: any = document.createElement("span");
            clock.className = "sp-clock";
            clock.textContent = "--:--";
            button.appendChild(clock);
            els.clock = clock;
        }
        else if (item === "network") {
            var network: any = document.createElement("span");
            network.className = "sp-network-preview mdi mdi-wifi-strength-4";
            button.appendChild(network);
            els.networkPreview = network;
        }
        else if (item === "voice") {
            var voice: any = document.createElement("span");
            voice.className = "sp-voice-preview mdi mdi-microphone";
            button.appendChild(voice);
            els.voicePreview = voice;
        }
        return button;
    }
    function renderClockBarLayout(this: any) {
        if (!els.clockBarSections)
            return;
        var layout: any = {
            left: ["temperature"],
            middle: ["time"],
            right: voiceServicesUiState().clockBarItemVisible ? ["voice", "network"] : ["network"],
        };
        els.clockBarItems = {};
        els.temps = {};
        els.clock = null;
        els.networkPreview = null;
        els.voicePreview = null;
        ["left", "middle", "right"].forEach(function (this: any, section?: any) {
            var container: any = els.clockBarSections[section];
            if (!container)
                return;
            container.innerHTML = "";
            var rendered: any = 0;
            layout[section].forEach(function (this: any, item?: any) {
                var itemEl: any = createClockBarItemElement(item, section);
                container.appendChild(itemEl);
                els.clockBarItems[item] = itemEl;
                rendered++;
            });
            container.className = "sp-clockbar-section sp-clockbar-" + section + (rendered ? "" : " sp-clockbar-section-empty");
        });
        updateTempPreview();
        updateClockText();
        updateNetworkPreview();
        updateVoicePreview();
    }
    function syncClockBarItemElement(this: any, item?: any) {
        var el: any = clockBarItemElement(item);
        if (!el)
            return;
        var active: any = clockBarItemActive(item);
        el.className = el.className
            .replace(/\s?sp-clockbar-hidden/g, "")
            .replace(/\s?sp-selected/g, "");
        if (!active)
            el.className += " sp-clockbar-hidden";
        if (state.clockBarSelectedItem === item)
            el.className += " sp-selected";
        el.setAttribute("title", clockBarItemLabel(item));
        el.setAttribute("aria-pressed", state.clockBarSelectedItem === item ? "true" : "false");
    }
    function updateClockBarItemUi(this: any) {
        renderClockBarLayout();
        clockBarItems().forEach(syncClockBarItemElement);
    }
    function syncInput(this: any, el?: any, val?: any) {
        if (el && document.activeElement !== el)
            el.value = val;
    }
    function updateSunInfo(this: any) {
        var el: any = els.sunInfo;
        if (!el)
            return;
        if (normalizeBrightnessMode(state.brightnessMode) !== "sunrise_sunset" ||
            (!state.sunrise && !state.sunset)) {
            el.classList.remove("sp-visible");
            return;
        }
        el.classList.add("sp-visible");
        var parts: any = [];
        if (state.sunrise)
            parts.push("Sunrise: " + state.sunrise);
        if (state.sunset)
            parts.push("Sunset: " + state.sunset);
        el.textContent = parts.join(" \u00a0/\u00a0 ");
    }
    function updateTempPreview(this: any) {
        if (!els.temps)
            return;
        var show: any = clockBarVisibleInPreview();
        var unit: any = clockBarTemperatureUnitSymbol();
        var sampleValues: any = ["17", "24", "21", "19", "22", "18"];
        clockBarTemperatureItemIds().forEach(function (this: any, item?: any, index?: any) {
            var el: any = els.temps[item];
            if (!el)
                return;
            var configured: any = primaryClockBarTemperatureEntity();
            var value: any = configured ? (sampleValues[index] || "--") : "--";
            if (index === 0 && state._outdoorVal != null)
                value = state._outdoorVal;
            el.className = "sp-temp" + (show ? " sp-visible" : "");
            el.textContent = value + unit;
        });
    }
    function normalizeNetworkTransport(this: any, value?: any) {
        value = String(value == null ? "" : value).trim().toLowerCase();
        return value === "ethernet" ? "ethernet" : "wifi";
    }
    function normalizeWifiStrengthPercent(this: any, value?: any) {
        var n: any = parseFloat(value);
        if (!isFinite(n))
            return 100;
        if (n < 0)
            return 0;
        if (n > 100)
            return 100;
        return n;
    }
    function networkPreviewIconSlug(this: any, transport?: any, strengthPercent?: any) {
        if (normalizeNetworkTransport(transport) === "ethernet")
            return "ethernet";
        var strength: any = normalizeWifiStrengthPercent(strengthPercent);
        if (strength < 25)
            return "wifi-strength-1";
        if (strength < 50)
            return "wifi-strength-2";
        if (strength < 75)
            return "wifi-strength-3";
        return "wifi-strength-4";
    }
    function updateNetworkPreview(this: any) {
        if (!els.networkPreview)
            return;
        var show: any = clockBarVisibleInPreview();
        els.networkPreview.className = "sp-network-preview mdi mdi-" +
            networkPreviewIconSlug(state.networkTransport, state.wifiStrengthPercent) +
            (show ? " sp-visible" : "");
    }
    function updateVoicePreview(this: any) {
        if (!els.voicePreview)
            return;
        var show: any = clockBarVisibleInPreview();
        els.voicePreview.className = "sp-voice-preview mdi mdi-microphone" +
            (show && voiceServicesUiState().iconVisible ? " sp-visible" : "");
    }
    return {
        getTzId,
        formatGmtOffset,
        timezoneOffsetMinutes,
        formatTimezoneOption,
        appendTimezoneOption,
        updateClockText,
        updateClock,
        clockBarTemperatureActive,
        clockBarTemperatureItemId,
        clockBarTemperatureItemIndex,
        isClockBarTemperatureItem,
        clockBarTemperatureItemIds,
        clockBarItems,
        clockBarDefaultSection,
        clockBarItemActive,
        clockBarItemElement,
        clockBarItemLabel,
        createClockBarItemElement,
        renderClockBarLayout,
        syncClockBarItemElement,
        updateClockBarItemUi,
        syncInput,
        updateSunInfo,
        updateTempPreview,
        normalizeNetworkTransport,
        normalizeWifiStrengthPercent,
        networkPreviewIconSlug,
        updateNetworkPreview,
        updateVoicePreview,
    };
}
