import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
import {
    normalizeSavedConfigVacuumIconOn,
    normalizeSavedConfigVacuumOptions,
    normalizeSavedConfigVacuumPrecision,
    normalizeSavedConfigVacuumSensor,
} from "../generated/saved_config_vacuum";
export function registerVacuumCardTypes(): GlobalDescriptors {
    // Vacuum card: touchscreen-friendly controls for Home Assistant vacuum entities.
    var VACUUM_CARD_MODES: any = [
        ["status", "Status"],
        ["start_stop", "Start / Stop"],
        ["dock", "Dock"],
        ["pause_resume", "Pause / Resume"],
        ["clean_spot", "Spot Clean"],
        ["locate", "Locate"],
        ["clean_area", "Clean Area"],
    ];
    function vacuumModeValues(this: any) {
        return entityModeValues("vacuum", "vacuum_mode", VACUUM_CARD_MODES);
    }
    function normalizeVacuumMode(this: any, mode?: any) {
        return normalizeSavedConfigVacuumSensor(String(mode || ""));
    }
    function vacuumModeNeedsArea(this: any, mode?: any) {
        return normalizeVacuumMode(mode) === "clean_area";
    }
    function vacuumModeDefaultIcon(this: any, mode?: any) {
        mode = normalizeVacuumMode(mode);
        if (mode === "dock")
            return "Robot Vacuum Variant";
        if (mode === "clean_spot")
            return "Vacuum";
        if (mode === "locate")
            return "Robot Vacuum Alert";
        if (mode === "clean_area")
            return "Vacuum Outline";
        return "Robot Vacuum";
    }
    function vacuumModeBadgeIcon(this: any, mode?: any) {
        mode = normalizeVacuumMode(mode);
        if (mode === "dock")
            return "home-import-outline";
        if (mode === "pause_resume")
            return "play-pause";
        if (mode === "clean_spot")
            return "vacuum";
        if (mode === "locate")
            return "map-marker-question";
        if (mode === "clean_area")
            return "map-marker-path";
        return "robot-vacuum";
    }
    function vacuumUsesDefaultIcon(this: any, icon?: any) {
        return entityModeCardUsesDefaultIcon(icon, [
            "Robot Vacuum",
            "Robot Vacuum Alert",
            "Robot Vacuum Off",
            "Robot Vacuum Variant",
            "Robot Vacuum Variant Alert",
            "Robot Vacuum Variant Off",
            "Vacuum",
            "Vacuum Outline",
        ]);
    }
    function normalizeVacuumConfig(this: any, b?: any) {
        if (!b)
            return;
        b.sensor = normalizeSavedConfigVacuumSensor(String(b.sensor || ""));
        b.unit = vacuumModeNeedsArea(b.sensor) ? (b.unit || "") : "";
        b.precision = normalizeSavedConfigVacuumPrecision(String(b.precision || ""));
        b.options = normalizeSavedConfigVacuumOptions(String(b.options || ""));
        b.icon_on = normalizeSavedConfigVacuumIconOn(String(b.icon_on || ""));
        if (!b.icon || b.icon === "Auto")
            b.icon = vacuumModeDefaultIcon(b.sensor);
    }
    var VACUUM_CARD_METADATA: any = {
        mode: {
            label: "Type",
            idSuffix: "vacuum-type",
            options: VACUUM_CARD_MODES,
            value: function (this: any, b?: any) {
                return normalizeVacuumMode(b.sensor);
            },
        },
        entity: {
            label: "Vacuum Entity",
            idSuffix: "vacuum-entity",
            placeholder: "e.g. vacuum.kitchen",
            domains: function (this: any) { return cardContractDomains("vacuum"); },
            bindName: "entity",
            rerender: true,
            requiredMessage: "Add a vacuum entity before saving.",
        },
        labelField: {
            label: "Label",
            idSuffix: "vacuum-label",
            field: "label",
            rerender: true,
        },
    };
    registerButtonType("vacuum", {
        label: function (this: any) { return cardContractCardLabel("vacuum"); },
        allowInSubpage: function (this: any) { return cardContractAllowInSubpage("vacuum"); },
        pickerKey: function (this: any) { return cardContractPickerKey("vacuum"); },
        hidden: function (this: any) { return cardContractHidden("vacuum"); },
        hideLabel: true,
        defaultConfig: function (this: any) { return cardContractDefaultConfig("vacuum"); },
        cardMetadata: VACUUM_CARD_METADATA,
        normalizeConfig: normalizeVacuumConfig,
        onSelect: function (this: any, b?: any) {
            var defaults: any = cardContractDefaultConfig("vacuum");
            Object.keys(defaults).forEach(function (this: any, key?: any) {
                if (key !== "entity")
                    b[key] = defaults[key];
            });
        },
        renderSettings: function (this: any, panel?: any, b?: any, slot?: any, helpers?: any) {
            var mode: any = normalizeVacuumMode(b.sensor);
            if (b.sensor !== mode) {
                b.sensor = mode;
                helpers.saveField("sensor", mode);
            }
            b.precision = "";
            b.options = "";
            b.icon_on = "Auto";
            if (!vacuumModeNeedsArea(mode) && b.unit) {
                b.unit = "";
                helpers.saveField("unit", "");
            }
            if (!b.icon || b.icon === "Auto") {
                b.icon = vacuumModeDefaultIcon(mode);
                helpers.saveField("icon", b.icon);
            }
            helpers.renderCardModeSelector(panel, b, helpers, Object.assign({}, VACUUM_CARD_METADATA, {
                mode: Object.assign({}, VACUUM_CARD_METADATA.mode, {
                    value: function (this: any) { return mode; },
                    onChange: function (this: any) {
                        var oldMode: any = mode;
                        mode = normalizeVacuumMode(this.value);
                        applyEntityModeCardModeChange(b, helpers, oldMode, mode, {
                            defaultIcon: vacuumModeDefaultIcon,
                            keepUnit: vacuumModeNeedsArea,
                            usesDefaultIcon: vacuumUsesDefaultIcon,
                        });
                        renderButtonSettings();
                    },
                }),
            }));
            helpers.renderCardEntityField(panel, b, helpers, VACUUM_CARD_METADATA);
            helpers.renderCardTextField(panel, b, helpers, Object.assign({}, VACUUM_CARD_METADATA.labelField, {
                placeholder: mode === "clean_area" ? "e.g. Clean Kitchen" : "e.g. Kitchen Vacuum",
            }));
            if (vacuumModeNeedsArea(mode)) {
                helpers.renderCardTextField(panel, b, helpers, {
                    label: "Area ID",
                    idSuffix: "vacuum-area-id",
                    field: "unit",
                    placeholder: "e.g. kitchen",
                    rerender: false,
                });
            }
            helpers.renderCardIconPicker(panel, b, helpers, {
                pickerIdSuffix: "vacuum-icon-picker",
                idSuffix: "vacuum-icon",
                field: "icon",
                fallback: function (this: any) { return vacuumModeDefaultIcon(mode); },
                label: "Icon",
            });
        },
        renderPreview: function (this: any, b?: any, helpers?: any) {
            var mode: any = normalizeVacuumMode(b.sensor);
            var label: any = b.label || b.entity || "Vacuum";
            var iconName: any = b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : iconSlug(vacuumModeDefaultIcon(mode));
            var stateBadge: any = mode === "status" ? '<span class="sp-sensor-badge mdi mdi-format-text"></span>' : "";
            return {
                iconHtml: stateBadge + '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>',
                labelHtml: cardBadgeLabelHtml(helpers, label, vacuumModeBadgeIcon(mode)),
            };
        },
    });
    return {
        "VACUUM_CARD_MODES": liveGlobal(() => VACUUM_CARD_MODES, (value?: any) => { VACUUM_CARD_MODES = value; }),
        "vacuumModeValues": staticGlobal(vacuumModeValues),
        "normalizeVacuumMode": staticGlobal(normalizeVacuumMode),
        "vacuumModeNeedsArea": staticGlobal(vacuumModeNeedsArea),
        "vacuumModeDefaultIcon": staticGlobal(vacuumModeDefaultIcon),
        "vacuumModeBadgeIcon": staticGlobal(vacuumModeBadgeIcon),
        "vacuumUsesDefaultIcon": staticGlobal(vacuumUsesDefaultIcon),
        "normalizeVacuumConfig": staticGlobal(normalizeVacuumConfig),
        "VACUUM_CARD_METADATA": liveGlobal(() => VACUUM_CARD_METADATA, (value?: any) => { VACUUM_CARD_METADATA = value; }),
    };
}
