import { state } from "../state/app_instance";
import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function registerActionCardTypes(): GlobalDescriptors {
    // Action card: one-tap Home Assistant shortcuts for scenes, scripts, buttons, and helpers.
    var ACTION_CARD_ACTIONS: any = [
        { value: "scene.turn_on", label: "Run Scene", placeholder: "e.g. scene.movie_mode", icon: "movie-open", domains: ["scene"] },
        { value: "script.turn_on", label: "Run Script", placeholder: "e.g. script.goodnight", icon: "script-text-play", domains: ["script"] },
        { value: "automation.trigger", label: "Trigger Automation", placeholder: "e.g. automation.goodnight", icon: "home-automation", domains: ["automation"] },
        { value: "button.press", label: "Press Button", placeholder: "e.g. button.restart_router", icon: "gesture-tap-button", domains: ["button"] },
        { value: "input_button.press", label: "Press Input Button", placeholder: "e.g. input_button.doorbell", icon: "gesture-tap-button", domains: ["input_button"] },
        { value: "input_boolean.toggle", label: "Toggle Helper", placeholder: "e.g. input_boolean.guest_mode", icon: "toggle-switch-variant", domains: ["input_boolean"] },
        { value: "input_number.set_value", label: "Set Number Helper", placeholder: "e.g. input_number.target_level", icon: "counter", domains: ["input_number"] },
        { value: "input_select.select_option", label: "Option Select", placeholder: "e.g. select.wled_preset", icon: "form-dropdown", domains: ["select", "input_select"] },
        { value: "local", label: "Local Action", placeholder: "e.g. zoom_mute", icon: "gesture-tap", domains: [] },
    ];
    var ACTION_CARD_OPTION_SELECT_ACTION: any = "input_select.select_option";
    var ACTION_CARD_LOCAL_ACTION: any = "local";
    function actionCardInfo(this: any, value?: any) {
        for (var i: any = 0; i < ACTION_CARD_ACTIONS.length; i++) {
            if (ACTION_CARD_ACTIONS[i].value === value)
                return ACTION_CARD_ACTIONS[i];
        }
        return null;
    }
    function actionCardIsOptionSelect(this: any, b?: any) {
        var value: any = typeof b === "string" ? b : b && b.sensor;
        return value === ACTION_CARD_OPTION_SELECT_ACTION || value === "select.select_option";
    }
    function actionCardIsLocal(this: any, b?: any) {
        if (typeof b === "string")
            return b === ACTION_CARD_LOCAL_ACTION;
        return !!(b && (b.type === "action" || b.type === "local") && b.sensor === ACTION_CARD_LOCAL_ACTION);
    }
    function clearActionModeOptions(this: any, b?: any) {
        if (!b)
            return "";
        b.options = copyCardBackgroundOptions("", b.options, b);
        return b.options;
    }
    function normalizeSavedConfigActionFields(this: any, b?: any) {
        if (!b)
            return;
        if (b && b.sensor === "select.select_option")
            b.sensor = ACTION_CARD_OPTION_SELECT_ACTION;
        if (!b.sensor)
            b.sensor = "scene.turn_on";
        if (!actionCardInfo(b.sensor))
            b.sensor = "scene.turn_on";
        b.precision = "";
        if (actionCardStateDisplayMode(b) !== "icon")
            b.icon_on = "Auto";
        if (actionCardIsOptionSelect(b)) {
            b.unit = "";
            clearActionModeOptions(b);
            if (!b.icon || b.icon === "Auto" || b.icon === "Chevron Down")
                b.icon = "Flash";
        }
        else if (actionCardIsLocal(b)) {
            b.unit = "";
            b.precision = "";
            clearActionModeOptions(b);
            b.icon_on = "Auto";
            if (!b.icon || b.icon === "Auto" || b.icon === "Flash")
                b.icon = "Gesture Tap";
        }
    }
    function normalizeActionCardConfig(this: any, b?: any) {
        if (!b)
            return;
        normalizeSavedConfigActionFields(b);
        b.options = normalizeActionOptions(b.options, b.sensor);
    }
    var ACTION_CARD_STATE_ENTITY_OPTION: any = "state_entity";
    var ACTION_CARD_STATE_UNIT_OPTION: any = "state_unit";
    var ACTION_CARD_STATE_PRECISION_OPTION: any = "state_precision";
    function actionCardStateEntity(this: any, b?: any) {
        return configOptionValue(b && b.options, ACTION_CARD_STATE_ENTITY_OPTION);
    }
    function actionCardStateUnit(this: any, b?: any) {
        return configOptionValue(b && b.options, ACTION_CARD_STATE_UNIT_OPTION);
    }
    function actionCardStatePrecision(this: any, b?: any) {
        var value: any = configOptionValue(b && b.options, ACTION_CARD_STATE_PRECISION_OPTION);
        if (value === "icon")
            return "icon";
        if (value === "text")
            return "text";
        return value === "1" || value === "2" ? value : "0";
    }
    function actionCardStateDisplayMode(this: any, b?: any) {
        var rawPrecision: any = configOptionValue(b && b.options, ACTION_CARD_STATE_PRECISION_OPTION);
        if (rawPrecision === "icon")
            return "icon";
        if (rawPrecision === "text")
            return "text";
        if (rawPrecision === "0" || rawPrecision === "1" || rawPrecision === "2" || actionCardStateUnit(b)) {
            return "numeric";
        }
        return actionCardStateEntity(b) ? "text" : "numeric";
    }
    function setActionCardStateOptions(this: any, b?: any, entity?: any, mode?: any, unit?: any, precision?: any) {
        if (!b)
            return "";
        var options: any = b.options;
        entity = String(entity || "").trim();
        if (!entity) {
            options = setConfigOptionValue(options, ACTION_CARD_STATE_ENTITY_OPTION, "");
            options = setConfigOptionValue(options, ACTION_CARD_STATE_UNIT_OPTION, "");
            options = setConfigOptionValue(options, ACTION_CARD_STATE_PRECISION_OPTION, "");
            b.options = options;
            return b.options;
        }
        options = setConfigOptionValue(options, ACTION_CARD_STATE_ENTITY_OPTION, entity);
        if (mode === "icon") {
            options = setConfigOptionValue(options, ACTION_CARD_STATE_UNIT_OPTION, "");
            options = setConfigOptionValue(options, ACTION_CARD_STATE_PRECISION_OPTION, "icon");
        }
        else if (mode === "text") {
            options = setConfigOptionValue(options, ACTION_CARD_STATE_UNIT_OPTION, "");
            options = setConfigOptionValue(options, ACTION_CARD_STATE_PRECISION_OPTION, "text");
        }
        else {
            options = setConfigOptionValue(options, ACTION_CARD_STATE_UNIT_OPTION, unit || "");
            options = setConfigOptionValue(options, ACTION_CARD_STATE_PRECISION_OPTION, precision || "0");
        }
        b.options = options;
        return b.options;
    }
    function actionCardNeedsExtraValue(this: any, value?: any) {
        return value === "input_number.set_value";
    }
    var ACTION_CARD_METADATA: any = {
        mode: {
            label: "Action",
            idSuffix: "action",
            options: ACTION_CARD_ACTIONS,
            value: function (this: any, b?: any) {
                return b.sensor || "scene.turn_on";
            },
        },
        entity: {
            idSuffix: "entity",
            bindName: "entity",
            rerender: true,
            requiredMessage: "Add an entity before saving.",
        },
        stateMode: {
            label: "Type",
            options: [
                ["icon", "Icon"],
                ["numeric", "Numeric"],
                ["text", "Text"],
            ],
        },
        largeNumbers: {
            label: "Large State Numbers",
            idSuffix: "large-state-numbers",
            supported: function (this: any, b?: any) {
                return !actionCardIsOptionSelect(b) && !actionCardIsLocal(b) && actionCardStateDisplayMode(b) === "numeric";
            },
        },
        stateUnitField: {
            label: "Unit",
            idSuffix: "action-state-unit",
            placeholder: "e.g. %",
            bindName: null,
        },
        confirmationToggle: {
            label: "Confirmation Required",
            idSuffix: "script-confirm-toggle",
            checked: function (this: any, b?: any) { return actionScriptConfirmationEnabled(b); },
        },
        scriptFields: {
            label: "Fields",
            idSuffix: "script-fields",
            placeholder: "e.g. mode: night",
            value: function (this: any, b?: any) { return actionScriptFields(b); },
        },
        confirmationMessage: {
            label: "Message",
            idSuffix: "script-confirm-message",
            placeholder: "Run this script?",
            bindName: null,
            value: function (this: any, b?: any) { return actionScriptConfirmationMessage(b); },
        },
        confirmationYes: {
            label: "Confirm Button",
            idSuffix: "script-confirm-yes",
            placeholder: "Yes",
            bindName: null,
            value: function (this: any, b?: any) { return actionScriptConfirmationYesText(b); },
        },
        confirmationNo: {
            label: "Cancel Button",
            idSuffix: "script-confirm-no",
            placeholder: "No",
            bindName: null,
            value: function (this: any, b?: any) { return actionScriptConfirmationNoText(b); },
        },
        preview: {
            optionBadge: "chevron-down",
            actionBadge: "flash",
        },
    };
    registerButtonType("action", {
        label: "Action",
        allowInSubpage: true,
        labelPlaceholder: "e.g. Movie Mode",
        cardMetadata: ACTION_CARD_METADATA,
        onSelect: function (this: any, b?: any) {
            b.entity = "";
            b.sensor = "scene.turn_on";
            b.unit = "";
            b.icon = "Flash";
            b.icon_on = "Auto";
            b.precision = "";
            b.options = "";
        },
        renderSettingsBeforeLabel: function (this: any, panel?: any, b?: any, slot?: any, helpers?: any) {
            normalizeActionCardConfig(b);
            var actionField: any = helpers.renderCardModeSelector(panel, b, helpers, Object.assign({}, ACTION_CARD_METADATA, {
                mode: Object.assign({}, ACTION_CARD_METADATA.mode, {
                    onChange: function (this: any) {
                        var wasLocal: any = actionCardIsLocal(b);
                        b.sensor = this.value;
                        helpers.saveField("sensor", b.sensor);
                        if (wasLocal !== actionCardIsLocal(b)) {
                            b.entity = "";
                            helpers.saveField("entity", "");
                        }
                        if (!actionCardNeedsExtraValue(b.sensor)) {
                            b.unit = "";
                            helpers.saveField("unit", "");
                        }
                        if (actionCardIsOptionSelect(b)) {
                            clearActionModeOptions(b);
                            helpers.saveField("options", b.options);
                        }
                        else if (actionCardIsLocal(b)) {
                            clearActionModeOptions(b);
                            helpers.saveField("options", b.options);
                            if (!b.icon || b.icon === "Auto" || b.icon === "Flash") {
                                b.icon = "Gesture Tap";
                                helpers.saveField("icon", b.icon);
                            }
                        }
                        else {
                            b.options = normalizeActionOptions(b.options, b.sensor);
                            helpers.saveField("options", b.options);
                            if (b.icon === "Gesture Tap") {
                                b.icon = "Flash";
                                helpers.saveField("icon", b.icon);
                            }
                        }
                        b.icon_on = "Auto";
                        b.precision = "";
                        helpers.saveField("icon_on", "Auto");
                        helpers.saveField("precision", "");
                        renderButtonSettings();
                    },
                }),
            }));
            var actionSelect: any = actionField.select;
            actionSelect.value = b.sensor;
        },
        renderSettings: function (this: any, panel?: any, b?: any, slot?: any, helpers?: any) {
            normalizeActionCardConfig(b);
            var info: any = actionCardInfo(b.sensor) || ACTION_CARD_ACTIONS[0];
            var isOptionSelect: any = actionCardIsOptionSelect(b);
            var isLocal: any = actionCardIsLocal(b);
            if (isLocal) {
                renderActionCardLocalSettings(panel, b, slot, helpers);
                return;
            }
            var entityField: any = helpers.renderCardEntityField(panel, b, helpers, {
                entity: Object.assign({}, ACTION_CARD_METADATA.entity, {
                    label: isOptionSelect ? "Select Entity" : "Action Entity",
                    placeholder: info.placeholder,
                    domains: info.domains,
                }),
            });
            var entityInp: any = entityField.input;
            if (actionCardNeedsExtraValue(b.sensor)) {
                var valueInput: any = helpers.textInput(helpers.idPrefix + "action-value", b.unit, "e.g. 50");
                var valueLabel: any = helpers.fieldLabel("Value", helpers.idPrefix + "action-value");
                var valueField: any = document.createElement("div");
                valueField.className = "sp-field";
                valueField.appendChild(valueLabel);
                valueField.appendChild(valueInput);
                panel.appendChild(valueField);
                helpers.bindField(valueInput, "unit", true);
            }
            if (!isOptionSelect) {
                helpers.renderCardIconPicker(panel, b, helpers, {
                    pickerIdSuffix: "icon-picker",
                    idSuffix: "icon",
                    field: "icon",
                    fallback: "Flash",
                });
            }
            entityInp._entityDomains = info.domains || [];
            refreshEntityDatalist(entityInp);
            if (isOptionSelect)
                return;
            if (actionCardIsScript(b)) {
                var fieldsInput: any = document.createElement("textarea");
                fieldsInput.className = "sp-input sp-textarea";
                fieldsInput.id = helpers.idPrefix + ACTION_CARD_METADATA.scriptFields.idSuffix;
                fieldsInput.placeholder = ACTION_CARD_METADATA.scriptFields.placeholder;
                fieldsInput.value = actionScriptFields(b);
                fieldsInput.rows = 3;
                fieldsInput.spellcheck = false;
                var fieldsWrapper: any = document.createElement("div");
                fieldsWrapper.className = "sp-field";
                fieldsWrapper.appendChild(helpers.fieldLabel(ACTION_CARD_METADATA.scriptFields.label, fieldsInput.id));
                fieldsWrapper.appendChild(fieldsInput);
                panel.appendChild(fieldsWrapper);
                function saveScriptFields(this: any) {
                    setActionScriptFields(b, fieldsInput.value);
                    helpers.saveField("options", b.options);
                }
                fieldsInput.addEventListener("input", saveScriptFields);
                fieldsInput.addEventListener("change", saveScriptFields);
                fieldsInput.addEventListener("blur", saveScriptFields);
                var confirmOn: any = actionScriptConfirmationEnabled(b);
                var confirmToggle: any = helpers.renderCardOptionToggle(panel, b, helpers, ACTION_CARD_METADATA.confirmationToggle);
                var confirmSection: any = condField();
                confirmSection.classList.add("sp-action-confirm-section");
                if (confirmOn)
                    confirmSection.classList.add("sp-visible");
                var messageField: any = helpers.renderCardTextField(confirmSection, b, helpers, ACTION_CARD_METADATA.confirmationMessage);
                var messageInput: any = messageField.input;
                messageInput.maxLength = 72;
                var yesField: any = helpers.renderCardTextField(confirmSection, b, helpers, ACTION_CARD_METADATA.confirmationYes);
                var yesInput: any = yesField.input;
                yesInput.maxLength = 20;
                var noField: any = helpers.renderCardTextField(confirmSection, b, helpers, ACTION_CARD_METADATA.confirmationNo);
                var noInput: any = noField.input;
                noInput.maxLength = 20;
                panel.appendChild(confirmSection);
                function saveScriptConfirmationOptions(this: any) {
                    setActionScriptConfirmationOptions(b, confirmToggle.input.checked, messageInput.value || actionScriptConfirmationDefaultMessage(), yesInput.value || SWITCH_CONFIRM_DEFAULT_YES, noInput.value || SWITCH_CONFIRM_DEFAULT_NO);
                    helpers.saveField("options", b.options);
                }
                confirmToggle.input.addEventListener("change", function (this: any) {
                    confirmSection.classList.toggle("sp-visible", this.checked);
                    if (this.checked) {
                        if (!messageInput.value)
                            messageInput.value = actionScriptConfirmationDefaultMessage();
                        if (!yesInput.value)
                            yesInput.value = SWITCH_CONFIRM_DEFAULT_YES;
                        if (!noInput.value)
                            noInput.value = SWITCH_CONFIRM_DEFAULT_NO;
                    }
                    saveScriptConfirmationOptions();
                });
                [messageInput, yesInput, noInput].forEach(function (this: any, input?: any) {
                    input.addEventListener("input", saveScriptConfirmationOptions);
                    input.addEventListener("change", saveScriptConfirmationOptions);
                    input.addEventListener("blur", saveScriptConfirmationOptions);
                    input.addEventListener("keydown", function (this: any, e?: any) {
                        if (e.key === "Enter") {
                            saveScriptConfirmationOptions();
                            this.blur();
                        }
                    });
                });
            }
            var stateEntity: any = actionCardStateEntity(b);
            var stateMode: any = actionCardStateDisplayMode(b);
            var stateUnit: any = actionCardStateUnit(b);
            var statePrecision: any = actionCardStatePrecision(b);
            var mode: any = helpers.renderCardSegmentControl(panel, b, helpers, {
                segment: Object.assign({}, ACTION_CARD_METADATA.stateMode, {
                    value: function (this: any) { return stateMode; },
                    onSelect: function (this: any, button?: any, cardHelpers?: any, value?: any) {
                        setStateMode(value, true);
                    },
                }),
            });
            var iconBtn: any = mode.buttons.icon;
            var numericBtn: any = mode.buttons.numeric;
            var textBtn: any = mode.buttons.text;
            var stateEntityField: any = helpers.renderCardEntityField(panel, b, helpers, {
                entity: {
                    label: "Sensor Entity",
                    idSuffix: "action-state-entity",
                    value: function (this: any) { return stateEntity; },
                    placeholder: "e.g. sensor.printer_percent_complete",
                    domains: ["sensor", "binary_sensor", "text_sensor"],
                    bindName: null,
                    rerender: false,
                },
            });
            var stateEntityInp: any = stateEntityField.input;
            var iconOnSection: any = helpers.renderCardIconPicker(panel, b, helpers, {
                pickerIdSuffix: "icon-on-picker",
                idSuffix: "icon-on",
                field: "icon_on",
                fallback: "Auto",
                label: "On Icon",
            });
            var numericSection: any = condField();
            var stateUnitField: any = helpers.renderCardTextField(numericSection, b, helpers, Object.assign({}, ACTION_CARD_METADATA.stateUnitField, {
                value: function (this: any) { return stateUnit; },
            }));
            var stateUnitInp: any = stateUnitField.input;
            var statePrecisionField: any = helpers.precisionField(helpers.idPrefix + "action-state-precision", stateMode === "numeric" ? statePrecision : "0", function (this: any) {
                statePrecision = this.value || "0";
                saveStateOptions();
            });
            var statePrecisionSelect: any = statePrecisionField.select;
            numericSection.appendChild(statePrecisionField.field);
            helpers.renderCardLargeNumbersToggle(numericSection, b, helpers, ACTION_CARD_METADATA);
            panel.appendChild(numericSection);
            function saveStateOptions(this: any) {
                stateEntity = stateEntityInp.value;
                stateUnit = stateUnitInp.value;
                helpers.saveField("options", setActionCardStateOptions(b, stateEntity, stateMode, stateUnit, statePrecision));
            }
            function setStateMode(this: any, modeValue?: any, persist?: any) {
                stateMode = modeValue === "icon" || modeValue === "text" ? modeValue : "numeric";
                iconBtn.classList.toggle("active", stateMode === "icon");
                numericBtn.classList.toggle("active", stateMode === "numeric");
                textBtn.classList.toggle("active", stateMode === "text");
                iconOnSection.style.display = stateMode === "icon" ? "" : "none";
                numericSection.classList.toggle("sp-visible", stateMode === "numeric");
                if (!persist)
                    return;
                if (stateMode === "icon" || stateMode === "text") {
                    stateUnit = "";
                    stateUnitInp.value = "";
                    statePrecision = "0";
                    statePrecisionSelect.value = "0";
                }
                if (stateMode !== "icon") {
                    b.icon_on = "Auto";
                    helpers.saveField("icon_on", "Auto");
                }
                saveStateOptions();
            }
            setStateMode(stateMode, false);
            stateEntityInp.addEventListener("input", saveStateOptions);
            stateEntityInp.addEventListener("change", saveStateOptions);
            stateEntityInp.addEventListener("blur", saveStateOptions);
            stateEntityInp.addEventListener("keydown", function (this: any, e?: any) {
                if (e.key === "Enter") {
                    saveStateOptions();
                    this.blur();
                }
            });
            stateUnitInp.addEventListener("input", saveStateOptions);
            stateUnitInp.addEventListener("change", saveStateOptions);
            stateUnitInp.addEventListener("blur", saveStateOptions);
            stateUnitInp.addEventListener("keydown", function (this: any, e?: any) {
                if (e.key === "Enter") {
                    saveStateOptions();
                    this.blur();
                }
            });
        },
        renderPreview: function (this: any, b?: any, helpers?: any) {
            var label: any = b.label || b.entity || (actionCardIsLocal(b) ? "Local Action" : "Action");
            if (actionCardIsLocal(b)) {
                var localIconName: any = b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : "gesture-tap";
                return {
                    iconHtml: '<span class="sp-btn-icon mdi mdi-' + localIconName + '"></span>',
                    labelHtml: cardBadgeLabelHtml(helpers, label, "chip"),
                };
            }
            if (actionCardIsOptionSelect(b)) {
                return {
                    iconHtml: cardSensorPreviewHtml(b, helpers, "Option", null),
                    labelHtml: cardBadgeLabelHtml(helpers, label, ACTION_CARD_METADATA.preview.optionBadge),
                };
            }
            var iconName: any = b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : "flash";
            if (actionCardStateEntity(b) && actionCardStateDisplayMode(b) === "numeric" &&
                cardLargeNumbersActiveForCardSize(b, helpers, ACTION_CARD_METADATA)) {
                return {
                    iconHtml: cardSensorPreviewHtml(b, helpers, "42", actionCardStateUnit(b) || ""),
                    labelHtml: cardBadgeLabelHtml(helpers, label, ACTION_CARD_METADATA.preview.actionBadge),
                };
            }
            var stateBadge: any = actionCardStateEntity(b)
                ? '<span class="sp-sensor-badge mdi mdi-' +
                    (actionCardStateDisplayMode(b) === "icon" ? "toggle-switch" :
                        (actionCardStateDisplayMode(b) === "text" ? "format-text" : "gauge")) +
                    '"></span>'
                : "";
            return {
                iconHtml: stateBadge + '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>',
                labelHtml: cardBadgeLabelHtml(helpers, label, ACTION_CARD_METADATA.preview.actionBadge),
            };
        },
    });
    function renderActionCardLocalSettings(this: any, panel?: any, b?: any, slot?: any, helpers?: any) {
        var pickerSection: any = document.createElement("div");
        pickerSection.className = "sp-field";
        panel.appendChild(pickerSection);
        helpers.renderCardIconPicker(panel, b, helpers, {
            pickerIdSuffix: "icon-picker",
            idSuffix: "icon",
            field: "icon",
            fallback: "Gesture Tap",
        });
        function buildDropdown(this: any, actions?: any) {
            pickerSection.innerHTML = "";
            pickerSection.className = "sp-field";
            pickerSection.appendChild(helpers.fieldLabel("Local Action", helpers.idPrefix + "action-sel"));
            var sel: any = document.createElement("select");
            sel.className = "sp-select";
            sel.id = helpers.idPrefix + "action-sel";
            var placeholder: any = document.createElement("option");
            placeholder.value = "";
            placeholder.textContent = "Choose an action…";
            sel.appendChild(placeholder);
            actions.forEach(function (this: any, a?: any) {
                var opt: any = document.createElement("option");
                opt.value = a.key;
                opt.textContent = a.label ? a.label + " (" + a.key + ")" : a.key;
                if (a.key === b.entity)
                    opt.selected = true;
                sel.appendChild(opt);
            });
            if (b.entity && !actions.some(function (this: any, a?: any) { return a.key === b.entity; })) {
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
                var action: any = actions.find(function (this: any, a?: any) { return a.key === key; });
                if (action && action.label && !b.label) {
                    b.label = action.label;
                    helpers.saveField("label", action.label);
                    var labelInp: any = document.getElementById(helpers.idPrefix + "label");
                    if (labelInp)
                        labelInp.value = action.label;
                }
            });
            pickerSection.appendChild(sel);
        }
        function buildEmpty(this: any) {
            pickerSection.innerHTML = "";
            pickerSection.className = "";
            var banner: any = document.createElement("div");
            banner.className = "sp-banner sp-error";
            banner.textContent =
                "No local actions are registered on this device. " +
                    "Add register_local_action() calls to your device’s on_boot lambda.";
            pickerSection.appendChild(banner);
        }
        function buildFallback(this: any) {
            pickerSection.innerHTML = "";
            pickerSection.className = "sp-local-picker-fallback";
            var banner: any = document.createElement("div");
            banner.className = "sp-banner sp-error";
            banner.textContent = "Could not reach device. Enter the action key manually.";
            pickerSection.appendChild(banner);
            var kf: any = document.createElement("div");
            kf.className = "sp-field";
            kf.appendChild(helpers.fieldLabel("Action Key", helpers.idPrefix + "local-key"));
            var keyInp: any = helpers.textInput(helpers.idPrefix + "local-key", b.entity, "e.g. zoom_mute");
            kf.appendChild(keyInp);
            pickerSection.appendChild(kf);
            helpers.bindField(keyInp, "entity", true);
            helpers.requireField(keyInp, "Add an action key before saving.");
        }
        pickerSection.textContent = "Loading actions…";
        fetch("/local_actions")
            .then(function (this: any, resp?: any) {
            if (!resp.ok)
                throw new Error("HTTP " + resp.status);
            return resp.json();
        })
            .then(function (this: any, data?: any) {
            if (!data.length) {
                buildEmpty();
            }
            else {
                buildDropdown(data);
            }
        })
            .catch(function (this: any) {
            buildFallback();
        });
    }
    return {
        "ACTION_CARD_ACTIONS": liveGlobal(() => ACTION_CARD_ACTIONS, (value?: any) => { ACTION_CARD_ACTIONS = value; }),
        "ACTION_CARD_OPTION_SELECT_ACTION": liveGlobal(() => ACTION_CARD_OPTION_SELECT_ACTION, (value?: any) => { ACTION_CARD_OPTION_SELECT_ACTION = value; }),
        "ACTION_CARD_LOCAL_ACTION": liveGlobal(() => ACTION_CARD_LOCAL_ACTION, (value?: any) => { ACTION_CARD_LOCAL_ACTION = value; }),
        "actionCardInfo": staticGlobal(actionCardInfo),
        "actionCardIsOptionSelect": staticGlobal(actionCardIsOptionSelect),
        "actionCardIsLocal": staticGlobal(actionCardIsLocal),
        "clearActionModeOptions": staticGlobal(clearActionModeOptions),
        "normalizeSavedConfigActionFields": staticGlobal(normalizeSavedConfigActionFields),
        "normalizeActionCardConfig": staticGlobal(normalizeActionCardConfig),
        "ACTION_CARD_STATE_ENTITY_OPTION": liveGlobal(() => ACTION_CARD_STATE_ENTITY_OPTION, (value?: any) => { ACTION_CARD_STATE_ENTITY_OPTION = value; }),
        "ACTION_CARD_STATE_UNIT_OPTION": liveGlobal(() => ACTION_CARD_STATE_UNIT_OPTION, (value?: any) => { ACTION_CARD_STATE_UNIT_OPTION = value; }),
        "ACTION_CARD_STATE_PRECISION_OPTION": liveGlobal(() => ACTION_CARD_STATE_PRECISION_OPTION, (value?: any) => { ACTION_CARD_STATE_PRECISION_OPTION = value; }),
        "actionCardStateEntity": staticGlobal(actionCardStateEntity),
        "actionCardStateUnit": staticGlobal(actionCardStateUnit),
        "actionCardStatePrecision": staticGlobal(actionCardStatePrecision),
        "actionCardStateDisplayMode": staticGlobal(actionCardStateDisplayMode),
        "setActionCardStateOptions": staticGlobal(setActionCardStateOptions),
        "actionCardNeedsExtraValue": staticGlobal(actionCardNeedsExtraValue),
        "ACTION_CARD_METADATA": liveGlobal(() => ACTION_CARD_METADATA, (value?: any) => { ACTION_CARD_METADATA = value; }),
        "renderActionCardLocalSettings": staticGlobal(renderActionCardLocalSettings),
    };
}
