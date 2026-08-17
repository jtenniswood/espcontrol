import {
    cardContractAllowInSubpage,
    cardContractCard,
    cardContractCardLabel,
    cardContractDefaultConfig,
    cardContractDomains,
    cardContractHidden,
    cardContractPickerKey,
} from "../generated/card_contract";
import type { CardRegistry, CardUiServices } from "../application/card_registry";
import type { ConfigLockOptionsFeature } from "../application/config_lock_options";
import type { ControlsFieldsFeature } from "../application/controls_fields";

export function registerLockCardTypes(
    registry: CardRegistry,
    lockOptions: ConfigLockOptionsFeature,
    fields: ControlsFieldsFeature,
    cardUi: CardUiServices,
): void {
    const { renderButtonSettings } = cardUi;
    const { cardBadgePreview } = fields;
    const {
        lockCommandMode,
        lockModeDefaultIcon,
        lockModeDefaultLabel,
        lockUsesDefaultIcon,
        normalizeLockMode,
    } = lockOptions;
    // Lock card: lock/unlock toggle with safe default-to-lock behavior and state display.
    const LOCK_CARD_METADATA: any = {
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
    registry.register("lock", {
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
}
