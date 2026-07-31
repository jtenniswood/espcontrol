import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function registerSliderCardTypes(): GlobalDescriptors {
    // Slider and cover button types: draggable brightness/position control.
    // Factory creates both "slider" (light.turn_on w/ brightness) and "cover"
    // variants. Slider cards are always vertical. For covers, b.sensor stores
    // "modal", "", "tilt", "toggle", or a one-tap cover command.
    function coverCommandMode(this: any, mode?: any) {
        return mode === "open" || mode === "close" || mode === "stop" || mode === "set_position";
    }
    function coverModeOptionValues(this: any, allowCommands?: any) {
        var spec: any = cardContractOptionSpec("cover", "cover_mode");
        var values: any = spec && spec.values ? spec.values : ["modal", "", "tilt", "toggle", "open", "close", "stop", "set_position"];
        return values.filter(function (this: any, value?: any) {
            return allowCommands || !coverCommandMode(value);
        });
    }
    function normalizeCoverMode(this: any, mode?: any, allowCommands?: any) {
        mode = String(mode || "");
        return coverModeOptionValues(allowCommands).indexOf(mode) >= 0 ? mode : "";
    }
    function coverModeOptionsForSettings(this: any, currentMode?: any) {
        return [
            ["modal", "All Controls"],
            ["", "Slider: Position"],
            ["tilt", "Slider: Tilt"],
            ["toggle", "Toggle"],
            ["open", "Open"],
            ["close", "Close"],
            ["stop", "Stop"],
            ["set_position", "Set Position"],
        ];
    }
    function normalizeCoverPosition(this: any, value?: any) {
        var n: any = parseInt(value, 10);
        var spec: any = cardContractOptionSpec("cover", "cover_position") || {};
        var fallback: any = parseInt(spec.defaultValue, 10);
        var min: any = typeof spec.min === "number" ? spec.min : 0;
        var max: any = typeof spec.max === "number" ? spec.max : 100;
        if (!isFinite(fallback))
            fallback = 50;
        if (!isFinite(n))
            n = fallback;
        if (n < min)
            n = min;
        if (n > max)
            n = max;
        return String(n);
    }
    function renderCoverControlTabSettings(this: any, panel?: any, b?: any, helpers?: any) {
        renderModalTabSettings(panel, b, helpers, {
            definitions: coverControlTabDefinitions,
            tabs: coverControlTabs,
            normalizeOptions: normalizeCoverOptions,
            setTabs: setCoverControlTabs,
            idPrefix: "cover-tab-",
            hideHeading: true,
        });
    }
    function sliderCardMetadata(this: any, opts?: any) {
        return {
            entity: {
                label: "Entity",
                idSuffix: "entity",
                placeholder: opts.entityPlaceholder,
                domains: function (this: any) { return cardContractDomains(opts.type); },
                bindName: "entity",
                rerender: true,
                requiredMessage: "Add an entity before saving.",
            },
            labelField: {
                label: "Label",
                idSuffix: "label",
                field: "label",
                placeholder: opts.placeholder,
                rerender: true,
            },
            coverInteraction: {
                mode: {
                    label: "Type",
                    idSuffix: "cover-interaction",
                    options: function (this: any, b?: any) { return coverModeOptionsForSettings(normalizeCoverMode(b && b.sensor, true)); },
                    value: function (this: any, b?: any) {
                        return normalizeCoverMode(b.sensor, true);
                    },
                },
            },
            coverPosition: {
                label: "Position",
                idSuffix: "cover-position",
                min: 0,
                max: 100,
                step: 1,
                placeholder: "e.g. 50",
                value: function (this: any, b?: any) {
                    return normalizeCoverPosition(b.unit);
                },
            },
            preview: {
                badge: opts.badgeIcon,
            },
        };
    }
    function sliderTypeFactory(this: any, opts?: any) {
        var metadata: any = sliderCardMetadata(opts);
        return {
            label: function (this: any) { return cardContractCardLabel(opts.type); },
            allowInSubpage: function (this: any) { return cardContractAllowInSubpage(opts.type); },
            pickerKey: function (this: any) { return cardContractPickerKey(opts.type); },
            hidden: function (this: any) { return cardContractHidden(opts.type); },
            hideLabel: !!opts.hideLabel,
            labelPlaceholder: opts.placeholder,
            defaultConfig: function (this: any) { return cardContractDefaultConfig(opts.type); },
            cardMetadata: metadata,
            onSelect: function (this: any, b?: any) {
                b.sensor = opts.type === "cover" ? "modal" : "";
                b.unit = "";
                b.icon = opts.defaultIcon;
                b.icon_on = opts.defaultIconOn;
            },
            renderSettings: function (this: any, panel?: any, b?: any, slot?: any, helpers?: any) {
                var cardSettingsPanel: any = null;
                var modalSettingsPanel: any = null;
                var modalSettingsDisclosure: any = null;
                function labelField(this: any) {
                    helpers.renderCardTextField(panel, b, helpers, metadata.labelField);
                }
                if (opts.lightControlType)
                    renderLightControlTypeField(panel, b, helpers);
                var coverMode: any = "";
                var coverPositionField: any = null;
                var coverPositionInput: any = null;
                var singleIconSection: any = null;
                var offIconSection: any = null;
                var coverTabsSection: any = null;
                var syncCoverIconUi: any = function (this: any) { };
                var syncCoverUi: any = function (this: any) {
                    syncCoverControlTabs();
                    syncCoverIconUi();
                };
                function syncCoverControlTabs(this: any) {
                    if (!opts.coverControlTabs || !coverTabsSection)
                        return;
                    coverTabsSection.innerHTML = "";
                    if (coverMode === "modal") {
                        if (modalSettingsDisclosure)
                            modalSettingsDisclosure.style.display = "";
                        renderCoverControlTabSettings(coverTabsSection, b, helpers);
                        return;
                    }
                    if (modalSettingsDisclosure)
                        modalSettingsDisclosure.style.display = "none";
                    var previousOptions: any = b.options || "";
                    b.options = "";
                    if (b.options !== previousOptions)
                        helpers.saveField("options", b.options);
                }
                function syncIconSection(this: any, section?: any, value?: any) {
                    if (!section)
                        return;
                    var picker: any = section.querySelector(".sp-icon-picker");
                    if (picker && picker._setIcon) {
                        picker._setIcon(value);
                        return;
                    }
                    var preview: any = section.querySelector(".sp-icon-picker-preview");
                    if (preview)
                        preview.className = "sp-icon-picker-preview mdi mdi-" + iconSlug(value);
                    var input: any = section.querySelector(".sp-icon-picker-input");
                    if (input)
                        input.value = value;
                }
                function coverModeDefaultIcon(this: any, mode?: any) {
                    if (mode === "open")
                        return opts.defaultIconOn;
                    if (mode === "stop")
                        return "Stop";
                    return opts.defaultIcon;
                }
                function useCoverModeDefaultIcon(this: any) {
                    return opts.interactionMode && (!b.icon ||
                        b.icon === "Auto" ||
                        b.icon === opts.defaultIcon ||
                        b.icon === opts.defaultIconOn ||
                        b.icon === "Minus" ||
                        b.icon === "Stop");
                }
                function applyCoverModeDefaultIcon(this: any, mode?: any) {
                    if (!useCoverModeDefaultIcon())
                        return;
                    var icon: any = coverModeDefaultIcon(mode);
                    if (b.icon === icon)
                        return;
                    b.icon = icon;
                    helpers.saveField("icon", b.icon);
                    syncIconSection(singleIconSection, b.icon);
                    syncIconSection(offIconSection, b.icon);
                }
                if (opts.interactionMode) {
                    var storedCoverMode: any = normalizeCoverMode(b.sensor, true);
                    coverMode = storedCoverMode;
                    if (b.sensor !== storedCoverMode) {
                        b.sensor = storedCoverMode;
                        helpers.saveField("sensor", storedCoverMode);
                    }
                    if (storedCoverMode !== "set_position" && b.unit) {
                        b.unit = "";
                        helpers.saveField("unit", "");
                    }
                    if (coverCommandMode(storedCoverMode) && b.icon_on !== "Auto") {
                        b.icon_on = "Auto";
                        helpers.saveField("icon_on", "Auto");
                    }
                    if (coverCommandMode(storedCoverMode)) {
                        applyCoverModeDefaultIcon(storedCoverMode);
                    }
                    var interactionField: any = helpers.renderCardModeSelector(panel, b, helpers, {
                        mode: Object.assign({}, metadata.coverInteraction.mode, {
                            value: function (this: any) { return coverMode; },
                            onChange: function (this: any) { setCoverMode(this.value, true); },
                        }),
                    });
                    var interactionSelect: any = interactionField.select;
                    var positionControl: any = helpers.renderCardNumberField(panel, b, helpers, metadata.coverPosition);
                    coverPositionField = positionControl.field;
                    coverPositionInput = positionControl.input;
                    if (coverMode === "set_position" && b.unit !== coverPositionInput.value) {
                        b.unit = coverPositionInput.value;
                        helpers.saveField("unit", b.unit);
                    }
                    function setCoverPosition(this: any, value?: any) {
                        if (!coverPositionInput)
                            return;
                        var position: any = normalizeCoverPosition(value);
                        coverPositionInput.value = position;
                        b.unit = position;
                        helpers.saveField("unit", position);
                    }
                    function setCoverMode(this: any, mode?: any, persist?: any) {
                        coverMode = normalizeCoverMode(mode, true);
                        interactionSelect.value = coverMode;
                        if (coverMode === "set_position") {
                            setCoverPosition(b.unit);
                        }
                        else if (b.unit) {
                            b.unit = "";
                            helpers.saveField("unit", "");
                            coverPositionInput.value = "50";
                        }
                        if (coverCommandMode(coverMode)) {
                            b.icon_on = "Auto";
                            helpers.saveField("icon_on", "Auto");
                            applyCoverModeDefaultIcon(coverMode);
                        }
                        if (persist) {
                            b.sensor = coverMode;
                            helpers.saveField("sensor", coverMode);
                        }
                        else {
                            b.sensor = coverMode;
                        }
                        syncCoverUi();
                    }
                    interactionSelect.addEventListener("change", function (this: any) { setCoverMode(this.value, true); });
                    coverPositionInput.addEventListener("change", function (this: any) { setCoverPosition(this.value); });
                    coverPositionInput.addEventListener("blur", function (this: any) { setCoverPosition(this.value); });
                }
                if (opts.renderLabelInSettings && !opts.labelAfterEntity && !opts.coverControlTabs)
                    labelField();
                helpers.renderCardEntityField(panel, b, helpers, metadata);
                if (opts.coverControlTabs) {
                    cardSettingsPanel = document.createElement("div");
                    modalSettingsPanel = document.createElement("div");
                    panel.appendChild(inlineDisclosure("Card Settings", cardSettingsPanel, false));
                    modalSettingsDisclosure = inlineDisclosure("Modal Settings", modalSettingsPanel, b._modalSettingsOpen === true);
                    panel.appendChild(modalSettingsDisclosure);
                    panel = cardSettingsPanel;
                }
                if (opts.renderLabelInSettings && (opts.labelAfterEntity || opts.coverControlTabs))
                    labelField();
                if (opts.coverControlTabs) {
                    coverTabsSection = document.createElement("div");
                    modalSettingsPanel.appendChild(coverTabsSection);
                }
                function iconField(this: any, label?: any, inputSuffix?: any, field?: any, currentVal?: any, defaultVal?: any) {
                    var picker: any = helpers.renderCardIconPicker(panel, b, helpers, {
                        pickerIdSuffix: inputSuffix + "-picker",
                        idSuffix: inputSuffix,
                        field: field,
                        value: currentVal,
                        fallback: defaultVal,
                        label: label,
                    });
                    var iconPicker: any = picker.querySelector(".sp-icon-picker");
                    if (iconPicker && iconPicker._setIcon)
                        iconPicker._setIcon(currentVal);
                    return picker;
                }
                if (opts.alwaysShowIconPair) {
                    var offIconVal: any = b.icon && b.icon !== "Auto" ? b.icon : opts.defaultIcon;
                    var onIconDefault: any = opts.onIconInheritsOff ? offIconVal : opts.defaultIconOn;
                    var onIconVal: any = b.icon_on && b.icon_on !== "Auto" ? b.icon_on : onIconDefault;
                    singleIconSection = iconField("Icon", "cover-icon", "icon", offIconVal, opts.defaultIcon);
                    offIconSection = iconField(opts.iconOffFieldLabel || "Closed Icon", "icon", "icon", offIconVal, opts.defaultIcon);
                    var onIconSection: any = iconField(opts.iconOnFieldLabel || "Open Icon", "icon-on", "icon_on", onIconVal, opts.defaultIconOn);
                    syncCoverIconUi = function (this: any) {
                        var singleIcon: any = opts.interactionMode && coverCommandMode(coverMode);
                        singleIconSection.style.display = singleIcon ? "" : "none";
                        offIconSection.style.display = singleIcon ? "none" : "";
                        onIconSection.style.display = singleIcon ? "none" : "";
                        if (coverPositionField) {
                            coverPositionField.style.display = coverMode === "set_position" ? "" : "none";
                        }
                    };
                    syncCoverUi();
                }
                else {
                    helpers.renderCardIconPicker(panel, b, helpers, {
                        pickerIdSuffix: "icon-picker",
                        idSuffix: "icon",
                        field: "icon",
                        fallback: "Auto",
                        label: "Icon",
                    });
                }
                if (!opts.interactionMode && b.sensor) {
                    b.sensor = "";
                    helpers.saveField("sensor", "");
                }
                if (!opts.alwaysShowIconPair) {
                    var hasIconOn: any = b.icon_on && b.icon_on !== "Auto";
                    var iconOnToggleSection: any = helpers.toggleSection(opts.iconOnLabel, helpers.idPrefix + "iconon-toggle", hasIconOn);
                    var iconOnToggle: any = iconOnToggleSection.toggle;
                    var iconOnCond: any = iconOnToggleSection.section;
                    panel.appendChild(iconOnToggle.row);
                    if (hasIconOn)
                        iconOnCond.classList.add("sp-visible");
                    var iconOnVal: any = hasIconOn ? b.icon_on : "Auto";
                    var iconOnSection: any = helpers.renderCardIconPicker(iconOnCond, b, helpers, {
                        pickerIdSuffix: "icon-on-picker",
                        idSuffix: "icon-on",
                        field: "icon_on",
                        value: iconOnVal,
                        fallback: "Auto",
                        label: opts.iconOnFieldLabel,
                    });
                    var iconOnPicker: any = iconOnSection.querySelector(".sp-icon-picker");
                    panel.appendChild(iconOnCond);
                    iconOnToggle.input.addEventListener("change", function (this: any) {
                        if (this.checked) {
                            iconOnCond.classList.add("sp-visible");
                        }
                        else {
                            b.icon_on = "Auto";
                            helpers.saveField("icon_on", "Auto");
                            iconOnCond.classList.remove("sp-visible");
                            var ionPreview: any = iconOnPicker.querySelector(".sp-icon-picker-preview");
                            if (ionPreview)
                                ionPreview.className = "sp-icon-picker-preview mdi mdi-cog";
                            var ionInput: any = iconOnPicker.querySelector(".sp-icon-picker-input");
                            if (ionInput)
                                ionInput.value = "Auto";
                        }
                    });
                }
            },
            renderPreview: function (this: any, b?: any, helpers?: any) {
                var label: any = b.label || b.entity || opts.fallbackLabel;
                var iconName: any = b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : opts.fallbackIcon;
                if (opts.interactionMode && (b.sensor === "toggle" || coverCommandMode(b.sensor))) {
                    return {
                        iconHtml: '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>',
                        labelHtml: cardBadgeLabelHtml(helpers, label, metadata.preview.badge),
                    };
                }
                return {
                    iconHtml: '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>' +
                        '<span class="sp-slider-preview"><span class="sp-slider-track">' +
                        '<span class="sp-slider-fill"></span>' +
                        '</span></span>',
                    labelHtml: cardBadgeLabelHtml(helpers, label, metadata.preview.badge),
                };
            },
        };
    }
    registerButtonType("light_brightness", sliderTypeFactory({
        type: "light_brightness",
        placeholder: "e.g. Living Room",
        entityPlaceholder: "e.g. light.living_room",
        defaultIcon: "Lightbulb Outline",
        defaultIconOn: "Lightbulb",
        fallbackLabel: "Brightness",
        fallbackIcon: "lightbulb",
        badgeIcon: "tune-vertical-variant",
        alwaysShowIconPair: true,
        onIconInheritsOff: false,
        iconOffFieldLabel: "Off Icon",
        iconOnFieldLabel: "On Icon",
        hideLabel: true,
        renderLabelInSettings: true,
        labelAfterEntity: true,
        lightControlType: true,
    }));
    registerButtonType("slider", sliderTypeFactory({
        type: "slider",
        placeholder: "e.g. Living Room",
        entityPlaceholder: "e.g. light.living_room",
        defaultIcon: "Auto",
        defaultIconOn: "Auto",
        fallbackLabel: "Slider",
        fallbackIcon: "lightbulb",
        badgeIcon: "tune-vertical-variant",
        alwaysShowIconPair: true,
        onIconInheritsOff: true,
        iconOffFieldLabel: "Off Icon",
        iconOnFieldLabel: "On Icon",
    }));
    registerButtonType("cover", sliderTypeFactory({
        type: "cover",
        placeholder: "e.g. Office Blind",
        entityPlaceholder: "e.g. cover.office_blind",
        defaultIcon: "Blinds",
        defaultIconOn: "Blinds Open",
        fallbackLabel: "Cover",
        fallbackIcon: "blinds",
        badgeIcon: "blinds-horizontal",
        alwaysShowIconPair: true,
        hideLabel: true,
        renderLabelInSettings: true,
        interactionMode: true,
        coverControlTabs: true,
    }));
    return {
        "coverCommandMode": staticGlobal(coverCommandMode),
        "coverModeOptionValues": staticGlobal(coverModeOptionValues),
        "normalizeCoverMode": staticGlobal(normalizeCoverMode),
        "coverModeOptionsForSettings": staticGlobal(coverModeOptionsForSettings),
        "normalizeCoverPosition": staticGlobal(normalizeCoverPosition),
        "renderCoverControlTabSettings": staticGlobal(renderCoverControlTabSettings),
        "sliderCardMetadata": staticGlobal(sliderCardMetadata),
        "sliderTypeFactory": staticGlobal(sliderTypeFactory),
    };
}
