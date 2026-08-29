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

export function registerVacuumCardTypes(
    registry: CardRegistry,
    robotOptions: ConfigRobotCardOptionsFeature,
    fields: ControlsFieldsFeature,
    cardUi: CardUiServices,
): void {
    const { renderButtonSettings } = cardUi;
    const { cardBadgeLabelHtml } = fields;
    const {
        vacuumModes,
        normalizeVacuumConfig,
        normalizeVacuumMode,
        vacuumModeBadgeIcon,
        vacuumModeDefaultIcon,
        vacuumModeNeedsArea,
        vacuumUsesDefaultIcon,
    } = robotOptions;
    // Vacuum card: touchscreen-friendly controls for Home Assistant vacuum entities.
    const VACUUM_CARD_METADATA: any = {
        mode: {
            label: "Type",
            idSuffix: "vacuum-type",
            options: vacuumModes,
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
    registry.register("vacuum", {
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
}
