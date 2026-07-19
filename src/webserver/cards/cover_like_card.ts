import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function registerCoverLikeCardHelpers(): GlobalDescriptors {
    function coverLikeModeValues(this: any, cardType?: any, optionName?: any, fallbackModes?: any) {
        var spec: any = cardContractOptionSpec(cardType, optionName);
        return spec && spec.values ? spec.values.slice() : fallbackModes.map(function (this: any, entry?: any) { return entry[0]; });
    }
    function normalizeCoverLikeMode(this: any, mode?: any, values?: any) {
        mode = String(mode || "");
        return values.indexOf(mode) >= 0 ? mode : "";
    }
    function renderCoverLikeConfirmationSettings(this: any, panel?: any, b?: any, helpers?: any, confirmConfig?: any, isCommandMode?: any, commandDirection?: any) {
        var metadata: any = confirmConfig.metadata;
        var confirmOn: any = confirmConfig.enabled(b);
        var confirmMode: any = isCommandMode ? commandDirection : (confirmConfig.mode(b) || "off");
        var confirmToggle: any = helpers.renderCardOptionToggle(panel, b, helpers, metadata.confirmationToggle);
        var confirmSection: any = condField();
        if (confirmOn)
            confirmSection.classList.add("sp-visible");
        if (!isCommandMode) {
            helpers.renderCardSegmentControl(confirmSection, b, helpers, Object.assign({}, metadata.confirmationMode, {
                value: function (this: any) { return confirmMode; },
                onSelect: function (this: any, b?: any, helpers?: any, value?: any) {
                    var previousDefault: any = confirmConfig.defaultMessageForMode(confirmMode);
                    confirmMode = value;
                    if (!messageInput.value || messageInput.value === previousDefault) {
                        messageInput.value = confirmConfig.defaultMessageForMode(confirmMode);
                    }
                    saveConfirmationOptions();
                },
            }));
        }
        var messageField: any = helpers.renderCardTextField(confirmSection, b, helpers, metadata.confirmationMessage);
        var messageInput: any = messageField.input;
        messageInput.maxLength = 72;
        var yesField: any = helpers.renderCardTextField(confirmSection, b, helpers, metadata.confirmationYes);
        var yesInput: any = yesField.input;
        yesInput.maxLength = 20;
        var noField: any = helpers.renderCardTextField(confirmSection, b, helpers, metadata.confirmationNo);
        var noInput: any = noField.input;
        noInput.maxLength = 20;
        panel.appendChild(confirmSection);
        function saveConfirmationOptions(this: any) {
            confirmConfig.setOptions(b, confirmToggle.input.checked ? confirmMode : "", messageInput.value || confirmConfig.defaultMessageForMode(confirmMode), yesInput.value || SWITCH_CONFIRM_DEFAULT_YES, noInput.value || SWITCH_CONFIRM_DEFAULT_NO);
            helpers.saveField("options", b.options);
        }
        confirmToggle.input.addEventListener("change", function (this: any) {
            confirmSection.classList.toggle("sp-visible", this.checked);
            if (this.checked) {
                if (!messageInput.value)
                    messageInput.value = confirmConfig.defaultMessageForMode(confirmMode);
                if (!yesInput.value)
                    yesInput.value = SWITCH_CONFIRM_DEFAULT_YES;
                if (!noInput.value)
                    noInput.value = SWITCH_CONFIRM_DEFAULT_NO;
            }
            saveConfirmationOptions();
        });
        [messageInput, yesInput, noInput].forEach(function (this: any, input?: any) {
            input.addEventListener("input", saveConfirmationOptions);
            input.addEventListener("change", saveConfirmationOptions);
            input.addEventListener("blur", saveConfirmationOptions);
        });
    }
    function registerCoverLikeCardType(this: any, config?: any) {
        var metadata: any = config.metadata;
        function normalizeMode(this: any, mode?: any) {
            return normalizeCoverLikeMode(mode, coverLikeModeValues(config.type, config.optionName, metadata.mode.options));
        }
        function commandMode(this: any, mode?: any) {
            return config.commandModes.indexOf(normalizeMode(mode)) >= 0;
        }
        function commandPlaceholder(this: any, mode?: any) {
            return "e.g. " + config.defaultLabel(mode) + " " + config.shortLabel;
        }
        function syncModeFields(this: any, b?: any, helpers?: any, mode?: any) {
            b.unit = "";
            b.precision = "";
            var normalizedOptions: any = config.normalizeOptions(b.options, mode);
            if (b.options !== normalizedOptions) {
                b.options = normalizedOptions;
                helpers.saveField("options", normalizedOptions);
            }
            if (commandMode(mode) && b.icon_on !== "Auto") {
                b.icon_on = "Auto";
                helpers.saveField("icon_on", "Auto");
            }
            else if (!commandMode(mode) && (!b.icon_on || b.icon_on === "Auto")) {
                b.icon_on = config.openIcon;
                helpers.saveField("icon_on", config.openIcon);
            }
        }
        registerButtonType(config.type, {
            label: function (this: any) { return cardContractCardLabel(config.type); },
            allowInSubpage: function (this: any) { return cardContractAllowInSubpage(config.type); },
            pickerKey: function (this: any) { return cardContractPickerKey(config.type); },
            hidden: function (this: any) { return cardContractHidden(config.type); },
            hideLabel: true,
            defaultConfig: function (this: any) { return cardContractDefaultConfig(config.type); },
            cardMetadata: metadata,
            onSelect: function (this: any, b?: any) {
                b.label = "";
                b.sensor = "";
                b.unit = "";
                b.precision = "";
                b.icon = config.closedIcon;
                b.icon_on = config.openIcon;
                b.options = "";
            },
            renderSettings: function (this: any, panel?: any, b?: any, slot?: any, helpers?: any) {
                var mode: any = normalizeMode(b.sensor);
                if (b.sensor !== mode) {
                    b.sensor = mode;
                    helpers.saveField("sensor", mode);
                }
                syncModeFields(b, helpers, mode);
                helpers.renderCardModeSelector(panel, b, helpers, Object.assign({}, metadata, {
                    mode: Object.assign({}, metadata.mode, {
                        value: function (this: any) { return mode; },
                        onChange: function (this: any) {
                            var oldMode: any = mode;
                            var hadDefaultIcon: any = config.usesDefaultIcon(b.icon);
                            mode = normalizeMode(this.value);
                            b.sensor = mode;
                            helpers.saveField("sensor", mode);
                            b.unit = "";
                            b.precision = "";
                            helpers.saveField("unit", "");
                            helpers.saveField("precision", "");
                            b.options = config.normalizeOptions(b.options, mode);
                            helpers.saveField("options", b.options);
                            if (hadDefaultIcon || b.icon === config.defaultIcon(oldMode)) {
                                b.icon = config.defaultIcon(mode);
                                helpers.saveField("icon", b.icon);
                            }
                            b.icon_on = commandMode(mode) ? "Auto" : (b.icon_on && b.icon_on !== "Auto" ? b.icon_on : config.openIcon);
                            helpers.saveField("icon_on", b.icon_on);
                            renderButtonSettings();
                        },
                    }),
                }));
                var labelHost: any = document.createElement("div");
                var labelControl: any = helpers.renderCardTextField(labelHost, b, helpers, Object.assign({}, metadata.labelField, {
                    placeholder: commandMode(mode) ? commandPlaceholder(mode) : config.labelPlaceholder,
                }));
                function setLabelVisible(this: any, value?: any) {
                    labelControl.field.style.display = value === "label" ? "" : "none";
                }
                var labelMode: any = config.labelDisplayMode(b);
                helpers.renderCardSegmentControl(panel, b, helpers, {
                    segment: Object.assign({}, metadata.display, {
                        value: function (this: any) { return labelMode; },
                        onSelect: function (this: any, button?: any, cardHelpers?: any, value?: any) {
                            labelMode = value;
                            config.setLabelDisplayMode(button, value);
                            cardHelpers.saveField("options", button.options);
                            setLabelVisible(value);
                            scheduleRender();
                        },
                    }),
                });
                setLabelVisible(labelMode);
                panel.appendChild(labelControl.field);
                helpers.renderCardEntityField(panel, b, helpers, metadata);
                var closedIconVal: any = b.icon && b.icon !== "Auto" ? b.icon : config.closedIcon;
                var iconOnVal: any = b.icon_on && b.icon_on !== "Auto" ? b.icon_on : config.openIcon;
                if (commandMode(mode)) {
                    helpers.renderCardIconPicker(panel, b, helpers, {
                        pickerIdSuffix: "icon-picker",
                        idSuffix: "icon",
                        field: "icon",
                        value: b.icon && b.icon !== "Auto" ? b.icon : config.defaultIcon(mode),
                        fallback: config.defaultIcon(mode),
                        label: "Icon",
                    });
                }
                else {
                    helpers.renderCardIconPair(panel, b, helpers, {
                        pickerIdSuffix: "icon-picker",
                        idSuffix: "icon",
                        field: "icon",
                        value: closedIconVal,
                        fallback: config.closedIcon,
                        label: "Closed Icon",
                    }, {
                        pickerIdSuffix: "icon-on-picker",
                        idSuffix: "icon-on",
                        field: "icon_on",
                        value: iconOnVal,
                        fallback: config.openIcon,
                        label: "Open Icon",
                    });
                }
                if (config.confirmation) {
                    renderCoverLikeConfirmationSettings(panel, b, helpers, config.confirmation, commandMode(mode), mode === "open" ? "on" : "off");
                }
            },
            renderPreview: function (this: any, b?: any, helpers?: any) {
                var mode: any = normalizeMode(b.sensor);
                var label: any = b.label || (commandMode(mode) ? config.defaultLabel(mode) : b.entity || config.defaultCardLabel);
                if (config.labelDisplayMode(b) === "status")
                    label = config.statusLabel || "Closed";
                return cardBadgePreview(b, helpers, {
                    label: label,
                    iconFallback: config.defaultIcon(mode),
                    badge: metadata.preview.badge,
                });
            },
        });
    }
    return {
        "coverLikeModeValues": staticGlobal(coverLikeModeValues),
        "normalizeCoverLikeMode": staticGlobal(normalizeCoverLikeMode),
        "renderCoverLikeConfirmationSettings": staticGlobal(renderCoverLikeConfirmationSettings),
        "registerCoverLikeCardType": staticGlobal(registerCoverLikeCardType),
    };
}
