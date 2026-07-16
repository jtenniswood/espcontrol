import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installSettingsPageHelpersModule(): GlobalDescriptors {
    // ── Settings Page Helpers ──────────────────────────────────────────
    // ── Settings UI helpers ─────────────────────────────────────────────
    var _settingsUiFeature: any = createSettingsUiFeature({
        document: document,
        textSpan: textSpan,
        createDisclosureChevron: createDisclosureChevron,
    });
    function settingsStatusHeader(this: any, title?: any) {
        return _settingsUiFeature.settingsStatusHeader(title);
    }
    function appendSettingsSection(this: any, parent?: any, title?: any, cards?: any) {
        _settingsUiFeature.appendSettingsSection(parent, title, cards);
    }
    function openVoiceServicesSettings(this: any) {
        if (isConfigLocked() || !els.voiceServicesCard)
            return;
        switchTab("settings");
        els.voiceServicesCard.classList.remove("collapsed");
        els.voiceServicesCard.scrollIntoView({ block: "center", behavior: "smooth" });
        if (els.setVoiceServicesToggle) {
            window.setTimeout(function (this: any) { els.setVoiceServicesToggle.focus(); }, 150);
        }
    }
    function coverArtTrackOverlayDurationSupported(this: any) {
        return !!(CFG && CFG.coverArtSquareOverlay);
    }
    function infoPanel(this: any, id?: any, text?: any) {
        return _settingsUiFeature.infoPanel(id, text);
    }
    function statusBadge(this: any, label?: any) {
        return _settingsUiFeature.statusBadge(label);
    }
    function disclosureBadge(this: any, text?: any, label?: any) {
        return _settingsUiFeature.disclosureBadge(text, label);
    }
    function inlineDisclosure(this: any, title?: any, bodyElement?: any, defaultOpen?: any, badgeElement?: any) {
        return _settingsUiFeature.inlineDisclosure(title, bodyElement, defaultOpen, badgeElement);
    }
    // ── Settings sync helpers ───────────────────────────────────────────
    function syncClockScreensaverControls(this: any) {
        var controlState: any = screensaverControlState(state.screensaverAction, state.clockBrightnessDay, state.clockBrightnessNight, state.screensaverDimmedBrightness);
        var mode: any = controlState.mode;
        var clockDisplay: any = controlState.clockVisible ? "" : "none";
        var dimDisplay: any = controlState.dimVisible ? "" : "none";
        state.clockScreensaverOn = mode === "clock";
        syncClockBarUi();
        if (els.setClockSelect)
            els.setClockSelect.value = mode;
        if (els.setSensorClockSelect)
            els.setSensorClockSelect.value = mode;
        syncOptionalClockBrightness(els.setClockBrightnessField, els.setDimBrightnessField || els.setClockField, clockDisplay);
        syncOptionalClockBrightness(els.setSensorClockBrightnessField, els.setSensorDimBrightnessField || els.setSensorClockField, clockDisplay);
        syncOptionalClockBrightness(els.setDimBrightnessField, els.setClockField, dimDisplay);
        syncOptionalClockBrightness(els.setSensorDimBrightnessField, els.setSensorClockField, dimDisplay);
        if (els.setDimBrightness) {
            els.setDimBrightness.value = state.screensaverDimmedBrightness;
            els.setDimBrightnessVal.textContent = controlState.dimBrightnessLabel;
        }
        if (els.setSensorDimBrightness) {
            els.setSensorDimBrightness.value = state.screensaverDimmedBrightness;
            els.setSensorDimBrightnessVal.textContent = controlState.dimBrightnessLabel;
        }
        if (els.setClockBrightnessDay) {
            els.setClockBrightnessDay.value = state.clockBrightnessDay;
            els.setClockBrightnessDayVal.textContent = controlState.dayBrightnessLabel;
        }
        if (els.setClockBrightnessNight) {
            els.setClockBrightnessNight.value = state.clockBrightnessNight;
            els.setClockBrightnessNightVal.textContent = controlState.nightBrightnessLabel;
        }
        if (els.setSensorClockBrightnessDay) {
            els.setSensorClockBrightnessDay.value = state.clockBrightnessDay;
            els.setSensorClockBrightnessDayVal.textContent = controlState.dayBrightnessLabel;
        }
        if (els.setSensorClockBrightnessNight) {
            els.setSensorClockBrightnessNight.value = state.clockBrightnessNight;
            els.setSensorClockBrightnessNightVal.textContent = controlState.nightBrightnessLabel;
        }
    }
    function syncMediaPlayerSleepPreventionUi(this: any) {
        if (els.setMediaPlayerSleepPreventionToggle) {
            els.setMediaPlayerSleepPreventionToggle.checked = !!state.mediaPlayerSleepPreventionOn;
        }
        if (els.setSensorMediaPlayerSleepPreventionToggle) {
            els.setSensorMediaPlayerSleepPreventionToggle.checked = !!state.mediaPlayerSleepPreventionOn;
        }
    }
    function syncCoverArtScreensaverUi(this: any) {
        if (els.setCoverArtToggle) {
            els.setCoverArtToggle.checked = !!state.coverArtScreensaverOn;
        }
        if (els.setCoverArtOptions) {
            els.setCoverArtOptions.classList.toggle("sp-visible", !!state.coverArtScreensaverOn);
        }
        if (els.setCoverArtOnlyOptions) {
            els.setCoverArtOnlyOptions.classList.toggle("sp-visible", !!state.coverArtScreensaverOn);
        }
        if (els.setCoverArtBadge) {
            els.setCoverArtBadge.className = "sp-card-badge" + (state.coverArtScreensaverOn ? "" : " sp-hidden");
        }
        if (els.setCoverArtDelay) {
            var coverArtDelay: any = normalizeCoverArtDelay(state.coverArtDelay);
            state.coverArtDelay = coverArtDelay;
            setSelectValue(els.setCoverArtDelay, coverArtDelay, formatDuration(coverArtDelay));
        }
        if (els.setCoverArtTrackOverlayDuration) {
            var value: any = state.coverArtTrackOverlayDuration;
            setSelectValue(els.setCoverArtTrackOverlayDuration, value, timedSettingLabel(value, formatDuration));
        }
        if (els.setCoverArtHideExternalInputToggle) {
            els.setCoverArtHideExternalInputToggle.checked = !!state.coverArtHideExternalInputOn;
        }
        if (els.setHomeAssistantArtworkProtocol) {
            els.setHomeAssistantArtworkProtocol.value =
                normalizeHomeAssistantArtworkProtocol(state.homeAssistantArtworkProtocol);
        }
        if (els.setCoverArtHomeAssistantPort) {
            els.setCoverArtHomeAssistantPort.value = String(normalizeHomeAssistantArtworkPort(state.coverArtHomeAssistantPort));
        }
        if (els.setCoverArtFilterToggle) {
            state.coverArtFilteringEnabled = !!state.coverArtFilteringEnabled || !!state.coverArtAttributeConditions;
            els.setCoverArtFilterToggle.checked = !!state.coverArtFilteringEnabled;
        }
        if (els.setCoverArtFilterOptions) {
            els.setCoverArtFilterOptions.classList.toggle("sp-visible", !!state.coverArtFilteringEnabled);
        }
        syncInput(els.setCoverArtConditions, state.coverArtAttributeConditions || "");
    }
    function syncOptionalClockBrightness(this: any, field?: any, previousField?: any, display?: any) {
        if (field)
            field.style.display = display;
        if (previousField)
            previousField.style.marginBottom = display === "none" ? "20px" : "";
    }
    function createScreensaverThenControls(this: any, selectId?: any) {
        var clockField: any = document.createElement("div");
        clockField.className = "sp-field";
        clockField.appendChild(fieldLabel("Then", selectId));
        var clockSelect: any = document.createElement("select");
        clockSelect.className = "sp-select";
        clockSelect.id = selectId;
        [
            { value: "off", label: "Display Off" },
            { value: "dim", label: "Screen Dimmed" },
            { value: "clock", label: "Clock" },
        ].forEach(function (this: any, opt?: any) {
            var o: any = document.createElement("option");
            o.value = opt.value;
            o.textContent = opt.label;
            clockSelect.appendChild(o);
        });
        clockSelect.value = normalizeScreensaverAction(state.screensaverAction);
        clockSelect.addEventListener("change", function (this: any) {
            state.screensaverAction = normalizeScreensaverAction(this.value);
            state.clockScreensaverOn = state.screensaverAction === "clock";
            syncClockScreensaverControls();
            postScreensaverAction(state.screensaverAction);
            postClockScreensaver(state.clockScreensaverOn);
        });
        clockField.appendChild(clockSelect);
        var dimBrightnessField: any = document.createElement("div");
        dimBrightnessField.style.display = normalizeScreensaverAction(state.screensaverAction) === "dim" ? "" : "none";
        var dimSlider: any = createRangeSlider("Dimmed Screen Brightness", state.screensaverDimmedBrightness, postScreensaverDimmedBrightness);
        dimSlider.range.min = "1";
        dimSlider.range.step = "1";
        dimSlider.range.addEventListener("input", function (this: any) {
            state.screensaverDimmedBrightness = normalizeScreensaverDimmedBrightness(this.value);
            syncClockScreensaverControls();
        });
        dimBrightnessField.appendChild(dimSlider.wrap);
        var clockBrightnessField: any = document.createElement("div");
        clockBrightnessField.className = "sp-clock-brightness-field";
        clockBrightnessField.style.display = normalizeScreensaverAction(state.screensaverAction) === "clock" ? "" : "none";
        var daySlider: any = createRangeSlider("Daytime Clock Brightness", state.clockBrightnessDay, postClockBrightnessDay);
        daySlider.range.min = "1";
        daySlider.range.step = "1";
        daySlider.range.addEventListener("input", function (this: any) {
            state.clockBrightnessDay = normalizeClockBrightness(this.value, 35);
            syncClockScreensaverControls();
        });
        clockBrightnessField.appendChild(daySlider.wrap);
        var nightSlider: any = createRangeSlider("Nighttime Clock Brightness", state.clockBrightnessNight, postClockBrightnessNight);
        nightSlider.range.min = "1";
        nightSlider.range.step = "1";
        nightSlider.range.addEventListener("input", function (this: any) {
            state.clockBrightnessNight = normalizeClockBrightness(this.value, state.clockBrightnessDay);
            syncClockScreensaverControls();
        });
        clockBrightnessField.appendChild(nightSlider.wrap);
        return {
            clockField: clockField,
            clockSelect: clockSelect,
            dimBrightnessField: dimBrightnessField,
            dimBrightness: dimSlider.range,
            dimBrightnessVal: dimSlider.val,
            brightnessField: clockBrightnessField,
            clockBrightnessDay: daySlider.range,
            clockBrightnessDayVal: daySlider.val,
            clockBrightnessNight: nightSlider.range,
            clockBrightnessNightVal: nightSlider.val,
        };
    }
    function createHourSelect(this: any, label?: any, id?: any, initial?: any, onChange?: any) {
        var wrap: any = document.createElement("div");
        wrap.className = "sp-field";
        wrap.appendChild(fieldLabel(label, id));
        var select: any = document.createElement("select");
        select.className = "sp-select";
        select.id = id;
        for (var h: any = 0; h < 24; h++) {
            var o: any = document.createElement("option");
            o.value = String(h);
            o.textContent = formatHour(h);
            select.appendChild(o);
        }
        select.value = String(normalizeHour(initial, 0));
        select.addEventListener("change", function (this: any) {
            onChange(normalizeHour(this.value, 0));
        });
        wrap.appendChild(select);
        return { wrap: wrap, select: select };
    }
    function createTimeInput(this: any, label?: any, id?: any, initial?: any, fallback?: any, onChange?: any) {
        var wrap: any = document.createElement("div");
        wrap.className = "sp-field";
        wrap.appendChild(fieldLabel(label, id));
        var input: any = document.createElement("input");
        input.type = "time";
        input.className = "sp-input";
        input.id = id;
        input.step = "60";
        input.value = normalizeTimeOfDay(initial, fallback);
        input.addEventListener("change", function (this: any) {
            var value: any = normalizeTimeOfDay(this.value, fallback);
            this.value = value;
            onChange(value);
        });
        wrap.appendChild(input);
        return { wrap: wrap, input: input };
    }
    function createEntityToggleSection(this: any, label?: any, id?: any, checked?: any, switchName?: any, entityLabel?: any, entityPostName?: any, placeholder?: any) {
        var toggle: any = toggleRow(label, id, checked);
        var field: any = condField();
        var inp: any = entityInput("", "", placeholder, ["sensor"]);
        field.appendChild(inp);
        toggle.input.addEventListener("change", function (this: any) { postSwitch(switchName, this.checked); });
        bindTextPost(inp, entityPostName, {});
        return { toggle: toggle, field: field, input: inp };
    }
    return {
        "_settingsUiFeature": liveGlobal(() => _settingsUiFeature, (value?: any) => { _settingsUiFeature = value; }),
        "settingsStatusHeader": staticGlobal(settingsStatusHeader),
        "appendSettingsSection": staticGlobal(appendSettingsSection),
        "openVoiceServicesSettings": staticGlobal(openVoiceServicesSettings),
        "coverArtTrackOverlayDurationSupported": staticGlobal(coverArtTrackOverlayDurationSupported),
        "infoPanel": staticGlobal(infoPanel),
        "statusBadge": staticGlobal(statusBadge),
        "disclosureBadge": staticGlobal(disclosureBadge),
        "inlineDisclosure": staticGlobal(inlineDisclosure),
        "syncClockScreensaverControls": staticGlobal(syncClockScreensaverControls),
        "syncMediaPlayerSleepPreventionUi": staticGlobal(syncMediaPlayerSleepPreventionUi),
        "syncCoverArtScreensaverUi": staticGlobal(syncCoverArtScreensaverUi),
        "syncOptionalClockBrightness": staticGlobal(syncOptionalClockBrightness),
        "createScreensaverThenControls": staticGlobal(createScreensaverThenControls),
        "createHourSelect": staticGlobal(createHourSelect),
        "createTimeInput": staticGlobal(createTimeInput),
        "createEntityToggleSection": staticGlobal(createEntityToggleSection),
    };
}
