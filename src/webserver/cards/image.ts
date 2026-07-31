import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function registerImageCardTypes(): GlobalDescriptors {
    // Read-only Home Assistant camera/image entity card.
    var IMAGE_CARD_METADATA: any = {
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
    function renderImageModalSettings(this: any, panel?: any, b?: any, helpers?: any) {
        var modeField: any = helpers.selectField("Expanded Image", helpers.idPrefix + "image-modal-mode", imageModalModeOptions(), imageModalMode(b));
        panel.appendChild(modeField.field);
        modeField.select.addEventListener("change", function (this: any) {
            setImageModalMode(b, this.value);
            helpers.saveField("options", b.options);
        });
    }
    registerButtonType("image", {
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
            b.options = normalizeImageOptions(b.options);
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
            b.options = normalizeImageOptions(b.options);
            if (!imageLabelEnabled(b))
                b.label = "";
            helpers.renderCardEntityField(panel, b, helpers, IMAGE_CARD_METADATA);
            renderImageLabelSettings(panel, b, helpers);
            renderImageModalSettings(panel, b, helpers);
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
    return {
        "IMAGE_CARD_METADATA": liveGlobal(() => IMAGE_CARD_METADATA, (value?: any) => { IMAGE_CARD_METADATA = value; }),
        "imageModalModeOptions": staticGlobal(imageModalModeOptions),
        "renderImageLabelSettings": staticGlobal(renderImageLabelSettings),
        "renderImageModalSettings": staticGlobal(renderImageModalSettings),
    };
}
