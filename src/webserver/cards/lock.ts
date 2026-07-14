import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function registerLockCardTypes(): GlobalDescriptors {
    // Lock card: lock/unlock toggle with safe default-to-lock behavior and state display.
    function lockCommandMode(this: any, mode?: any) {
        return mode === "lock" || mode === "unlock";
    }
    function lockModeOptionValues(this: any) {
        var spec: any = cardContractOptionSpec("lock", "lock_mode");
        return spec && spec.values ? spec.values.slice() : [];
    }
    function normalizeLockMode(this: any, mode?: any) {
        mode = String(mode || "");
        return lockModeOptionValues().indexOf(mode) >= 0 ? mode : "";
    }
    function lockModeDefaultIcon(this: any, mode?: any) {
        return mode === "unlock" ? "Lock Open" : "Lock";
    }
    function lockModeDefaultLabel(this: any, mode?: any) {
        if (mode === "lock")
            return "Lock";
        if (mode === "unlock")
            return "Unlock";
        return "Lock";
    }
    function lockUsesDefaultIcon(this: any, icon?: any) {
        return !icon || icon === "Auto" || icon === "Lock" || icon === "Lock Open";
    }
    var LOCK_CARD_METADATA: any = {
        mode: {
            label: "Type",
            idSuffix: "lock-type",
            options: [
                ["", "Toggle"],
                ["lock", "Lock"],
                ["unlock", "Unlock"],
            ],
            value: function (this: any, b?: any) {
                return normalizeLockMode(b.sensor);
            },
        },
        entity: {
            label: "Entity",
            idSuffix: "entity",
            placeholder: "e.g. lock.front_door",
            domains: function (this: any) { return cardContractDomains("lock"); },
            bindName: "entity",
            rerender: true,
            requiredMessage: "Add an entity before saving.",
        },
        labelField: {
            label: "Label",
            idSuffix: "label",
            field: "label",
            rerender: true,
        },
        preview: {
            badge: "lock",
        },
    };
    registerButtonType("lock", {
        label: function (this: any) { return cardContractCardLabel("lock"); },
        allowInSubpage: function (this: any) { return cardContractAllowInSubpage("lock"); },
        pickerKey: function (this: any) { return cardContractPickerKey("lock"); },
        hidden: function (this: any) { return cardContractHidden("lock"); },
        hideLabel: true,
        defaultConfig: function (this: any) { return cardContractDefaultConfig("lock"); },
        cardMetadata: LOCK_CARD_METADATA,
        onSelect: function (this: any, b?: any) {
            b.label = "";
            b.sensor = "";
            b.unit = "";
            b.precision = "";
            b.icon = "Lock";
            b.icon_on = "Lock Open";
        },
        renderSettings: function (this: any, panel?: any, b?: any, slot?: any, helpers?: any) {
            var mode: any = normalizeLockMode(b.sensor);
            if (b.sensor !== mode) {
                b.sensor = mode;
                helpers.saveField("sensor", mode);
            }
            b.unit = "";
            b.precision = "";
            if (lockCommandMode(mode) && b.icon_on !== "Auto") {
                b.icon_on = "Auto";
                helpers.saveField("icon_on", "Auto");
            }
            else if (!lockCommandMode(mode) && (!b.icon_on || b.icon_on === "Auto")) {
                b.icon_on = "Lock Open";
                helpers.saveField("icon_on", "Lock Open");
            }
            helpers.renderCardModeSelector(panel, b, helpers, Object.assign({}, LOCK_CARD_METADATA, {
                mode: Object.assign({}, LOCK_CARD_METADATA.mode, {
                    value: function (this: any) { return mode; },
                    onChange: function (this: any) {
                        var oldMode: any = mode;
                        var hadDefaultIcon: any = lockUsesDefaultIcon(b.icon);
                        mode = normalizeLockMode(this.value);
                        b.sensor = mode;
                        helpers.saveField("sensor", mode);
                        b.unit = "";
                        b.precision = "";
                        helpers.saveField("unit", "");
                        helpers.saveField("precision", "");
                        if (hadDefaultIcon || b.icon === lockModeDefaultIcon(oldMode)) {
                            b.icon = lockModeDefaultIcon(mode);
                            helpers.saveField("icon", b.icon);
                        }
                        if (lockCommandMode(mode)) {
                            b.icon_on = "Auto";
                        }
                        else if (!b.icon_on || b.icon_on === "Auto") {
                            b.icon_on = "Lock Open";
                        }
                        helpers.saveField("icon_on", b.icon_on);
                        renderButtonSettings();
                    },
                }),
            }));
            helpers.renderCardTextField(panel, b, helpers, Object.assign({}, LOCK_CARD_METADATA.labelField, {
                placeholder: lockCommandMode(mode) ? "e.g. " + lockModeDefaultLabel(mode) + " Front Door" : "e.g. Front Door",
            }));
            helpers.renderCardEntityField(panel, b, helpers, LOCK_CARD_METADATA);
            var lockedIconVal: any = b.icon && b.icon !== "Auto" ? b.icon : "Lock";
            var unlockedIconVal: any = b.icon_on && b.icon_on !== "Auto" ? b.icon_on : "Lock Open";
            if (lockCommandMode(mode)) {
                helpers.renderCardIconPicker(panel, b, helpers, {
                    pickerIdSuffix: "icon-picker",
                    idSuffix: "icon",
                    field: "icon",
                    value: b.icon && b.icon !== "Auto" ? b.icon : lockModeDefaultIcon(mode),
                    fallback: lockModeDefaultIcon(mode),
                    label: "Icon",
                });
            }
            else {
                helpers.renderCardIconPair(panel, b, helpers, {
                    pickerIdSuffix: "icon-picker",
                    idSuffix: "icon",
                    field: "icon",
                    value: lockedIconVal,
                    fallback: "Lock",
                    label: "Locked Icon",
                }, {
                    pickerIdSuffix: "icon-on-picker",
                    idSuffix: "icon-on",
                    field: "icon_on",
                    value: unlockedIconVal,
                    fallback: "Lock Open",
                    label: "Unlocked Icon",
                });
            }
        },
        renderPreview: function (this: any, b?: any, helpers?: any) {
            var mode: any = normalizeLockMode(b.sensor);
            var label: any = b.label || (lockCommandMode(mode) ? lockModeDefaultLabel(mode) : b.entity || "Lock");
            return cardBadgePreview(b, helpers, {
                label: label,
                iconFallback: lockModeDefaultIcon(mode),
                badge: LOCK_CARD_METADATA.preview.badge,
            });
        },
    });
    return {
        "lockCommandMode": staticGlobal(lockCommandMode),
        "lockModeOptionValues": staticGlobal(lockModeOptionValues),
        "normalizeLockMode": staticGlobal(normalizeLockMode),
        "lockModeDefaultIcon": staticGlobal(lockModeDefaultIcon),
        "lockModeDefaultLabel": staticGlobal(lockModeDefaultLabel),
        "lockUsesDefaultIcon": staticGlobal(lockUsesDefaultIcon),
        "LOCK_CARD_METADATA": liveGlobal(() => LOCK_CARD_METADATA, (value?: any) => { LOCK_CARD_METADATA = value; }),
    };
}
