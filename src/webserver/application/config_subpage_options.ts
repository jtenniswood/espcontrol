import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installConfigSubpageOptionsModule(): GlobalDescriptors {
    // ── Subpage Card Options ───────────────────────────────────────────
    function normalizeSubpageKind(this: any, value?: any) {
        value = String(value || "").trim();
        return subpagePresetDefaults(value) ? value : "";
    }
    function subpageKind(this: any, b?: any) {
        return normalizeSubpageKind(configOptionValue(b && b.options, SUBPAGE_KIND_OPTION));
    }
    var SUBPAGE_KIND_PRESET_DEFINITIONS: any = [
        { value: "", label: "Generic" },
        { value: "switch", label: "Switch", preset: { label: "Switch", icon: "Power Plug", entityDomains: ["light", "switch", "input_boolean", "fan"], placeholder: "e.g. switch.living_room" } },
        { value: "lights", label: "Lights", preset: { label: "Lighting", icon: "Lightbulb", entityDomains: ["light"], placeholder: "e.g. light.living_room" } },
        { value: "climate", label: "Climate", preset: { label: "Climate", icon: "Thermostat", entityDomains: ["climate"], placeholder: "e.g. climate.living_room" } },
        { value: "presence", label: "Presence", preset: { label: "Presence", icon: "Account", entityDomains: ["person", "device_tracker", "binary_sensor", "input_boolean"], placeholder: "e.g. person.jane" } },
        { value: "media", label: "Media", preset: { label: "Media", icon: "Speaker", entityDomains: ["media_player"], placeholder: "e.g. media_player.living_room" } },
        { value: "alarm", label: "Alarm", preset: { label: "Alarm", icon: "Security", entityDomains: ["alarm_control_panel"], placeholder: "e.g. alarm_control_panel.home" } },
        { value: "cover", label: "Cover", preset: { label: "Cover", icon: "Blinds", entityDomains: ["cover"], placeholder: "e.g. cover.office_blind" } },
        { value: "garage", label: "Garage Door", preset: { label: "Garage", icon: "Garage", entityDomains: ["cover"], placeholder: "e.g. cover.garage_door" } },
        { value: "gate", label: "Gate", preset: { label: "Gate", icon: "Gate", entityDomains: ["cover"], placeholder: "e.g. cover.driveway_gate" } },
        { value: "lock", label: "Lock", preset: { label: "Lock", icon: "Lock", entityDomains: ["lock"], placeholder: "e.g. lock.front_door" } },
        { value: "vacuum", label: "Vacuum", preset: { label: "Vacuum", icon: "Robot Vacuum", entityDomains: ["vacuum"], placeholder: "e.g. vacuum.downstairs" } },
        { value: "lawn_mower", label: "Lawn Mower", preset: { label: "Lawn Mower", icon: "Robot Mower", entityDomains: ["lawn_mower"], placeholder: "e.g. lawn_mower.backyard" } },
        { value: "weather", label: "Weather", preset: { label: "Weather", icon: "Weather Partly Cloudy", entityDomains: ["weather"], placeholder: "e.g. weather.home" } },
        { value: "sensor", label: "Sensor", preset: { label: "Sensor", icon: "Gauge", entityDomains: ["sensor", "binary_sensor", "text_sensor"], placeholder: "e.g. sensor.open_windows" } },
        { value: "image", label: "Camera/Image", preset: { label: "Camera", icon: "Camera", entityDomains: ["camera", "image"], placeholder: "e.g. camera.front_door" } },
    ];
    function subpageKindOptions(this: any) {
        return SUBPAGE_KIND_PRESET_DEFINITIONS.map(function (this: any, definition?: any) {
            return [definition.value, definition.label];
        });
    }
    function subpagePresetDefaults(this: any, kind?: any) {
        kind = String(kind || "").trim();
        for (var i: any = 0; i < SUBPAGE_KIND_PRESET_DEFINITIONS.length; i++) {
            var definition: any = SUBPAGE_KIND_PRESET_DEFINITIONS[i];
            if (definition.value === kind)
                return definition.preset || null;
        }
        return null;
    }
    function applySubpagePresetConfig(this: any, b?: any, forceDisplayDefaults?: any) {
        if (!b)
            return;
        var defaults: any = subpagePresetDefaults(subpageKind(b));
        if (!defaults)
            return;
        if (forceDisplayDefaults || !b.label)
            b.label = defaults.label;
        if (forceDisplayDefaults || !b.icon || b.icon === "Auto")
            b.icon = defaults.icon;
        b.icon_on = "Auto";
        b.sensor = "indicator";
        b.unit = "";
        b.precision = "";
    }
    function normalizeSubpageOptions(this: any, options?: any, sensor?: any, precision?: any) {
        var out: any = "";
        var kind: any = normalizeSubpageKind(configOptionValue(options, SUBPAGE_KIND_OPTION));
        if (kind)
            out = setConfigOptionValue(out, SUBPAGE_KIND_OPTION, kind);
        if (sensor && sensor !== "indicator" && precision !== "text" &&
            (configOptionEnabled(options, SENSOR_LARGE_NUMBERS_OPTION) || largeNumbersExplicitlyDisabled(options))) {
            out = copyLargeNumbersOption(out, options);
        }
        return out;
    }
    return {
        "normalizeSubpageKind": staticGlobal(normalizeSubpageKind),
        "subpageKind": staticGlobal(subpageKind),
        "SUBPAGE_KIND_PRESET_DEFINITIONS": liveGlobal(() => SUBPAGE_KIND_PRESET_DEFINITIONS, (value?: any) => { SUBPAGE_KIND_PRESET_DEFINITIONS = value; }),
        "subpageKindOptions": staticGlobal(subpageKindOptions),
        "subpagePresetDefaults": staticGlobal(subpagePresetDefaults),
        "applySubpagePresetConfig": staticGlobal(applySubpagePresetConfig),
        "normalizeSubpageOptions": staticGlobal(normalizeSubpageOptions),
    };
}
