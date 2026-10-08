import {
    cardContractAllowInSubpage,
    cardContractCard,
    cardContractCardLabel,
    cardContractDefaultConfig,
    cardContractDomains,
    cardContractHidden,
    cardContractPickerKey,
} from "../generated/card_contract";
import type { CardRegistry } from "../application/card_registry";
import type { ConfigConfirmationOptionsFeature } from "../application/config_confirmation_options";
import type { LightCardRegistration } from "./light_temperature";
import type { ControlsFieldsFeature } from "../application/controls_fields";
import {
    SWITCH_CONFIRM_DEFAULT_MESSAGE,
    SWITCH_CONFIRM_DEFAULT_NO,
    SWITCH_CONFIRM_DEFAULT_YES,
} from "../application/config_option_core";
export function registerSwitchCardTypes(
    registry: CardRegistry,
    confirmationOptions: ConfigConfirmationOptionsFeature,
    lightCards: LightCardRegistration,
    fields: ControlsFieldsFeature,
): void {
    const { cardBadgeLabelHtml, cardLargeNumbersActiveForCardSize, cardSensorPreviewHtml, condField } = fields;
    const {
        controlTypeMetadata: LIGHT_CONTROL_TYPE_METADATA,
        renderControlTypeField: renderLightControlTypeField,
    } = lightCards;
    const {
        setSwitchConfirmationOptions,
        switchConfirmationDefaultMessageForMode,
        switchConfirmationEnabled,
        switchConfirmationMessage,
        switchConfirmationMode,
        switchConfirmationNoText,
        switchConfirmationYesText,
    } = confirmationOptions;
    // Default button type: HA entity toggle (on/off switch)
    const SWITCH_CARD_METADATA: any = {
        entity: {
            label: "Entity",
            placeholder: "e.g. light.kitchen",
            domains: function (this: any) { return cardContractDomains(""); },
            requiredMessage: "Add an entity before saving.",
        },
        labelField: {
            label: "Label",
            placeholder: "e.g. Kitchen",
        },
        iconOff: {
            field: "icon",
            fallback: "Auto",
            label: "Off Icon",
        },
        iconOn: {
            field: "icon_on",
            fallback: "Auto",
            label: "On Icon",
        },
        activeDisplay: {
            label: "Active Display",
            idSuffix: "sensor-when-on-toggle",
            checked: function (this: any, b?: any) { return !!b.sensor; },
        },
        largeNumbers: {
            label: "Large Active Display Numbers",
            idSuffix: "large-active-display-numbers",
            supported: function (this: any, b?: any) {
                return !!(b && b.sensor && b.precision !== "text");
            },
        },
        sensorMode: {
            label: "Type",
            options: [
                ["numeric", "Numeric"],
                ["text", "Text"],
            ],
        },
        sensorEntity: {
            label: "Sensor Entity",
            idSuffix: "sensor",
            placeholder: "e.g. sensor.printer_percent_complete",
            domains: ["sensor", "binary_sensor", "text_sensor"],
            bindName: "sensor",
        },
        unitField: {
            label: "Unit",
            idSuffix: "unit",
            placeholder: "e.g. %",
            bindName: "unit",
            rerender: false,
        },
        confirmationToggle: {
            label: "Confirmation Required",
            idSuffix: "confirm-toggle",
            checked: function (this: any, b?: any) { return switchConfirmationEnabled(b); },
        },
        confirmationMode: {
            label: "When",
            options: [
                ["off", "Off"],
                ["on", "On"],
                ["both", "Both"],
            ],
        },
        confirmationMessage: {
            label: "Message",
            idSuffix: "confirm-message",
            placeholder: SWITCH_CONFIRM_DEFAULT_MESSAGE,
            bindName: null,
            value: function (this: any, b?: any) { return switchConfirmationMessage(b); },
        },
        confirmationYes: {
            label: "Confirm Button",
            idSuffix: "confirm-yes",
            placeholder: SWITCH_CONFIRM_DEFAULT_YES,
            bindName: null,
            value: function (this: any, b?: any) { return switchConfirmationYesText(b); },
        },
        confirmationNo: {
            label: "Cancel Button",
            idSuffix: "confirm-no",
            placeholder: SWITCH_CONFIRM_DEFAULT_NO,
            bindName: null,
            value: function (this: any, b?: any) { return switchConfirmationNoText(b); },
        },
        preview: {
            switchBadge: "toggle-switch-variant-off",
            numericBadge: "gauge",
            textBadge: "format-text",
        },
    };
    const LIGHT_SWITCH_CARD_METADATA: any = {
        mode: LIGHT_CONTROL_TYPE_METADATA.mode,
        entity: {
            label: "Entity",
            placeholder: "e.g. light.living_room",
            domains: function (this: any) { return cardContractDomains("light_switch"); },
            requiredMessage: "Add a light entity before saving.",
        },
        labelField: {
            label: "Label",
            placeholder: "e.g. Living Room",
        },
        iconOff: {
            field: "icon",
            fallback: "Auto",
            label: "Off Icon",
        },
        iconOn: {
            field: "icon_on",
            fallback: "Auto",
            label: "On Icon",
        },
        preview: {
            badge: "lightbulb",
        },
    };
    registry.register("", {
        label: function (this: any) { return cardContractCardLabel(""); },
        allowInSubpage: function (this: any) { return cardContractAllowInSubpage(""); },
        pickerKey: function (this: any) { return cardContractPickerKey(""); },
        hidden: function (this: any) { return cardContractHidden(""); },
        hideLabel: true,
        defaultConfig: function (this: any) { return cardContractDefaultConfig(""); },
        cardMetadata: SWITCH_CARD_METADATA,
        renderSettings: function (this: any, panel?: any, b?: any, slot?: any, helpers?: any) {
            var showSensor: any = !!b.sensor;
            var sensorMode: any = b.precision === "text" ? "text" : "numeric";
            helpers.renderCardEntityField(panel, b, helpers, SWITCH_CARD_METADATA);
            var cardSettingsDisclosure: any = helpers.disclosureSection("Card Settings", helpers.idPrefix + "switch-card-settings", false);
            var cardSettings: any = cardSettingsDisclosure.section;
            helpers.renderBasicCardFields(cardSettings, b, helpers, SWITCH_CARD_METADATA, {
                entity: false,
            });
            var sensorToggle: any = helpers.renderCardOptionToggle(cardSettings, b, helpers, SWITCH_CARD_METADATA.activeDisplay);
            var sensorSection: any = condField();
            if (showSensor)
                sensorSection.classList.add("sp-visible");
            var mode: any = helpers.renderCardSegmentControl(sensorSection, b, helpers, Object.assign({}, SWITCH_CARD_METADATA.sensorMode, {
                value: function (this: any) { return sensorMode; },
                onSelect: function (this: any, b?: any, helpers?: any, value?: any) { setSensorMode(value, true); },
            }));
            var numericBtn: any = mode.buttons.numeric;
            var textBtn: any = mode.buttons.text;
            var sensorField: any = helpers.renderCardEntityField(sensorSection, b, helpers, {
                entity: SWITCH_CARD_METADATA.sensorEntity,
            });
            var sensorInp: any = sensorField.input;
            var numericSection: any = condField();
            var unitField: any = helpers.renderCardTextField(numericSection, b, helpers, SWITCH_CARD_METADATA.unitField);
            var unitInp: any = unitField.input;
            var precisionField: any = helpers.precisionField(helpers.idPrefix + "precision", sensorMode === "numeric" ? (b.precision || "0") : "0", function (this: any) {
                b.precision = this.value === "0" ? "" : this.value;
                helpers.saveField("precision", b.precision);
            });
            var precisionSelect: any = precisionField.select;
            numericSection.appendChild(precisionField.field);
            helpers.renderCardLargeNumbersToggle(numericSection, b, helpers, SWITCH_CARD_METADATA);
            sensorSection.appendChild(numericSection);
            cardSettings.appendChild(sensorSection);
            function setSensorMode(this: any, mode?: any, persist?: any) {
                sensorMode = mode;
                numericBtn.classList.toggle("active", mode === "numeric");
                textBtn.classList.toggle("active", mode === "text");
                numericSection.classList.toggle("sp-visible", mode === "numeric");
                if (!persist)
                    return;
                if (mode === "text") {
                    b.precision = "text";
                    b.unit = "";
                    unitInp.value = "";
                    helpers.saveField("precision", "text");
                    helpers.saveField("unit", "");
                }
                else {
                    b.precision = "";
                    helpers.saveField("precision", "");
                    precisionSelect.value = "0";
                }
            }
            setSensorMode(sensorMode, false);
            sensorToggle.input.addEventListener("change", function (this: any) {
                showSensor = this.checked;
                sensorSection.classList.toggle("sp-visible", showSensor);
                helpers.saveField("sensor", b.sensor || "");
                if (showSensor) {
                    setSensorMode(sensorMode, true);
                    return;
                }
                b.sensor = "";
                b.unit = "";
                b.precision = "";
                sensorInp.value = "";
                unitInp.value = "";
                helpers.saveField("sensor", "");
                helpers.saveField("unit", "");
                helpers.saveField("precision", "");
                setSensorMode("numeric", false);
            });
            var confirmOn: any = switchConfirmationEnabled(b);
            var confirmMode: any = switchConfirmationMode(b) || "off";
            var confirmToggle: any = helpers.renderCardOptionToggle(cardSettings, b, helpers, SWITCH_CARD_METADATA.confirmationToggle);
            var confirmSection: any = condField();
            if (confirmOn)
                confirmSection.classList.add("sp-visible");
            helpers.renderCardSegmentControl(confirmSection, b, helpers, Object.assign({}, SWITCH_CARD_METADATA.confirmationMode, {
                value: function (this: any) { return confirmMode; },
                onSelect: function (this: any, b?: any, helpers?: any, value?: any) {
                    var previousDefault: any = switchConfirmationDefaultMessageForMode(confirmMode);
                    confirmMode = value;
                    if (!messageInput.value || messageInput.value === previousDefault) {
                        messageInput.value = switchConfirmationDefaultMessageForMode(confirmMode);
                    }
                    saveConfirmationOptions();
                },
            }));
            var messageField: any = helpers.renderCardTextField(confirmSection, b, helpers, SWITCH_CARD_METADATA.confirmationMessage);
            var messageInput: any = messageField.input;
            messageInput.maxLength = 72;
            var yesField: any = helpers.renderCardTextField(confirmSection, b, helpers, SWITCH_CARD_METADATA.confirmationYes);
            var yesInput: any = yesField.input;
            yesInput.maxLength = 20;
            var noField: any = helpers.renderCardTextField(confirmSection, b, helpers, SWITCH_CARD_METADATA.confirmationNo);
            var noInput: any = noField.input;
            noInput.maxLength = 20;
            cardSettings.appendChild(confirmSection);
            panel.appendChild(cardSettingsDisclosure.panel);
            function saveConfirmationOptions(this: any) {
                setSwitchConfirmationOptions(b, confirmToggle.input.checked ? confirmMode : "", messageInput.value || switchConfirmationDefaultMessageForMode(confirmMode), yesInput.value || SWITCH_CONFIRM_DEFAULT_YES, noInput.value || SWITCH_CONFIRM_DEFAULT_NO);
                helpers.saveField("options", b.options);
            }
            confirmToggle.input.addEventListener("change", function (this: any) {
                confirmSection.classList.toggle("sp-visible", this.checked);
                if (this.checked) {
                    if (!messageInput.value)
                        messageInput.value = switchConfirmationDefaultMessageForMode(confirmMode);
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
        },
        renderPreview: function (this: any, b?: any, helpers?: any) {
            var label: any = b.label || b.entity || "Configure";
            var badgeIcon: any = b.sensor
                ? (b.precision === "text" ? SWITCH_CARD_METADATA.preview.textBadge : SWITCH_CARD_METADATA.preview.numericBadge)
                : SWITCH_CARD_METADATA.preview.switchBadge;
            var preview: any = {
                labelHtml: cardBadgeLabelHtml(helpers, label, badgeIcon),
            };
            if (b.sensor && b.precision !== "text" &&
                cardLargeNumbersActiveForCardSize(b, helpers, SWITCH_CARD_METADATA)) {
                preview.iconHtml = cardSensorPreviewHtml(b, helpers, "42", b.unit || "");
            }
            return preview;
        },
    });
    registry.register("light_switch", {
        label: function (this: any) { return cardContractCardLabel("light_switch"); },
        allowInSubpage: function (this: any) { return cardContractAllowInSubpage("light_switch"); },
        hideLabel: true,
        pickerKey: function (this: any) { return cardContractPickerKey("light_switch"); },
        hidden: function (this: any) { return cardContractHidden("light_switch"); },
        defaultConfig: function (this: any) { return cardContractDefaultConfig("light_switch"); },
        isAvailable: function (this: any) {
            return false;
        },
        labelPlaceholder: "e.g. Living Room",
        cardMetadata: LIGHT_SWITCH_CARD_METADATA,
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
            helpers.renderBasicCardFields(panel, b, helpers, LIGHT_SWITCH_CARD_METADATA);
        },
        renderPreview: function (this: any, b?: any, helpers?: any) {
            var label: any = b.label || b.entity || "Configure";
            return {
                labelHtml: cardBadgeLabelHtml(helpers, label, LIGHT_SWITCH_CARD_METADATA.preview.badge),
            };
        },
    });
}
