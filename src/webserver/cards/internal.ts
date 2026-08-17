import {
    cardContractAllowInSubpage,
    cardContractCardLabel,
    cardContractDefaultConfig,
    cardContractDomains,
    cardContractHidden,
    cardContractPickerKey,
} from "../generated/card_contract";
import { iconSlug } from "../application/ui_primitives";
import type { CardRegistry } from "../application/card_registry";
import type { ConfigInternalRelayOptionsFeature } from "../application/config_internal_relay_options";
import type { ControlsFieldsFeature } from "../application/controls_fields";

export function registerInternalCardTypes(
    registry: CardRegistry,
    internalRelayOptionsFeature: ConfigInternalRelayOptionsFeature,
    documentService: Document,
    fields: ControlsFieldsFeature,
): void {
    const { cardBadgeLabelHtml, condField } = fields;
    const {
        internalRelayDefaultIcon,
        internalRelayDefaultOnIcon,
        internalRelayLabelFor,
        internalRelayMode,
        internalRelayOptions,
        internalRelayUsesDefaultIcon,
        internalRelayUsesDefaultOnIcon,
    } = internalRelayOptionsFeature;
    // Internal relay card: controls built-in relay hardware locally on the device.
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
    const INTERNAL_CARD_METADATA: any = {
        mode: {
            label: "Type",
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
    registry.register("internal", {
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
            var controlsDisclosure: any = helpers.disclosureSection("Controls", helpers.idPrefix + "internal-controls", false);
            renderInternalRelayField(controlsDisclosure.section, b, helpers);
            panel.appendChild(controlsDisclosure.panel);
            var cardSettingsDisclosure: any = helpers.disclosureSection("Card Settings", helpers.idPrefix + "internal-card-settings", false);
            panel.appendChild(cardSettingsDisclosure.panel);
        },
        renderSettings: function (this: any, panel?: any, b?: any, slot?: any, helpers?: any) {
            ensureInternalRelaySelection(b);
            var controlsButton: any = panel.querySelector("#" + helpers.idPrefix + "internal-controls");
            var controls: any = controlsButton && controlsButton.nextElementSibling || panel;
            var mode: any = internalRelayMode(b);
            if (internalRelayUsesDefaultIcon(mode, b.icon))
                b.icon = internalRelayDefaultIcon(mode);
            if (mode === "switch" && internalRelayUsesDefaultOnIcon(b.icon_on)) {
                b.icon_on = internalRelayDefaultOnIcon();
            }
            var modeControl: any = helpers.renderCardSegmentControl(controls, b, helpers, {
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
                var section: any = helpers.renderCardIconPicker(documentService.createElement("div"), b, helpers, {
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
}
