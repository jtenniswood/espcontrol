import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function registerSensorCardTypes(): GlobalDescriptors {
    // Read-only sensor card: displays either numeric data or a text state.
    var SENSOR_CARD_LOCAL_SENSOR: any = "local";
    function sensorCardIsLocal(this: any, b?: any) {
        if (!b)
            return false;
        return b.type === "local_sensor" || (b.type === "sensor" && b.sensor === SENSOR_CARD_LOCAL_SENSOR);
    }
    var SENSOR_CARD_METADATA: any = {
        source: {
            label: "Source",
            options: [
                ["ha", "Home Assistant"],
                [SENSOR_CARD_LOCAL_SENSOR, "Local Sensor"],
            ],
            value: function (this: any, b?: any) {
                return sensorCardIsLocal(b) ? SENSOR_CARD_LOCAL_SENSOR : "ha";
            },
        },
        entity: {
            label: "Sensor Entity",
            idSuffix: "sensor",
            placeholder: "e.g. sensor.living_room_temperature",
            domains: function (this: any) { return cardContractDomains("sensor"); },
            bindName: "sensor",
            rerender: true,
            requiredMessage: "Add a sensor entity before saving.",
        },
        mode: {
            label: "Type",
            idSuffix: "sensor-type",
            options: [
                ["numeric", "Numeric"],
                ["time", "Time"],
                ["text", "Text"],
                ["icon", "Icon"],
            ],
            value: function (this: any, b?: any) {
                if (b.precision === "icon")
                    return "icon";
                if (b.precision === "time")
                    return "time";
                return b.precision === "text" ? "text" : "numeric";
            },
        },
        largeNumbers: {
            label: "Large Sensor Numbers",
            idSuffix: "large-sensor-numbers",
            supported: function (this: any, b?: any) {
                return !sensorCardIsLocal(b) && b.precision !== "icon" && b.precision !== "text" && b.precision !== "time";
            },
        },
        preview: {
            iconBadge: "toggle-switch",
            numericBadge: "gauge",
            textBadge: "format-text",
        },
    };
    registerButtonType("sensor", {
        label: function (this: any) { return cardContractCardLabel("sensor"); },
        allowInSubpage: function (this: any) { return cardContractAllowInSubpage("sensor"); },
        pickerKey: function (this: any) { return cardContractPickerKey("sensor"); },
        hidden: function (this: any) { return cardContractHidden("sensor"); },
        hideLabel: true,
        defaultConfig: function (this: any) { return cardContractDefaultConfig("sensor"); },
        cardMetadata: SENSOR_CARD_METADATA,
        onSelect: function (this: any, b?: any) {
            b.entity = "";
            b.icon_on = "Auto";
            if (!b.precision)
                b.precision = "";
            if (b.precision !== "icon" && b.precision !== "text")
                b.icon = "Auto";
            b.options = normalizeSensorOptions(b.options, b.precision);
        },
        renderSettings: function (this: any, panel?: any, b?: any, slot?: any, helpers?: any) {
            var sourceControl: any = helpers.renderCardSegmentControl(panel, b, helpers, {
                segment: Object.assign({}, SENSOR_CARD_METADATA.source, {
                    onSelect: function (this: any, button?: any, cardHelpers?: any, value?: any) {
                        setSource(value);
                    },
                }),
            });
            var sourceButtons: any = sourceControl.buttons;
            sourceButtons.ha.classList.toggle("active", !sensorCardIsLocal(b));
            sourceButtons[SENSOR_CARD_LOCAL_SENSOR].classList.toggle("active", sensorCardIsLocal(b));
            function setSource(this: any, value?: any) {
                var local: any = value === SENSOR_CARD_LOCAL_SENSOR;
                if (local === sensorCardIsLocal(b))
                    return;
                b.type = "sensor";
                b.entity = "";
                b.label = "";
                b.sensor = local ? SENSOR_CARD_LOCAL_SENSOR : "";
                b.unit = "";
                b.icon = "Auto";
                b.icon_on = "Auto";
                b.precision = "";
                b.options = "";
                helpers.saveField("type", "sensor");
                helpers.saveField("entity", "");
                helpers.saveField("label", "");
                helpers.saveField("sensor", b.sensor);
                helpers.saveField("unit", "");
                helpers.saveField("icon", "Auto");
                helpers.saveField("icon_on", "Auto");
                helpers.saveField("precision", "");
                helpers.saveField("options", "");
                renderButtonSettings();
            }
            if (sensorCardIsLocal(b)) {
                renderSensorLocalSettings(panel, b, slot, helpers);
                return;
            }
            var displayMode: any = b.precision === "icon" || b.precision === "text" || b.precision === "time" ? b.precision : "numeric";
            var isTextMode: any = displayMode === "text";
            var modeField: any = helpers.renderCardModeSelector(panel, b, helpers, {
                mode: Object.assign({}, SENSOR_CARD_METADATA.mode, {
                    onChange: function (this: any) {
                        setMode(this.value, true);
                    },
                }),
            });
            var modeSelect: any = modeField.select;
            helpers.renderCardEntityField(panel, b, helpers, SENSOR_CARD_METADATA);
            var numericSection: any = condField();
            var labelField: any = helpers.renderCardTextField(numericSection, b, helpers, {
                label: "Label",
                idSuffix: "label",
                field: "label",
                placeholder: "e.g. Living Room",
                rerender: true,
            });
            var labelInp: any = labelField.input;
            var unitField: any = helpers.renderCardTextField(numericSection, b, helpers, {
                label: "Unit",
                idSuffix: "unit",
                field: "unit",
                placeholder: "e.g. \u00B0C",
                rerender: true,
            });
            var unitInp: any = unitField.input;
            unitInp.className = "sp-input";
            var precisionField: any = helpers.precisionField(helpers.idPrefix + "precision", !isTextMode ? (b.precision || "0") : "0", function (this: any) {
                b.precision = this.value === "0" ? "" : this.value;
                helpers.saveField("precision", b.precision);
            });
            var precisionSelect: any = precisionField.select;
            numericSection.appendChild(precisionField.field);
            helpers.renderCardLargeNumbersToggle(numericSection, b, helpers, SENSOR_CARD_METADATA);
            panel.appendChild(numericSection);
            var timeSection: any = condField();
            helpers.renderCardTextField(timeSection, b, helpers, {
                label: "Label",
                idSuffix: "time-label",
                field: "label",
                placeholder: "e.g. UPS Runtime",
                rerender: true,
            });
            var timeUnitField: any = helpers.selectField("Input Unit", helpers.idPrefix + "time-unit", [
                ["", "Auto"],
                ["seconds", "Seconds"],
                ["minutes", "Minutes"],
                ["hours", "Hours"],
                ["days", "Days"],
            ], sensorTimeUnit(b), function (this: any) {
                setSensorTimeUnit(b, this.value);
                helpers.saveField("options", b.options);
            });
            timeSection.appendChild(timeUnitField.field);
            panel.appendChild(timeSection);
            var textSection: any = condField();
            var textIconPicker: any = helpers.renderCardIconPicker(textSection, b, helpers, {
                pickerIdSuffix: "icon-picker",
                idSuffix: "icon",
                field: "icon",
                fallback: "Auto",
            });
            panel.appendChild(textSection);
            var iconSection: any = condField();
            var offIconPicker: any = helpers.renderCardIconPicker(iconSection, b, helpers, {
                pickerIdSuffix: "icon-off-picker",
                idSuffix: "icon-off",
                field: "icon",
                fallback: "Auto",
                label: "Icon",
            });
            var onIconPicker: any = helpers.renderCardIconPicker(iconSection, b, helpers, {
                pickerIdSuffix: "icon-on-picker",
                idSuffix: "icon-on",
                field: "icon_on",
                fallback: "Auto",
                label: "On Icon",
            });
            panel.appendChild(iconSection);
            var hasStateLabels: any = sensorStateLabelsEnabled(b);
            var advancedToggleSection: any = helpers.toggleSection("Advanced", helpers.idPrefix + "sensor-advanced-toggle", hasStateLabels);
            var advancedToggle: any = advancedToggleSection.toggle;
            var advanced: any = advancedToggleSection.section;
            panel.appendChild(advancedToggle.row);
            if (hasStateLabels && isTextMode)
                advanced.classList.add("sp-visible");
            var stateTextGrid: any = document.createElement("div");
            stateTextGrid.className = "sp-state-translation-grid";
            advanced.appendChild(stateTextGrid);
            var inputTextField: any = helpers.textField("Input Status", helpers.idPrefix + "sensor-state-input", sensorStateInput(b), "e.g. high");
            var inputTextInp: any = inputTextField.input;
            stateTextGrid.appendChild(inputTextField.field);
            var outputTextField: any = helpers.textField("Display Text", helpers.idPrefix + "sensor-state-output", sensorStateOutput(b), "e.g. Please empty");
            var outputTextInp: any = outputTextField.input;
            stateTextGrid.appendChild(outputTextField.field);
            var inputText2Field: any = helpers.textField("Input Status 2", helpers.idPrefix + "sensor-state-input-2", sensorStateInput2(b), "e.g. low");
            var inputText2Inp: any = inputText2Field.input;
            stateTextGrid.appendChild(inputText2Field.field);
            var outputText2Field: any = helpers.textField("Display Text 2", helpers.idPrefix + "sensor-state-output-2", sensorStateOutput2(b), "e.g. Full");
            var outputText2Inp: any = outputText2Field.input;
            stateTextGrid.appendChild(outputText2Field.field);
            function saveStateTranslation(this: any) {
                setSensorStateTranslations(b, advancedToggle.input.checked, inputTextInp.value, outputTextInp.value, inputText2Inp.value, outputText2Inp.value);
                helpers.saveField("options", b.options);
            }
            inputTextInp.addEventListener("change", saveStateTranslation);
            outputTextInp.addEventListener("change", saveStateTranslation);
            inputText2Inp.addEventListener("change", saveStateTranslation);
            outputText2Inp.addEventListener("change", saveStateTranslation);
            advancedToggle.input.addEventListener("change", function (this: any) {
                if (this.checked) {
                    if (!isTextMode)
                        setMode("text", true);
                    advanced.classList.add("sp-visible");
                }
                else {
                    advanced.classList.remove("sp-visible");
                    inputTextInp.value = "";
                    outputTextInp.value = "";
                    inputText2Inp.value = "";
                    outputText2Inp.value = "";
                }
                saveStateTranslation();
            });
            panel.appendChild(advanced);
            function resetIconPicker(this: any, picker?: any, value?: any, slug?: any) {
                var iconPreview: any = picker.querySelector(".sp-icon-picker-preview");
                if (iconPreview)
                    iconPreview.className = "sp-icon-picker-preview mdi mdi-" + slug;
                var iconInput: any = picker.querySelector(".sp-icon-picker-input");
                if (iconInput)
                    iconInput.value = value;
            }
            function syncAdvancedVisibility(this: any) {
                advancedToggle.row.style.display = isTextMode ? "" : "none";
                if (!isTextMode)
                    advanced.classList.remove("sp-visible");
            }
            function setMode(this: any, mode?: any, persist?: any) {
                displayMode = mode === "icon" || mode === "text" || mode === "time" ? mode : "numeric";
                isTextMode = displayMode === "text";
                modeSelect.value = displayMode;
                numericSection.classList.toggle("sp-visible", displayMode === "numeric");
                timeSection.classList.toggle("sp-visible", displayMode === "time");
                textSection.classList.toggle("sp-visible", isTextMode);
                iconSection.classList.toggle("sp-visible", displayMode === "icon");
                syncAdvancedVisibility();
                if (displayMode === "time")
                    timeUnitField.select.value = sensorTimeUnit(b);
                if (!persist)
                    return;
                if (displayMode === "time") {
                    b.precision = "time";
                    b.unit = "";
                    b.icon = "Auto";
                    b.icon_on = "Auto";
                    b.options = normalizeSensorOptions(b.options, "time");
                    unitInp.value = "";
                    helpers.saveField("precision", "time");
                    helpers.saveField("unit", "");
                    helpers.saveField("icon", "Auto");
                    helpers.saveField("icon_on", "Auto");
                    helpers.saveField("options", b.options);
                    advancedToggle.input.checked = false;
                    advanced.classList.remove("sp-visible");
                    inputTextInp.value = "";
                    outputTextInp.value = "";
                    inputText2Inp.value = "";
                    outputText2Inp.value = "";
                    resetIconPicker(textIconPicker, "Auto", "cog");
                    resetIconPicker(offIconPicker, "Auto", "cog");
                    resetIconPicker(onIconPicker, "Auto", "cog");
                }
                else if (isTextMode) {
                    b.precision = "text";
                    b.label = "";
                    b.unit = "";
                    b.icon_on = "Auto";
                    b.options = normalizeSensorOptions(b.options, "text");
                    labelInp.value = "";
                    unitInp.value = "";
                    helpers.saveField("precision", "text");
                    helpers.saveField("label", "");
                    helpers.saveField("unit", "");
                    helpers.saveField("icon_on", "Auto");
                    helpers.saveField("options", b.options);
                    resetIconPicker(onIconPicker, "Auto", "cog");
                }
                else if (displayMode === "icon") {
                    b.precision = "icon";
                    b.unit = "";
                    b.options = normalizeSensorOptions(b.options, "icon");
                    unitInp.value = "";
                    helpers.saveField("precision", "icon");
                    helpers.saveField("unit", "");
                    helpers.saveField("options", b.options);
                    advancedToggle.input.checked = false;
                    advanced.classList.remove("sp-visible");
                    inputTextInp.value = "";
                    outputTextInp.value = "";
                    inputText2Inp.value = "";
                    outputText2Inp.value = "";
                }
                else {
                    b.precision = "";
                    b.icon = "Auto";
                    b.icon_on = "Auto";
                    b.options = normalizeSensorOptions(b.options, "");
                    helpers.saveField("precision", "");
                    helpers.saveField("icon", "Auto");
                    helpers.saveField("icon_on", "Auto");
                    helpers.saveField("options", b.options);
                    advancedToggle.input.checked = false;
                    advanced.classList.remove("sp-visible");
                    inputTextInp.value = "";
                    outputTextInp.value = "";
                    inputText2Inp.value = "";
                    outputText2Inp.value = "";
                    resetIconPicker(textIconPicker, "Auto", "cog");
                    resetIconPicker(offIconPicker, "Auto", "cog");
                    resetIconPicker(onIconPicker, "Auto", "cog");
                    precisionSelect.value = "0";
                }
            }
            setMode(displayMode, false);
        },
        renderPreview: function (this: any, b?: any, helpers?: any) {
            if (sensorCardIsLocal(b))
                return sensorLocalPreview(b, helpers);
            if (b.precision === "icon") {
                var stateIconName: any = b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : "cog";
                return {
                    iconHtml: '<span class="sp-btn-icon mdi mdi-' + stateIconName + '"></span>',
                    labelHtml: cardBadgeLabelHtml(helpers, b.label || b.sensor || "Sensor", SENSOR_CARD_METADATA.preview.iconBadge),
                };
            }
            if (b.precision === "text") {
                var iconName: any = b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : "cog";
                return {
                    iconHtml: '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>',
                    labelHtml: cardBadgeLabelHtml(helpers, "State", SENSOR_CARD_METADATA.preview.textBadge),
                };
            }
            if (b.precision === "time") {
                return {
                    iconHtml: cardSensorPreviewHtml(b, helpers, "1h 30m", ""),
                    labelHtml: cardBadgeLabelHtml(helpers, b.label || b.sensor || "Sensor", SENSOR_CARD_METADATA.preview.numericBadge),
                };
            }
            var label: any = b.label || b.sensor || "Sensor";
            var unit: any = b.unit || "";
            var prec: any = parseInt(b.precision || "0", 10) || 0;
            var sampleVal: any = (0).toFixed(prec);
            return {
                iconHtml: cardSensorPreviewHtml(b, helpers, sampleVal, unit),
                labelHtml: cardBadgeLabelHtml(helpers, label, SENSOR_CARD_METADATA.preview.numericBadge),
            };
        },
    });
    function renderSensorLocalSettings(this: any, panel?: any, b?: any, slot?: any, helpers?: any) {
        b.type = "sensor";
        b.sensor = SENSOR_CARD_LOCAL_SENSOR;
        var isTextMode: any = b.precision === "text";
        var showAll: any = false;
        var fetchedSensors: any = null;
        var modeField: any = document.createElement("div");
        modeField.className = "sp-field";
        modeField.appendChild(helpers.fieldLabel("Display"));
        var modeSeg: any = document.createElement("div");
        modeSeg.className = "sp-segment";
        var numericBtn: any = document.createElement("button");
        numericBtn.type = "button";
        numericBtn.textContent = "Numeric";
        var textBtn: any = document.createElement("button");
        textBtn.type = "button";
        textBtn.textContent = "Text";
        modeSeg.appendChild(numericBtn);
        modeSeg.appendChild(textBtn);
        modeField.appendChild(modeSeg);
        panel.appendChild(modeField);
        var pickerSection: any = document.createElement("div");
        panel.appendChild(pickerSection);
        var numericSection: any = condField();
        var lf: any = document.createElement("div");
        lf.className = "sp-field";
        lf.appendChild(helpers.fieldLabel("Label", helpers.idPrefix + "label"));
        var labelInp: any = helpers.textInput(helpers.idPrefix + "label", b.label, "e.g. Living Room");
        lf.appendChild(labelInp);
        numericSection.appendChild(lf);
        helpers.bindField(labelInp, "label", true);
        var uf: any = document.createElement("div");
        uf.className = "sp-field";
        uf.appendChild(helpers.fieldLabel("Unit", helpers.idPrefix + "unit"));
        var unitInp: any = helpers.textInput(helpers.idPrefix + "unit", b.unit, "e.g. \u00B0C");
        unitInp.className = "sp-input";
        uf.appendChild(unitInp);
        numericSection.appendChild(uf);
        helpers.bindField(unitInp, "unit", true);
        var pf: any = document.createElement("div");
        pf.className = "sp-field";
        pf.appendChild(helpers.fieldLabel("Unit Precision", helpers.idPrefix + "precision"));
        var precisionSelect: any = document.createElement("select");
        precisionSelect.className = "sp-select";
        precisionSelect.id = helpers.idPrefix + "precision";
        var precOpts: any = [["0", "10"], ["1", "10.2"], ["2", "10.21"]];
        for (var i: any = 0; i < precOpts.length; i++) {
            var opt: any = document.createElement("option");
            opt.value = precOpts[i][0];
            opt.textContent = precOpts[i][1];
            precisionSelect.appendChild(opt);
        }
        precisionSelect.value = !isTextMode ? (b.precision || "0") : "0";
        precisionSelect.addEventListener("change", function (this: any) {
            b.precision = this.value === "0" ? "" : this.value;
            helpers.saveField("precision", b.precision);
        });
        pf.appendChild(precisionSelect);
        numericSection.appendChild(pf);
        panel.appendChild(numericSection);
        var textSection: any = condField();
        var textIconPicker: any = helpers.makeIconPicker(helpers.idPrefix + "icon-picker", helpers.idPrefix + "icon", b.icon || "Auto", function (this: any, opt?: any) {
            b.icon = opt;
            helpers.saveField("icon", opt);
        });
        textSection.appendChild(textIconPicker);
        panel.appendChild(textSection);
        function setMode(this: any, mode?: any, persist?: any) {
            isTextMode = mode === "text";
            numericBtn.classList.toggle("active", !isTextMode);
            textBtn.classList.toggle("active", isTextMode);
            numericSection.classList.toggle("sp-visible", !isTextMode);
            textSection.classList.toggle("sp-visible", isTextMode);
            if (!persist)
                return;
            if (isTextMode) {
                b.precision = "text";
                b.label = "";
                b.unit = "";
                b.icon_on = "Auto";
                labelInp.value = "";
                unitInp.value = "";
                helpers.saveField("precision", "text");
                helpers.saveField("label", "");
                helpers.saveField("unit", "");
                helpers.saveField("icon_on", "Auto");
            }
            else {
                b.precision = "";
                b.icon = "Auto";
                helpers.saveField("precision", "");
                helpers.saveField("icon", "Auto");
                var iconPreview: any = textIconPicker.querySelector(".sp-icon-picker-preview");
                if (iconPreview)
                    iconPreview.className = "sp-icon-picker-preview mdi mdi-cog";
                var iconInput: any = textIconPicker.querySelector(".sp-icon-picker-input");
                if (iconInput)
                    iconInput.value = "Auto";
                precisionSelect.value = "0";
            }
        }
        numericBtn.addEventListener("click", function (this: any) {
            setMode("numeric", true);
            if (fetchedSensors)
                buildDropdown(fetchedSensors);
        });
        textBtn.addEventListener("click", function (this: any) {
            setMode("text", true);
            if (fetchedSensors)
                buildDropdown(fetchedSensors);
        });
        setMode(isTextMode ? "text" : "numeric", false);
        function buildDropdown(this: any, sensors?: any) {
            pickerSection.innerHTML = "";
            pickerSection.className = "";
            var wantType: any = isTextMode ? "text" : "numeric";
            var filtered: any = sensors.filter(function (this: any, s?: any) {
                return s.type === wantType && (showAll || !s.internal);
            });
            var sf: any = document.createElement("div");
            sf.className = "sp-field";
            sf.appendChild(helpers.fieldLabel("Local Sensor", helpers.idPrefix + "sensor-sel"));
            var sel: any = document.createElement("select");
            sel.className = "sp-select";
            sel.id = helpers.idPrefix + "sensor-sel";
            var placeholder: any = document.createElement("option");
            placeholder.value = "";
            placeholder.textContent = "Choose a sensor…";
            sel.appendChild(placeholder);
            filtered.forEach(function (this: any, s?: any) {
                var opt: any = document.createElement("option");
                opt.value = s.key;
                opt.textContent = s.name + (s.type === "text" ? " (text)" : "");
                if (s.key === b.entity)
                    opt.selected = true;
                sel.appendChild(opt);
            });
            if (b.entity && !filtered.some(function (this: any, s?: any) { return s.key === b.entity; })) {
                var curOpt: any = document.createElement("option");
                curOpt.value = b.entity;
                curOpt.textContent = b.entity + " (current)";
                curOpt.selected = true;
                sel.appendChild(curOpt);
            }
            sel.addEventListener("change", function (this: any) {
                var key: any = this.value;
                if (!key)
                    return;
                b.entity = key;
                helpers.saveField("entity", key);
                var sensor: any = sensors.find(function (this: any, s?: any) { return s.key === key; });
                if (!sensor)
                    return;
                if (!b.label) {
                    b.label = sensor.name;
                    labelInp.value = sensor.name;
                    helpers.saveField("label", sensor.name);
                }
                if (!b.unit && sensor.unit) {
                    b.unit = sensor.unit;
                    unitInp.value = sensor.unit;
                    helpers.saveField("unit", sensor.unit);
                }
                setMode(sensor.type === "text" ? "text" : "numeric", true);
            });
            sf.appendChild(sel);
            pickerSection.appendChild(sf);
            var tog: any = toggleRow("Show internal sensors", helpers.idPrefix + "show-all", showAll);
            tog.input.addEventListener("change", function (this: any) {
                showAll = this.checked;
                buildDropdown(sensors);
            });
            pickerSection.appendChild(tog.row);
        }
        function buildManualInput(this: any) {
            pickerSection.innerHTML = "";
            pickerSection.className = "sp-local-picker-fallback";
            var errDiv: any = document.createElement("div");
            errDiv.className = "sp-banner sp-error";
            errDiv.textContent = "Could not reach device. Enter sensor key manually.";
            pickerSection.appendChild(errDiv);
            var kf: any = document.createElement("div");
            kf.className = "sp-field";
            kf.appendChild(helpers.fieldLabel("Sensor Key", helpers.idPrefix + "local-sensor-key"));
            var keyInp: any = helpers.textInput(helpers.idPrefix + "local-sensor-key", b.entity, "e.g. room_temp");
            kf.appendChild(keyInp);
            pickerSection.appendChild(kf);
            helpers.bindField(keyInp, "entity", true);
            helpers.requireField(keyInp, "Add a sensor key before saving.");
        }
        var loadingDiv: any = document.createElement("div");
        loadingDiv.className = "sp-field";
        loadingDiv.textContent = "Loading sensors…";
        pickerSection.appendChild(loadingDiv);
        fetch("/local_sensors")
            .then(function (this: any, resp?: any) {
            if (!resp.ok)
                throw new Error("HTTP " + resp.status);
            return resp.json();
        })
            .then(function (this: any, data?: any) {
            fetchedSensors = data;
            buildDropdown(data);
        })
            .catch(function (this: any) {
            buildManualInput();
        });
    }
    function sensorLocalPreview(this: any, b?: any, helpers?: any) {
        if (b.precision === "text") {
            var iconName: any = b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : "cog";
            return {
                iconHtml: '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>',
                labelHtml: cardBadgeLabelHtml(helpers, "State", SENSOR_CARD_METADATA.preview.numericBadge),
            };
        }
        var label: any = b.label || b.entity || "Local Sensor";
        var unit: any = b.unit ? helpers.escHtml(b.unit) : "";
        var prec: any = parseInt(b.precision || "0", 10) || 0;
        var sampleVal: any = (0).toFixed(prec);
        return {
            iconHtml: cardSensorPreviewHtml(b, helpers, sampleVal, unit),
            labelHtml: cardBadgeLabelHtml(helpers, label, SENSOR_CARD_METADATA.preview.numericBadge),
        };
    }
    return {
        "SENSOR_CARD_LOCAL_SENSOR": liveGlobal(() => SENSOR_CARD_LOCAL_SENSOR, (value?: any) => { SENSOR_CARD_LOCAL_SENSOR = value; }),
        "sensorCardIsLocal": staticGlobal(sensorCardIsLocal),
        "SENSOR_CARD_METADATA": liveGlobal(() => SENSOR_CARD_METADATA, (value?: any) => { SENSOR_CARD_METADATA = value; }),
        "renderSensorLocalSettings": staticGlobal(renderSensorLocalSettings),
        "sensorLocalPreview": staticGlobal(sensorLocalPreview),
    };
}
