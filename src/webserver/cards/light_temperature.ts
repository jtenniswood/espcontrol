import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function registerLightTemperatureCardTypes(): GlobalDescriptors {
    // Light temperature slider card: controls color_temp_kelvin on a light entity.
    // Slider bottom = min kelvin (warm), top = max kelvin (cool).
    // Config fields: unit="min-max" (kelvin range),
    // precision="color" (dynamic fill color by current temperature),
    // sensor is unused; legacy "kelvin" values are ignored by firmware.
    function lightTempSpec(this: any) {
        var card: any = cardContractCard("light_temperature");
        return card && card.behavior && card.behavior.lightTemperature || {};
    }
    function lightTempDefaultRange(this: any) {
        return lightTempSpec().defaultRange || "2000-6500";
    }
    function lightTempMinLimit(this: any) {
        var value: any = lightTempSpec().min;
        return typeof value === "number" ? value : 1000;
    }
    function lightTempMaxLimit(this: any) {
        var value: any = lightTempSpec().max;
        return typeof value === "number" ? value : 10000;
    }
    function lightTempMinMaxLimit(this: any) {
        var value: any = lightTempSpec().minMax;
        return typeof value === "number" ? value : 9900;
    }
    function lightTempStep(this: any) {
        var value: any = lightTempSpec().step;
        return typeof value === "number" ? value : 100;
    }
    function lightTempLegacySensorValues(this: any) {
        var values: any = lightTempSpec().legacySensorValues;
        return values ? values.slice() : ["kelvin"];
    }
    function lightTempSensorNeedsCleanup(this: any, value?: any) {
        return lightTempLegacySensorValues().indexOf(value || "") >= 0;
    }
    function lightTempParseRange(this: any, unit?: any) {
        var defaults: any = lightTempDefaultRange().split("-");
        var defaultMin: any = parseInt(defaults[0], 10);
        var defaultMax: any = parseInt(defaults[1], 10);
        if (!isFinite(defaultMin))
            defaultMin = 2000;
        if (!isFinite(defaultMax) || defaultMax <= defaultMin)
            defaultMax = 6500;
        var parts: any = (unit || lightTempDefaultRange()).split("-");
        var mn: any = parseInt(parts[0], 10);
        var mx: any = parseInt(parts[1], 10);
        if (!isFinite(mn) || mn < lightTempMinLimit())
            mn = defaultMin;
        if (!isFinite(mx) || mx <= mn)
            mx = defaultMax;
        return [mn, mx];
    }
    function lightTempClampMin(this: any, v?: any, absMin?: any) {
        var n: any = parseInt(v, 10);
        if (!isFinite(n))
            n = absMin;
        if (n < absMin)
            n = absMin;
        if (n > lightTempMinMaxLimit())
            n = lightTempMinMaxLimit();
        return n;
    }
    function lightTempClampMax(this: any, v?: any, mn?: any) {
        var n: any = parseInt(v, 10);
        if (!isFinite(n))
            n = mn + lightTempStep();
        if (n <= mn)
            n = mn + lightTempStep();
        if (n > lightTempMaxLimit())
            n = lightTempMaxLimit();
        return n;
    }
    var LIGHT_CONTROL_TYPE_OPTIONS: any = [
        ["light_control", "All Controls"],
        ["light_switch", "Switch"],
        ["light_brightness", "Brightness"],
        ["light_temperature", "Colour Temperature"],
    ];
    var LIGHT_CONTROL_TYPE_METADATA: any = {
        mode: {
            label: "Type",
            idSuffix: "light-control-type",
            options: function (this: any, b?: any) {
                return LIGHT_CONTROL_TYPE_OPTIONS;
            },
            value: function (this: any, b?: any) { return normalizeLightControlType(b.type); },
            onChange: function (this: any, b?: any, helpers?: any) {
                setLightControlType(b, this.value, helpers);
            },
        },
    };
    var LIGHT_TEMPERATURE_CARD_METADATA: any = {
        mode: LIGHT_CONTROL_TYPE_METADATA.mode,
        entity: {
            label: "Entity",
            placeholder: "e.g. light.living_room",
            domains: function (this: any) { return cardContractDomains("light_temperature"); },
        },
        labelField: {
            label: "Label",
            placeholder: "e.g. Living Room",
        },
        icon: {
            field: "icon",
            fallback: "Auto",
        },
        preview: {
            badge: "lightbulb",
        },
    };
    var LIGHT_FULL_CONTROL_CARD_METADATA: any = {
        mode: LIGHT_CONTROL_TYPE_METADATA.mode,
        entity: {
            label: "Entity",
            placeholder: "e.g. light.living_room",
            domains: function (this: any) { return cardContractDomains("light_control"); },
        },
        labelField: {
            label: "Label",
            placeholder: "e.g. Living Room",
        },
        iconOff: {
            field: "icon",
            fallback: "Lightbulb Outline",
            label: "Off Icon",
        },
        iconOn: {
            field: "icon_on",
            fallback: "Lightbulb",
            label: "On Icon",
        },
        preview: {
            badge: "lightbulb-on",
        },
    };
    function normalizeLightControlType(this: any, type?: any) {
        if (type === "light_switch")
            return "light_switch";
        if (type === "light_control")
            return "light_control";
        return type === "light_temperature" ? "light_temperature" : "light_brightness";
    }
    function setLightControlType(this: any, b?: any, type?: any, helpers?: any) {
        var nextType: any = normalizeLightControlType(type);
        if (b.type === nextType)
            return;
        b.type = nextType;
        var td: any = BUTTON_TYPES[nextType];
        if (td && td.onSelect)
            td.onSelect(b);
        helpers.saveField("type", nextType);
        helpers.saveField("sensor", b.sensor || "");
        helpers.saveField("unit", b.unit || "");
        helpers.saveField("precision", b.precision || "");
        helpers.saveField("options", b.options || "");
        helpers.saveField("icon", b.icon || "Auto");
        helpers.saveField("icon_on", b.icon_on || "Auto");
        renderButtonSettings();
    }
    function renderLightControlTypeField(this: any, panel?: any, b?: any, helpers?: any) {
        return helpers.renderCardModeSelector(panel, b, helpers, LIGHT_CONTROL_TYPE_METADATA);
    }
    function renderLightControlTabSettings(this: any, panel?: any, b?: any, helpers?: any) {
        renderModalTabSettings(panel, b, helpers, {
            definitions: lightControlTabDefinitions,
            tabs: lightControlTabs,
            normalizeOptions: normalizeLightControlOptions,
            setTabs: setLightControlTabs,
            idPrefix: "light-tab-",
            groupLabel: "Modal Controls",
            groupIdSuffix: "light-modal-controls",
        });
    }
    registerButtonType("light_temperature", {
        label: function (this: any) { return cardContractCardLabel("light_temperature"); },
        allowInSubpage: function (this: any) { return cardContractAllowInSubpage("light_temperature"); },
        hideLabel: true,
        pickerKey: function (this: any) { return cardContractPickerKey("light_temperature"); },
        hidden: function (this: any) { return cardContractHidden("light_temperature"); },
        defaultConfig: function (this: any) { return cardContractDefaultConfig("light_temperature"); },
        isAvailable: function (this: any) {
            return false;
        },
        labelPlaceholder: "e.g. Living Room",
        cardMetadata: LIGHT_TEMPERATURE_CARD_METADATA,
        onSelect: function (this: any, b?: any) {
            b.sensor = "";
            b.unit = "2000-6500";
            b.precision = "";
            b.icon = "Lightbulb";
            b.icon_on = "Auto";
        },
        renderSettings: function (this: any, panel?: any, b?: any, slot?: any, helpers?: any) {
            renderLightControlTypeField(panel, b, helpers);
            helpers.renderBasicCardFields(panel, b, helpers, LIGHT_TEMPERATURE_CARD_METADATA, {
                icon: false,
            });
            if (lightTempSensorNeedsCleanup(b.sensor)) {
                b.sensor = "";
                helpers.saveField("sensor", "");
            }
            // Kelvin range
            var range: any = lightTempParseRange(b.unit);
            var curMin: any = range[0], curMax: any = range[1];
            function saveRange(this: any, mn?: any, mx?: any) {
                b.unit = mn + "-" + mx;
                helpers.saveField("unit", b.unit);
            }
            var minF: any = document.createElement("div");
            minF.className = "sp-field";
            minF.appendChild(helpers.fieldLabel("Min Color Temp (K)", helpers.idPrefix + "kmin"));
            var minInp: any = document.createElement("input");
            minInp.type = "number";
            minInp.className = "sp-input";
            minInp.id = helpers.idPrefix + "kmin";
            minInp.min = "1000";
            minInp.max = "9900";
            minInp.step = "100";
            minInp.placeholder = "2000";
            minInp.value = curMin;
            minF.appendChild(minInp);
            panel.appendChild(minF);
            var maxF: any = document.createElement("div");
            maxF.className = "sp-field";
            maxF.appendChild(helpers.fieldLabel("Max Color Temp (K)", helpers.idPrefix + "kmax"));
            var maxInp: any = document.createElement("input");
            maxInp.type = "number";
            maxInp.className = "sp-input";
            maxInp.id = helpers.idPrefix + "kmax";
            maxInp.min = "1100";
            maxInp.max = "10000";
            maxInp.step = "100";
            maxInp.placeholder = "6500";
            maxInp.value = curMax;
            maxF.appendChild(maxInp);
            panel.appendChild(maxF);
            function onRangeChange(this: any) {
                var mn: any = lightTempClampMin(minInp.value, lightTempMinLimit());
                var mx: any = lightTempClampMax(maxInp.value, mn);
                minInp.value = mn;
                maxInp.value = mx;
                saveRange(mn, mx);
            }
            minInp.addEventListener("change", onRangeChange);
            maxInp.addEventListener("change", onRangeChange);
            minInp.addEventListener("blur", onRangeChange);
            maxInp.addEventListener("blur", onRangeChange);
            helpers.renderCardIconPicker(panel, b, helpers, LIGHT_TEMPERATURE_CARD_METADATA.icon);
        },
        renderPreview: function (this: any, b?: any, helpers?: any) {
            var label: any = b.label || b.entity || "Light Temp";
            return cardBadgePreview(b, helpers, {
                label: label,
                iconFallback: "Lightbulb",
                iconExtraHtml: '<span class="sp-slider-preview"><span class="sp-slider-track">' +
                    '<span class="sp-slider-fill"></span>' +
                    '</span></span>',
                badge: LIGHT_TEMPERATURE_CARD_METADATA.preview.badge,
            });
        },
    });
    registerButtonType("light_control", {
        label: function (this: any) { return cardContractCardLabel("light_control"); },
        allowInSubpage: function (this: any) { return cardContractAllowInSubpage("light_control"); },
        hideLabel: true,
        pickerKey: function (this: any) { return cardContractPickerKey("light_control"); },
        hidden: function (this: any) { return cardContractHidden("light_control"); },
        defaultConfig: function (this: any) { return cardContractDefaultConfig("light_control"); },
        isAvailable: function (this: any) {
            return false;
        },
        labelPlaceholder: "e.g. Living Room",
        cardMetadata: LIGHT_FULL_CONTROL_CARD_METADATA,
        onSelect: function (this: any, b?: any) {
            b.sensor = "";
            b.unit = "";
            b.precision = "";
            b.options = "";
            b.icon = "Lightbulb Outline";
            b.icon_on = "Lightbulb";
        },
        renderSettings: function (this: any, panel?: any, b?: any, slot?: any, helpers?: any) {
            renderLightControlTypeField(panel, b, helpers);
            b.options = normalizeLightControlOptions(b.options);
            helpers.renderCardEntityField(panel, b, helpers, LIGHT_FULL_CONTROL_CARD_METADATA);
            renderLightControlTabSettings(panel, b, helpers);
            var cardSettingsDisclosure: any = helpers.disclosureSection("Card Settings", helpers.idPrefix + "light-card-settings", false);
            var cardSettings: any = cardSettingsDisclosure.section;
            helpers.renderCardTextField(cardSettings, b, helpers, LIGHT_FULL_CONTROL_CARD_METADATA.labelField);
            helpers.renderCardIconPair(cardSettings, b, helpers, LIGHT_FULL_CONTROL_CARD_METADATA.iconOff, LIGHT_FULL_CONTROL_CARD_METADATA.iconOn);
            panel.appendChild(cardSettingsDisclosure.panel);
        },
        renderPreview: function (this: any, b?: any, helpers?: any) {
            var label: any = b.label || b.entity || "Light";
            return cardBadgePreview(b, helpers, {
                label: label,
                iconFallback: "Lightbulb Outline",
                badge: LIGHT_FULL_CONTROL_CARD_METADATA.preview.badge,
            });
        },
    });
    return {
        "lightTempSpec": staticGlobal(lightTempSpec),
        "lightTempDefaultRange": staticGlobal(lightTempDefaultRange),
        "lightTempMinLimit": staticGlobal(lightTempMinLimit),
        "lightTempMaxLimit": staticGlobal(lightTempMaxLimit),
        "lightTempMinMaxLimit": staticGlobal(lightTempMinMaxLimit),
        "lightTempStep": staticGlobal(lightTempStep),
        "lightTempLegacySensorValues": staticGlobal(lightTempLegacySensorValues),
        "lightTempSensorNeedsCleanup": staticGlobal(lightTempSensorNeedsCleanup),
        "lightTempParseRange": staticGlobal(lightTempParseRange),
        "lightTempClampMin": staticGlobal(lightTempClampMin),
        "lightTempClampMax": staticGlobal(lightTempClampMax),
        "LIGHT_CONTROL_TYPE_OPTIONS": liveGlobal(() => LIGHT_CONTROL_TYPE_OPTIONS, (value?: any) => { LIGHT_CONTROL_TYPE_OPTIONS = value; }),
        "LIGHT_CONTROL_TYPE_METADATA": liveGlobal(() => LIGHT_CONTROL_TYPE_METADATA, (value?: any) => { LIGHT_CONTROL_TYPE_METADATA = value; }),
        "LIGHT_TEMPERATURE_CARD_METADATA": liveGlobal(() => LIGHT_TEMPERATURE_CARD_METADATA, (value?: any) => { LIGHT_TEMPERATURE_CARD_METADATA = value; }),
        "LIGHT_FULL_CONTROL_CARD_METADATA": liveGlobal(() => LIGHT_FULL_CONTROL_CARD_METADATA, (value?: any) => { LIGHT_FULL_CONTROL_CARD_METADATA = value; }),
        "normalizeLightControlType": staticGlobal(normalizeLightControlType),
        "setLightControlType": staticGlobal(setLightControlType),
        "renderLightControlTypeField": staticGlobal(renderLightControlTypeField),
        "renderLightControlTabSettings": staticGlobal(renderLightControlTabSettings),
    };
}
