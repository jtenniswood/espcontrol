import { state } from "../state/app_instance";
import type { ConfigCodecFeature } from "./config_codec";
import type { UiRuntimeState } from "./state";
import type { EntityStateFeature } from "./entity_state";
import type { AppStatusPreviewFeature } from "./app_status_preview";
import type { ArtworkPostApiFeature } from "./artwork_post_api";
import type { ControlsFieldsFeature } from "./controls_fields";
import type { SettingsPageHelpersFeature } from "./settings_page_helpers";
import type { CoverArtScreensaverController } from "../features/cover_art_screensaver_controller";
import type { MediaPlaybackController } from "../features/media_playback_controller";

export interface SettingsCoverArtSectionFeature {
    buildCoverArtSettingsCard(...args: any[]): any;
}

export function createSettingsCoverArtSectionFeature(codec: Pick<ConfigCodecFeature, "bindTextPost">, runtime: UiRuntimeState, entityState: Pick<EntityStateFeature, "entityName" | "entityInput">, statusPreview: Pick<AppStatusPreviewFeature, "syncInput">, artworkPostApi: ArtworkPostApiFeature, fields: Pick<ControlsFieldsFeature, "condField" | "fieldLabel" | "makeCollapsibleCard" | "toggleRow">, helpers: Pick<SettingsPageHelpersFeature, "applyCoverArtScreensaverState" | "applyMediaPlaybackState" | "coverArtScreensaverState" | "coverArtTrackOverlayDurationSupported" | "infoPanel" | "inlineDisclosure" | "mediaPlaybackState" | "statusBadge" | "syncCoverArtScreensaverUi" | "syncMediaPlayerSleepPreventionUi">, coverArtScreensaver: CoverArtScreensaverController, mediaPlayback: MediaPlaybackController, _companionSupported = false): SettingsCoverArtSectionFeature {
    const { applyCoverArtScreensaverState, applyMediaPlaybackState, coverArtScreensaverState, coverArtTrackOverlayDurationSupported, infoPanel, inlineDisclosure, mediaPlaybackState, statusBadge, syncCoverArtScreensaverUi, syncMediaPlayerSleepPreventionUi } = helpers;
    const _coverArtScreensaverController = coverArtScreensaver;
    const _mediaPlaybackController = mediaPlayback;
    const { condField, fieldLabel, makeCollapsibleCard, toggleRow } = fields;
    const { entityName, entityInput } = entityState;
    const { bindTextPost } = codec;
    const { syncInput } = statusPreview;
    const els = runtime.els;
    const {
        postMediaPlayerSleepPrevention,
        postMediaPlayerSleepPreventionEntity,
        postCoverArtScreensaver,
        postCoverArtSource,
        postCoverArtMediaPlayerEntity,
        postCoverArtSecondaryMediaPlayerEntity,
        postCoverArtConditions,
        postCoverArtHideExternalInput,
        postCoverArtDelay,
        postCoverArtTrackOverlayDuration,
    } = artworkPostApi;
    // ── Settings Cover Art Section ─────────────────────────────────────
    function buildCoverArtSettingsCard(this: any) {
        var coverArtBody: any = document.createElement("div");
        var coverArtToggle: any = toggleRow("Show Cover Art", "sp-set-ss-cover-art-enable", state.coverArtScreensaverOn);
        coverArtBody.appendChild(coverArtToggle.row);
        coverArtToggle.input.addEventListener("change", function (this: any) {
            applyCoverArtScreensaverState(_coverArtScreensaverController.setEnabled(coverArtScreensaverState(), this.checked));
            syncCoverArtScreensaverUi();
            postCoverArtScreensaver(state.coverArtScreensaverOn);
        });
        els.setCoverArtToggle = coverArtToggle.input;
        if (_companionSupported) {
            var coverArtSourceField: any = document.createElement("div");
            coverArtSourceField.className = "sp-field";
            coverArtSourceField.appendChild(fieldLabel("Source", "sp-set-ss-cover-art-source"));
            var coverArtSourceSelect: any = document.createElement("select");
            coverArtSourceSelect.className = "sp-select";
            coverArtSourceSelect.id = "sp-set-ss-cover-art-source";
            ["Home Assistant", "Mac Companion"].sort(function (a, b) {
                return a.localeCompare(b);
            }).forEach(function (source: string) {
                var option: any = document.createElement("option");
                option.value = source;
                option.textContent = source;
                coverArtSourceSelect.appendChild(option);
            });
            coverArtSourceSelect.value = state.coverArtSource === "Mac Companion" ? "Mac Companion" : "Home Assistant";
            coverArtSourceSelect.addEventListener("change", function (this: HTMLSelectElement) {
                state.coverArtSource = this.value;
                coverArtHomeAssistantOptions?.classList.toggle("sp-visible", this.value === "Home Assistant");
                postCoverArtSource(this.value);
            });
            coverArtSourceField.appendChild(coverArtSourceSelect);
            coverArtBody.appendChild(coverArtSourceField);
            els.setCoverArtSource = coverArtSourceSelect;
        }
        var coverArtOptions: any = condField();
        var coverArtOnlyOptions: any = condField();
        var coverArtHomeAssistantOptions: any = condField();
        var coverArtAdvancedBody: any = document.createElement("div");
        var coverArtScreensaverSettingsBody: any = document.createElement("div");
        var sleepPreventionToggle: any = toggleRow("Keep Screen Awake During Playback", "sp-set-ss-media-sleep-prevention", state.mediaPlayerSleepPreventionOn);
        coverArtScreensaverSettingsBody.appendChild(sleepPreventionToggle.row);
        sleepPreventionToggle.input.addEventListener("change", function (this: any) {
            applyMediaPlaybackState(_mediaPlaybackController.setSleepPreventionEnabled(mediaPlaybackState(), this.checked));
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
        coverArtHomeAssistantOptions.appendChild(coverArtEntityField);
        bindTextPost(coverArtEntityInp, entityName("screen_saver_cover_art_entity"), {
            onBlur: function (this: any, value?: any) {
                applyMediaPlaybackState(_mediaPlaybackController.setCoverArtEntity(mediaPlaybackState(), value));
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
            { label: "3 seconds", value: 3 },
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
            applyCoverArtScreensaverState(_coverArtScreensaverController.setDelay(coverArtScreensaverState(), this.value));
            postCoverArtDelay(state.coverArtDelay);
        });
        coverArtDelayField.appendChild(coverArtDelaySelect);
        coverArtScreensaverSettingsBody.appendChild(coverArtDelayField);
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
                applyCoverArtScreensaverState(_coverArtScreensaverController.setTrackOverlayDuration(coverArtScreensaverState(), this.value));
                postCoverArtTrackOverlayDuration(state.coverArtTrackOverlayDuration);
            });
            trackOverlayField.appendChild(trackOverlaySelect);
            coverArtScreensaverSettingsBody.appendChild(trackOverlayField);
            els.setCoverArtTrackOverlayDuration = trackOverlaySelect;
        }
        coverArtOnlyOptions.appendChild(inlineDisclosure("Screensaver Settings", coverArtScreensaverSettingsBody, false));
        var secondaryCoverArtSettingsBody: any = document.createElement("div");
        secondaryCoverArtSettingsBody.appendChild(infoPanel(
            "sp-set-ss-cover-art-secondary-player-info",
            "Enable if you use an external media player connected to your speakers Line In, TV, or HDMI source. If you add a second media player, cover art, track details, and progress be displayed when the external source is used."));
        var coverArtShowExternalInputToggle: any = toggleRow("Show external sources", "sp-set-ss-cover-art-show-external-input", !state.coverArtHideExternalInputOn);
        secondaryCoverArtSettingsBody.appendChild(coverArtShowExternalInputToggle.row);
        coverArtShowExternalInputToggle.input.addEventListener("change", function (this: any) {
            applyCoverArtScreensaverState(_coverArtScreensaverController.setShowExternalSources(coverArtScreensaverState(), this.checked));
            syncCoverArtScreensaverUi();
            postCoverArtHideExternalInput(state.coverArtHideExternalInputOn);
        });
        els.setCoverArtHideExternalInputToggle = coverArtShowExternalInputToggle.input;
        var secondaryCoverArtEntityOptions: any = condField();
        var secondaryCoverArtEntityField: any = document.createElement("div");
        secondaryCoverArtEntityField.className = "sp-field";
        secondaryCoverArtEntityField.appendChild(fieldLabel("External Source Media Entity", "sp-set-ss-cover-art-secondary-player"));
        var secondaryCoverArtEntityInp: any = entityInput("sp-set-ss-cover-art-secondary-player", state.coverArtSecondaryMediaPlayerEntity, "e.g. media_player.apple_tv", ["media_player"]);
        secondaryCoverArtEntityField.appendChild(secondaryCoverArtEntityInp);
        secondaryCoverArtEntityOptions.appendChild(secondaryCoverArtEntityField);
        secondaryCoverArtSettingsBody.appendChild(secondaryCoverArtEntityOptions);
        bindTextPost(secondaryCoverArtEntityInp, entityName("screen_saver_cover_art_secondary_entity"), {
            onBlur: function (this: any, value?: any) {
                state.coverArtSecondaryMediaPlayerEntity = value;
            },
            post: postCoverArtSecondaryMediaPlayerEntity,
        });
        els.setCoverArtSecondaryMediaPlayer = secondaryCoverArtEntityInp;
        els.setCoverArtSecondaryMediaPlayerOptions = secondaryCoverArtEntityOptions;
        coverArtHomeAssistantOptions.appendChild(inlineDisclosure("External sources", secondaryCoverArtSettingsBody, !state.coverArtHideExternalInputOn));
        applyCoverArtScreensaverState(_coverArtScreensaverController.initialState(coverArtScreensaverState()));
        var coverArtFilterToggle: any = toggleRow("Advanced Filtering", "sp-set-ss-cover-art-filtering", state.coverArtFilteringEnabled);
        coverArtAdvancedBody.appendChild(coverArtFilterToggle.row);
        coverArtFilterToggle.input.addEventListener("change", function (this: any) {
            applyCoverArtScreensaverState(_coverArtScreensaverController.setFilteringEnabled(coverArtScreensaverState(), this.checked));
            if (!state.coverArtFilteringEnabled) {
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
                applyCoverArtScreensaverState(_coverArtScreensaverController.setAttributeConditions(coverArtScreensaverState(), value));
                syncCoverArtScreensaverUi();
            },
            post: postCoverArtConditions,
        });
        els.setCoverArtConditions = coverArtConditionsInp;
        els.setCoverArtFilterOptions = coverArtFilterOptions;
        coverArtHomeAssistantOptions.appendChild(inlineDisclosure("Advanced Options", coverArtAdvancedBody, !!state.coverArtAttributeConditions));
        coverArtHomeAssistantOptions.classList.toggle("sp-visible", state.coverArtSource !== "Mac Companion");
        coverArtOnlyOptions.appendChild(coverArtHomeAssistantOptions);
        els.setCoverArtHomeAssistantOptions = coverArtHomeAssistantOptions;
        els.setCoverArtOnlyOptions = coverArtOnlyOptions;
        coverArtOptions.appendChild(coverArtOnlyOptions);
        els.setCoverArtOptions = coverArtOptions;
        coverArtBody.appendChild(coverArtOptions);
        var coverArtBadge: any = statusBadge("Media cover art on");
        els.setCoverArtBadge = coverArtBadge;
        syncCoverArtScreensaverUi();
        var coverArtCard: any = makeCollapsibleCard("Cover Art Screen Saver", coverArtBody, true, coverArtBadge);
        return coverArtCard;
    }
    return {
        buildCoverArtSettingsCard,
    };
}
