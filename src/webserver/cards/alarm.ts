import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function registerAlarmCardTypes(): GlobalDescriptors {
    // Alarm cards: one-tap alarm_control_panel actions.
    var ALARM_CONTROL_PANEL_VALUE: any = "control_panel";
    function alarmControlPanelValue(this: any) {
        return alarmBehaviorSpec().controlPanelValue || ALARM_CONTROL_PANEL_VALUE;
    }
    function alarmUsesDefaultIcon(this: any, icon?: any) {
        return !icon || icon === "Auto" || icon === "Security" || icon === "Shield Home" || icon === "Alarm";
    }
    function alarmCardTypeOptions(this: any) {
        var options: any = [
            { value: alarmControlPanelValue(), label: "Combined Control" },
        ];
        var actions: any = alarmActionSpecs();
        for (var i: any = 0; i < actions.length; i++)
            options.push(actions[i]);
        return options;
    }
    function alarmCardTypeOptionsForSettings(this: any) {
        return alarmCardTypeOptions();
    }
    function alarmLabelIsGenerated(this: any, label?: any) {
        if (!label)
            return true;
        var actions: any = alarmActionSpecs();
        for (var i: any = 0; i < actions.length; i++) {
            if (label === actions[i].label)
                return true;
        }
        return false;
    }
    function alarmIconIsGenerated(this: any, icon?: any) {
        if (!icon || icon === "Auto" || alarmUsesDefaultIcon(icon))
            return true;
        var actions: any = alarmActionSpecs();
        for (var i: any = 0; i < actions.length; i++) {
            if (alarmActionIconIsGenerated(actions[i].value, icon))
                return true;
        }
        return false;
    }
    function setAlarmCardType(this: any, b?: any, value?: any, helpers?: any) {
        var info: any = alarmActionInfo(value);
        var wasAlarmAction: any = b.type === "alarm_action";
        if (value === alarmControlPanelValue() || !info) {
            var shouldUseControlLabel: any = wasAlarmAction && alarmLabelIsGenerated(b.label);
            var shouldUseControlIcon: any = alarmIconIsGenerated(b.icon);
            b.type = "alarm";
            b.sensor = "";
            b.unit = "";
            b.precision = "";
            b.icon_on = "Auto";
            if (shouldUseControlLabel)
                b.label = "";
            if (shouldUseControlIcon)
                b.icon = "Security";
            b.options = normalizeAlarmOptions(b.options);
            helpers.saveField("type", b.type);
            helpers.saveField("sensor", "");
            helpers.saveField("unit", "");
            helpers.saveField("precision", "");
            helpers.saveField("icon_on", "Auto");
            helpers.saveField("label", b.label || "");
            helpers.saveField("icon", b.icon || "Security");
            helpers.saveField("options", b.options || "");
            renderButtonSettings();
            return;
        }
        info = info || alarmActionSpecs()[0];
        var oldInfo: any = alarmActionInfo(b.sensor);
        var shouldUseGeneratedLabel: any = !wasAlarmAction || alarmLabelIsGenerated(b.label);
        var shouldUseGeneratedIcon: any = !wasAlarmAction || alarmIconIsGenerated(b.icon) ||
            (oldInfo && alarmActionIconIsGenerated(oldInfo.value, b.icon));
        b.type = "alarm_action";
        b.sensor = info.value;
        b.unit = "";
        b.precision = "";
        b.icon_on = "Auto";
        if (shouldUseGeneratedLabel)
            b.label = info.label;
        if (shouldUseGeneratedIcon)
            b.icon = info.icon;
        b.options = normalizeAlarmOptions(b.options);
        helpers.saveField("type", b.type);
        helpers.saveField("sensor", b.sensor || "");
        helpers.saveField("unit", "");
        helpers.saveField("precision", "");
        helpers.saveField("icon_on", "Auto");
        helpers.saveField("label", b.label || "");
        helpers.saveField("icon", b.icon || "Auto");
        helpers.saveField("options", b.options || "");
        renderButtonSettings();
    }
    var ALARM_CARD_METADATA: any = {
        mode: {
            label: "Type",
            idSuffix: "alarm-card-type",
            options: alarmCardTypeOptionsForSettings,
            value: function (this: any, b?: any) {
                return b.type === "alarm"
                    ? alarmControlPanelValue()
                    : (alarmActionInfo(b.sensor) || alarmActionSpecs()[0]).value;
            },
        },
        entity: {
            label: "Alarm Entity",
            placeholder: "e.g. alarm_control_panel.house",
            domains: function (this: any, b?: any) { return cardContractDomains(b && b.type === "alarm_action" ? "alarm_action" : "alarm"); },
            bindName: "entity",
            rerender: true,
            requiredMessage: "Add an alarm_control_panel entity before saving.",
        },
        labelDisplay: {
            label: "Label Display",
            options: [
                ["name", "Name"],
                ["status", "Status"],
            ],
        },
        iconDisplay: {
            label: "Icon Display",
            options: [
                ["static", "Static"],
                ["status", "Status"],
            ],
        },
    };
    function renderAlarmCardTypeField(this: any, panel?: any, b?: any, helpers?: any) {
        helpers.renderCardModeSelector(panel, b, helpers, Object.assign({}, ALARM_CARD_METADATA, {
            mode: Object.assign({}, ALARM_CARD_METADATA.mode, {
                options: alarmCardTypeOptionsForSettings(),
                onChange: function (this: any) {
                    setAlarmCardType(b, this.value, helpers);
                },
            }),
        }));
    }
    function renderAlarmVisibleActionsField(this: any, panel?: any, b?: any, helpers?: any) {
        var actions: any = alarmActionSpecs();
        if (!actions.length)
            return null;
        var field: any = document.createElement("div");
        field.className = "sp-field";
        field.appendChild(helpers.fieldLabel("Visible Actions", helpers.idPrefix + "alarm-visible-actions"));
        var inputs: any = [];
        function selectedActions(this: any) {
            var selected: any = [];
            for (var i: any = 0; i < inputs.length; i++) {
                if (inputs[i].input.checked)
                    selected.push(inputs[i].value);
            }
            return selected;
        }
        function syncInputs(this: any, values?: any) {
            values = values || alarmVisibleActions(b);
            var selectedCount: any = values.length;
            for (var i: any = 0; i < inputs.length; i++) {
                inputs[i].input.checked = values.indexOf(inputs[i].value) >= 0;
                inputs[i].input.disabled = !inputs[i].input.checked && selectedCount >= alarmMaxVisibleActions();
            }
        }
        var visible: any = alarmVisibleActions(b);
        for (var i: any = 0; i < actions.length; i++) {
            var action: any = actions[i];
            var row: any = helpers.toggleRow(action.label, helpers.idPrefix + "alarm-visible-action-" + action.value, visible.indexOf(action.value) >= 0);
            field.appendChild(row.row);
            inputs.push({ value: action.value, input: row.input });
            row.input.addEventListener("change", function (this: any) {
                var selected: any = selectedActions();
                setAlarmVisibleActions(b, selected);
                helpers.saveField("options", b.options);
                syncInputs(alarmVisibleActions(b));
                scheduleRender();
            });
        }
        syncInputs(visible);
        panel.appendChild(field);
        return field;
    }
    registerButtonType("alarm", {
        label: function (this: any) { return cardContractCardLabel("alarm"); },
        allowInSubpage: function (this: any) { return cardContractAllowInSubpage("alarm"); },
        pickerKey: function (this: any) { return cardContractPickerKey("alarm"); },
        hidden: function (this: any) { return cardContractHidden("alarm"); },
        hideLabel: true,
        labelPlaceholder: "e.g. House Alarm",
        defaultConfig: function (this: any) { return cardContractDefaultConfig("alarm"); },
        cardMetadata: ALARM_CARD_METADATA,
        onSelect: function (this: any, b?: any) {
            b.entity = "";
            b.label = "";
            b.sensor = "";
            b.unit = "";
            b.precision = "";
            b.icon = "Security";
            b.icon_on = "Auto";
            b.options = "";
        },
        renderSettingsBeforeLabel: function (this: any, panel?: any, b?: any, slot?: any, helpers?: any) {
            renderAlarmCardTypeField(panel, b, helpers);
        },
        renderSettings: function (this: any, panel?: any, b?: any, slot?: any, helpers?: any) {
            b.sensor = "";
            b.unit = "";
            b.precision = "";
            b.icon_on = "Auto";
            if (!b.icon || b.icon === "Auto")
                b.icon = "Security";
            var normalizedOptions: any = normalizeAlarmOptions(b.options);
            if (b.options !== normalizedOptions) {
                b.options = normalizedOptions;
                helpers.saveField("options", normalizedOptions);
            }
            helpers.renderCardEntityField(panel, b, helpers, {
                entity: Object.assign({}, ALARM_CARD_METADATA.entity, {
                    idSuffix: "alarm-entity",
                }),
            });
            var cardSettingsDisclosure: any = helpers.disclosureSection("Card Settings", helpers.idPrefix + "alarm-card-settings", false);
            var cardSettings: any = cardSettingsDisclosure.section;
            var modalSettingsDisclosure: any = helpers.disclosureSection("Modal Settings", helpers.idPrefix + "alarm-modal-settings", false);
            var modalSettings: any = modalSettingsDisclosure.section;
            var labelHost: any = condField();
            helpers.renderCardTextField(labelHost, b, helpers, {
                label: "Label",
                idSuffix: "alarm-label",
                field: "label",
                placeholder: "e.g. House Alarm",
                rerender: true,
            });
            function setLabelVisible(this: any, value?: any) {
                labelHost.classList.toggle("sp-visible", value === "name");
            }
            helpers.renderCardSegmentControl(cardSettings, b, helpers, {
                segment: Object.assign({}, ALARM_CARD_METADATA.labelDisplay, {
                    value: function (this: any) { return alarmLabelDisplayMode(b); },
                    onSelect: function (this: any, button?: any, cardHelpers?: any, value?: any) {
                        setAlarmLabelDisplayMode(button, value);
                        cardHelpers.saveField("options", button.options);
                        setLabelVisible(value);
                        scheduleRender();
                    },
                }),
            });
            setLabelVisible(alarmLabelDisplayMode(b));
            cardSettings.appendChild(labelHost);
            var iconHost: any = condField();
            helpers.renderCardIconPicker(iconHost, b, helpers, {
                pickerIdSuffix: "alarm-icon-picker",
                idSuffix: "alarm-icon",
                field: "icon",
                fallback: "Security",
                label: "Icon",
            });
            function setIconVisible(this: any, value?: any) {
                iconHost.classList.toggle("sp-visible", value === "static");
            }
            helpers.renderCardSegmentControl(cardSettings, b, helpers, {
                segment: Object.assign({}, ALARM_CARD_METADATA.iconDisplay, {
                    value: function (this: any) { return alarmIconDisplayMode(b); },
                    onSelect: function (this: any, button?: any, cardHelpers?: any, value?: any) {
                        setAlarmIconDisplayMode(button, value);
                        cardHelpers.saveField("options", button.options);
                        setIconVisible(value);
                        scheduleRender();
                    },
                }),
            });
            setIconVisible(alarmIconDisplayMode(b));
            cardSettings.appendChild(iconHost);
            panel.appendChild(cardSettingsDisclosure.panel);
            renderAlarmVisibleActionsField(modalSettings, b, helpers);
            function savePinOptions(this: any) {
                setAlarmPinRequired(b, "arm", armPinToggle.input.checked);
                setAlarmPinRequired(b, "disarm", disarmPinToggle.input.checked);
                helpers.saveField("options", b.options);
            }
            var pinSettingsDisclosure: any = helpers.disclosureSection("PIN Settings", helpers.idPrefix + "alarm-pin-settings", false);
            var pinSettings: any = pinSettingsDisclosure.section;
            var armPinToggle: any = helpers.renderCardOptionToggle(pinSettings, b, helpers, {
                label: "PIN required for arming",
                idSuffix: "alarm-pin-arm",
                checked: function (this: any) { return alarmPinRequired(b, "arm"); },
                onChange: savePinOptions,
            });
            var disarmPinToggle: any = helpers.renderCardOptionToggle(pinSettings, b, helpers, {
                label: "PIN required for disarming",
                idSuffix: "alarm-pin-disarm",
                checked: function (this: any) { return alarmPinRequired(b, "disarm"); },
                onChange: savePinOptions,
            });
            modalSettings.appendChild(pinSettingsDisclosure.panel);
            panel.appendChild(modalSettingsDisclosure.panel);
        },
        renderPreview: function (this: any, b?: any, helpers?: any) {
            var label: any = (b.label && b.label.trim()) || (b.entity && b.entity.trim()) || "Alarm";
            if (alarmLabelDisplayMode(b) === "status")
                label = "Disarmed";
            var iconName: any = iconSlug(b.icon && b.icon !== "Auto" ? b.icon : "Security");
            if (alarmIconDisplayMode(b) === "status")
                iconName = iconSlug("Shield Off");
            return {
                iconHtml: '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>',
                labelHtml: '<span class="sp-btn-label">' + helpers.escHtml(label) + '</span>',
            };
        },
    });
    registerButtonType("alarm_action", {
        label: function (this: any) { return cardContractCardLabel("alarm_action"); },
        allowInSubpage: function (this: any) { return cardContractAllowInSubpage("alarm_action"); },
        labelPlaceholder: "e.g. Arm Away",
        pickerKey: function (this: any) { return cardContractPickerKey("alarm_action"); },
        hidden: function (this: any) { return cardContractHidden("alarm_action"); },
        defaultConfig: function (this: any) { return cardContractDefaultConfig("alarm_action"); },
        cardMetadata: ALARM_CARD_METADATA,
        isAvailable: function (this: any) { return false; },
        onSelect: function (this: any, b?: any) {
            var info: any = alarmActionSpecs()[0];
            b.entity = "";
            b.label = info.label;
            b.sensor = info.value;
            b.unit = "";
            b.icon = info.icon;
            b.icon_on = "Auto";
            b.precision = "";
            b.options = "";
        },
        renderSettingsBeforeLabel: function (this: any, panel?: any, b?: any, slot?: any, helpers?: any) {
            b.sensor = alarmActionInfo(b.sensor) ? b.sensor : "away";
            renderAlarmCardTypeField(panel, b, helpers);
        },
        renderSettings: function (this: any, panel?: any, b?: any, slot?: any, helpers?: any) {
            b.sensor = alarmActionInfo(b.sensor) ? b.sensor : "away";
            b.unit = "";
            b.precision = "";
            b.icon_on = "Auto";
            b.options = normalizeAlarmOptions(b.options);
            helpers.renderCardEntityField(panel, b, helpers, {
                entity: Object.assign({}, ALARM_CARD_METADATA.entity, {
                    idSuffix: "alarm-action-entity",
                }),
            });
            helpers.renderCardIconPicker(panel, b, helpers, {
                pickerIdSuffix: "alarm-action-icon-picker",
                idSuffix: "alarm-action-icon",
                field: "icon",
                fallback: function (this: any) { return alarmActionInfo(b.sensor).icon; },
                label: "Icon",
            });
            var pinMode: any = b.sensor === "disarm" ? "disarm" : "arm";
            helpers.renderCardOptionToggle(panel, b, helpers, {
                label: "PIN required",
                idSuffix: "alarm-action-pin",
                checked: function (this: any) { return alarmPinRequired(b, pinMode); },
                onChange: function (this: any, button?: any, cardHelpers?: any, checked?: any) {
                    setAlarmPinRequired(button, pinMode, checked);
                    cardHelpers.saveField("options", button.options);
                },
            });
        },
        renderPreview: function (this: any, b?: any, helpers?: any) {
            var info: any = alarmActionInfo(b.sensor) || alarmActionSpecs()[0];
            var label: any = b.label || info.label;
            var iconName: any = iconSlug(b.icon || info.icon);
            return {
                iconHtml: '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>',
                labelHtml: '<span class="sp-btn-label">' + helpers.escHtml(label) + '</span>',
            };
        },
    });
    return {
        "ALARM_CONTROL_PANEL_VALUE": liveGlobal(() => ALARM_CONTROL_PANEL_VALUE, (value?: any) => { ALARM_CONTROL_PANEL_VALUE = value; }),
        "alarmControlPanelValue": staticGlobal(alarmControlPanelValue),
        "alarmUsesDefaultIcon": staticGlobal(alarmUsesDefaultIcon),
        "alarmCardTypeOptions": staticGlobal(alarmCardTypeOptions),
        "alarmCardTypeOptionsForSettings": staticGlobal(alarmCardTypeOptionsForSettings),
        "alarmLabelIsGenerated": staticGlobal(alarmLabelIsGenerated),
        "alarmIconIsGenerated": staticGlobal(alarmIconIsGenerated),
        "setAlarmCardType": staticGlobal(setAlarmCardType),
        "ALARM_CARD_METADATA": liveGlobal(() => ALARM_CARD_METADATA, (value?: any) => { ALARM_CARD_METADATA = value; }),
        "renderAlarmCardTypeField": staticGlobal(renderAlarmCardTypeField),
        "renderAlarmVisibleActionsField": staticGlobal(renderAlarmVisibleActionsField),
    };
}
