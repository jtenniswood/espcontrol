import { state } from "../state/app_instance";
import { configOptionEnabled, configOptionValue } from "../model/config_primitives";
import {
    cardContractAllowInSubpage,
    cardContractCard,
    cardContractCardLabel,
    cardContractDefaultConfig,
    cardContractDomains,
    cardContractHidden,
    cardContractPickerKey,
} from "../generated/card_contract";
import { WEB_UI_COLORS } from "../state/ui_tokens";
import { escHtml, iconSlug } from "../application/ui_primitives";
import type { CardRegistry, CardUiServices } from "../application/card_registry";
import type { ConfigMediaOptionsFeature } from "../application/config_media_options";
import type { ControlsFieldsFeature } from "../application/controls_fields";
import type { SettingsUiFeature } from "../features/settings";
import {
    MEDIA_COVER_ART_OPTION,
    MEDIA_PLAYLIST_CONTENT_TYPE_OPTION,
} from "../application/config_option_core";
import { CARD_SIZE_SINGLE } from "../model/grid";
export function registerMediaCardTypes(
    registry: CardRegistry,
    mediaOptions: ConfigMediaOptionsFeature,
    deviceId: string,
    fields: ControlsFieldsFeature,
    settingsUi: Pick<SettingsUiFeature, "infoPanel">,
    cardUi: CardUiServices,
): void {
    const { renderButtonSettings, renderPreview } = cardUi;
    const { cardBadgeLabelHtml, cardLargeNumbersActiveForCardSize, cardSensorPreviewHtml } = fields;
    const { infoPanel } = settingsUi;
    const {
        mediaBehaviorSpec,
        mediaCoverArtCardsSupported,
        mediaModeOptionValues,
        mediaDefaultMode,
        mediaEditorMode,
        mediaEditorValidMode,
        mediaNowPlayingControlValues,
        mediaNowPlayingControls,
        mediaStateDisplayModeSupported,
        mediaPlaylistSourceOptions,
        mediaPlaylistSourceDefinition,
        mediaPlaylistContentIdPlaceholder,
        parseMediaPlaylistContentId,
        buildMediaPlaylistContentId,
        mediaPlaylistContentTypeKnown,
        mediaPlaylistContentTypeOptions,
        normalizeMediaOptions,
        mediaCoverArtDetailsEnabled,
        setMediaCoverArtDetailsEnabled,
        mediaCoverArtSecondaryEntity,
        setMediaCoverArtSecondaryEntity,
        mediaVolumeMax,
        setMediaVolumeMax,
        mediaSpeakerGroupEntity,
        setMediaSpeakerGroupEntity,
        mediaLabelDisplayMode,
        setMediaLabelDisplayMode,
        mediaNumberDisplayMode,
        setMediaNumberDisplayMode,
        mediaPlaylistContentId,
        mediaPlaylistContentType,
        setMediaPlaylistContentId,
        setMediaPlaylistContentType,
        mediaPlaylistPlayerSource,
        setMediaPlaylistPlayerSource,
    } = mediaOptions;
    // Media player card: playback buttons, volume, track position, or now-playing details.
    function mediaNowPlayingProgressEnabled(this: any, b?: any) {
        return mediaNowPlayingControls(b) === "progress";
    }
    function mediaNowPlayingPlayPauseEnabled(this: any, b?: any) {
        return mediaNowPlayingControls(b) === "play_pause";
    }
    function mediaLabelIsGenerated(this: any, label?: any) {
        label = String(label || "").trim();
        return !label || [
            "Media",
            "Play/Pause",
            "Previous",
            "Skip Previous",
            "Next",
            "Skip Next",
            "Volume",
            "Position",
            "Now Playing",
            "Cover Art",
            "Media Control",
            "Media Control Modal",
            "All Controls",
            "Speaker Group",
        ].indexOf(label) >= 0;
    }
    function mediaModeOptions(this: any) {
        var options: any = [
            ["control_modal", "All Controls"],
            ["cover_art", "Cover Art"],
            ["playlist", "Track, Album or Playlist"],
            ["speaker_group", "Speaker Group"],
            ["play_pause", "Play/Pause"],
            ["previous", "Previous"],
            ["next", "Next"],
            ["volume", "Volume"],
            ["position", "Track Position"],
            ["now_playing", "Now Playing"],
        ];
        return mediaCoverArtCardsSupported() ? options : options.filter(function (this: any, option?: any) {
            return option[0] !== "cover_art";
        });
    }
    var MEDIA_CARD_METADATA: any = {
        mode: {
            label: "Type",
            idSuffix: "media-mode",
            options: mediaModeOptions,
            value: function (this: any, b?: any) {
                return mediaEditorValidMode(b.sensor);
            },
        },
        entity: {
            label: "Entity",
            idSuffix: "entity",
            placeholder: "e.g. media_player.living_room",
            domains: function (this: any) { return cardContractDomains("media"); },
            bindName: "entity",
            rerender: true,
            requiredMessage: "Add an entity before saving.",
        },
        displayMode: {
            label: "Type",
            inputId: "media-display",
            options: [
                ["", "Label"],
                ["state", "State"],
            ],
        },
        nowPlayingControls: {
            label: "Controls",
            inputId: "media-controls",
            options: [
                ["", "None"],
                ["progress", "Track Position"],
                ["play_pause", "Play/Pause"],
            ],
        },
        controlLabelDisplay: {
            label: "Label",
            inputId: "media-control-label-display",
            options: [
                ["label", "Label"],
                ["status", "State"],
            ],
        },
        controlNumberDisplay: {
            label: "Top Left",
            inputId: "media-control-number-display",
            options: [
                ["icon", "Icon"],
                ["volume", "Volume"],
            ],
        },
        largeNumbers: {
            label: "Large Media Numbers",
            idSuffix: "large-media-numbers",
            supported: function (this: any, b?: any) {
                var mode: any = mediaEditorMode(b && b.sensor);
                return mode === "volume" || mode === "position";
            },
        },
        preview: {
            badge: "speaker",
        },
    };
    registry.register("media", {
        label: function (this: any) { return cardContractCardLabel("media"); },
        allowInSubpage: function (this: any) { return cardContractAllowInSubpage("media"); },
        pickerKey: function (this: any) { return cardContractPickerKey("media"); },
        hidden: function (this: any) { return cardContractHidden("media"); },
        hideLabel: true,
        labelPlaceholder: "e.g. Living Room Speaker",
        defaultConfig: function (this: any) { return cardContractDefaultConfig("media"); },
        cardMetadata: MEDIA_CARD_METADATA,
        onSelect: function (this: any, b?: any) {
            b.entity = "";
            b.sensor = "cover_art";
            b.unit = "";
            b.precision = (b.sensor === "play_pause" || b.sensor === "position") && b.precision === "state" ? "state" : "";
            b.icon = "Auto";
            b.icon_on = "Auto";
            b.options = "";
        },
        renderSettingsBeforeLabel: function (this: any, panel?: any, b?: any, slot?: any, helpers?: any) {
            function validMode(this: any, value?: any) {
                return mediaEditorValidMode(value);
            }
            function mediaDefaultIcon(this: any, value?: any) {
                var mode: any = mediaEditorMode(value);
                if (mode === "previous")
                    return "Skip Previous";
                if (mode === "next")
                    return "Skip Next";
                if (mode === "volume")
                    return "Volume High";
                if (mode === "position")
                    return "Progress Clock";
                if (mode === "now_playing")
                    return "Music";
                if (mode === "cover_art")
                    return "Music";
                if (mode === "control_modal")
                    return "Play Pause";
                if (mode === "speaker_group")
                    return "Speaker Multiple";
                if (mode === "playlist")
                    return "Music";
                return "Play Pause";
            }
            function isMediaDefaultIcon(this: any, value?: any, icon?: any) {
                if (!icon || icon === "Auto")
                    return true;
                if (value === "controls" && icon === "Speaker")
                    return true;
                return icon === mediaDefaultIcon(value);
            }
            function mediaActionLabel(this: any, value?: any) {
                var mode: any = mediaEditorMode(value);
                if (mode === "previous")
                    return "Previous";
                if (mode === "next")
                    return "Next";
                if (mode === "volume")
                    return "Volume";
                if (mode === "play_pause")
                    return "Play/Pause";
                if (mode === "control_modal")
                    return "All Controls";
                if (mode === "speaker_group")
                    return "Speaker Group";
                if (mode === "cover_art")
                    return "Cover Art";
                if (mode === "playlist")
                    return "Playlist";
                return "";
            }
            var rawMode: any = b.sensor;
            b.sensor = validMode(b.sensor);
            if (rawMode === "controls" && isMediaDefaultIcon(rawMode, b.icon))
                b.icon = "Auto";
            helpers.renderCardModeSelector(panel, b, helpers, Object.assign({}, MEDIA_CARD_METADATA, {
                mode: Object.assign({}, MEDIA_CARD_METADATA.mode, {
                    onChange: function (this: any) {
                        var oldMode: any = b.sensor;
                        b.sensor = validMode(this.value);
                        if (isMediaDefaultIcon(oldMode, b.icon)) {
                            b.icon = "Auto";
                            helpers.saveField("icon", b.icon);
                        }
                        if (b.sensor === "now_playing") {
                            b.precision = mediaNowPlayingControls(b);
                            helpers.saveField("precision", b.precision);
                        }
                        else if (b.sensor === "play_pause" || b.sensor === "position") {
                            b.precision = b.precision === "state" ? "state" : "";
                            helpers.saveField("precision", b.precision);
                        }
                        else if (b.precision) {
                            b.precision = "";
                            helpers.saveField("precision", "");
                        }
                        if (b.sensor === "previous" || b.sensor === "next") {
                            b.label = mediaActionLabel(b.sensor);
                            b.icon = mediaDefaultIcon(b.sensor);
                            helpers.saveField("label", b.label);
                            helpers.saveField("icon", b.icon);
                        }
                        if (b.sensor === "playlist") {
                            var oldPlaylistDefaultLabel: any = mediaActionLabel(oldMode);
                            if (!b.label || b.label === oldPlaylistDefaultLabel || b.label === "Media") {
                                b.label = mediaActionLabel(b.sensor);
                                helpers.saveField("label", b.label);
                            }
                            b.icon = mediaDefaultIcon(b.sensor);
                            helpers.saveField("icon", b.icon);
                        }
                        if (b.sensor === "volume") {
                            var oldDefaultLabel: any = mediaActionLabel(oldMode);
                            if (!b.label || b.label === oldDefaultLabel || b.label === "Media") {
                                b.label = mediaActionLabel(b.sensor);
                                helpers.saveField("label", b.label);
                            }
                            b.icon = "Auto";
                            helpers.saveField("icon", b.icon);
                        }
                        if (b.sensor === "control_modal" && mediaLabelIsGenerated(b.label)) {
                            b.label = mediaActionLabel(b.sensor);
                            helpers.saveField("label", b.label);
                        }
                        if (b.sensor === "speaker_group") {
                            if (mediaLabelIsGenerated(b.label)) {
                                b.label = mediaActionLabel(b.sensor);
                                helpers.saveField("label", b.label);
                            }
                            b.icon = "Auto";
                            helpers.saveField("icon", b.icon);
                        }
                        if (b.sensor === "cover_art" && mediaLabelIsGenerated(b.label)) {
                            b.label = mediaActionLabel(b.sensor);
                            helpers.saveField("label", b.label);
                        }
                        if ((oldMode === "control_modal" || oldMode === "speaker_group" || oldMode === "cover_art") &&
                            b.sensor !== "control_modal" && b.sensor !== "speaker_group" &&
                            mediaLabelIsGenerated(b.label)) {
                            b.label = mediaActionLabel(b.sensor);
                            helpers.saveField("label", b.label);
                        }
                        var normalizedOptions: any = normalizeMediaOptions(b.options, b.sensor);
                        if (b.options !== normalizedOptions) {
                            b.options = normalizedOptions;
                            helpers.saveField("options", b.options);
                        }
                        helpers.saveField("sensor", b.sensor);
                        renderPreview();
                        renderButtonSettings();
                    },
                }),
            }));
        },
        renderSettings: function (this: any, panel?: any, b?: any, slot?: any, helpers?: any) {
            function validMode(this: any, value?: any) {
                return mediaEditorValidMode(value);
            }
            b.sensor = validMode(b.sensor);
            if (b.sensor === "now_playing" && configOptionEnabled(b.options, MEDIA_COVER_ART_OPTION)) {
                b.sensor = "cover_art";
                helpers.saveField("sensor", b.sensor);
            }
            b.unit = "";
            b.precision = b.sensor === "now_playing"
                ? mediaNowPlayingControls(b)
                : ((b.sensor === "play_pause" || b.sensor === "position") && b.precision === "state" ? "state" : "");
            b.icon_on = "Auto";
            var normalizedOptions: any = normalizeMediaOptions(b.options, b.sensor);
            if (b.options !== normalizedOptions) {
                b.options = normalizedOptions;
                helpers.saveField("options", b.options);
            }
            if (b.sensor === "previous" && b.label === "Skip Previous") {
                b.label = "Previous";
                helpers.saveField("label", b.label);
            }
            if (b.sensor === "next" && b.label === "Skip Next") {
                b.label = "Next";
                helpers.saveField("label", b.label);
            }
            if ((b.sensor === "previous" || b.sensor === "next") && !b.label) {
                b.label = b.sensor === "previous" ? "Previous" : "Next";
            }
            if (b.sensor === "volume") {
                if (!b.label || b.label === "Media")
                    b.label = "Volume";
                if (b.icon !== "Auto") {
                    b.icon = "Auto";
                    helpers.saveField("icon", b.icon);
                }
            }
            if (b.sensor === "play_pause" && b.icon !== "Auto") {
                b.icon = "Auto";
                helpers.saveField("icon", b.icon);
            }
            if (b.sensor === "control_modal" && mediaLabelIsGenerated(b.label)) {
                b.label = "All Controls";
                helpers.saveField("label", b.label);
            }
            if (b.sensor === "speaker_group" && mediaLabelIsGenerated(b.label)) {
                b.label = "Speaker Group";
                helpers.saveField("label", b.label);
            }
            if (b.sensor === "playlist") {
                if (!b.label || b.label === "Media")
                    b.label = "Playlist";
                if (!b.icon || b.icon === "Auto") {
                    b.icon = "Music";
                    helpers.saveField("icon", b.icon);
                }
            }
            if (b.sensor === "previous" && (!b.icon || b.icon === "Auto"))
                b.icon = "Skip Previous";
            if (b.sensor === "next" && (!b.icon || b.icon === "Auto"))
                b.icon = "Skip Next";
            helpers.renderCardEntityField(panel, b, helpers, b.sensor === "playlist"
                ? {
                    entity: Object.assign({}, MEDIA_CARD_METADATA.entity, {
                        label: "Speaker Entity",
                        requiredMessage: "Add a speaker entity before saving.",
                    }),
                }
                : MEDIA_CARD_METADATA);
            function renderSpeakerDiscoveryEntityField(this: any, target?: any) {
            if (b.sensor === "control_modal" || b.sensor === "speaker_group") {
                target = target || panel;
                var groupEntityField: any = helpers.textField(
                    "Speaker Discovery Entity (optional)",
                    helpers.idPrefix + "speaker-group-entity",
                    mediaSpeakerGroupEntity(b),
                    "Default: sensor.speaker_group", "", false);
                var groupEntityHintText: any = "Leave this blank to use sensor.speaker_group. Only enter another helper if you have more than one speaker helper or have changed the default entity name.";
                var groupEntityHint: any = document.createElement("button");
                groupEntityHint.type = "button";
                groupEntityHint.className = "mdi mdi-information-outline sp-field-info-button";
                groupEntityHint.setAttribute("aria-label", "About the speaker discovery entity");
                groupEntityHint.setAttribute("aria-expanded", "false");
                var groupEntityTooltip: any = document.createElement("div");
                groupEntityTooltip.className = "sp-field-info-text";
                groupEntityTooltip.id = helpers.idPrefix + "speaker-group-entity-tooltip";
                groupEntityTooltip.setAttribute("aria-live", "polite");
                groupEntityTooltip.textContent = groupEntityHintText;
                groupEntityHint.setAttribute("aria-describedby", groupEntityTooltip.id);
                var groupEntityHintHovered: any = false;
                var groupEntityHintFocused: any = false;
                var groupEntityHintTapped: any = false;
                function setGroupEntityTooltipVisible(this: any, visible?: any) {
                    groupEntityTooltip.classList.toggle("sp-visible", !!visible);
                    groupEntityHint.setAttribute("aria-expanded", visible ? "true" : "false");
                }
                function syncGroupEntityTooltipVisibility(this: any) {
                    setGroupEntityTooltipVisible(groupEntityHintHovered || groupEntityHintFocused || groupEntityHintTapped);
                }
                groupEntityHint.addEventListener("mouseenter", function (this: any) {
                    groupEntityHintHovered = true;
                    syncGroupEntityTooltipVisibility();
                });
                groupEntityHint.addEventListener("mouseleave", function (this: any) {
                    groupEntityHintHovered = false;
                    syncGroupEntityTooltipVisibility();
                });
                groupEntityHint.addEventListener("focus", function (this: any) {
                    groupEntityHintFocused = true;
                    syncGroupEntityTooltipVisibility();
                });
                groupEntityHint.addEventListener("click", function (this: any) {
                    if (document.activeElement !== groupEntityHint)
                        groupEntityHintTapped = !groupEntityHintTapped;
                    syncGroupEntityTooltipVisibility();
                });
                groupEntityHint.addEventListener("blur", function (this: any) {
                    groupEntityHintFocused = false;
                    groupEntityHintTapped = false;
                    syncGroupEntityTooltipVisibility();
                });
                groupEntityHint.addEventListener("keydown", function (this: any, event?: any) {
                    if (event.key === "Escape") {
                        groupEntityHintFocused = false;
                        groupEntityHintTapped = false;
                        setGroupEntityTooltipVisible(false);
                    }
                });
                groupEntityField.field.querySelector("label").appendChild(groupEntityHint);
                groupEntityField.field.insertBefore(groupEntityTooltip, groupEntityField.input);
                target.appendChild(groupEntityField.field);
                groupEntityField.input.pattern = "(?:media_player|sensor)\\.[A-Za-z0-9_]+";
                groupEntityField.input.addEventListener("change", function (this: any) {
                    setMediaSpeakerGroupEntity(b, groupEntityField.input.value);
                    groupEntityField.input.value = mediaSpeakerGroupEntity(b);
                    helpers.saveField("options", b.options);
                });
            }
            }
            var displayMode: any = helpers.renderCardSegmentControl(panel, b, helpers, {
                segment: Object.assign({}, MEDIA_CARD_METADATA.displayMode, {
                    inputId: helpers.idPrefix + "media-display",
                    value: function (this: any) { return b.precision === "state" ? "state" : ""; },
                    onSelect: function (this: any, button?: any, cardHelpers?: any, value?: any) { setDisplayMode(value); },
                }),
            });
            var displayField: any = displayMode.segment.parentNode;
            var labelModeBtn: any = displayMode.buttons[""];
            var stateModeBtn: any = displayMode.buttons.state;
            function syncDisplayField(this: any) {
                if (b.sensor === "play_pause" || b.sensor === "position") {
                    displayField.style.display = "";
                }
                else {
                    displayField.style.display = "none";
                    if (b.precision && !mediaNowPlayingControls(b)) {
                        b.precision = "";
                        helpers.saveField("precision", "");
                    }
                }
                labelModeBtn.classList.toggle("active", b.precision !== "state");
                stateModeBtn.classList.toggle("active", b.precision === "state");
            }
            function setDisplayMode(this: any, mode?: any) {
                b.precision = mode === "state" ? "state" : "";
                helpers.saveField("precision", b.precision);
                renderButtonSettings();
            }
            panel.appendChild(displayField);
            syncDisplayField();
            if (b.sensor === "position") {
                helpers.renderCardLargeNumbersToggle(panel, b, helpers, MEDIA_CARD_METADATA);
            }
            if (b.sensor === "now_playing") {
                var controls: any = helpers.renderCardSegmentControl(panel, b, helpers, {
                    segment: Object.assign({}, MEDIA_CARD_METADATA.nowPlayingControls, {
                        inputId: helpers.idPrefix + "media-controls",
                        value: function (this: any) { return mediaNowPlayingControls(b); },
                        onSelect: function (this: any, button?: any, cardHelpers?: any, value?: any) {
                            button.precision = value;
                            cardHelpers.saveField("precision", button.precision);
                            renderButtonSettings();
                        },
                    }),
                });
                controls.segment.classList.add("sp-segment-scroll");
            }
            if (b.sensor === "now_playing") {
                var controlsMode: any = mediaNowPlayingControls(b);
                if (b.precision !== controlsMode) {
                    b.precision = controlsMode;
                    helpers.saveField("precision", b.precision);
                }
            }
            if (b.sensor === "cover_art") {
                var cardSettingsDisclosure: any = helpers.disclosureSection(
                    "Card Settings",
                    helpers.idPrefix + "media-cover-art-card-settings",
                    false);
                cardSettingsDisclosure.panel.classList.add("sp-media-card-settings");
                var cardSettings: any = cardSettingsDisclosure.section;
                var detailsToggle: any = helpers.toggleRow(
                    "Show Track Details",
                    helpers.idPrefix + "media-cover-art-details",
                    mediaCoverArtDetailsEnabled(b));
                cardSettings.appendChild(detailsToggle.row);
                detailsToggle.input.addEventListener("change", function (this: any) {
                    setMediaCoverArtDetailsEnabled(b, this.checked);
                    helpers.saveField("options", b.options);
                    renderPreview();
                });
                panel.appendChild(cardSettingsDisclosure.panel);

                var secondaryPlayerDisclosure: any = helpers.disclosureSection(
                    "External Sources",
                    helpers.idPrefix + "media-cover-art-secondary-player",
                    false);
                var secondaryPlayerSettings: any = secondaryPlayerDisclosure.section;
                secondaryPlayerSettings.appendChild(infoPanel(
                    helpers.idPrefix + "media-cover-art-secondary-player-info",
                    "Use a second media entity when the primary player switches to a Line In, TV, or HDMI source. Artwork, track details, progress, and controls will follow the second player while it has current media."));
                var secondaryEntityField: any = helpers.renderCardEntityField(secondaryPlayerSettings, b, helpers, {
                    entity: {
                        label: "External Source Media Entity",
                        idSuffix: "media-cover-art-secondary-entity",
                        value: function (this: any) { return mediaCoverArtSecondaryEntity(b); },
                        placeholder: "e.g. media_player.apple_tv",
                        domains: ["media_player"],
                        bindName: null,
                        rerender: false,
                    },
                });
                var secondaryEntityInput: any = secondaryEntityField.input;
                function saveSecondaryEntity(this: any) {
                    setMediaCoverArtSecondaryEntity(b, secondaryEntityInput.value);
                    helpers.saveField("options", b.options);
                }
                secondaryEntityInput.addEventListener("input", saveSecondaryEntity);
                secondaryEntityInput.addEventListener("change", saveSecondaryEntity);
                secondaryEntityInput.addEventListener("blur", saveSecondaryEntity);
                secondaryEntityInput.addEventListener("keydown", function (this: any, event?: any) {
                    if (event.key === "Enter") {
                        saveSecondaryEntity();
                        this.blur();
                    }
                });
                panel.appendChild(secondaryPlayerDisclosure.panel);
            }
            if (b.sensor === "control_modal") {
                var numberDisplay: any = helpers.renderCardSegmentControl(panel, b, helpers, {
                    segment: Object.assign({}, MEDIA_CARD_METADATA.controlNumberDisplay, {
                        inputId: helpers.idPrefix + "media-control-number-display",
                        value: function (this: any) { return mediaNumberDisplayMode(b); },
                        onSelect: function (this: any, button?: any, cardHelpers?: any, value?: any) {
                            setMediaNumberDisplayMode(button, value);
                            cardHelpers.saveField("options", button.options);
                            renderButtonSettings();
                        },
                    }),
                });
                numberDisplay.segment.classList.add("sp-segment-scroll");
                if (mediaNumberDisplayMode(b) === "icon") {
                    helpers.renderCardIconPicker(panel, b, helpers, {
                        pickerIdSuffix: "icon-picker",
                        idSuffix: "icon",
                        field: "icon",
                        fallback: "Play Pause",
                    });
                }
                var labelDisplay: any = helpers.renderCardSegmentControl(panel, b, helpers, {
                    segment: Object.assign({}, MEDIA_CARD_METADATA.controlLabelDisplay, {
                        inputId: helpers.idPrefix + "media-control-label-display",
                        value: function (this: any) { return mediaLabelDisplayMode(b); },
                        onSelect: function (this: any, button?: any, cardHelpers?: any, value?: any) {
                            setMediaLabelDisplayMode(button, value);
                            cardHelpers.saveField("options", button.options);
                            renderButtonSettings();
                        },
                    }),
                });
                labelDisplay.segment.classList.add("sp-segment-scroll");
                if (mediaLabelDisplayMode(b) === "label") {
                    helpers.renderCardTextField(panel, b, helpers, {
                        label: "Label",
                        idSuffix: "label",
                        field: "label",
                        placeholder: "All Controls",
                        rerender: true,
                    });
                }
            }
            if (b.sensor !== "now_playing" &&
                b.sensor !== "cover_art" &&
                b.sensor !== "control_modal" &&
                b.sensor !== "playlist" &&
                (b.sensor !== "play_pause" || b.precision !== "state") &&
                (b.sensor !== "position" || b.precision !== "state")) {
                helpers.renderCardTextField(panel, b, helpers, {
                    label: "Label",
                    idSuffix: "label",
                    field: "label",
                    placeholder: b.sensor === "position" ? "Position" : "e.g. Living Room Speaker",
                    rerender: true,
                });
            }
            var mediaAdvancedSettings: any = panel;
            if (b.sensor === "control_modal") {
                var mediaAdvancedDisclosure: any = helpers.disclosureSection(
                    "Advanced",
                    helpers.idPrefix + "media-advanced",
                    false);
                mediaAdvancedSettings = mediaAdvancedDisclosure.section;
                panel.appendChild(mediaAdvancedDisclosure.panel);
            }
            if (b.sensor === "volume" || b.sensor === "control_modal" || b.sensor === "speaker_group") {
                if (b.sensor === "volume") helpers.renderCardLargeNumbersToggle(panel, b, helpers, MEDIA_CARD_METADATA);
                var maxField: any = helpers.renderCardNumberField(mediaAdvancedSettings, b, helpers, {
                    label: "Maximum Volume",
                    idSuffix: "volume-max",
                    min: 1,
                    max: 100,
                    step: 1,
                    placeholder: "100",
                    value: function (this: any) {
                        var maxVolume: any = mediaVolumeMax(b);
                        return maxVolume === "100" ? "" : maxVolume;
                    },
                });
                maxField.input.addEventListener("change", function (this: any) {
                    setMediaVolumeMax(b, maxField.input.value);
                    maxField.input.value = mediaVolumeMax(b) === "100" ? "" : mediaVolumeMax(b);
                    helpers.saveField("options", b.options);
                });
            }
            var playlistCardSettings: any = null;
            if (b.sensor === "playlist") {
                var playlistSourceDisclosure: any = helpers.disclosureSection("Source", helpers.idPrefix + "playlist-source-settings", false);
                var playlistSourceSettings: any = playlistSourceDisclosure.section;
                panel.appendChild(playlistSourceDisclosure.panel);
                var playlistCardSettingsDisclosure: any = helpers.disclosureSection("Card Settings", helpers.idPrefix + "playlist-card-settings", false);
                playlistCardSettings = playlistCardSettingsDisclosure.section;
                panel.appendChild(playlistCardSettingsDisclosure.panel);
                var playlistInfo: any = document.createElement("div");
                playlistInfo.className = "sp-info-panel";
                playlistInfo.setAttribute("role", "note");
                var playlistInfoIcon: any = document.createElement("span");
                playlistInfoIcon.className = "mdi mdi-information-outline";
                playlistInfoIcon.setAttribute("aria-hidden", "true");
                var playlistInfoText: any = document.createElement("span");
                playlistInfoText.appendChild(document.createTextNode("Need help finding the media content ID? "));
                var playlistInfoLink: any = document.createElement("a");
                playlistInfoLink.href = "https://jtenniswood.github.io/espcontrol/card-types/media/#media-content";
                playlistInfoLink.target = "_blank";
                playlistInfoLink.rel = "noopener";
                playlistInfoLink.textContent = "Learn how to configure media content buttons";
                playlistInfoText.appendChild(playlistInfoLink);
                playlistInfoText.appendChild(document.createTextNode("."));
                playlistInfo.appendChild(playlistInfoIcon);
                playlistInfo.appendChild(playlistInfoText);
                playlistSourceSettings.appendChild(playlistInfo);
                var playlistContentType: any = mediaPlaylistContentType(b);
                var explicitPlaylistContentType: any = configOptionValue(b && b.options, MEDIA_PLAYLIST_CONTENT_TYPE_OPTION);
                var parsedPlaylistContentId: any = parseMediaPlaylistContentId(mediaPlaylistContentId(b), playlistContentType);
                if (!explicitPlaylistContentType &&
                    parsedPlaylistContentId.contentType &&
                    parsedPlaylistContentId.contentType !== playlistContentType) {
                    playlistContentType = parsedPlaylistContentId.contentType;
                    setMediaPlaylistContentType(b, playlistContentType);
                    helpers.saveField("options", b.options);
                }
                var sourceField: any = helpers.selectField("Source", helpers.idPrefix + "playlist-source", mediaPlaylistSourceOptions(), parsedPlaylistContentId.source);
                playlistSourceSettings.appendChild(sourceField.field);
                var contentTypeField: any = helpers.selectField("Media Type", helpers.idPrefix + "playlist-content-type", mediaPlaylistContentTypeOptions(), mediaPlaylistContentTypeKnown(playlistContentType) ? playlistContentType : "__custom");
                playlistSourceSettings.appendChild(contentTypeField.field);
                var customContentTypeField: any = helpers.textField("Custom Media Content Type", helpers.idPrefix + "playlist-content-type-custom", mediaPlaylistContentTypeKnown(playlistContentType) ? "" : playlistContentType, "e.g. favorite", "", false);
                playlistSourceSettings.appendChild(customContentTypeField.field);
                function updateCustomContentTypeVisibility(this: any) {
                    customContentTypeField.field.hidden = contentTypeField.select.value !== "__custom";
                }
                function selectedPlaylistContentType(this: any) {
                    return contentTypeField.select.value === "__custom"
                        ? customContentTypeField.input.value
                        : contentTypeField.select.value;
                }
                function savePlaylistContentIdFromFields(this: any) {
                    var selectedSource: any = sourceField.select.value;
                    var selectedType: any = selectedPlaylistContentType();
                    setMediaPlaylistContentType(b, selectedType);
                    setMediaPlaylistContentId(b, buildMediaPlaylistContentId(selectedSource, selectedType, contentIdField.input.value));
                    contentIdField.input.value = selectedSource === "__custom" || selectedSource === "url"
                        ? mediaPlaylistContentId(b)
                        : parseMediaPlaylistContentId(mediaPlaylistContentId(b), selectedType).id;
                    helpers.saveField("options", b.options);
                }
                var contentIdField: any = helpers.textField("ID", helpers.idPrefix + "playlist-content-id", parsedPlaylistContentId.id, mediaPlaylistContentIdPlaceholder(parsedPlaylistContentId.source, playlistContentType), "", false);
                playlistSourceSettings.appendChild(contentIdField.field);
                helpers.requireField(contentIdField.input, "Add a media ID before saving.");
                function syncContentIdPlaceholder(this: any) {
                    contentIdField.input.placeholder = mediaPlaylistContentIdPlaceholder(sourceField.select.value, selectedPlaylistContentType());
                }
                sourceField.select.addEventListener("change", function (this: any) {
                    if (sourceField.select.value === "__custom" || sourceField.select.value === "url") {
                        contentIdField.input.value = mediaPlaylistContentId(b);
                    }
                    else {
                        contentIdField.input.value = parseMediaPlaylistContentId(mediaPlaylistContentId(b), selectedPlaylistContentType()).id;
                    }
                    syncContentIdPlaceholder();
                    savePlaylistContentIdFromFields();
                });
                contentIdField.input.addEventListener("change", function (this: any) {
                    savePlaylistContentIdFromFields();
                });
                contentTypeField.select.addEventListener("change", function (this: any) {
                    updateCustomContentTypeVisibility();
                    syncContentIdPlaceholder();
                    savePlaylistContentIdFromFields();
                });
                customContentTypeField.input.addEventListener("change", function (this: any) {
                    savePlaylistContentIdFromFields();
                    customContentTypeField.input.value = mediaPlaylistContentTypeKnown(mediaPlaylistContentType(b))
                        ? "" : mediaPlaylistContentType(b);
                    syncContentIdPlaceholder();
                });
                updateCustomContentTypeVisibility();
                syncContentIdPlaceholder();
                var playerSourceField: any = helpers.renderCardTextField(playlistSourceSettings, b, helpers, {
                    label: "Player Source / Input",
                    idSuffix: "playlist-player-source",
                    bindName: "",
                    placeholder: "Optional, e.g. Spotify or Line-in",
                    value: function (this: any) { return mediaPlaylistPlayerSource(b); },
                });
                playerSourceField.input.addEventListener("change", function (this: any) {
                    setMediaPlaylistPlayerSource(b, playerSourceField.input.value);
                    helpers.saveField("options", b.options);
                });
                helpers.renderCardTextField(playlistCardSettings, b, helpers, {
                    label: "Label",
                    idSuffix: "label",
                    field: "label",
                    placeholder: "e.g. Morning Playlist",
                    rerender: true,
                });
            }
            if (b.sensor !== "play_pause" && b.sensor !== "now_playing" &&
                b.sensor !== "cover_art" &&
                b.sensor !== "position" && b.sensor !== "volume" &&
                b.sensor !== "control_modal") {
                helpers.renderCardIconPicker(playlistCardSettings || panel, b, helpers, {
                    pickerIdSuffix: "icon-picker",
                    idSuffix: "icon",
                    field: "icon",
                    fallback: "Speaker",
                });
            }
            renderSpeakerDiscoveryEntityField(mediaAdvancedSettings);
        },
        renderPreview: function (this: any, b?: any, helpers?: any) {
            function modeInfo(this: any, value?: any) {
                if (value === "controls")
                    value = "play_pause";
                if (value === "previous")
                    return { mode: "previous", label: "Previous", icon: "skip-previous" };
                if (value === "next")
                    return { mode: "next", label: "Next", icon: "skip-next" };
                if (value === "volume")
                    return { mode: "volume", label: "Volume", icon: "volume-high" };
                if (value === "position")
                    return { mode: "position", label: "Position", icon: "progress-clock" };
                if (value === "now_playing")
                    return { mode: "now_playing", label: "Now Playing", icon: "music" };
                if (value === "cover_art")
                    return { mode: "cover_art", label: "Cover Art", icon: "music" };
                if (value === "control_modal")
                    return { mode: "control_modal", label: "All Controls", icon: "play-pause" };
                if (value === "speaker_group")
                    return { mode: "speaker_group", label: "Speaker Group", icon: "speaker-multiple" };
                if (value === "playlist")
                    return { mode: "playlist", label: "Playlist", icon: "music" };
                return { mode: "play_pause", label: "Play/Pause", icon: "play-pause" };
            }
            var info: any = modeInfo(mediaEditorValidMode(b.sensor));
            var mode: any = info.mode;
            var label: any = (b.label && b.label.trim()) || info.label;
            if (mode === "control_modal") {
                var controlIcon: any = b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : info.icon;
                return {
                    iconHtml: mediaNumberDisplayMode(b) === "volume"
                        ? cardSensorPreviewHtml(b, helpers, "42", null)
                        : '<span class="sp-btn-icon mdi mdi-' + controlIcon + '"></span>',
                    labelHtml: cardBadgeLabelHtml(helpers, mediaLabelDisplayMode(b) === "status" ? "Playing" : label, MEDIA_CARD_METADATA.preview.badge),
                };
            }
            if (mode === "speaker_group") {
                var groupIcon: any = b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : info.icon;
                return {
                    buttonClass: "sp-media-group-active",
                    iconHtml: '<span class="sp-btn-icon mdi mdi-' + groupIcon + '"></span>',
                    labelHtml: cardBadgeLabelHtml(helpers, label, MEDIA_CARD_METADATA.preview.badge),
                };
            }
            if (mode === "volume") {
                return {
                    iconHtml: cardSensorPreviewHtml(b, helpers, "42", null),
                    labelHtml: cardBadgeLabelHtml(helpers, label, MEDIA_CARD_METADATA.preview.badge),
                };
            }
            if (mode === "position") {
                var bgColor: any = WEB_UI_COLORS.secondary;
                var progressColor: any = WEB_UI_COLORS.secondary;
                var positionLabel: any = b.precision === "state" ? "Paused" : label;
                var positionClass: any = "sp-sensor-preview sp-media-position-time" +
                    (cardLargeNumbersActiveForCardSize(b, helpers, MEDIA_CARD_METADATA) ? " sp-sensor-preview-large" : "");
                return {
                    iconHtml: '<span class="sp-slider-preview" style="inset:-2px;background:#' + helpers.escHtml(bgColor) + '">' +
                        '<span class="sp-slider-track"><span class="sp-slider-fill" style="width:50%;height:100%;background:#' +
                        helpers.escHtml(progressColor) + '"></span></span></span>' +
                        '<span class="' + positionClass + '">' +
                        '<span class="sp-sensor-value">0:00</span></span>',
                    labelHtml: cardBadgeLabelHtml(helpers, positionLabel, MEDIA_CARD_METADATA.preview.badge),
                };
            }
            if (mode === "cover_art") {
                var coverArtColor: any = WEB_UI_COLORS.tertiary;
                if (mediaCoverArtDetailsEnabled(b)) {
                    var singleCoverArtCard: any = ((helpers && helpers.cardSize) || CARD_SIZE_SINGLE) === CARD_SIZE_SINGLE;
                    var controlFontClass: any = deviceId === "guition-esp32-p4-jc4880p443"
                        ? " sp-media-cover-control-fonts"
                        : "";
                    return {
                        buttonClass: "sp-image-card sp-media-cover-details-card" + controlFontClass +
                            (singleCoverArtCard ? " sp-media-cover-details-single" : ""),
                        iconHtml: '<span class="sp-image-preview sp-media-cover-artwork" style="background-color:#' +
                            helpers.escHtml(coverArtColor) + '"></span>' +
                            '<span class="sp-media-cover-tint"></span>' +
                            '<span class="sp-media-now-title sp-media-cover-details-title">Track Title</span>',
                        labelHtml: '<span class="sp-btn-label-row sp-media-cover-details-row"><span class="sp-btn-label sp-media-now-artist">Artist Name</span></span>',
                    };
                }
                return {
                    buttonClass: "sp-image-card",
                    iconHtml: '<span class="sp-image-preview" style="background:#' +
                        helpers.escHtml(coverArtColor) + '"></span>',
                    labelHtml: '<span class="sp-image-label"><span class="sp-image-label-stack">' +
                        '<span class="sp-image-label-text sp-image-label-shadow" aria-hidden="true">Cover Art</span>' +
                        '<span class="sp-image-label-text sp-image-label-main">Cover Art</span></span></span>',
                };
            }
            if (mode === "now_playing") {
                var progressBg: any = "";
                if (mediaNowPlayingProgressEnabled(b)) {
                    var nowBgColor: any = WEB_UI_COLORS.secondary;
                    progressBg =
                        '<span class="sp-slider-preview" style="inset:-2px;background:#' + helpers.escHtml(nowBgColor) + '">' +
                            '<span class="sp-slider-track"><span class="sp-slider-fill" style="width:50%;height:100%;background:#' + WEB_UI_COLORS.secondary + '">' +
                            '</span></span></span>';
                }
                else if (mediaNowPlayingPlayPauseEnabled(b)) {
                    var playBgColor: any = WEB_UI_COLORS.secondary;
                    progressBg =
                        '<span class="sp-slider-preview" style="inset:-2px;background:#' + helpers.escHtml(playBgColor) + '">' +
                            '</span>';
                }
                return {
                    iconHtml: progressBg + '<span class="sp-media-now-title">Track Title</span>',
                    labelHtml: '<span class="sp-btn-label-row"><span class="sp-btn-label sp-media-now-artist">Artist Name</span></span>',
                };
            }
            return {
                iconHtml: '<span class="sp-btn-icon mdi mdi-' + (b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : info.icon) + '"></span>',
                labelHtml: cardBadgeLabelHtml(helpers, mode === "play_pause" && b.precision === "state" ? "Playing" : label, MEDIA_CARD_METADATA.preview.badge),
            };
        },
    });
    registry.register("media_cover_art", {
        label: "Cover Art",
        allowInSubpage: function (this: any) { return cardContractAllowInSubpage("media"); },
        // Retain the old registration only to normalize any saved alias. Cover Art is
        // selected from the Media card's Type field and is not a top-level card type.
        pickerKey: "media",
        hidden: true,
        hideLabel: true,
        cardMetadata: MEDIA_CARD_METADATA,
        defaultConfig: function (this: any) {
            var config: any = cardContractDefaultConfig("media");
            config.sensor = "cover_art";
            config.label = "Cover Art";
            return config;
        },
        normalizeConfig: function (this: any, config?: any) {
            config.type = "media";
            config.sensor = "cover_art";
            config.unit = "";
            config.precision = "";
            config.icon_on = "Auto";
            config.options = normalizeMediaOptions(config.options, config.sensor);
        },
    });
}
