import { configOptionValue, setConfigOptionValue } from "../model/config_primitives";
import {
    cardContractAllowInSubpage,
    cardContractCard,
    cardContractCardLabel,
    cardContractDefaultConfig,
    cardContractDomains,
    cardContractHidden,
    cardContractPickerKey,
} from "../generated/card_contract";
import { escHtml, iconSlug } from "../application/ui_primitives";
import { WEB_UI_COLORS } from "../state/ui_tokens";
import type { CardRegistry, CardUiServices } from "../application/card_registry";
import type { ConfigImageOptionsFeature } from "../application/config_image_options";
import type { ControlsFieldsFeature } from "../application/controls_fields";
export function registerImageCardTypes(
    registry: CardRegistry,
    imageOptions: ConfigImageOptionsFeature,
    fields: ControlsFieldsFeature,
    cardUi: CardUiServices,
): void {
    const { renderPreview } = cardUi;
    const { toggleRow } = fields;
    const {
        imageModalMode,
        imageLabelEnabled,
        imageIconEnabled,
        normalizeImageOptions,
        validImageRefreshTrigger,
        setImageLabelEnabled,
        setImageIconEnabled,
        setImageModalMode,
    } = imageOptions;
    // Read-only Home Assistant camera/image entity card.
    const IMAGE_CARD_METADATA: any = {
        entity: {
            label: "Camera Entity",
            idSuffix: "entity",
            placeholder: "e.g. camera.front_door",
            domains: function (this: any) { return cardContractDomains("image"); },
            bindName: "entity",
            rerender: true,
            requiredMessage: "Add a camera entity before saving.",
        },
    };
    function imageModalModeOptions(this: any) {
        return [
            ["fill", "Crop to fit"],
            ["fit", "Show full image"],
        ];
    }
    function renderImageLabelSettings(this: any, panel?: any, b?: any, helpers?: any) {
        var iconToggle: any = helpers.toggleRow("Show Icon", helpers.idPrefix + "image-icon-toggle", imageIconEnabled(b));
        panel.appendChild(iconToggle.row);
        if (imageIconEnabled(b) && (!b.icon || b.icon === "Auto"))
            b.icon = "Camera";
        var iconField: any = helpers.renderCardIconPicker(panel, b, helpers, {
            label: "Icon",
            idSuffix: "image-icon",
            pickerIdSuffix: "image-icon-picker",
            fallback: "Camera",
            value: function (this: any) { return b.icon && b.icon !== "Auto" ? b.icon : "Camera"; },
            onChange: function (this: any) { renderPreview(); },
        });
        iconField.classList.add("sp-cond-field");
        var labelToggle: any = helpers.toggleRow("Show Label", helpers.idPrefix + "image-label-toggle", imageLabelEnabled(b));
        panel.appendChild(labelToggle.row);
        var labelField: any = helpers.renderCardTextField(panel, b, helpers, {
            text: {
                label: "Label",
                idSuffix: "image-label",
                placeholder: "Uses entity name when blank",
                bindName: "label",
                rerender: true,
            },
        });
        function syncLabelField(this: any) {
            labelField.field.hidden = !imageLabelEnabled(b);
        }
        function syncIconField(this: any) {
            iconField.classList.toggle("sp-visible", imageIconEnabled(b));
        }
        labelToggle.input.addEventListener("change", function (this: any) {
            setImageLabelEnabled(b, this.checked);
            helpers.saveField("options", b.options);
            helpers.saveField("label", b.label);
            syncLabelField();
            renderPreview();
        });
        iconToggle.input.addEventListener("change", function (this: any) {
            setImageIconEnabled(b, this.checked);
            if (this.checked && (!b.icon || b.icon === "Auto")) {
                b.icon = "Camera";
                helpers.saveField("icon", b.icon);
            }
            else if (!this.checked) {
                b.icon = "Auto";
                helpers.saveField("icon", b.icon);
            }
            helpers.saveField("options", b.options);
            syncIconField();
            renderPreview();
        });
        syncLabelField();
        syncIconField();
    }
    function renderImageModalSettings(this: any, panel?: any, b?: any, helpers?: any, entityInput?: any) {
        var modeField: any = helpers.selectField("Expanded Image", helpers.idPrefix + "image-modal-mode", imageModalModeOptions(), imageModalMode(b));
        panel.appendChild(modeField.field);
        modeField.select.addEventListener("change", function (this: any) {
            setImageModalMode(b, this.value);
            helpers.saveField("options", b.options);
        });
        const isCamera = () => String(b.entity || "").startsWith("camera.");
        const refresh = helpers.selectField("Expanded view refresh", helpers.idPrefix + "image-refresh-mode", [
            ["off", "Off"], ["periodic", "Periodic"], ["activity", "On activity"],
        ], configOptionValue(b.options, "image_modal_refresh_mode") || "off");
        const interval = helpers.selectField("Refresh interval", helpers.idPrefix + "image-refresh-interval", [
            ["5", "5 seconds"], ["10", "10 seconds"], ["30", "30 seconds"],
        ], configOptionValue(b.options, "image_modal_refresh_interval") || "10");
        const trigger = helpers.entityField("Trigger entity", helpers.idPrefix + "image-refresh-trigger",
            configOptionValue(b.options, "image_modal_refresh_trigger"),
            "e.g. binary_sensor.front_door_motion", ["binary_sensor", "event"]);
        const help = document.createElement("p");
        help.className = "sp-setting-note";
        help.textContent = "While expanded, activity refreshes the image every 5 seconds for 30 seconds. Further activations extend the window. The return-home timeout still applies.";
        panel.appendChild(refresh.field);
        panel.appendChild(interval.field);
        panel.appendChild(trigger.field);
        panel.appendChild(help);
        helpers.requireField(trigger.input, "Choose a binary sensor or event entity for activity refresh.",
            () => isCamera() && refresh.select.value === "activity", (value: string) => validImageRefreshTrigger(value.trim()));
        function syncVisibility() {
            refresh.field.hidden = !isCamera();
            interval.field.hidden = !isCamera() || refresh.select.value !== "periodic";
            trigger.field.hidden = help.hidden = !isCamera() || refresh.select.value !== "activity";
        }
        function syncRefreshSettings() {
            syncVisibility();
            let options = setConfigOptionValue(b.options, "image_modal_refresh_mode", refresh.select.value);
            options = setConfigOptionValue(options, "image_modal_refresh_interval", interval.select.value);
            options = setConfigOptionValue(options, "image_modal_refresh_trigger", trigger.input.value.trim());
            b.options = normalizeImageOptions(options, b.entity, true);
            helpers.saveField("options", b.options);
        }
        refresh.select.addEventListener("change", syncRefreshSettings);
        interval.select.addEventListener("change", syncRefreshSettings);
        trigger.input.addEventListener("input", syncRefreshSettings);
        trigger.input.addEventListener("change", syncRefreshSettings);
        entityInput.addEventListener("input", syncRefreshSettings);
        entityInput.addEventListener("change", syncRefreshSettings);
        syncVisibility();
    }
    registry.register("image", {
        label: function (this: any) { return cardContractCardLabel("image"); },
        allowInSubpage: function (this: any) { return cardContractAllowInSubpage("image"); },
        pickerKey: function (this: any) { return cardContractPickerKey("image"); },
        hidden: function (this: any) { return cardContractHidden("image"); },
        hideLabel: true,
        defaultConfig: function (this: any) { return cardContractDefaultConfig("image"); },
        cardMetadata: IMAGE_CARD_METADATA,
        onSelect: function (this: any, b?: any) {
            b.label = "";
            b.icon = "Auto";
            b.icon_on = "Auto";
            b.sensor = "";
            b.unit = "";
            b.precision = "";
            b.options = normalizeImageOptions(b.options, b.entity, true);
        },
        renderSettings: function (this: any, panel?: any, b?: any, slot?: any, helpers?: any) {
            if (imageIconEnabled(b)) {
                if (!b.icon || b.icon === "Auto")
                    b.icon = "Camera";
            }
            else {
                b.icon = "Auto";
            }
            b.icon_on = "Auto";
            b.sensor = "";
            b.unit = "";
            b.precision = "";
            b.options = normalizeImageOptions(b.options, b.entity, true);
            if (!imageLabelEnabled(b))
                b.label = "";
            const entityField = helpers.renderCardEntityField(panel, b, helpers, IMAGE_CARD_METADATA);
            renderImageLabelSettings(panel, b, helpers);
            var modalSettingsDisclosure: any = helpers.disclosureSection("Modal Settings", helpers.idPrefix + "image-modal-settings", false);
            renderImageModalSettings(modalSettingsDisclosure.section, b, helpers, entityField.input);
            panel.appendChild(modalSettingsDisclosure.panel);
        },
        renderPreview: function (this: any, b?: any, helpers?: any) {
            var tertiaryColor: any = WEB_UI_COLORS.tertiary;
            var label: any = imageLabelEnabled(b) ? String((b && b.label) || "Camera").trim() : "";
            var iconName: any = b && b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : "camera";
            var icon: any = imageIconEnabled(b) ? '<span class="sp-image-preview-icon mdi mdi-' + iconName + '"></span>' : "";
            return {
                buttonClass: "sp-image-card",
                iconHtml: '<span class="sp-image-preview" style="background:#' + helpers.escHtml(tertiaryColor) + '">' +
                    icon +
                    '</span>',
                labelHtml: label
                    ? '<span class="sp-image-label"><span class="sp-image-label-stack">' +
                        '<span class="sp-image-label-text sp-image-label-shadow" aria-hidden="true">' +
                        helpers.escHtml(label) +
                        '</span><span class="sp-image-label-text sp-image-label-main">' +
                        helpers.escHtml(label) +
                        '</span></span></span>'
                    : "",
            };
        },
    });
}
