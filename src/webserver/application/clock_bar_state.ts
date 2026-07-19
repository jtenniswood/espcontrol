import { state } from "../state/app_instance";
import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installClockBarStateModule(): GlobalDescriptors {
    // ── Clock Bar State ───────────────────────────────────────────────────
    function clockBarVisibleInPreview(this: any) {
        return !!state.clockBarOn;
    }
    function timezonePrefersFahrenheit(this: any, timezone?: any) {
        var tz: any = getTzId(effectiveTimezoneOptionForWeb(timezone || state.timezone));
        var fahrenheitZones: any = {
            "America/Adak": true,
            "America/Anchorage": true,
            "America/Boise": true,
            "America/Chicago": true,
            "America/Denver": true,
            "America/Detroit": true,
            "America/Juneau": true,
            "America/Los_Angeles": true,
            "America/New_York": true,
            "America/Phoenix": true,
            "America/Puerto_Rico": true,
            "Pacific/Guam": true,
            "Pacific/Honolulu": true,
            "Pacific/Pago_Pago": true,
        };
        return !!fahrenheitZones[tz];
    }
    function temperatureUnitSymbol(this: any) {
        var unit: any = normalizeTemperatureUnit(state.temperatureUnit);
        if (unit === "\u00B0F")
            return "\u00B0F";
        if (unit === "\u00B0C")
            return "\u00B0C";
        return timezonePrefersFahrenheit(state.timezone) ? "\u00B0F" : "\u00B0C";
    }
    function clockBarTemperatureUnitSymbol(this: any) {
        return state.temperatureDegreeSymbolOn ? "\u00B0" : "";
    }
    var MAX_CLOCK_BAR_TEMPERATURES: any = 1;
    function defaultClockBarTemperatureEntity(this: any, index?: any) {
        if (index === 0)
            return "sensor.outdoor_temperature";
        return "";
    }
    function normalizeClockBarTemperatureEntries(this: any, value?: any) {
        var input: any = Array.isArray(value) ? value : String(value || "").split(/[|,\n]/);
        return input.map(function (this: any, entry?: any) {
            return String(entry || "").trim();
        }).slice(0, MAX_CLOCK_BAR_TEMPERATURES);
    }
    function normalizeClockBarTemperatureEntities(this: any, value?: any) {
        var input: any = normalizeClockBarTemperatureEntries(value);
        var out: any = [];
        input.forEach(function (this: any, entry?: any) {
            if (entry && out.indexOf(entry) === -1)
                out.push(entry);
        });
        return out.slice(0, MAX_CLOCK_BAR_TEMPERATURES);
    }
    function serializeClockBarTemperatureEntities(this: any, list?: any) {
        return normalizeClockBarTemperatureEntities(list).join("|");
    }
    function legacyClockBarTemperatureEntities(this: any) {
        var list: any = [];
        if (state._outdoorOn && state.outdoorEntity)
            list.push(state.outdoorEntity);
        if (state._indoorOn && state.indoorEntity)
            list.push(state.indoorEntity);
        return normalizeClockBarTemperatureEntities(list);
    }
    function clockBarTemperatureEntries(this: any) {
        var list: any = normalizeClockBarTemperatureEntries(state.clockBarTemperatureEntities);
        if (!list.length && !state._clockBarTemperatureEntitiesReceived)
            return legacyClockBarTemperatureEntities();
        return list;
    }
    function clockBarTemperatureEntities(this: any) {
        return normalizeClockBarTemperatureEntities(clockBarTemperatureEntries());
    }
    function primaryClockBarTemperatureEntity(this: any) {
        return clockBarTemperatureEntities()[0] || state.outdoorEntity || "";
    }
    function clockBarTemperatureVisible(this: any) {
        return !!(state._outdoorOn && primaryClockBarTemperatureEntity());
    }
    function applyClockBarTemperatureEntities(this: any, list?: any, postDevice?: any) {
        state.clockBarTemperatureEntities = normalizeClockBarTemperatureEntries(list);
        state._clockBarTemperatureEntitiesReceived = true;
        var configured: any = clockBarTemperatureEntities();
        if (!state._clockBarTemperatureVisibilityReceived) {
            state._outdoorOn = configured.length > 0;
        }
        state._indoorOn = false;
        state.outdoorEntity = configured[0] || "";
        state.indoorEntity = "";
        if (postDevice) {
            postClockBarTemperatureEntities(serializeClockBarTemperatureEntities(state.clockBarTemperatureEntities));
            postSwitch(entityName("outdoor_temp_enable"), state._outdoorOn);
            postSwitch(entityName("indoor_temp_enable"), state._indoorOn);
            postText(entityName("outdoor_temp_entity"), state.outdoorEntity);
            postText(entityName("indoor_temp_entity"), state.indoorEntity);
        }
        syncTemperatureUi();
        updateTempPreview();
        updateClockBarItemUi();
    }
    function saveClockBarTemperatureSettings(this: any, entity?: any, degreeSymbolOn?: any) {
        entity = String(entity || "").trim();
        state.clockBarTemperatureEntities = entity ? [entity] : [];
        state._clockBarTemperatureEntitiesReceived = true;
        state._clockBarTemperatureVisibilityReceived = true;
        state._outdoorOn = !!entity;
        state._indoorOn = false;
        state.outdoorEntity = entity;
        state.indoorEntity = "";
        state.temperatureDegreeSymbolOn = !!degreeSymbolOn;
        postClockBarTemperatureEntities(serializeClockBarTemperatureEntities(state.clockBarTemperatureEntities));
        postSwitch(entityName("outdoor_temp_enable"), state._outdoorOn);
        postSwitch(entityName("indoor_temp_enable"), false);
        postText(entityName("outdoor_temp_entity"), state.outdoorEntity);
        postText(entityName("indoor_temp_entity"), "");
        postTemperatureDegreeSymbol(state.temperatureDegreeSymbolOn);
        syncTemperatureUi();
        syncClockBarUi();
    }
    function setClockBarItemVisible(this: any, item?: any, visible?: any) {
        visible = !!visible;
        if (isClockBarTemperatureItem(item)) {
            var entity: any = primaryClockBarTemperatureEntity();
            if (visible && !entity) {
                entity = defaultClockBarTemperatureEntity(0);
                state.clockBarTemperatureEntities = [entity];
                state._clockBarTemperatureEntitiesReceived = true;
                state.outdoorEntity = entity;
                postClockBarTemperatureEntities(entity);
                postText(entityName("outdoor_temp_entity"), entity);
            }
            state._clockBarTemperatureVisibilityReceived = true;
            state._outdoorOn = visible && !!entity;
            state._indoorOn = false;
            postSwitch(entityName("outdoor_temp_enable"), state._outdoorOn);
            postSwitch(entityName("indoor_temp_enable"), false);
            postText(entityName("indoor_temp_entity"), "");
        }
        else if (item === "time") {
            state.clockBarTimeOn = visible;
            postClockBarTime(state.clockBarTimeOn);
        }
        else if (item === "voice" && voiceServicesSupported()) {
            state.voiceServicesOn = visible;
            postVoiceServices(state.voiceServicesOn);
        }
        else if (item === "network") {
            state.networkStatusOn = visible;
            postNetworkStatusIcon(state.networkStatusOn);
        }
        syncClockBarUi();
        syncTemperatureUi();
    }
    function syncTemperatureUi(this: any) {
        if (els.setIndoorToggle)
            els.setIndoorToggle.checked = !!state._indoorOn;
        if (els.setIndoorField) {
            els.setIndoorField.className = "sp-cond-field" + (state._indoorOn ? " sp-visible" : "");
        }
        if (els.setOutdoorToggle)
            els.setOutdoorToggle.checked = !!state._outdoorOn;
        if (els.setOutdoorField) {
            els.setOutdoorField.className = "sp-cond-field" + (state._outdoorOn ? " sp-visible" : "");
        }
    }
    function syncClockBarUi(this: any) {
        var visible: any = clockBarVisibleInPreview();
        if (!visible && state.clockBarSelectedItem) {
            state.clockBarSelectedItem = "";
            hideSettingsOverlay();
        }
        syncPreviewGridTop();
        if (els.topbar)
            els.topbar.className = "sp-topbar" + (visible ? "" : " sp-hidden");
        if (els.setClockBarToggle)
            els.setClockBarToggle.checked = !!state.clockBarOn;
        if (els.setClockBarTimeToggle)
            els.setClockBarTimeToggle.checked = !!state.clockBarTimeOn;
        if (els.setNetworkStatusToggle) {
            els.setNetworkStatusToggle.checked = !!state.networkStatusOn;
        }
        if (els.setVoiceServicesToggle) {
            els.setVoiceServicesToggle.checked = !!state.voiceServicesOn;
        }
        if (els.setClockBarBadge) {
            els.setClockBarBadge.className = "sp-card-badge" + (state.clockBarOn ? "" : " sp-hidden");
        }
        if (els.setTemperatureDegreeSymbolToggle) {
            els.setTemperatureDegreeSymbolToggle.checked = !!state.temperatureDegreeSymbolOn;
        }
        if (els.setSubpageChevronToggle) {
            els.setSubpageChevronToggle.checked = !!state.subpageChevronsOn;
        }
        updateClockBarItemUi();
        renderSelectionBar(ctx());
        updateNetworkPreview();
        updateVoicePreview();
        updateTempPreview();
    }
    return {
        "clockBarVisibleInPreview": staticGlobal(clockBarVisibleInPreview),
        "timezonePrefersFahrenheit": staticGlobal(timezonePrefersFahrenheit),
        "temperatureUnitSymbol": staticGlobal(temperatureUnitSymbol),
        "clockBarTemperatureUnitSymbol": staticGlobal(clockBarTemperatureUnitSymbol),
        "MAX_CLOCK_BAR_TEMPERATURES": liveGlobal(() => MAX_CLOCK_BAR_TEMPERATURES, (value?: any) => { MAX_CLOCK_BAR_TEMPERATURES = value; }),
        "defaultClockBarTemperatureEntity": staticGlobal(defaultClockBarTemperatureEntity),
        "normalizeClockBarTemperatureEntries": staticGlobal(normalizeClockBarTemperatureEntries),
        "normalizeClockBarTemperatureEntities": staticGlobal(normalizeClockBarTemperatureEntities),
        "serializeClockBarTemperatureEntities": staticGlobal(serializeClockBarTemperatureEntities),
        "legacyClockBarTemperatureEntities": staticGlobal(legacyClockBarTemperatureEntities),
        "clockBarTemperatureEntries": staticGlobal(clockBarTemperatureEntries),
        "clockBarTemperatureEntities": staticGlobal(clockBarTemperatureEntities),
        "primaryClockBarTemperatureEntity": staticGlobal(primaryClockBarTemperatureEntity),
        "clockBarTemperatureVisible": staticGlobal(clockBarTemperatureVisible),
        "applyClockBarTemperatureEntities": staticGlobal(applyClockBarTemperatureEntities),
        "saveClockBarTemperatureSettings": staticGlobal(saveClockBarTemperatureSettings),
        "setClockBarItemVisible": staticGlobal(setClockBarItemVisible),
        "syncTemperatureUi": staticGlobal(syncTemperatureUi),
        "syncClockBarUi": staticGlobal(syncClockBarUi),
    };
}
