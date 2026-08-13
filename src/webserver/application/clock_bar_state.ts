import { state } from "../state/app_instance";
import { normalizeTemperatureUnit } from "../model/settings";
import type { ClockBarController } from "../features/clock_bar_controller";
import type { UiRuntimeState } from "./state";
import type { CoreFeature } from "./core";
import type { EnvironmentStateFeature } from "./environment_state";

export interface ClockBarFeature {
    controllerState(): any;
    applyControllerState(next?: any): void;
    uiState(): any;
    setEnabled(enabled?: any): void;
    setNightModeEnabled(enabled?: any): void;
    visibleInPreview(): boolean;
    temperatureUnitSymbol(): string;
    clockBarTemperatureUnitSymbol(): string;
    normalizeTemperatureEntries(value?: any): string[];
    normalizeTemperatureEntities(value?: any): string[];
    serializeTemperatureEntities(list?: any): string;
    temperatureEntities(): string[];
    primaryTemperatureEntity(): string;
    temperatureVisible(): boolean;
    applyTemperatureEntities(list?: any, postDevice?: any): void;
    saveTemperatureSettings(entity?: any, degreeSymbolOn?: any): void;
    setItemVisible(item?: any, visible?: any): void;
    syncTemperatureUi(): void;
    syncUi(): void;
}

export function createClockBarFeature(
    clockBarController: ClockBarController,
    runtime: UiRuntimeState,
    core: Pick<CoreFeature, "syncPreviewGridTop">,
    environment: EnvironmentStateFeature,
    dependencies: {
        hideSettingsOverlay(): void;
        timezoneId(value?: any): string;
        postTemperatureEntities(value: string): void;
        postSwitch(name: string, value: boolean): void;
        entityName(key: string): string;
        postText(name: string, value: string): void;
        updateTemperaturePreview(): void;
        updateItemUi(): void;
        postTemperatureDegreeSymbol(value: boolean): void;
        isTemperatureItem(item?: any): boolean;
        postTime(value: boolean): void;
        postVoiceServices(value: boolean): void;
        postNetworkStatus(value: boolean): void;
        renderSelectionBar(): void;
        updateNetworkPreview(): void;
        updateVoicePreview(): void;
    },
): ClockBarFeature {
    const { syncPreviewGridTop } = core;
    const els = runtime.els;
    const { effectiveTimezoneOptionForWeb, voiceServicesUiState, setVoiceServicesEnabled } = environment;
    // ── Clock Bar State ───────────────────────────────────────────────────
    var clockBarControllerInstance: ClockBarController = clockBarController;
    function clockBarControllerState(this: any) {
        return {
            enabled: !!state.clockBarOn,
            timeEnabled: !!state.clockBarTimeOn,
            nightModeEnabled: !!state.clockBarNightModeOn,
            selectedItem: state.clockBarSelectedItem || "",
        };
    }
    function applyClockBarControllerState(this: any, next?: any) {
        state.clockBarOn = next.enabled;
        state.clockBarTimeOn = next.timeEnabled;
        state.clockBarNightModeOn = next.nightModeEnabled;
        state.clockBarSelectedItem = next.selectedItem;
    }
    function clockBarUiState(this: any) {
        return clockBarControllerInstance.uiState(clockBarControllerState());
    }
    function setClockBarEnabled(this: any, enabled?: any) {
        var current: any = clockBarControllerState();
        var next: any = clockBarControllerInstance.setEnabled(current, enabled);
        // The editor belongs to the selected preview item. Close it before the
        // controller removes that selection, otherwise its modal can outlive
        // the Clock Bar that contained the item.
        if (!next.enabled && current.selectedItem)
            dependencies.hideSettingsOverlay();
        applyClockBarControllerState(next);
    }
    function setNightModeEnabled(enabled?: any) {
        applyClockBarControllerState(clockBarControllerInstance.setNightModeEnabled(clockBarControllerState(), enabled));
    }
    function clockBarVisibleInPreview(this: any) {
        return clockBarUiState().previewVisible;
    }
    function timezonePrefersFahrenheit(this: any, timezone?: any) {
        var tz: any = dependencies.timezoneId(effectiveTimezoneOptionForWeb(timezone || state.timezone));
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
    const maxTemperatures = 1;
    function defaultClockBarTemperatureEntity(this: any, index?: any) {
        if (index === 0)
            return "sensor.outdoor_temperature";
        return "";
    }
    function normalizeClockBarTemperatureEntries(this: any, value?: any) {
        var input: any = Array.isArray(value) ? value : String(value || "").split(/[|,\n]/);
        return input.map(function (this: any, entry?: any) {
            return String(entry || "").trim();
        }).slice(0, maxTemperatures);
    }
    function normalizeClockBarTemperatureEntities(this: any, value?: any) {
        var input: any = normalizeClockBarTemperatureEntries(value);
        var out: any = [];
        input.forEach(function (this: any, entry?: any) {
            if (entry && out.indexOf(entry) === -1)
                out.push(entry);
        });
        return out.slice(0, maxTemperatures);
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
            dependencies.postTemperatureEntities(serializeClockBarTemperatureEntities(state.clockBarTemperatureEntities));
            dependencies.postSwitch(dependencies.entityName("outdoor_temp_enable"), state._outdoorOn);
            dependencies.postSwitch(dependencies.entityName("indoor_temp_enable"), state._indoorOn);
            dependencies.postText(dependencies.entityName("outdoor_temp_entity"), state.outdoorEntity);
            dependencies.postText(dependencies.entityName("indoor_temp_entity"), state.indoorEntity);
        }
        syncTemperatureUi();
        dependencies.updateTemperaturePreview();
        dependencies.updateItemUi();
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
        dependencies.postTemperatureEntities(serializeClockBarTemperatureEntities(state.clockBarTemperatureEntities));
        dependencies.postSwitch(dependencies.entityName("outdoor_temp_enable"), state._outdoorOn);
        dependencies.postSwitch(dependencies.entityName("indoor_temp_enable"), false);
        dependencies.postText(dependencies.entityName("outdoor_temp_entity"), state.outdoorEntity);
        dependencies.postText(dependencies.entityName("indoor_temp_entity"), "");
        dependencies.postTemperatureDegreeSymbol(state.temperatureDegreeSymbolOn);
        syncTemperatureUi();
        syncClockBarUi();
    }
    function setClockBarItemVisible(this: any, item?: any, visible?: any) {
        visible = !!visible;
        if (dependencies.isTemperatureItem(item)) {
            var entity: any = primaryClockBarTemperatureEntity();
            if (visible && !entity) {
                entity = defaultClockBarTemperatureEntity(0);
                state.clockBarTemperatureEntities = [entity];
                state._clockBarTemperatureEntitiesReceived = true;
                state.outdoorEntity = entity;
                dependencies.postTemperatureEntities(entity);
                dependencies.postText(dependencies.entityName("outdoor_temp_entity"), entity);
            }
            state._clockBarTemperatureVisibilityReceived = true;
            state._outdoorOn = visible && !!entity;
            state._indoorOn = false;
            dependencies.postSwitch(dependencies.entityName("outdoor_temp_enable"), state._outdoorOn);
            dependencies.postSwitch(dependencies.entityName("indoor_temp_enable"), false);
            dependencies.postText(dependencies.entityName("indoor_temp_entity"), "");
        }
        else if (item === "time") {
            state.clockBarTimeOn = visible;
            dependencies.postTime(state.clockBarTimeOn);
        }
        else if (item === "voice" && voiceServicesUiState().clockBarItemVisible) {
            setVoiceServicesEnabled(visible);
            dependencies.postVoiceServices(state.voiceServicesOn);
        }
        else if (item === "network") {
            state.networkStatusOn = visible;
            dependencies.postNetworkStatus(state.networkStatusOn);
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
        var before: any = clockBarControllerState();
        applyClockBarControllerState(clockBarControllerInstance.reconcile(before));
        var uiState: any = clockBarUiState();
        var visible: any = uiState.previewVisible;
        if (!visible && before.selectedItem) {
            dependencies.hideSettingsOverlay();
        }
        syncPreviewGridTop();
        if (els.topbar)
            els.topbar.className = "sp-topbar" + (visible ? "" : " sp-hidden");
        if (els.setClockBarToggle)
            els.setClockBarToggle.checked = uiState.previewVisible;
        if (els.setClockBarTimeToggle)
            els.setClockBarTimeToggle.checked = !!state.clockBarTimeOn;
        if (els.setClockBarNightModeToggle)
            els.setClockBarNightModeToggle.checked = !!state.clockBarNightModeOn;
        if (els.setNetworkStatusToggle) {
            els.setNetworkStatusToggle.checked = !!state.networkStatusOn;
        }
        if (els.setVoiceServicesToggle) {
            els.setVoiceServicesToggle.checked = voiceServicesUiState().iconVisible;
        }
        if (els.setBatteryStatusToggle) {
            els.setBatteryStatusToggle.checked = !!state.batteryStatusOn;
        }
        if (els.setClockBarBadge) {
            els.setClockBarBadge.className = "sp-card-badge" + (uiState.badgeVisible ? "" : " sp-hidden");
        }
        if (els.setBatteryStatusBadge) {
            els.setBatteryStatusBadge.className = "sp-card-badge" + (state.batteryStatusOn ? "" : " sp-hidden");
        }
        if (els.setTemperatureDegreeSymbolToggle) {
            els.setTemperatureDegreeSymbolToggle.checked = !!state.temperatureDegreeSymbolOn;
        }
        if (els.setSubpageChevronToggle) {
            els.setSubpageChevronToggle.checked = !!state.subpageChevronsOn;
        }
        dependencies.updateItemUi();
        dependencies.renderSelectionBar();
        dependencies.updateNetworkPreview();
        dependencies.updateVoicePreview();
        dependencies.updateTemperaturePreview();
    }
    return {
        controllerState: clockBarControllerState,
        applyControllerState: applyClockBarControllerState,
        uiState: clockBarUiState,
        setEnabled: setClockBarEnabled,
        setNightModeEnabled,
        visibleInPreview: clockBarVisibleInPreview,
        temperatureUnitSymbol,
        clockBarTemperatureUnitSymbol,
        normalizeTemperatureEntries: normalizeClockBarTemperatureEntries,
        normalizeTemperatureEntities: normalizeClockBarTemperatureEntities,
        serializeTemperatureEntities: serializeClockBarTemperatureEntities,
        temperatureEntities: clockBarTemperatureEntities,
        primaryTemperatureEntity: primaryClockBarTemperatureEntity,
        temperatureVisible: clockBarTemperatureVisible,
        applyTemperatureEntities: applyClockBarTemperatureEntities,
        saveTemperatureSettings: saveClockBarTemperatureSettings,
        setItemVisible: setClockBarItemVisible,
        syncTemperatureUi,
        syncUi: syncClockBarUi,
    };
}
