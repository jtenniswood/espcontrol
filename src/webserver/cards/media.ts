import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function registerMediaCardTypes(): GlobalDescriptors {
    // Media player card: playback buttons, volume, track position, or now-playing details.
    function mediaBehaviorSpec(this: any) {
        var card: any = cardContractCard("media");
        return card && card.behavior && card.behavior.media || {};
    }
    function mediaModeOptionValues(this: any) {
        var spec: any = cardContractOptionSpec("media", "media_mode");
        return spec && spec.values ? spec.values.slice() :
            ["control_modal", "play_pause", "previous", "next", "volume", "position", "now_playing", "cover_art", "playlist"];
    }
    function mediaDefaultMode(this: any) {
        return mediaBehaviorSpec().defaultMode || "play_pause";
    }
    function mediaEditorMode(this: any, value?: any) {
        value = String(value || "");
        var legacy: any = mediaBehaviorSpec().legacyModes || {};
        value = legacy[value] || value;
        return mediaModeOptionValues().indexOf(value) >= 0 ? value : mediaDefaultMode();
    }
    function mediaEditorValidMode(this: any, value?: any) {
        return mediaEditorMode(value);
    }
    function mediaNowPlayingControls(this: any, b?: any) {
        if (!b || b.sensor !== "now_playing")
            return "";
        return mediaNowPlayingControlValues().indexOf(b.precision || "") >= 0 ? b.precision : "";
    }
    function mediaNowPlayingControlValues(this: any) {
        var spec: any = cardContractOptionSpec("media", "media_now_playing_controls");
        return spec && spec.values ? spec.values.slice() : ["", "progress", "play_pause"];
    }
    function mediaStateDisplayModeSupported(this: any, mode?: any) {
        var modes: any = mediaBehaviorSpec().stateDisplayModes || ["play_pause", "position"];
        return modes.indexOf(mediaEditorMode(mode)) >= 0;
    }
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
        ].indexOf(label) >= 0;
    }
    var MEDIA_CARD_METADATA: any = {
        mode: {
            label: "Type",
            idSuffix: "media-mode",
            options: [
                ["control_modal", "All Controls"],
                ["play_pause", "Play/Pause Button"],
                ["previous", "Previous Button"],
                ["next", "Next Button"],
                ["volume", "Volume Button"],
                ["position", "Track Position"],
                ["now_playing", "Now Playing"],
                ["cover_art", "Cover Art"],
                ["playlist", "Media Content"],
            ],
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
        coverArtAction: {
            label: "Press Action",
            inputId: "media-cover-art-action",
            options: [
                ["play_pause", "Play/Pause"],
                ["control_modal", "All Controls"],
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
    var MEDIA_PLAYLIST_SOURCE_DEFINITIONS: any = [
        { value: "spotify", label: "Spotify", prefix: "spotify" },
        { value: "apple_music", label: "Apple Music", prefix: "apple_music" },
        { value: "youtube_music", label: "YouTube Music", prefix: "youtube_music" },
        { value: "plex", label: "Plex", prefix: "plex" },
        { value: "jellyfin", label: "Jellyfin", prefix: "jellyfin" },
        { value: "media_source", label: "Home Assistant Media Source", prefix: "media-source" },
        { value: "url", label: "Web URL", prefix: "" },
        { value: "__custom", label: "Custom / full URI", prefix: "" },
    ];
    function mediaPlaylistSourceOptions(this: any) {
        return MEDIA_PLAYLIST_SOURCE_DEFINITIONS.map(function (this: any, source?: any) {
            return [source.value, source.label];
        });
    }
    function mediaPlaylistSourceDefinition(this: any, value?: any) {
        value = String(value || "");
        for (var i: any = 0; i < MEDIA_PLAYLIST_SOURCE_DEFINITIONS.length; i++) {
            if (MEDIA_PLAYLIST_SOURCE_DEFINITIONS[i].value === value)
                return MEDIA_PLAYLIST_SOURCE_DEFINITIONS[i];
        }
        return MEDIA_PLAYLIST_SOURCE_DEFINITIONS[0];
    }
    function mediaPlaylistContentIdPlaceholder(this: any, source?: any, contentType?: any) {
        source = String(source || "spotify");
        contentType = String(contentType || "playlist");
        if (source === "spotify")
            return "e.g. 1LG2Lnt9EDQS1DqoE8E2uO";
        if (source === "media_source")
            return "e.g. music/morning-mix";
        if (source === "url")
            return "e.g. https://example.com/music/stream.mp3";
        if (source === "__custom")
            return "e.g. spotify:" + contentType + ":1LG2Lnt9EDQS1DqoE8E2uO";
        return "Enter the " + contentType + " ID";
    }
    function parseMediaPlaylistContentId(this: any, value?: any, contentType?: any) {
        value = String(value || "").trim();
        contentType = String(contentType || "playlist").trim() || "playlist";
        if (!value)
            return { source: "spotify", id: "" };
        if (/^https?:\/\//i.test(value))
            return { source: "url", id: value };
        var spotifyMatch: any = value.match(/^spotify:([^:]+):(.+)$/i);
        if (spotifyMatch)
            return { source: "spotify", contentType: spotifyMatch[1], id: spotifyMatch[2] };
        var mediaSourceMatch: any = value.match(/^media-source:\/\/(.+)$/i);
        if (mediaSourceMatch)
            return { source: "media_source", id: mediaSourceMatch[1] };
        var colonMatch: any = value.match(/^([a-z][a-z0-9_-]*):([^:]+):(.+)$/i);
        if (colonMatch) {
            var prefix: any = colonMatch[1].toLowerCase();
            for (var i: any = 0; i < MEDIA_PLAYLIST_SOURCE_DEFINITIONS.length; i++) {
                var source: any = MEDIA_PLAYLIST_SOURCE_DEFINITIONS[i];
                if (source.prefix && source.prefix.toLowerCase() === prefix) {
                    return { source: source.value, contentType: colonMatch[2], id: colonMatch[3] };
                }
            }
        }
        return { source: "__custom", id: value };
    }
    function buildMediaPlaylistContentId(this: any, source?: any, contentType?: any, id?: any) {
        source = String(source || "spotify");
        contentType = String(contentType || "playlist").trim() || "playlist";
        id = String(id || "").trim();
        if (!id)
            return "";
        if (source === "__custom" || source === "url")
            return id;
        if (source === "media_source")
            return "media-source://" + id.replace(/^\/+/, "");
        var definition: any = mediaPlaylistSourceDefinition(source);
        return definition.prefix + ":" + contentType + ":" + id;
    }
    function mediaPlaylistContentTypeKnown(this: any, value?: any) {
        return mediaPlaylistContentTypeOptions().some(function (this: any, option?: any) { return option[0] === value; });
    }
    function mediaPlaylistContentTypeOptions(this: any) {
        return [
            ["playlist", "Playlist"],
            ["music", "Music"],
            ["album", "Album"],
            ["artist", "Artist"],
            ["track", "Track"],
            ["channel", "Channel"],
            ["episode", "Episode"],
            ["podcast", "Podcast"],
            ["tvshow", "TV Show"],
            ["video", "Video"],
            ["movie", "Movie"],
            ["app", "App"],
            ["url", "URL"],
            ["__custom", "Custom"],
        ];
    }
    registerButtonType("media", {
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
            b.sensor = "play_pause";
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
                        if (b.sensor === "cover_art" && mediaLabelIsGenerated(b.label)) {
                            b.label = mediaActionLabel(b.sensor);
                            helpers.saveField("label", b.label);
                        }
                        if (oldMode === "control_modal" && b.sensor !== "control_modal" &&
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
                helpers.renderCardSegmentControl(panel, b, helpers, {
                    segment: Object.assign({}, MEDIA_CARD_METADATA.coverArtAction, {
                        inputId: helpers.idPrefix + "media-cover-art-action",
                        value: function (this: any) { return mediaCoverArtAction(b); },
                        onSelect: function (this: any, button?: any, cardHelpers?: any, value?: any) {
                            setMediaCoverArtAction(button, value);
                            cardHelpers.saveField("options", button.options);
                            renderButtonSettings();
                        },
                    }),
                });
            }
            if (b.sensor === "control_modal") {
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
            if (b.sensor === "volume") {
                helpers.renderCardLargeNumbersToggle(panel, b, helpers, MEDIA_CARD_METADATA);
                var maxField: any = helpers.renderCardNumberField(panel, b, helpers, {
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
                var playlistSourceDisclosure: any = helpers.disclosureSection("Source", helpers.idPrefix + "playlist-source-settings", true);
                var playlistSourceSettings: any = playlistSourceDisclosure.section;
                panel.appendChild(playlistSourceDisclosure.panel);
                var playlistCardSettingsDisclosure: any = helpers.disclosureSection("Card Settings", helpers.idPrefix + "playlist-card-settings", true);
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
                return {
                    buttonClass: "sp-image-card",
                    iconHtml: '<span class="sp-media-cover-preview"></span>',
                    labelHtml: '<span class="sp-btn-label-row"><span class="sp-btn-label">Now Playing</span>' +
                        '<span class="sp-type-badge mdi mdi-' + MEDIA_CARD_METADATA.preview.badge + '"></span></span>',
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
                    iconHtml: progressBg + '<span class="sp-media-now-title">Midnight City</span>',
                    labelHtml: '<span class="sp-btn-label-row"><span class="sp-btn-label sp-media-now-artist">M83</span>' +
                        '<span class="sp-type-badge mdi mdi-' + MEDIA_CARD_METADATA.preview.badge + '"></span></span>',
                };
            }
            return {
                iconHtml: '<span class="sp-btn-icon mdi mdi-' + (b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : info.icon) + '"></span>',
                labelHtml: cardBadgeLabelHtml(helpers, mode === "play_pause" && b.precision === "state" ? "Playing" : label, MEDIA_CARD_METADATA.preview.badge),
            };
        },
    });
    registerButtonType("media_cover_art", {
        label: "Cover Art",
        allowInSubpage: function (this: any) { return cardContractAllowInSubpage("media"); },
        pickerKey: "media_cover_art",
        hidden: true,
        hideLabel: true,
        cardMetadata: MEDIA_CARD_METADATA,
        defaultConfig: function (this: any) {
            var config: any = cardContractDefaultConfig("media");
            config.sensor = "cover_art";
            config.label = "Cover Art";
            return config;
        },
    });
    return {
        "mediaBehaviorSpec": staticGlobal(mediaBehaviorSpec),
        "mediaModeOptionValues": staticGlobal(mediaModeOptionValues),
        "mediaDefaultMode": staticGlobal(mediaDefaultMode),
        "mediaEditorMode": staticGlobal(mediaEditorMode),
        "mediaEditorValidMode": staticGlobal(mediaEditorValidMode),
        "mediaNowPlayingControls": staticGlobal(mediaNowPlayingControls),
        "mediaNowPlayingControlValues": staticGlobal(mediaNowPlayingControlValues),
        "mediaStateDisplayModeSupported": staticGlobal(mediaStateDisplayModeSupported),
        "mediaNowPlayingProgressEnabled": staticGlobal(mediaNowPlayingProgressEnabled),
        "mediaNowPlayingPlayPauseEnabled": staticGlobal(mediaNowPlayingPlayPauseEnabled),
        "mediaLabelIsGenerated": staticGlobal(mediaLabelIsGenerated),
        "MEDIA_CARD_METADATA": liveGlobal(() => MEDIA_CARD_METADATA, (value?: any) => { MEDIA_CARD_METADATA = value; }),
        "MEDIA_PLAYLIST_SOURCE_DEFINITIONS": liveGlobal(() => MEDIA_PLAYLIST_SOURCE_DEFINITIONS, (value?: any) => { MEDIA_PLAYLIST_SOURCE_DEFINITIONS = value; }),
        "mediaPlaylistSourceOptions": staticGlobal(mediaPlaylistSourceOptions),
        "mediaPlaylistSourceDefinition": staticGlobal(mediaPlaylistSourceDefinition),
        "mediaPlaylistContentIdPlaceholder": staticGlobal(mediaPlaylistContentIdPlaceholder),
        "parseMediaPlaylistContentId": staticGlobal(parseMediaPlaylistContentId),
        "buildMediaPlaylistContentId": staticGlobal(buildMediaPlaylistContentId),
        "mediaPlaylistContentTypeKnown": staticGlobal(mediaPlaylistContentTypeKnown),
        "mediaPlaylistContentTypeOptions": staticGlobal(mediaPlaylistContentTypeOptions),
    };
}
