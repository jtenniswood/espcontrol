import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function registerLawnMowerCardTypes(): GlobalDescriptors {
    // Lawn Mower card: touchscreen-friendly controls for Home Assistant mower entities.
    var LAWN_MOWER_CARD_MODES: any = [
        ["status", "Status"],
        ["start_mowing", "Start Mowing"],
        ["dock", "Dock"],
        ["pause_resume", "Pause / Resume"],
    ];
    function lawnMowerModeValues(this: any) {
        return entityModeValues("lawn_mower", "lawn_mower_mode", LAWN_MOWER_CARD_MODES);
    }
    function normalizeLawnMowerMode(this: any, mode?: any) {
        return normalizeEntityMode(mode, lawnMowerModeValues(), "start_mowing");
    }
    function lawnMowerModeDefaultIcon(this: any, mode?: any) {
        mode = normalizeLawnMowerMode(mode);
        if (mode === "dock")
            return "Robot Mower Outline";
        return "Robot Mower";
    }
    function lawnMowerModeBadgeIcon(this: any, mode?: any) {
        mode = normalizeLawnMowerMode(mode);
        if (mode === "status")
            return "format-text";
        if (mode === "dock")
            return "home-import-outline";
        if (mode === "pause_resume")
            return "play-pause";
        return "robot-mower";
    }
    function lawnMowerUsesDefaultIcon(this: any, icon?: any) {
        return entityModeCardUsesDefaultIcon(icon, [
            "Lawnmower",
            "Robot Mower",
            "Robot Mower Outline",
        ]);
    }
    function normalizeLawnMowerConfig(this: any, b?: any) {
        normalizeEntityModeCardConfig(b, {
            normalizeMode: normalizeLawnMowerMode,
            defaultIcon: lawnMowerModeDefaultIcon,
        });
    }
    var LAWN_MOWER_CARD_METADATA: any = {
        mode: {
            label: "Type",
            idSuffix: "lawn-mower-type",
            options: LAWN_MOWER_CARD_MODES,
            value: function (this: any, b?: any) {
                return normalizeLawnMowerMode(b.sensor);
            },
        },
        entity: {
            label: "Lawn Mower Entity",
            idSuffix: "lawn-mower-entity",
            placeholder: "e.g. lawn_mower.backyard",
            domains: function (this: any) { return cardContractDomains("lawn_mower"); },
            bindName: "entity",
            rerender: true,
            requiredMessage: "Add a lawn mower entity before saving.",
        },
        labelField: {
            label: "Label",
            idSuffix: "lawn-mower-label",
            field: "label",
            placeholder: "e.g. Backyard Mower",
            rerender: true,
        },
    };
    registerButtonType("lawn_mower", {
        label: function (this: any) { return cardContractCardLabel("lawn_mower"); },
        allowInSubpage: function (this: any) { return cardContractAllowInSubpage("lawn_mower"); },
        pickerKey: function (this: any) { return cardContractPickerKey("lawn_mower"); },
        hidden: function (this: any) { return cardContractHidden("lawn_mower"); },
        hideLabel: true,
        defaultConfig: function (this: any) { return cardContractDefaultConfig("lawn_mower"); },
        cardMetadata: LAWN_MOWER_CARD_METADATA,
        normalizeConfig: normalizeLawnMowerConfig,
        onSelect: function (this: any, b?: any) {
            var defaults: any = cardContractDefaultConfig("lawn_mower");
            Object.keys(defaults).forEach(function (this: any, key?: any) {
                if (key !== "entity")
                    b[key] = defaults[key];
            });
        },
        renderSettings: function (this: any, panel?: any, b?: any, slot?: any, helpers?: any) {
            var mode: any = normalizeLawnMowerMode(b.sensor);
            if (b.sensor !== mode) {
                b.sensor = mode;
                helpers.saveField("sensor", mode);
            }
            b.unit = "";
            b.precision = "";
            b.options = "";
            b.icon_on = "Auto";
            if (!b.icon || b.icon === "Auto") {
                b.icon = lawnMowerModeDefaultIcon(mode);
                helpers.saveField("icon", b.icon);
            }
            helpers.renderCardModeSelector(panel, b, helpers, Object.assign({}, LAWN_MOWER_CARD_METADATA, {
                mode: Object.assign({}, LAWN_MOWER_CARD_METADATA.mode, {
                    value: function (this: any) { return mode; },
                    onChange: function (this: any) {
                        var oldMode: any = mode;
                        mode = normalizeLawnMowerMode(this.value);
                        applyEntityModeCardModeChange(b, helpers, oldMode, mode, {
                            defaultIcon: lawnMowerModeDefaultIcon,
                            usesDefaultIcon: lawnMowerUsesDefaultIcon,
                        });
                        renderButtonSettings();
                    },
                }),
            }));
            helpers.renderCardEntityField(panel, b, helpers, LAWN_MOWER_CARD_METADATA);
            helpers.renderCardTextField(panel, b, helpers, LAWN_MOWER_CARD_METADATA.labelField);
            helpers.renderCardIconPicker(panel, b, helpers, {
                pickerIdSuffix: "lawn-mower-icon-picker",
                idSuffix: "lawn-mower-icon",
                field: "icon",
                fallback: function (this: any) { return lawnMowerModeDefaultIcon(mode); },
                label: "Icon",
            });
        },
        renderPreview: function (this: any, b?: any, helpers?: any) {
            var mode: any = normalizeLawnMowerMode(b.sensor);
            var label: any = b.label || b.entity || "Lawn Mower";
            var iconName: any = b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : iconSlug(lawnMowerModeDefaultIcon(mode));
            var stateBadge: any = mode === "status" ? '<span class="sp-sensor-badge mdi mdi-format-text"></span>' : "";
            return {
                iconHtml: stateBadge + '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>',
                labelHtml: cardBadgeLabelHtml(helpers, label, lawnMowerModeBadgeIcon(mode)),
            };
        },
    });
    return {
        "LAWN_MOWER_CARD_MODES": liveGlobal(() => LAWN_MOWER_CARD_MODES, (value?: any) => { LAWN_MOWER_CARD_MODES = value; }),
        "lawnMowerModeValues": staticGlobal(lawnMowerModeValues),
        "normalizeLawnMowerMode": staticGlobal(normalizeLawnMowerMode),
        "lawnMowerModeDefaultIcon": staticGlobal(lawnMowerModeDefaultIcon),
        "lawnMowerModeBadgeIcon": staticGlobal(lawnMowerModeBadgeIcon),
        "lawnMowerUsesDefaultIcon": staticGlobal(lawnMowerUsesDefaultIcon),
        "normalizeLawnMowerConfig": staticGlobal(normalizeLawnMowerConfig),
        "LAWN_MOWER_CARD_METADATA": liveGlobal(() => LAWN_MOWER_CARD_METADATA, (value?: any) => { LAWN_MOWER_CARD_METADATA = value; }),
    };
}
