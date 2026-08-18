import {
    cardContractAllowInSubpage,
    cardContractCard,
    cardContractCardLabel,
    cardContractDefaultConfig,
    cardContractDomains,
    cardContractHidden,
    cardContractPickerKey,
} from "../generated/card_contract";
import { iconSlug } from "../application/ui_primitives";
import type { CardRegistry, CardUiServices } from "../application/card_registry";
import {
    applyEntityModeCardModeChange,
} from "./entity_mode_card";
import type { ConfigRobotCardOptionsFeature } from "../application/config_robot_card_options";
import type { ControlsFieldsFeature } from "../application/controls_fields";

export function registerLawnMowerCardTypes(
    registry: CardRegistry,
    robotOptions: ConfigRobotCardOptionsFeature,
    fields: ControlsFieldsFeature,
    cardUi: CardUiServices,
): void {
    const { renderButtonSettings } = cardUi;
    const { cardBadgeLabelHtml } = fields;
    const {
        lawnMowerModes,
        lawnMowerModeBadgeIcon,
        lawnMowerModeDefaultIcon,
        lawnMowerUsesDefaultIcon,
        normalizeLawnMowerConfig,
        normalizeLawnMowerMode,
    } = robotOptions;
    // Lawn Mower card: touchscreen-friendly controls for Home Assistant mower entities.
    const LAWN_MOWER_CARD_METADATA: any = {
        mode: {
            label: "Type",
            idSuffix: "lawn-mower-type",
            options: lawnMowerModes,
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
    registry.register("lawn_mower", {
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
}
