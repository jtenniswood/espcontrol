import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installSettingsCoverArtSectionModule(): GlobalDescriptors {
    // ── Settings Cover Art Section ─────────────────────────────────────
    function buildCoverArtSettingsCard(this: any) {
        var coverArtBody: any = document.createElement("div");
        var coverArtToggle: any = toggleRow("Show Cover Art", "sp-set-ss-cover-art-enable", state.coverArtScreensaverOn);
        coverArtBody.appendChild(coverArtToggle.row);
        coverArtToggle.input.addEventListener("change", function (this: any) {
            state.coverArtScreensaverOn = this.checked;
            syncCoverArtScreensaverUi();
            postCoverArtScreensaver(state.coverArtScreensaverOn);
        });
        els.setCoverArtToggle = coverArtToggle.input;
        var coverArtOptions: any = condField();
        var coverArtOnlyOptions: any = condField();
        var coverArtAdvancedBody: any = document.createElement("div");
        var sleepPreventionToggle: any = toggleRow("Keep Screen Awake During Playback", "sp-set-ss-media-sleep-prevention", state.mediaPlayerSleepPreventionOn);
        coverArtOptions.appendChild(sleepPreventionToggle.row);
        sleepPreventionToggle.input.addEventListener("change", function (this: any) {
            state.mediaPlayerSleepPreventionOn = this.checked;
            syncMediaPlayerSleepPreventionUi();
            syncCoverArtScreensaverUi();
            postMediaPlayerSleepPrevention(state.mediaPlayerSleepPreventionOn);
        });
        els.setMediaPlayerSleepPreventionToggle = sleepPreventionToggle.input;
        var coverArtEntityField: any = document.createElement("div");
        coverArtEntityField.className = "sp-field";
        coverArtEntityField.appendChild(fieldLabel("Media Player Entity", "sp-set-ss-cover-art-player"));
        var coverArtEntityInp: any = entityInput("sp-set-ss-cover-art-player", state.coverArtMediaPlayerEntity, "e.g. media_player.living_room", ["media_player"]);
        coverArtEntityField.appendChild(coverArtEntityInp);
        coverArtOnlyOptions.appendChild(coverArtEntityField);
        bindTextPost(coverArtEntityInp, entityName("screen_saver_cover_art_entity"), {
            onBlur: function (this: any, value?: any) {
                state.coverArtMediaPlayerEntity = value;
                state.mediaPlayerSleepPreventionEntity = value;
            },
            post: function (this: any, value?: any) {
                postCoverArtMediaPlayerEntity(value);
                postMediaPlayerSleepPreventionEntity(value);
            },
        });
        els.setCoverArtMediaPlayer = coverArtEntityInp;
        var coverArtDelayField: any = document.createElement("div");
        coverArtDelayField.className = "sp-field";
        coverArtDelayField.appendChild(fieldLabel("Show After", "sp-set-ss-cover-art-delay"));
        var coverArtDelaySelect: any = document.createElement("select");
        coverArtDelaySelect.className = "sp-select";
        coverArtDelaySelect.id = "sp-set-ss-cover-art-delay";
        [
            { label: "Immediately", value: 0 },
            { label: "5 seconds", value: 5 },
            { label: "10 seconds", value: 10 },
            { label: "30 seconds", value: 30 },
            { label: "1 minute", value: 60 },
            { label: "5 minutes", value: 300 },
        ].forEach(function (this: any, opt?: any) {
            var o: any = document.createElement("option");
            o.value = opt.value;
            o.textContent = opt.label;
            coverArtDelaySelect.appendChild(o);
        });
        coverArtDelaySelect.addEventListener("change", function (this: any) {
            state.coverArtDelay = parseFloat(this.value) || 0;
            postCoverArtDelay(state.coverArtDelay);
        });
        coverArtDelayField.appendChild(coverArtDelaySelect);
        coverArtOnlyOptions.appendChild(coverArtDelayField);
        els.setCoverArtDelay = coverArtDelaySelect;
        if (coverArtTrackOverlayDurationSupported()) {
            var trackOverlayField: any = document.createElement("div");
            trackOverlayField.className = "sp-field";
            trackOverlayField.appendChild(fieldLabel("Show Track Details For", "sp-set-ss-track-overlay"));
            var trackOverlaySelect: any = document.createElement("select");
            trackOverlaySelect.className = "sp-select";
            trackOverlaySelect.id = "sp-set-ss-track-overlay";
            [
                { label: "Never", value: 0 },
                { label: "3 seconds", value: 3 },
                { label: "5 seconds", value: 5 },
                { label: "10 seconds", value: 10 },
                { label: "15 seconds", value: 15 },
                { label: "20 seconds", value: 20 },
                { label: "30 seconds", value: 30 },
                { label: "60 seconds", value: 60 },
                { label: "Always", value: -1 },
            ].forEach(function (this: any, opt?: any) {
                var o: any = document.createElement("option");
                o.value = opt.value;
                o.textContent = opt.label;
                trackOverlaySelect.appendChild(o);
            });
            trackOverlaySelect.addEventListener("change", function (this: any) {
                state.coverArtTrackOverlayDuration = parseFloat(this.value) || 0;
                postCoverArtTrackOverlayDuration(state.coverArtTrackOverlayDuration);
            });
            trackOverlayField.appendChild(trackOverlaySelect);
            coverArtOnlyOptions.appendChild(trackOverlayField);
            els.setCoverArtTrackOverlayDuration = trackOverlaySelect;
        }
        var coverArtHideExternalInputToggle: any = toggleRow("Hide for external source inputs", "sp-set-ss-cover-art-hide-external-input", state.coverArtHideExternalInputOn);
        coverArtAdvancedBody.appendChild(coverArtHideExternalInputToggle.row);
        coverArtHideExternalInputToggle.input.addEventListener("change", function (this: any) {
            state.coverArtHideExternalInputOn = this.checked;
            postCoverArtHideExternalInput(state.coverArtHideExternalInputOn);
        });
        els.setCoverArtHideExternalInputToggle = coverArtHideExternalInputToggle.input;
        state.coverArtFilteringEnabled = !!state.coverArtAttributeConditions;
        var coverArtFilterToggle: any = toggleRow("Advanced Filtering", "sp-set-ss-cover-art-filtering", state.coverArtFilteringEnabled);
        coverArtAdvancedBody.appendChild(coverArtFilterToggle.row);
        coverArtFilterToggle.input.addEventListener("change", function (this: any) {
            state.coverArtFilteringEnabled = this.checked;
            if (!state.coverArtFilteringEnabled) {
                state.coverArtAttributeConditions = "";
                syncInput(els.setCoverArtConditions, "");
                postCoverArtConditions("");
            }
            syncCoverArtScreensaverUi();
        });
        els.setCoverArtFilterToggle = coverArtFilterToggle.input;
        var coverArtFilterOptions: any = condField();
        var coverArtConditionsField: any = document.createElement("div");
        coverArtConditionsField.className = "sp-field";
        coverArtConditionsField.appendChild(fieldLabel("Only Show When", "sp-set-ss-cover-art-conditions"));
        var coverArtConditionsInp: any = document.createElement("input");
        coverArtConditionsInp.className = "sp-input";
        coverArtConditionsInp.id = "sp-set-ss-cover-art-conditions";
        coverArtConditionsInp.type = "text";
        coverArtConditionsInp.maxLength = 240;
        coverArtConditionsInp.placeholder = "app_id=com.apple.TVMusic; media_content_type=music";
        coverArtConditionsInp.value = state.coverArtAttributeConditions || "";
        coverArtConditionsField.appendChild(coverArtConditionsInp);
        coverArtFilterOptions.appendChild(coverArtConditionsField);
        coverArtAdvancedBody.appendChild(coverArtFilterOptions);
        bindTextPost(coverArtConditionsInp, entityName("screen_saver_cover_art_conditions"), {
            onBlur: function (this: any, value?: any) {
                state.coverArtAttributeConditions = value;
                state.coverArtFilteringEnabled = !!value || state.coverArtFilteringEnabled;
                syncCoverArtScreensaverUi();
            },
            post: postCoverArtConditions,
        });
        els.setCoverArtConditions = coverArtConditionsInp;
        els.setCoverArtFilterOptions = coverArtFilterOptions;
        coverArtOnlyOptions.appendChild(inlineDisclosure("Advanced Options", coverArtAdvancedBody, !!state.coverArtAttributeConditions || !state.coverArtHideExternalInputOn));
        els.setCoverArtOnlyOptions = coverArtOnlyOptions;
        coverArtOptions.appendChild(coverArtOnlyOptions);
        els.setCoverArtOptions = coverArtOptions;
        coverArtBody.appendChild(coverArtOptions);
        var coverArtBadge: any = statusBadge("Media cover art on");
        els.setCoverArtBadge = coverArtBadge;
        syncCoverArtScreensaverUi();
        var coverArtCard: any = makeCollapsibleCard("Cover Art", coverArtBody, true, coverArtBadge);
        return coverArtCard;
    }
    return {
        "buildCoverArtSettingsCard": staticGlobal(buildCoverArtSettingsCard),
    };
}
