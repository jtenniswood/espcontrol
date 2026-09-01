import { state } from "../state/app_instance";
import { WEB_UI_COLORS } from "../state/ui_tokens";
import { escHtml } from "./ui_primitives";
import {
    buttonConfigDisabledForDevice as isButtonConfigDisabledForDevice,
    cardTypePickerDetails,
    cardTypePickerOptions,
    defaultCardTypeForPicker,
    infoOnlyCardVisible,
    previewValue,
    registryValue,
} from "../features/preview";
import type { ApplicationLayoutState } from "./application_context";
import type { CardRegistry } from "./card_registry";
import type { ConfigConfirmationOptionsFeature } from "./config_confirmation_options";
import type { ConfigCodecFeature } from "./config_codec";
import type { UiRuntimeState } from "./state";
import type { ScreenRotationFeature } from "./screen_rotation_state";
import type { ControlsShellFeature } from "./controls_shell";
import type { GridFeature } from "./grid";
import type { ButtonSettingsSelectionFeature } from "./button_settings_selection";
export interface PreviewRenderDependencies {
    readonly document: Document;
    readonly layout: ApplicationLayoutState;
    readonly cards: CardRegistry;
    readonly confirmationOptions: ConfigConfirmationOptionsFeature;
    readonly codec: ConfigCodecFeature;
    readonly runtime: UiRuntimeState;
    readonly screenRotation: ScreenRotationFeature;
    readonly shell: Pick<ControlsShellFeature, "isConfigLocked">;
    readonly grid: Pick<GridFeature, "ctx" | "resolveIcon" | "sizeClass">;
    readonly selection: Pick<ButtonSettingsSelectionFeature, "renderSelectionBar" | "updatePreviewHint">;
}
export interface PreviewRenderFeature {
    render(): void;
    registryValue(typeDefinition?: any, key?: any, fallback?: any): any;
    configDisabled(button?: any): boolean;
    defaultTypeForPicker(key?: any): any;
    pickerOptions(isSubpage?: any, selectedTypeKey?: any): any[];
    pickerKeys(isSubpage?: any, selectedTypeKey?: any): any[];
    typeVisibleInPicker(key?: any, isSubpage?: any): boolean;
}

export function createPreviewRenderFeature(dependencies: PreviewRenderDependencies): PreviewRenderFeature {
    const document = dependencies.document;
    const els = dependencies.runtime.els;
    const { cardOnPattern } = dependencies.confirmationOptions;
    const { getSubpage } = dependencies.codec;
    const { gridPreviewBlocked: gridPreviewBlockedByRotationStartup } = dependencies.screenRotation;
    const { isConfigLocked } = dependencies.shell;
    const { ctx, resolveIcon, sizeClass } = dependencies.grid;
    const { renderSelectionBar, updatePreviewHint } = dependencies.selection;
    // ── Preview rendering (unified) ────────────────────────────────────────
    function previewHtmlValue(this: any, typePreview?: any, key?: any, fallback?: any) {
        return previewValue(typePreview, key, fallback);
    }
    function buttonTypeRegistryValue(this: any, typeDef?: any, key?: any, fallback?: any) {
        return registryValue(typeDef, key, fallback);
    }
    function buttonTypeDisabledForDevice(this: any, key?: any) {
        var disabled: any = dependencies.layout.config.disabledCardTypes || [];
        return disabled.indexOf(key || "") !== -1;
    }
    function buttonConfigDisabledForDevice(this: any, button?: any) {
        return isButtonConfigDisabledForDevice(
            dependencies.cards.definitions,
            dependencies.layout.config.disabledCardTypes || [],
            button,
        );
    }
    function buttonTypeInfoOnlyVisible(this: any, key?: any) {
        return infoOnlyCardVisible(key || "", !!dependencies.layout.config.infoOnly);
    }
    function defaultButtonTypeForPicker(this: any, key?: any) {
        return defaultCardTypeForPicker(key);
    }
    function buttonTypePickerDetails(this: any, key?: any, label?: any) {
        return cardTypePickerDetails(key || "", label || "");
    }
    function buttonTypePickerOptionList(this: any, isSub?: any, selectedTypeKey?: any) {
        return cardTypePickerOptions(
            dependencies.cards.definitions,
            dependencies.layout.config.disabledCardTypes || [],
            !!dependencies.layout.config.infoOnly,
            !!isSub,
            selectedTypeKey,
        );
    }
    function buttonTypePickerKeys(this: any, isSub?: any, selectedTypeKey?: any) {
        return buttonTypePickerOptionList(!!isSub, selectedTypeKey).map(function (this: any, opt?: any) {
            return opt.key;
        });
    }
    function buttonTypeVisibleInPicker(this: any, key?: any, isSub?: any) {
        return buttonTypePickerKeys(!!isSub, null).indexOf(key) >= 0;
    }
    function renderPreview(this: any) {
        var main: any = els.previewMain;
        main.innerHTML = "";
        main.className = "sp-main" + (state.subpageChevronsOn ? "" : " sp-hide-subpage-chevrons");
        if (gridPreviewBlockedByRotationStartup()) {
            main.className += " sp-grid-loading";
            main.setAttribute("aria-busy", "true");
            return;
        }
        main.removeAttribute("aria-busy");
        var c: any = ctx();
        updatePreviewHint(c);
        for (var pos: any = 0; pos < c.maxSlots; pos++) {
            var slot: any = c.grid[pos];
            if (slot === -1)
                continue;
            if (slot === -2) {
                var backBtn: any = document.createElement("div");
                var bkSz: any = c.sizes[-2];
                var backLabel: any = c.isSub ? (getSubpage(state.editingSubpage).backLabel || "Back") : "Back";
                backBtn.className = "sp-btn sp-back-btn" + sizeClass(bkSz) +
                    (c.selected.indexOf(-2) !== -1 ? " sp-selected" : "");
                backBtn.innerHTML =
                    '<span class="sp-btn-icon sp-back-hit mdi mdi-chevron-left"></span>' +
                        '<span class="sp-btn-label">' + escHtml(backLabel) + '</span>';
                backBtn.style.backgroundColor = "#" + WEB_UI_COLORS.secondary;
                backBtn.style.cursor = "pointer";
                backBtn.setAttribute("data-pos", pos);
                backBtn.draggable = !isConfigLocked();
                main.appendChild(backBtn);
            }
            else if (slot > 0) {
                var bIdx: any = slot - 1;
                if (c.isSub && bIdx >= c.buttons.length)
                    continue;
                var b: any = c.buttons[bIdx];
                if (state.settingsDraft &&
                    state.settingsDraft.slot === slot &&
                    state.settingsDraft.isSub === c.isSub &&
                    (!c.isSub || state.settingsDraft.homeSlot === state.editingSubpage)) {
                    b = state.settingsDraft.button;
                }
                if (!buttonTypeInfoOnlyVisible(b.type || "")) {
                    var hidden: any = document.createElement("div");
                    hidden.className = "sp-empty-cell sp-info-only-hidden";
                    hidden.setAttribute("data-pos", pos);
                    main.appendChild(hidden);
                    continue;
                }
                var iconName: any = resolveIcon(b);
                var label: any = b.label || b.entity || "Configure";
                var color: any = (b.type === "sensor" || b.type === "local_sensor" || b.type === "door_window" || b.type === "presence" || b.type === "weather" || b.type === "weather_forecast" || b.type === "calendar" || b.type === "clock" || b.type === "timezone")
                    ? WEB_UI_COLORS.tertiary : WEB_UI_COLORS.secondary;
                var previewTypeDef: any = dependencies.cards.definitions[b.type || ""] || null;
                if (previewTypeDef && c.isSub && !buttonTypeRegistryValue(previewTypeDef, "allowInSubpage", false)) {
                    previewTypeDef = null;
                }
                var slotSz: any = c.sizes[slot];
                var typePreview: any = previewTypeDef && previewTypeDef.renderPreview
                    ? previewTypeDef.renderPreview(b, { escHtml: escHtml, cardSize: slotSz || 1 })
                    : null;
                var btn: any = document.createElement("div");
                btn.className = "sp-btn" +
                    (typePreview && typePreview.buttonClass ? " " + typePreview.buttonClass : "") +
                    sizeClass(slotSz) +
                    (c.selected.indexOf(slot) !== -1 ? " sp-selected" : "");
                btn.style.backgroundColor = "#" + color;
                btn.draggable = !isConfigLocked();
                btn.setAttribute("data-pos", pos);
                btn.setAttribute("data-slot", slot);
                var hasWhenOn: any = !typePreview && (b.sensor || (b.icon_on && b.icon_on !== "Auto"));
                if (!typePreview && hasWhenOn && typeof cardOnPattern === "function" && cardOnPattern(b) === "stripes") {
                    var onColor: any = state.onColor && state.onColor.length === 6 ? state.onColor : WEB_UI_COLORS.primary;
                    btn.style.backgroundImage =
                        "repeating-linear-gradient(135deg,#" + onColor + " 0,#" + onColor +
                            " 12px,rgba(255,255,255,.22) 12px,rgba(255,255,255,.22) 20px)";
                }
                var badgeIcon: any = b.sensor ? "gauge" : "swap-horizontal";
                var sensorBadge: any = hasWhenOn
                    ? '<span class="sp-sensor-badge mdi mdi-' + badgeIcon + '"></span>'
                    : '';
                var labelHtml: any = previewHtmlValue(typePreview, "labelHtml", '<span class="sp-btn-label">' + escHtml(label) + '</span>');
                var iconHtml: any = previewHtmlValue(typePreview, "iconHtml", '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>');
                btn.innerHTML =
                    sensorBadge +
                        iconHtml +
                        labelHtml;
                main.appendChild(btn);
            }
            else {
                var empty: any = document.createElement("div");
                empty.className = "sp-empty-cell";
                empty.setAttribute("data-pos", pos);
                empty.innerHTML = '<span class="sp-add-pill"><span class="sp-add-icon mdi mdi-plus"></span></span>';
                main.appendChild(empty);
            }
        }
        renderSelectionBar(c);
    }
    return {
        render: renderPreview,
        registryValue: buttonTypeRegistryValue,
        configDisabled: buttonConfigDisabledForDevice,
        defaultTypeForPicker: defaultButtonTypeForPicker,
        pickerOptions: buttonTypePickerOptionList,
        pickerKeys: buttonTypePickerKeys,
        typeVisibleInPicker: buttonTypeVisibleInPicker,
    };
}
