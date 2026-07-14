import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function registerScreenLockCardTypes(): GlobalDescriptors {
    // Local display card: toggles screen lock on the device without Home Assistant.
    // Lock behaviour is configurable: what the screen shows while locked
    // (screensaver or off) is stored in the sensor field, how it unlocks
    // (immediate tap / slide gesture / PIN) in the unit field, and the PIN,
    // when used, in the entity field. These reuse spare base fields so the
    // card stays in the fully declarative static family (no options hook).
    function lockDisplayValues(this: any) {
        var spec: any = cardContractOptionSpec("screen_lock", "lock_display");
        return spec && spec.values ? spec.values.slice() : ["screensaver", "screen_off"];
    }
    function unlockValues(this: any) {
        var spec: any = cardContractOptionSpec("screen_lock", "unlock");
        return spec && spec.values ? spec.values.slice() : ["immediate", "gesture", "pin"];
    }
    function normalizeLockDisplay(this: any, value?: any) {
        value = String(value || "");
        return lockDisplayValues().indexOf(value) >= 0 ? value : "screensaver";
    }
    function normalizeUnlock(this: any, value?: any) {
        value = String(value || "");
        return unlockValues().indexOf(value) >= 0 ? value : "immediate";
    }
    function screenLockDisplayMode(this: any, b?: any) {
        return normalizeLockDisplay(b && b.sensor);
    }
    function screenLockUnlockMode(this: any, b?: any) {
        return normalizeUnlock(b && b.unit);
    }
    function screenLockPin(this: any, b?: any) {
        return b && b.entity ? String(b.entity) : "";
    }
    var SCREEN_LOCK_CARD_METADATA: any = {
        lockDisplay: {
            label: "When Locked",
            inputId: "screen-lock-display",
            options: [
                ["screensaver", "Screensaver"],
                ["screen_off", "Screen Off"],
            ],
        },
        unlock: {
            label: "Unlock With",
            inputId: "screen-lock-unlock",
            options: [
                ["immediate", "Immediate"],
                ["gesture", "Slide"],
                ["pin", "PIN"],
            ],
        },
        preview: {
            badge: "lock",
        },
    };
    registerButtonType("screen_lock", {
        label: function (this: any) { return cardContractCardLabel("screen_lock"); },
        allowInSubpage: function (this: any) { return cardContractAllowInSubpage("screen_lock"); },
        pickerKey: function (this: any) { return cardContractPickerKey("screen_lock"); },
        hidden: function (this: any) { return cardContractHidden("screen_lock"); },
        hideLabel: true,
        labelPlaceholder: "e.g. Screen Lock",
        defaultConfig: function (this: any) { return cardContractDefaultConfig("screen_lock"); },
        cardMetadata: SCREEN_LOCK_CARD_METADATA,
        onSelect: function (this: any, b?: any) {
            var defaults: any = cardContractDefaultConfig("screen_lock");
            Object.keys(defaults).forEach(function (this: any, key?: any) { b[key] = defaults[key]; });
        },
        renderSettings: function (this: any, panel?: any, b?: any, slot?: any, helpers?: any) {
            var displayMode: any = screenLockDisplayMode(b);
            var unlockMode: any = screenLockUnlockMode(b);
            helpers.renderCardSegmentControl(panel, b, helpers, {
                segment: Object.assign({}, SCREEN_LOCK_CARD_METADATA.lockDisplay, {
                    inputId: helpers.idPrefix + "screen-lock-display",
                    value: function (this: any) { return displayMode; },
                    onSelect: function (this: any, button?: any, cardHelpers?: any, value?: any) {
                        displayMode = normalizeLockDisplay(value);
                        b.sensor = displayMode;
                        helpers.saveField("sensor", b.sensor);
                    },
                }),
            });
            helpers.renderCardSegmentControl(panel, b, helpers, {
                segment: Object.assign({}, SCREEN_LOCK_CARD_METADATA.unlock, {
                    inputId: helpers.idPrefix + "screen-lock-unlock",
                    value: function (this: any) { return unlockMode; },
                    onSelect: function (this: any, button?: any, cardHelpers?: any, value?: any) {
                        unlockMode = normalizeUnlock(value);
                        b.unit = unlockMode;
                        helpers.saveField("unit", b.unit);
                        syncPinVisibility();
                    },
                }),
            });
            var pinCond: any = condField();
            var pinFieldEl: any = document.createElement("div");
            pinFieldEl.className = "sp-field";
            pinFieldEl.appendChild(helpers.fieldLabel("PIN", helpers.idPrefix + "screen-lock-pin"));
            var pinInput: any = document.createElement("input");
            pinInput.type = "text";
            pinInput.className = "sp-input";
            pinInput.id = helpers.idPrefix + "screen-lock-pin";
            pinInput.setAttribute("inputmode", "numeric");
            pinInput.maxLength = 8;
            pinInput.placeholder = "e.g. 1234";
            pinInput.value = screenLockPin(b);
            pinFieldEl.appendChild(pinInput);
            pinCond.appendChild(pinFieldEl);
            panel.appendChild(pinCond);
            function savePin(this: any) {
                var digits: any = String(pinInput.value || "").replace(/[^0-9]/g, "");
                if (digits !== pinInput.value) pinInput.value = digits;
                b.entity = digits;
                helpers.saveField("entity", b.entity);
            }
            pinInput.addEventListener("input", savePin);
            pinInput.addEventListener("change", savePin);
            pinInput.addEventListener("blur", savePin);
            function syncPinVisibility(this: any) {
                pinCond.classList.toggle("sp-visible", unlockMode === "pin");
            }
            syncPinVisibility();
        },
        renderPreview: function (this: any, b?: any, helpers?: any) {
            return cardBadgePreview(b, helpers, {
                label: "Screen Unlocked",
                iconFallback: "Lock Open",
                badge: SCREEN_LOCK_CARD_METADATA.preview.badge,
            });
        },
    });
    return {
        "SCREEN_LOCK_CARD_METADATA": liveGlobal(() => SCREEN_LOCK_CARD_METADATA, (value?: any) => { SCREEN_LOCK_CARD_METADATA = value; }),
    };
}
