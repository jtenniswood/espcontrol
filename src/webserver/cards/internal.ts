import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function registerInternalCardTypes(): GlobalDescriptors {
    // Internal relay card: controls built-in relay hardware locally on the device.
    function internalRelayOptions(this: any) {
        return (CFG.features && CFG.features.internalRelays) || [];
    }
    function internalRelaySpec(this: any) {
        var card: any = cardContractCard("internal");
        return card && card.behavior && card.behavior.internalRelay || {};
    }
    function internalRelayModeOptionValues(this: any) {
        var spec: any = cardContractOptionSpec("internal", "internal_mode");
        return spec && spec.values ? spec.values.slice() : ["switch", "push"];
    }
    function normalizeInternalRelayMode(this: any, mode?: any) {
        mode = String(mode || "");
        return internalRelayModeOptionValues().indexOf(mode) >= 0 ? mode : "switch";
    }
    function internalRelayDefaultIcon(this: any, mode?: any) {
        var icons: any = internalRelaySpec().defaultIcons || {};
        return icons[normalizeInternalRelayMode(mode)] || (mode === "push" ? "Gesture Tap" : "Lightbulb Outline");
    }
    function internalRelayDefaultOnIcon(this: any) {
        return internalRelaySpec().defaultIconOn || "Lightbulb";
    }
    function internalRelayUsesDefaultIcon(this: any, mode?: any, icon?: any) {
        if (!icon || icon === "Auto" || icon === internalRelayDefaultIcon(mode))
            return true;
        return mode === "switch" && icon === "Power Plug";
    }
    function internalRelayUsesDefaultOnIcon(this: any, icon?: any) {
        return !icon || icon === "Auto" || icon === internalRelayDefaultOnIcon() || icon === "Power";
    }
    function internalRelayMode(this: any, b?: any) {
        return normalizeInternalRelayMode(b && b.sensor === "push" ? "push" : "switch");
    }
    function internalRelayLabelFor(this: any, key?: any) {
        var relays: any = internalRelayOptions();
        for (var i: any = 0; i < relays.length; i++) {
            if (relays[i].key === key)
                return relays[i].label;
        }
        return key ? key.replace(/_/g, " ").replace(/\b\w/g, function (this: any, ch?: any) { return ch.toUpperCase(); }) : "Relay";
    }
    function ensureInternalRelaySelection(this: any, b?: any) {
        var relays: any = internalRelayOptions();
        if (!relays.length)
            return;
        for (var i: any = 0; i < relays.length; i++) {
            if (relays[i].key === b.entity)
                return;
        }
        b.entity = relays[0].key;
    }
    function renderInternalRelayField(this: any, panel?: any, b?: any, helpers?: any) {
        ensureInternalRelaySelection(b);
        var relays: any = internalRelayOptions();
        var relayField: any = helpers.selectField("Internal Relay", helpers.idPrefix + "internal-relay", relays.length ? relays.map(function (this: any, relay?: any) {
            return { value: relay.key, label: relay.label };
        }) : [["", "No relays"]], relays.length ? b.entity : "");
        var relaySelect: any = relayField.select;
        relaySelect.disabled = !relays.length;
        relaySelect.addEventListener("change", function (this: any) {
            b.entity = this.value;
            helpers.saveField("entity", b.entity);
        });
        panel.appendChild(relayField.field);
    }
    var INTERNAL_CARD_METADATA: any = {
        mode: {
            label: "Mode",
            inputId: "internal-mode",
            options: [
                ["switch", "Switch"],
                ["push", "Push Button"],
            ],
            value: internalRelayMode,
        },
        labelField: {
            label: "Label",
            idSuffix: "label",
            field: "label",
            placeholder: "e.g. Porch Light",
            rerender: true,
        },
        preview: {
            switchBadge: "power-plug",
            pushBadge: "gesture-tap",
        },
    };
    registerButtonType("internal", {
        label: function (this: any) { return cardContractCardLabel("internal"); },
        allowInSubpage: function (this: any) { return cardContractAllowInSubpage("internal"); },
        pickerKey: function (this: any) { return cardContractPickerKey("internal"); },
        hidden: function (this: any) { return cardContractHidden("internal"); },
        hideLabel: true,
        labelPlaceholder: "e.g. Porch Light",
        defaultConfig: function (this: any) { return cardContractDefaultConfig("internal"); },
        cardMetadata: INTERNAL_CARD_METADATA,
        isAvailable: function (this: any) {
            return internalRelayOptions().length > 0;
        },
        onSelect: function (this: any, b?: any) {
            var defaults: any = cardContractDefaultConfig("internal");
            Object.keys(defaults).forEach(function (this: any, key?: any) { b[key] = defaults[key]; });
            ensureInternalRelaySelection(b);
        },
        renderSettingsBeforeLabel: function (this: any, panel?: any, b?: any, slot?: any, helpers?: any) {
            renderInternalRelayField(panel, b, helpers);
        },
        renderSettings: function (this: any, panel?: any, b?: any, slot?: any, helpers?: any) {
            ensureInternalRelaySelection(b);
            var mode: any = internalRelayMode(b);
            if (internalRelayUsesDefaultIcon(mode, b.icon))
                b.icon = internalRelayDefaultIcon(mode);
            if (mode === "switch" && internalRelayUsesDefaultOnIcon(b.icon_on)) {
                b.icon_on = internalRelayDefaultOnIcon();
            }
            var modeControl: any = helpers.renderCardSegmentControl(panel, b, helpers, {
                segment: Object.assign({}, INTERNAL_CARD_METADATA.mode, {
                    inputId: helpers.idPrefix + "internal-mode",
                    value: function (this: any) { return mode; },
                    onSelect: function (this: any, button?: any, cardHelpers?: any, value?: any) { setMode(value); },
                }),
            });
            var switchBtn: any = modeControl.buttons.switch;
            var pushBtn: any = modeControl.buttons.push;
            helpers.renderCardTextField(panel, b, helpers, INTERNAL_CARD_METADATA.labelField);
            function makeLabeledIconPicker(this: any, label?: any, inputSuffix?: any, pickerSuffix?: any, value?: any, onSelect?: any) {
                var section: any = helpers.renderCardIconPicker(document.createElement("div"), b, helpers, {
                    pickerIdSuffix: pickerSuffix,
                    idSuffix: inputSuffix,
                    field: inputSuffix === "icon-on" ? "icon_on" : "icon",
                    value: value,
                    fallback: value || "Auto",
                    label: label,
                    onChange: function (this: any, button?: any, cardHelpers?: any, nextValue?: any) {
                        onSelect(nextValue);
                    },
                });
                var picker: any = section.querySelector(".sp-icon-picker");
                return { section: section, picker: picker };
            }
            function syncPicker(this: any, picker?: any, value?: any) {
                if (!picker)
                    return;
                var preview: any = picker.querySelector(".sp-icon-picker-preview");
                if (preview)
                    preview.className = "sp-icon-picker-preview mdi mdi-" + iconSlug(value);
                var input: any = picker.querySelector(".sp-icon-picker-input");
                if (input)
                    input.value = value;
            }
            var switchIconCond: any = condField();
            var pushIconCond: any = condField();
            panel.appendChild(switchIconCond);
            panel.appendChild(pushIconCond);
            var onIcon: any = makeLabeledIconPicker("On Icon", "icon-on", "icon-on-picker", b.icon_on || internalRelayDefaultOnIcon(), function (this: any, opt?: any) {
                b.icon_on = opt;
                helpers.saveField("icon_on", opt);
            });
            var offIcon: any = makeLabeledIconPicker("Off Icon", "icon-off", "icon-off-picker", b.icon || internalRelayDefaultIcon("switch"), function (this: any, opt?: any) {
                syncIcon(opt);
            });
            var pushIcon: any = makeLabeledIconPicker("Icon", "icon", "icon-picker", b.icon || internalRelayDefaultIcon("push"), function (this: any, opt?: any) {
                syncIcon(opt);
            });
            switchIconCond.appendChild(onIcon.section);
            switchIconCond.appendChild(offIcon.section);
            pushIconCond.appendChild(pushIcon.section);
            function syncIcon(this: any, value?: any) {
                b.icon = value;
                helpers.saveField("icon", value);
                syncPicker(offIcon.picker, value);
                syncPicker(pushIcon.picker, value);
            }
            function syncOnIcon(this: any, value?: any) {
                b.icon_on = value;
                helpers.saveField("icon_on", value);
                syncPicker(onIcon.picker, value);
            }
            function syncModeUi(this: any) {
                switchBtn.classList.toggle("active", mode === "switch");
                pushBtn.classList.toggle("active", mode === "push");
                switchIconCond.classList.toggle("sp-visible", mode === "switch");
                pushIconCond.classList.toggle("sp-visible", mode === "push");
            }
            function setMode(this: any, nextMode?: any) {
                if (mode === nextMode)
                    return;
                var wasDefaultIcon: any = internalRelayUsesDefaultIcon(mode, b.icon);
                mode = nextMode;
                b.sensor = mode === "push" ? "push" : "";
                helpers.saveField("sensor", b.sensor);
                if (wasDefaultIcon) {
                    syncIcon(internalRelayDefaultIcon(mode));
                }
                if (mode === "push") {
                    syncOnIcon("Auto");
                }
                else if (!b.icon_on || b.icon_on === "Auto") {
                    syncOnIcon(internalRelayDefaultOnIcon());
                }
                syncModeUi();
            }
            syncModeUi();
        },
        renderPreview: function (this: any, b?: any, helpers?: any) {
            var mode: any = internalRelayMode(b);
            var label: any = b.label || internalRelayLabelFor(b.entity);
            var iconName: any = b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : iconSlug(internalRelayDefaultIcon(mode));
            var badge: any = mode === "push" ? INTERNAL_CARD_METADATA.preview.pushBadge : INTERNAL_CARD_METADATA.preview.switchBadge;
            return {
                iconHtml: '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>',
                labelHtml: cardBadgeLabelHtml(helpers, label, badge),
            };
        },
    });
    return {
        "internalRelayOptions": staticGlobal(internalRelayOptions),
        "internalRelaySpec": staticGlobal(internalRelaySpec),
        "internalRelayModeOptionValues": staticGlobal(internalRelayModeOptionValues),
        "normalizeInternalRelayMode": staticGlobal(normalizeInternalRelayMode),
        "internalRelayDefaultIcon": staticGlobal(internalRelayDefaultIcon),
        "internalRelayDefaultOnIcon": staticGlobal(internalRelayDefaultOnIcon),
        "internalRelayUsesDefaultIcon": staticGlobal(internalRelayUsesDefaultIcon),
        "internalRelayUsesDefaultOnIcon": staticGlobal(internalRelayUsesDefaultOnIcon),
        "internalRelayMode": staticGlobal(internalRelayMode),
        "internalRelayLabelFor": staticGlobal(internalRelayLabelFor),
        "ensureInternalRelaySelection": staticGlobal(ensureInternalRelaySelection),
        "renderInternalRelayField": staticGlobal(renderInternalRelayField),
        "INTERNAL_CARD_METADATA": liveGlobal(() => INTERNAL_CARD_METADATA, (value?: any) => { INTERNAL_CARD_METADATA = value; }),
    };
}
