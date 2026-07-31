import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function registerClimateCardTypes(): GlobalDescriptors {
    // Climate card: thermostat status plus full-screen climate controls.
    var CLIMATE_CARD_METADATA: any = {
        entity: {
            label: "Climate Entity",
            idSuffix: "entity",
            placeholder: "e.g. climate.living_room",
            domains: function (this: any) { return cardContractDomains("climate"); },
            bindName: "entity",
            rerender: true,
            requiredMessage: "Add a climate entity before saving.",
        },
        labelDisplay: {
            label: "Label Display",
            options: [
                ["label", "Label"],
                ["status", "Status"],
                ["actual", "Actual"],
                ["target", "Target"],
            ],
        },
        numberDisplay: {
            label: "Icon & Temperatures",
            options: [
                ["icon", "Icon"],
                ["actual", "Actual"],
                ["target", "Target"],
            ],
        },
        temperatureStep: {
            label: "Temperature Step",
            options: [
                ["1", "1 degree"],
                ["0.5", "0.5 degree"],
            ],
        },
        largeNumbers: {
            label: "Large Temperature Numbers",
            idSuffix: "large-temperature-numbers",
            supported: function (this: any, b?: any) {
                return climateNumberDisplayMode(b) !== "icon";
            },
        },
        preview: {
            badge: "thermostat",
        },
    };
    registerButtonType("climate", {
        label: function (this: any) { return cardContractCardLabel("climate"); },
        allowInSubpage: function (this: any) { return cardContractAllowInSubpage("climate"); },
        pickerKey: function (this: any) { return cardContractPickerKey("climate"); },
        hidden: function (this: any) { return cardContractHidden("climate"); },
        hideLabel: true,
        labelPlaceholder: "e.g. Living Room",
        defaultConfig: function (this: any) { return cardContractDefaultConfig("climate"); },
        cardMetadata: CLIMATE_CARD_METADATA,
        onSelect: function (this: any, b?: any) {
            b.entity = "";
            b.label = "Climate";
            b.sensor = "";
            b.unit = "";
            b.type = "climate_control";
            b.precision = "";
            b.icon = "Thermostat";
            b.icon_on = "Auto";
            b.options = "";
        },
        renderSettings: function (this: any, panel?: any, b?: any, slot?: any, helpers?: any) {
            if (b.type !== "climate_control") {
                b.type = "climate_control";
                helpers.saveField("type", b.type);
            }
            b.sensor = "";
            b.unit = "";
            if (!b.icon)
                b.icon = "Thermostat";
            if (!b.icon_on)
                b.icon_on = "Auto";
            var climateConfig: any = parseClimatePrecisionConfig(b.precision);
            var normalizedPrecision: any = climatePrecisionConfig(climateConfig.precision, climateConfig.min, climateConfig.max);
            if (b.precision !== normalizedPrecision) {
                b.precision = normalizedPrecision;
                helpers.saveField("precision", normalizedPrecision);
            }
            helpers.renderCardEntityField(panel, b, helpers, CLIMATE_CARD_METADATA);
            var modalTabsDisclosure: any = helpers.disclosureSection("Modal Settings", helpers.idPrefix + "climate-modal-tabs", b._modalSettingsOpen === true);
            renderModalTabSettings(modalTabsDisclosure.section, b, helpers, {
                definitions: climateControlTabDefinitions,
                tabs: climateControlTabs,
                normalizeOptions: function (this: any, options?: any) { return normalizeClimateOptions(options, true); },
                setTabs: setClimateControlTabs,
                idPrefix: "climate-tab-",
                hideHeading: true,
            });
            panel.appendChild(modalTabsDisclosure.panel);
            var labelField: any = condField();
            labelField.classList.add("sp-climate-settings-gap");
            helpers.renderCardTextField(labelField, b, helpers, {
                label: "Label",
                idSuffix: "label",
                field: "label",
                placeholder: "Climate",
                rerender: true,
            });
            function syncLabelField(this: any) {
                labelField.classList.toggle("sp-visible", climateLabelDisplayMode(b) === "label");
            }
            var cardSettingsDisclosure: any = helpers.disclosureSection("Card Settings", helpers.idPrefix + "climate-card-settings", false);
            var cardSettings: any = cardSettingsDisclosure.section;
            helpers.renderCardSegmentControl(cardSettings, b, helpers, {
                segment: Object.assign({}, CLIMATE_CARD_METADATA.labelDisplay, {
                    value: function (this: any) { return climateLabelDisplayMode(b); },
                    onSelect: function (this: any, button?: any, cardHelpers?: any, value?: any) {
                        setClimateLabelDisplayMode(button, value);
                        cardHelpers.saveField("options", button.options);
                        syncLabelField();
                        scheduleRender();
                    },
                }),
            });
            syncLabelField();
            cardSettings.appendChild(labelField);
            helpers.renderCardSegmentControl(cardSettings, b, helpers, {
                segment: Object.assign({}, CLIMATE_CARD_METADATA.numberDisplay, {
                    value: function (this: any) { return climateNumberDisplayMode(b); },
                    onSelect: function (this: any, button?: any, cardHelpers?: any, value?: any) {
                        setClimateNumberDisplayMode(button, value);
                        cardHelpers.saveField("options", button.options);
                        syncIconFields();
                        scheduleRender();
                    },
                }),
            });
            var iconFields: any = condField();
            iconFields.classList.add("sp-climate-settings-gap");
            helpers.renderCardIconPicker(iconFields, b, helpers, {
                pickerIdSuffix: "climate-icon-picker",
                idSuffix: "climate-icon",
                field: "icon",
                fallback: "Thermostat",
                label: "Off Icon",
                onChange: function (this: any) { scheduleRender(); },
            });
            helpers.renderCardIconPicker(iconFields, b, helpers, {
                pickerIdSuffix: "climate-icon-on-picker",
                idSuffix: "climate-icon-on",
                field: "icon_on",
                fallback: "Auto",
                label: "On Icon",
                onChange: function (this: any) { scheduleRender(); },
            });
            function syncIconFields(this: any) {
                iconFields.classList.toggle("sp-visible", climateNumberDisplayMode(b) === "icon");
            }
            syncIconFields();
            cardSettings.appendChild(iconFields);
            var precisionField: any = helpers.selectField("Temperature Settings", helpers.idPrefix + "climate-precision", [
                ["", "10"],
                ["1", "10.2"],
            ], climateConfig.precision);
            var precision: any = precisionField.select;
            function saveClimateAdvancedSettings(this: any) {
                b.precision = climatePrecisionConfig(precision.value, minInp.value, maxInp.value);
                helpers.saveField("precision", b.precision);
                scheduleRender();
            }
            precision.addEventListener("change", saveClimateAdvancedSettings);
            var stepField: any = helpers.selectField(CLIMATE_CARD_METADATA.temperatureStep.label, helpers.idPrefix + "climate-temperature-step", CLIMATE_CARD_METADATA.temperatureStep.options, climateTemperatureStep(b));
            stepField.select.addEventListener("change", function (this: any) {
                setClimateTemperatureStep(b, stepField.select.value);
                helpers.saveField("options", b.options);
                scheduleRender();
            });
            helpers.renderCardLargeNumbersToggle(cardSettings, b, helpers, CLIMATE_CARD_METADATA);
            panel.appendChild(cardSettingsDisclosure.panel);
            var advancedDisclosure: any = helpers.disclosureSection("Advanced", helpers.idPrefix + "climate-advanced", false);
            var advanced: any = advancedDisclosure.section;
            advanced.appendChild(precisionField.field);
            advanced.appendChild(stepField.field);
            var minField: any = helpers.textField("Minimum Temperature", helpers.idPrefix + "climate-min", climateConfig.min, "e.g. -25");
            var minInp: any = minField.input;
            minInp.inputMode = "decimal";
            advanced.appendChild(minField.field);
            var maxField: any = helpers.textField("Maximum Temperature", helpers.idPrefix + "climate-max", climateConfig.max, "e.g. 5");
            var maxInp: any = maxField.input;
            maxInp.inputMode = "decimal";
            advanced.appendChild(maxField.field);
            minInp.addEventListener("change", saveClimateAdvancedSettings);
            maxInp.addEventListener("change", saveClimateAdvancedSettings);
            panel.appendChild(advancedDisclosure.panel);
        },
        renderPreview: function (this: any, b?: any, helpers?: any) {
            var climateConfig: any = parseClimatePrecisionConfig(b.precision);
            var prec: any = parseInt(climateConfig.precision || "0", 10) || 0;
            var unit: any = temperatureUnitSymbol();
            var actualVal: any = (21).toFixed(prec);
            var targetVal: any = (20).toFixed(prec);
            var numberMode: any = climateNumberDisplayMode(b);
            var numberVal: any = numberMode === "actual" ? actualVal : targetVal;
            var labelMode: any = climateLabelDisplayMode(b);
            var label: any = (b.label && b.label.trim()) || "Climate";
            if (labelMode === "status") {
                label = "Idle";
            }
            else if (labelMode === "actual") {
                label = actualVal + unit;
            }
            else if (labelMode === "target") {
                label = targetVal + unit;
            }
            function climateLabelHtml(this: any) {
                return cardBadgeLabelHtml(helpers, label, CLIMATE_CARD_METADATA.preview.badge);
            }
            if (numberMode === "icon") {
                var iconName: any = b.icon && b.icon !== "Auto" ? b.icon : "Thermostat";
                return {
                    iconHtml: '<span class="sp-btn-icon mdi mdi-' + iconSlug(iconName) + '"></span>',
                    labelHtml: climateLabelHtml(),
                };
            }
            return {
                buttonClass: "sp-climate-temp-card",
                iconHtml: cardSensorPreviewHtml(b, helpers, numberVal, unit),
                labelHtml: climateLabelHtml(),
            };
        },
    });
    registerButtonType("climate_control", Object.assign({}, BUTTON_TYPES.climate, {
        label: function (this: any) { return cardContractCardLabel("climate_control"); },
        allowInSubpage: function (this: any) { return cardContractAllowInSubpage("climate_control"); },
        pickerKey: function (this: any) { return cardContractPickerKey("climate_control"); },
        hidden: function (this: any) { return cardContractHidden("climate_control"); },
        defaultConfig: function (this: any) { return cardContractDefaultConfig("climate_control"); },
    }));
    return {
        "CLIMATE_CARD_METADATA": liveGlobal(() => CLIMATE_CARD_METADATA, (value?: any) => { CLIMATE_CARD_METADATA = value; }),
    };
}
