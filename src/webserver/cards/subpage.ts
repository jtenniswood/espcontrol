import { state } from "../state/app_instance";
import { setConfigOptionValue } from "../model/config_primitives";
import { escHtml, iconSlug } from "../application/ui_primitives";
import {
    COMPANION_STATS_OPTIONS,
    COMPANION_SYSTEM_METRICS,
    companionMetricPreviewValue,
} from "./companion";
import type { CardRegistry, CardUiServices } from "../application/card_registry";
import type { ConfigCodecFeature } from "../application/config_codec";
import type { CoreFeature } from "../application/core";
import type { ButtonSettingsSelectionFeature } from "../application/button_settings_selection";
import type { ControlsFieldsFeature } from "../application/controls_fields";
import { SUBPAGE_CONNECTOR_OPTION, SUBPAGE_KIND_OPTION } from "../application/config_option_core";
import {
    applySubpagePresetConfig,
    normalizeSubpageKind,
    normalizeSubpageOptions,
    subpageConnector,
    subpageKind,
    subpageKindOptions,
    subpagePresetDefaults,
} from "../application/config_subpage_options";

function companionStatMetric(entity: any): any {
    return COMPANION_SYSTEM_METRICS.find(function (metric) {
        return metric.id === entity || metric.freeId === entity;
    });
}

function companionStatMode(entity: any): string {
    var metric: any = companionStatMetric(entity);
    return metric && metric.freeId === entity ? "free" : "used";
}

function companionStatEntity(metric: any, mode: string): string {
    return mode === "free" && metric.freeId ? metric.freeId : metric.id;
}

export function registerSubpageCardTypes(
    registry: CardRegistry,
    codec: ConfigCodecFeature,
    core: Pick<CoreFeature, "subpageStateDisplayMode">,
    selection: Pick<ButtonSettingsSelectionFeature, "closeSettings">,
    fields: ControlsFieldsFeature,
    cardUi: CardUiServices,
): void {
    const { renderButtonSettings } = cardUi;
    const { cardSensorPreviewHtml, condField, fieldLabel } = fields;
    const { enterSubpage } = codec;
    const { subpageStateDisplayMode } = core;
    // Navigation folder: tap opens a nested grid screen with its own button layout
    var SUBPAGE_CARD_METADATA: any = {
        kind: {
            label: "Type",
            idSuffix: "subpage-kind",
            options: function (this: any, b?: any) { return subpageKindOptions(subpageConnector(b)); },
        },
        labelField: {
            label: "Label",
            placeholder: "e.g. Lighting",
        },
        icon: {
            field: "icon",
            fallback: "Auto",
            label: "Icon",
        },
        showState: {
            label: "Show State",
            idSuffix: "state-toggle",
            checked: function (this: any, b?: any) { return subpageStateDisplayMode(b) !== "off"; },
        },
        stateMode: {
            label: "Type",
            options: [
                ["icon", "Icon"],
                ["numeric", "Numeric"],
                ["text", "Text"],
            ],
        },
        iconStateEntity: {
            label: "State Entity",
            idSuffix: "icon-state-entity",
            placeholder: "e.g. cover.office_blind",
            domains: ["light", "switch", "input_boolean", "binary_sensor", "cover", "lock", "media_player", "fan", "person", "device_tracker"],
            bindName: null,
            value: function (this: any, b?: any) {
                return subpageStateDisplayMode(b) === "icon" ? (b.entity || "") : "";
            },
        },
        presetEntity: {
            label: function (this: any, b?: any) {
                var defaults: any = subpagePresetDefaults(subpageKind(b));
                return defaults ? defaults.label + " Entity" : "Entity";
            },
            idSuffix: "preset-state-entity",
            placeholder: function (this: any, b?: any) {
                var defaults: any = subpagePresetDefaults(subpageKind(b));
                return defaults ? defaults.placeholder : "e.g. light.living_room";
            },
            domains: function (this: any, b?: any) {
                var defaults: any = subpagePresetDefaults(subpageKind(b));
                return defaults && defaults.entityDomains ? defaults.entityDomains : [];
            },
            bindName: "entity",
            rerender: true,
            requiredMessage: function (this: any, b?: any) {
                var defaults: any = subpagePresetDefaults(subpageKind(b));
                return "Add a " + (defaults ? defaults.label.toLowerCase() : "status") + " entity before saving.";
            },
        },
        sensorEntity: {
            label: "Sensor Entity",
            idSuffix: "sensor",
            placeholder: "e.g. sensor.open_windows",
            domains: ["sensor", "binary_sensor", "text_sensor"],
            bindName: null,
            value: function (this: any, b?: any) {
                return b.sensor && b.sensor !== "indicator" ? b.sensor : "";
            },
        },
        iconOn: {
            field: "icon_on",
            fallback: "Auto",
            label: "On Icon",
        },
        unitField: {
            label: "Unit",
            idSuffix: "unit",
            placeholder: "e.g. %",
            bindName: "unit",
            rerender: false,
        },
        largeNumbers: {
            label: "Large State Numbers",
            idSuffix: "large-state-numbers",
            supported: function (this: any, b?: any) {
                return subpageStateDisplayMode(b) === "numeric";
            },
        },
        preview: {
            badge: "chevron-right",
        },
    };
    var subpageDefinition: any = {
        label: "Subpage",
        allowInSubpage: false,
        hideLabel: true,
        labelPlaceholder: "e.g. Lighting",
        cardMetadata: SUBPAGE_CARD_METADATA,
        onSelect: function (this: any, b?: any) {
            b.entity = "";
            b.sensor = "";
            b.unit = "";
            b.icon = "Auto";
            b.icon_on = "Auto";
            b.options = "";
        },
        renderSettings: function (this: any, panel?: any, b?: any, slot?: any, helpers?: any) {
            var kind: any = subpageKind(b);
            helpers.renderCardModeSelector(panel, b, helpers, {
                mode: Object.assign({}, SUBPAGE_CARD_METADATA.kind, {
                    value: function (this: any) { return kind; },
                    onChange: function (this: any, button?: any, cardHelpers?: any) {
                        var nextKind: any = normalizeSubpageKind(this.value);
                        button.options = setConfigOptionValue(button.options, SUBPAGE_KIND_OPTION, nextKind);
                        if (nextKind === "companion_stat") {
                            var metric: any = COMPANION_SYSTEM_METRICS[0];
                            button.entity = metric ? metric.id : "";
                            button.label = metric ? metric.label : "Processor";
                            button.icon = "Gauge";
                            button.icon_on = "Auto";
                            button.sensor = "indicator";
                            button.unit = metric ? metric.unit : "%";
                            button.precision = "";
                        }
                        else {
                            applySubpagePresetConfig(button, true);
                        }
                        button.options = normalizeSubpageOptions(button.options, button.sensor, button.precision);
                        cardHelpers.saveField("options", button.options);
                        cardHelpers.saveField("label", button.label || "");
                        cardHelpers.saveField("icon", button.icon || "Auto");
                        cardHelpers.saveField("icon_on", button.icon_on || "Auto");
                        cardHelpers.saveField("sensor", button.sensor || "");
                        cardHelpers.saveField("unit", button.unit || "");
                        cardHelpers.saveField("precision", button.precision || "");
                        renderButtonSettings();
                    },
                }),
            });
            if (kind === "companion_stat") {
                var initialMetric: any = companionStatMetric(b.entity) || COMPANION_SYSTEM_METRICS[0];
                if (initialMetric && !companionStatMetric(b.entity)) {
                    b.entity = initialMetric.id;
                    b.sensor = "indicator";
                    b.unit = initialMetric.unit;
                }
                helpers.renderCardTextField(panel, b, helpers, SUBPAGE_CARD_METADATA.labelField);
                helpers.renderCardIconPicker(panel, b, helpers, SUBPAGE_CARD_METADATA.icon);

                var statField: any = document.createElement("div");
                statField.className = "sp-field";
                statField.appendChild(fieldLabel("Statistic", helpers.idPrefix + "companion-stat"));
                var statSelect: any = document.createElement("select");
                statSelect.className = "sp-select";
                statSelect.id = helpers.idPrefix + "companion-stat";
                var statOptions: any = COMPANION_STATS_OPTIONS;
                statOptions.forEach(function (item: any) {
                    var option: any = document.createElement("option");
                    option.value = item[0];
                    option.textContent = item[1];
                    option.selected = item[0] === initialMetric.mode;
                    statSelect.appendChild(option);
                });
                statField.appendChild(statSelect);
                panel.appendChild(statField);

                var displaySelect: any = null;
                if (initialMetric.freeId) {
                    var displayField: any = document.createElement("div");
                    displayField.className = "sp-field";
                    displayField.appendChild(fieldLabel("Show", helpers.idPrefix + "companion-stat-display"));
                    displaySelect = document.createElement("select");
                    displaySelect.className = "sp-select";
                    displaySelect.id = helpers.idPrefix + "companion-stat-display";
                    [["used", "Used"], ["free", "Free"]].forEach(function (item: any) {
                        var option: any = document.createElement("option");
                        option.value = item[0];
                        option.textContent = item[1];
                        displaySelect.appendChild(option);
                    });
                    displaySelect.value = companionStatMode(b.entity);
                    displayField.appendChild(displaySelect);
                    panel.appendChild(displayField);
                }

                function saveCompanionStat(metric: any, mode: string): void {
                    if (!metric) return;
                    var previousMetric: any = companionStatMetric(b.entity);
                    var currentLabel: string = String(b.label || "");
                    if (!currentLabel || (previousMetric && currentLabel === previousMetric.label)) {
                        b.label = metric.label;
                        helpers.saveField("label", b.label);
                    }
                    b.entity = companionStatEntity(metric, mode);
                    b.sensor = "indicator";
                    b.unit = metric.unit;
                    b.precision = "";
                    b.icon_on = "Auto";
                    helpers.saveField("entity", b.entity);
                    helpers.saveField("sensor", b.sensor);
                    helpers.saveField("unit", b.unit);
                    helpers.saveField("precision", b.precision);
                    helpers.saveField("icon_on", b.icon_on);
                    renderButtonSettings();
                }
                statSelect.addEventListener("change", function (this: any) {
                    var selectedMode: string = this.value;
                    var metric: any = COMPANION_SYSTEM_METRICS.find(function (candidate) {
                        return candidate.mode === selectedMode;
                    });
                    saveCompanionStat(metric, displaySelect ? displaySelect.value : "used");
                });
                if (displaySelect) {
                    displaySelect.addEventListener("change", function (this: any) {
                        saveCompanionStat(companionStatMetric(b.entity) || initialMetric, this.value);
                    });
                }
                appendEditSubpageButton(panel, slot);
                return;
            }
            if (subpagePresetDefaults(kind)) {
                helpers.renderCardTextField(panel, b, helpers, SUBPAGE_CARD_METADATA.labelField);
                helpers.renderCardIconPicker(panel, b, helpers, SUBPAGE_CARD_METADATA.icon);
                helpers.renderCardEntityField(panel, b, helpers, {
                    entity: SUBPAGE_CARD_METADATA.presetEntity,
                });
                appendEditSubpageButton(panel, slot);
                return;
            }
            var mode: any = subpageStateDisplayMode(b);
            var showState: any = mode !== "off";
            var sensorEntity: any = b.sensor && b.sensor !== "indicator" ? b.sensor : "";
            var iconStateEntity: any = mode === "icon" ? (b.entity || "") : "";
            helpers.renderCardTextField(panel, b, helpers, SUBPAGE_CARD_METADATA.labelField);
            var iconSectionMain: any = helpers.renderCardIconPicker(panel, b, helpers, SUBPAGE_CARD_METADATA.icon);
            var showStateToggle: any = helpers.renderCardOptionToggle(panel, b, helpers, SUBPAGE_CARD_METADATA.showState);
            var stateCond: any = condField();
            if (showState)
                stateCond.classList.add("sp-visible");
            var modeControl: any = helpers.renderCardSegmentControl(stateCond, b, helpers, Object.assign({}, SUBPAGE_CARD_METADATA.stateMode, {
                value: function (this: any) { return mode; },
                onSelect: function (this: any, b?: any, helpers?: any, value?: any) { setMode(value, true); },
            }));
            var iconBtn: any = modeControl.buttons.icon;
            var numericBtn: any = modeControl.buttons.numeric;
            var textBtn: any = modeControl.buttons.text;
            var stateIconSection: any = condField();
            var iconEntityField: any = helpers.renderCardEntityField(stateIconSection, b, helpers, {
                entity: SUBPAGE_CARD_METADATA.iconStateEntity,
            });
            var iconEntityInp: any = iconEntityField.input;
            helpers.renderCardIconPicker(stateIconSection, b, helpers, SUBPAGE_CARD_METADATA.iconOn);
            stateCond.appendChild(stateIconSection);
            var sensorField: any = condField();
            var sensorEntityField: any = helpers.renderCardEntityField(sensorField, b, helpers, {
                entity: SUBPAGE_CARD_METADATA.sensorEntity,
            });
            var sensorInp: any = sensorEntityField.input;
            helpers.requireField(sensorInp, "Add a sensor entity before saving.", function (this: any) {
                return showState && (mode === "numeric" || mode === "text");
            });
            function saveSensorEntity(this: any) {
                sensorEntity = sensorInp.value;
                if (showState && mode !== "icon") {
                    b.sensor = sensorEntity;
                    helpers.saveField("sensor", b.sensor);
                }
            }
            sensorInp.addEventListener("input", saveSensorEntity);
            sensorInp.addEventListener("change", saveSensorEntity);
            sensorInp.addEventListener("blur", saveSensorEntity);
            sensorInp.addEventListener("keydown", function (this: any, e?: any) {
                if (e.key === "Enter") {
                    saveSensorEntity();
                    this.blur();
                }
            });
            function saveIconStateEntity(this: any) {
                iconStateEntity = iconEntityInp.value;
                if (showState && mode === "icon") {
                    b.entity = iconStateEntity;
                    helpers.saveField("entity", b.entity);
                }
            }
            iconEntityInp.addEventListener("input", saveIconStateEntity);
            iconEntityInp.addEventListener("change", saveIconStateEntity);
            iconEntityInp.addEventListener("blur", saveIconStateEntity);
            iconEntityInp.addEventListener("keydown", function (this: any, e?: any) {
                if (e.key === "Enter") {
                    saveIconStateEntity();
                    this.blur();
                }
            });
            var numericSection: any = condField();
            var unitField: any = helpers.renderCardTextField(numericSection, b, helpers, SUBPAGE_CARD_METADATA.unitField);
            var unitInp: any = unitField.input;
            var precisionField: any = helpers.precisionField(helpers.idPrefix + "precision", mode === "numeric" ? (b.precision || "0") : "0", function (this: any) {
                if (mode !== "numeric")
                    return;
                b.precision = this.value === "0" ? "" : this.value;
                helpers.saveField("precision", b.precision);
            });
            var precisionSelect: any = precisionField.select;
            numericSection.appendChild(precisionField.field);
            helpers.renderCardLargeNumbersToggle(numericSection, b, helpers, SUBPAGE_CARD_METADATA);
            sensorField.appendChild(numericSection);
            stateCond.appendChild(sensorField);
            panel.appendChild(stateCond);
            function setMode(this: any, nextMode?: any, persist?: any) {
                mode = nextMode;
                showState = mode !== "off";
                showStateToggle.input.checked = showState;
                var iconLabel: any = iconSectionMain.querySelector(".sp-field-label");
                if (iconLabel)
                    iconLabel.textContent = mode === "icon" ? "Off Icon" : "Icon";
                stateCond.classList.toggle("sp-visible", showState);
                iconBtn.classList.toggle("active", mode === "icon");
                numericBtn.classList.toggle("active", mode === "numeric");
                textBtn.classList.toggle("active", mode === "text");
                stateIconSection.classList.toggle("sp-visible", mode === "icon");
                sensorField.classList.toggle("sp-visible", mode === "numeric" || mode === "text");
                numericSection.classList.toggle("sp-visible", mode === "numeric");
                if (mode !== "numeric" && mode !== "text")
                    helpers.clearFieldError(sensorInp);
                if (!persist)
                    return;
                if (mode === "off") {
                    b.sensor = "";
                    b.entity = "";
                    b.unit = "";
                    b.precision = "";
                    b.icon_on = "Auto";
                    iconStateEntity = "";
                    iconEntityInp.value = "";
                    helpers.saveField("sensor", "");
                    helpers.saveField("entity", "");
                    helpers.saveField("unit", "");
                    helpers.saveField("precision", "");
                    helpers.saveField("icon_on", "Auto");
                }
                else if (mode === "icon") {
                    b.sensor = "indicator";
                    b.entity = iconStateEntity;
                    b.unit = "";
                    b.precision = "";
                    helpers.saveField("sensor", "indicator");
                    helpers.saveField("entity", b.entity);
                    helpers.saveField("unit", "");
                    helpers.saveField("precision", "");
                }
                else if (mode === "numeric") {
                    b.sensor = sensorEntity;
                    b.entity = "";
                    b.unit = unitInp.value;
                    b.precision = precisionSelect.value === "0" ? "" : precisionSelect.value;
                    b.icon_on = "Auto";
                    iconStateEntity = "";
                    iconEntityInp.value = "";
                    helpers.saveField("sensor", b.sensor);
                    helpers.saveField("entity", "");
                    helpers.saveField("unit", b.unit);
                    helpers.saveField("precision", b.precision);
                    helpers.saveField("icon_on", "Auto");
                }
                else if (mode === "text") {
                    b.sensor = sensorEntity;
                    b.entity = "";
                    b.unit = "";
                    b.precision = "text";
                    b.icon_on = "Auto";
                    iconStateEntity = "";
                    iconEntityInp.value = "";
                    helpers.saveField("sensor", b.sensor);
                    helpers.saveField("entity", "");
                    helpers.saveField("unit", "");
                    helpers.saveField("precision", "text");
                    helpers.saveField("icon_on", "Auto");
                }
            }
            showStateToggle.input.addEventListener("change", function (this: any) {
                setMode(this.checked ? (mode === "off" ? "icon" : mode) : "off", true);
            });
            setMode(mode, false);
            appendEditSubpageButton(panel, slot);
        },
        renderPreview: function (this: any, b?: any, helpers?: any) {
            var defaults: any = subpagePresetDefaults(subpageKind(b));
            var companionMetric: any = subpageKind(b) === "companion_stat" ? companionStatMetric(b.entity) : null;
            if (companionMetric) {
                return {
                    iconHtml: cardSensorPreviewHtml(b, helpers, companionMetricPreviewValue("0"), companionMetric.unit),
                    labelHtml: subpageBadgeLabelHtml(helpers, b.label || companionMetric.label),
                };
            }
            var label: any = b.label || (defaults && defaults.label) || b.entity || "Configure";
            var mode: any = subpageStateDisplayMode(b);
            if (mode === "icon") {
                var stateIconName: any = b.icon && b.icon !== "Auto" ? iconSlug(b.icon) :
                    (defaults ? iconSlug(defaults.icon) : "cog");
                return {
                    iconHtml: '<span class="sp-btn-icon mdi mdi-' + stateIconName + '"></span>',
                    labelHtml: subpageBadgeLabelHtml(helpers, label),
                };
            }
            if (mode === "numeric") {
                var unit: any = b.unit || "";
                var prec: any = parseInt(b.precision || "0", 10) || 0;
                var sampleVal: any = (0).toFixed(prec);
                return {
                    iconHtml: cardSensorPreviewHtml(b, helpers, sampleVal, unit),
                    labelHtml: subpageBadgeLabelHtml(helpers, b.label || b.sensor || "Subpage"),
                };
            }
            if (mode === "text") {
                var iconName: any = b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : "cog";
                return {
                    iconHtml: '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>',
                    labelHtml: subpageBadgeLabelHtml(helpers, "State"),
                };
            }
            return {
                labelHtml: subpageBadgeLabelHtml(helpers, label),
            };
        },
        contextMenuItems: function (this: any, slot?: any, b?: any, helpers?: any) {
            helpers.addCtxItem("cog", "Edit Subpage", function (this: any) { enterSubpage(slot); });
        },
    };
    const registeredSubpageDefinition = registry.register("subpage", subpageDefinition);
    registry.register("companion_subpage", {
        ...registeredSubpageDefinition,
        label: "Subpage",
        onSelect: function (this: any, b?: any) {
            b.options = setConfigOptionValue(b.options, SUBPAGE_CONNECTOR_OPTION, "mac_companion");
        },
    });
    function subpageBadgeLabelHtml(this: any, helpers?: any, label?: any) {
        return '<span class="sp-btn-label-row"><span class="sp-btn-label">' +
            helpers.escHtml(label) +
            '</span><span class="sp-subpage-badge mdi mdi-' +
            SUBPAGE_CARD_METADATA.preview.badge +
            '"></span></span>';
    }
    function appendEditSubpageButton(this: any, panel?: any, slot?: any) {
        var configBtn: any = document.createElement("button");
        configBtn.className = "sp-action-btn sp-edit-subpage-btn";
        configBtn.textContent = "Edit Subpage";
        configBtn.addEventListener("click", function (this: any) { selection.closeSettings(); enterSubpage(slot); });
        panel.appendChild(configBtn);
    }
}
