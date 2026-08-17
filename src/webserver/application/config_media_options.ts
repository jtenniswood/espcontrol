import type { DeviceConfig } from "../state/types";
import {
    configOptionEnabled,
    configOptionValue,
    setConfigOption,
    setConfigOptionValue,
} from "../model/config_primitives";
import { cardContractCard } from "../generated/card_contract";
import {
    MEDIA_COVER_ART_DETAILS_OPTION,
    MEDIA_COVER_ART_SECONDARY_ENTITY_OPTION,
    MEDIA_LABEL_DISPLAY_OPTION,
    MEDIA_NUMBER_DISPLAY_OPTION,
    MEDIA_PLAYLIST_CONTENT_ID_OPTION,
    MEDIA_PLAYLIST_CONTENT_TYPE_OPTION,
    MEDIA_PLAYLIST_PLAYER_SOURCE_OPTION,
    MEDIA_SPEAKER_GROUP_ENTITY_OPTION,
    MEDIA_VOLUME_MAX_OPTION,
    cardContractOptionDefaultValue,
    cardContractOptionSpec,
    copyLargeNumbersOption,
} from "./config_option_core";
export function createConfigMediaOptionsFeature(
    deviceProfile: Pick<DeviceConfig, "disabledCardTypes">,
) {
    function mediaBehaviorSpec(this: any) {
        var card: any = cardContractCard("media");
        return card && card.behavior && card.behavior.media || {};
    }
    function mediaCoverArtCardsSupported(this: any) {
        var disabled: readonly string[] = deviceProfile.disabledCardTypes || [];
        return disabled.indexOf("media_cover_art") === -1;
    }
    function mediaModeOptionValues(this: any) {
        var spec: any = cardContractOptionSpec("media", "media_mode");
        var values: any = spec && spec.values ? spec.values.slice() :
            ["control_modal", "cover_art", "speaker_group", "play_pause", "previous", "next", "volume", "position", "now_playing", "playlist"];
        return mediaCoverArtCardsSupported() ? values : values.filter(function (this: any, value?: any) {
            return value !== "cover_art";
        });
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
    function mediaNowPlayingControlValues(this: any) {
        var spec: any = cardContractOptionSpec("media", "media_now_playing_controls");
        return spec && spec.values ? spec.values.slice() : ["", "progress", "play_pause"];
    }
    function mediaNowPlayingControls(this: any, button?: any) {
        if (!button || button.sensor !== "now_playing")
            return "";
        return mediaNowPlayingControlValues().indexOf(button.precision || "") >= 0 ? button.precision : "";
    }
    function mediaStateDisplayModeSupported(this: any, mode?: any) {
        var modes: any = mediaBehaviorSpec().stateDisplayModes || ["play_pause", "position"];
        return modes.indexOf(mediaEditorMode(mode)) >= 0;
    }
    const mediaPlaylistSourceDefinitions: any = [
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
        return mediaPlaylistSourceDefinitions.map(function (source?: any) { return [source.value, source.label]; });
    }
    function mediaPlaylistSourceDefinition(this: any, value?: any) {
        value = String(value || "");
        for (var i: any = 0; i < mediaPlaylistSourceDefinitions.length; i++) {
            if (mediaPlaylistSourceDefinitions[i].value === value)
                return mediaPlaylistSourceDefinitions[i];
        }
        return mediaPlaylistSourceDefinitions[0];
    }
    function mediaPlaylistContentIdPlaceholder(this: any, source?: any, contentType?: any) {
        source = String(source || "spotify");
        contentType = String(contentType || "playlist");
        if (source === "spotify") return "e.g. 1LG2Lnt9EDQS1DqoE8E2uO";
        if (source === "media_source") return "e.g. music/morning-mix";
        if (source === "url") return "e.g. https://example.com/music/stream.mp3";
        if (source === "__custom") return "e.g. spotify:" + contentType + ":1LG2Lnt9EDQS1DqoE8E2uO";
        return "Enter the " + contentType + " ID";
    }
    function parseMediaPlaylistContentId(this: any, value?: any, contentType?: any) {
        value = String(value || "").trim();
        contentType = String(contentType || "playlist").trim() || "playlist";
        if (!value) return { source: "spotify", id: "" };
        if (/^https?:\/\//i.test(value)) return { source: "url", id: value };
        var spotifyMatch: any = value.match(/^spotify:([^:]+):(.+)$/i);
        if (spotifyMatch) return { source: "spotify", contentType: spotifyMatch[1], id: spotifyMatch[2] };
        var mediaSourceMatch: any = value.match(/^media-source:\/\/(.+)$/i);
        if (mediaSourceMatch) return { source: "media_source", id: mediaSourceMatch[1] };
        var colonMatch: any = value.match(/^([a-z][a-z0-9_-]*):([^:]+):(.+)$/i);
        if (colonMatch) {
            var prefix: any = colonMatch[1].toLowerCase();
            for (var i: any = 0; i < mediaPlaylistSourceDefinitions.length; i++) {
                var source: any = mediaPlaylistSourceDefinitions[i];
                if (source.prefix && source.prefix.toLowerCase() === prefix)
                    return { source: source.value, contentType: colonMatch[2], id: colonMatch[3] };
            }
        }
        return { source: "__custom", id: value };
    }
    function buildMediaPlaylistContentId(this: any, source?: any, contentType?: any, id?: any) {
        source = String(source || "spotify");
        contentType = String(contentType || "playlist").trim() || "playlist";
        id = String(id || "").trim();
        if (!id) return "";
        if (source === "__custom" || source === "url") return id;
        if (source === "media_source") return "media-source://" + id.replace(/^\/+/, "");
        var definition: any = mediaPlaylistSourceDefinition(source);
        return definition.prefix + ":" + contentType + ":" + id;
    }
    function mediaPlaylistContentTypeOptions(this: any) {
        return [["playlist", "Playlist"], ["music", "Music"], ["album", "Album"],
            ["artist", "Artist"], ["track", "Track"], ["channel", "Channel"],
            ["episode", "Episode"], ["podcast", "Podcast"], ["tvshow", "TV Show"],
            ["video", "Video"], ["movie", "Movie"], ["app", "App"], ["url", "URL"],
            ["__custom", "Custom"]];
    }
    function mediaPlaylistContentTypeKnown(this: any, value?: any) {
        return mediaPlaylistContentTypeOptions().some(function (option?: any) { return option[0] === value; });
    }
    // ── Media Card Options ─────────────────────────────────────────────
    function normalizeMediaVolumeMax(this: any, value?: any) {
        value = String(value || "").trim();
        var spec: any = cardContractOptionSpec("media", MEDIA_VOLUME_MAX_OPTION) || {};
        var fallback: any = cardContractOptionDefaultValue("media", MEDIA_VOLUME_MAX_OPTION, "100");
        if (!value)
            return fallback;
        var parsed: any = parseInt(value, 10);
        if (!isFinite(parsed))
            return fallback;
        if (typeof spec.min === "number" && parsed < spec.min)
            parsed = spec.min;
        if (typeof spec.max === "number" && parsed > spec.max)
            parsed = spec.max;
        return String(parsed);
    }
    function normalizeMediaOptions(this: any, options?: any, mode?: any) {
        mode = mediaEditorMode(mode);
        if (mode === "control_modal") {
            var controlOut: any = "";
            var controlGroupEntity: any = normalizeMediaSpeakerGroupEntity(configOptionValue(options, MEDIA_SPEAKER_GROUP_ENTITY_OPTION));
            if (controlGroupEntity) {
                controlOut = setConfigOptionValue(controlOut, MEDIA_SPEAKER_GROUP_ENTITY_OPTION, controlGroupEntity);
            }
            var labelMode: any = normalizeMediaLabelDisplayMode(configOptionValue(options, MEDIA_LABEL_DISPLAY_OPTION));
            var numberMode: any = normalizeMediaNumberDisplayMode(configOptionValue(options, MEDIA_NUMBER_DISPLAY_OPTION));
            if (labelMode !== "status") {
                controlOut = setConfigOptionValue(controlOut, MEDIA_LABEL_DISPLAY_OPTION, labelMode);
            }
            if (numberMode !== "icon") {
                controlOut = setConfigOptionValue(controlOut, MEDIA_NUMBER_DISPLAY_OPTION, numberMode);
            }
            var controlMaxVolume: any = normalizeMediaVolumeMax(configOptionValue(options, MEDIA_VOLUME_MAX_OPTION));
            if (controlMaxVolume !== cardContractOptionDefaultValue("media", MEDIA_VOLUME_MAX_OPTION, "100")) {
                controlOut = setConfigOptionValue(controlOut, MEDIA_VOLUME_MAX_OPTION, controlMaxVolume);
            }
            return controlOut;
        }
        if (mode === "speaker_group") {
            var groupOut: any = "";
            var groupEntity: any = normalizeMediaSpeakerGroupEntity(configOptionValue(options, MEDIA_SPEAKER_GROUP_ENTITY_OPTION));
            if (groupEntity) {
                groupOut = setConfigOptionValue(groupOut, MEDIA_SPEAKER_GROUP_ENTITY_OPTION, groupEntity);
            }
            var groupMaxVolume: any = normalizeMediaVolumeMax(configOptionValue(options, MEDIA_VOLUME_MAX_OPTION));
            if (groupMaxVolume !== cardContractOptionDefaultValue("media", MEDIA_VOLUME_MAX_OPTION, "100")) {
                groupOut = setConfigOptionValue(groupOut, MEDIA_VOLUME_MAX_OPTION, groupMaxVolume);
            }
            return groupOut;
        }
        if (mode === "playlist") {
            var playlistOut: any = "";
            var contentId: any = configOptionValue(options, MEDIA_PLAYLIST_CONTENT_ID_OPTION).trim();
            if (contentId)
                playlistOut = setConfigOptionValue(playlistOut, MEDIA_PLAYLIST_CONTENT_ID_OPTION, contentId);
            var defaultType: any = cardContractOptionDefaultValue("media", MEDIA_PLAYLIST_CONTENT_TYPE_OPTION, "playlist");
            var contentType: any = configOptionValue(options, MEDIA_PLAYLIST_CONTENT_TYPE_OPTION).trim() || defaultType;
            if (contentType !== defaultType) {
                playlistOut = setConfigOptionValue(playlistOut, MEDIA_PLAYLIST_CONTENT_TYPE_OPTION, contentType);
            }
            var playerSource: any = configOptionValue(options, MEDIA_PLAYLIST_PLAYER_SOURCE_OPTION).trim();
            if (playerSource)
                playlistOut = setConfigOptionValue(playlistOut, MEDIA_PLAYLIST_PLAYER_SOURCE_OPTION, playerSource);
            return playlistOut;
        }
        if (mode === "cover_art") {
            var coverArtOut: any = "";
            if (configOptionEnabled(options, MEDIA_COVER_ART_DETAILS_OPTION)) {
                coverArtOut = setConfigOption(coverArtOut, MEDIA_COVER_ART_DETAILS_OPTION, true);
            }
            var secondaryEntity: any = configOptionValue(options, MEDIA_COVER_ART_SECONDARY_ENTITY_OPTION).trim();
            if (secondaryEntity) {
                coverArtOut = setConfigOptionValue(coverArtOut, MEDIA_COVER_ART_SECONDARY_ENTITY_OPTION, secondaryEntity);
            }
            return coverArtOut;
        }
        if (mode !== "volume" && mode !== "position")
            return "";
        var out: any = "";
        var maxVolume: any = normalizeMediaVolumeMax(configOptionValue(options, MEDIA_VOLUME_MAX_OPTION));
        if (mode === "volume" && maxVolume !== cardContractOptionDefaultValue("media", MEDIA_VOLUME_MAX_OPTION, "100")) {
            out = setConfigOptionValue(out, MEDIA_VOLUME_MAX_OPTION, maxVolume);
        }
        out = copyLargeNumbersOption(out, options);
        return out;
    }
    function mediaCoverArtDetailsEnabled(this: any, b?: any) {
        return !!(b && configOptionEnabled(b.options, MEDIA_COVER_ART_DETAILS_OPTION));
    }
    function setMediaCoverArtDetailsEnabled(this: any, b?: any, enabled?: any) {
        if (!b)
            return "";
        b.options = setConfigOption(b.options, MEDIA_COVER_ART_DETAILS_OPTION, !!enabled);
        b.options = normalizeMediaOptions(b.options, b.sensor);
        return b.options;
    }
    function mediaCoverArtSecondaryEntity(this: any, b?: any) {
        return configOptionValue(b && b.options, MEDIA_COVER_ART_SECONDARY_ENTITY_OPTION).trim();
    }
    function setMediaCoverArtSecondaryEntity(this: any, b?: any, entity?: any) {
        if (!b)
            return "";
        b.options = setConfigOptionValue(
            b.options, MEDIA_COVER_ART_SECONDARY_ENTITY_OPTION, String(entity || "").trim());
        b.options = normalizeMediaOptions(b.options, b.sensor);
        return b.options;
    }
    function normalizeMediaLabelDisplayMode(this: any, value?: any) {
        value = String(value || "").trim();
        var spec: any = cardContractOptionSpec("media", MEDIA_LABEL_DISPLAY_OPTION);
        var values: any = spec && spec.values ? spec.values : ["label", "status"];
        var fallback: any = cardContractOptionDefaultValue("media", MEDIA_LABEL_DISPLAY_OPTION, "status");
        return values.indexOf(value) >= 0 ? value : fallback;
    }
    function normalizeMediaNumberDisplayMode(this: any, value?: any) {
        value = String(value || "").trim();
        var spec: any = cardContractOptionSpec("media", MEDIA_NUMBER_DISPLAY_OPTION);
        var values: any = spec && spec.values ? spec.values : ["icon", "volume"];
        return values.indexOf(value) >= 0 ? value : "icon";
    }
    function mediaVolumeMax(this: any, b?: any) {
        return normalizeMediaVolumeMax(configOptionValue(b && b.options, MEDIA_VOLUME_MAX_OPTION));
    }
    function normalizeMediaSpeakerGroupEntity(this: any, value?: any) {
        return String(value || "").trim();
    }
    function mediaSpeakerGroupEntity(this: any, b?: any) {
        return normalizeMediaSpeakerGroupEntity(configOptionValue(b && b.options, MEDIA_SPEAKER_GROUP_ENTITY_OPTION));
    }
    function setMediaSpeakerGroupEntity(this: any, b?: any, value?: any) {
        if (!b)
            return "";
        b.options = setConfigOptionValue(b.options, MEDIA_SPEAKER_GROUP_ENTITY_OPTION, normalizeMediaSpeakerGroupEntity(value));
        b.options = normalizeMediaOptions(b.options, b.sensor);
        return b.options;
    }
    function setMediaVolumeMax(this: any, b?: any, value?: any) {
        if (!b)
            return "";
        var normalized: any = normalizeMediaVolumeMax(value);
        b.options = setConfigOptionValue(b.options, MEDIA_VOLUME_MAX_OPTION, normalized === "100" ? "" : normalized);
        b.options = normalizeMediaOptions(b.options, b.sensor);
        return b.options;
    }
    function mediaLabelDisplayMode(this: any, b?: any) {
        return normalizeMediaLabelDisplayMode(configOptionValue(b && b.options, MEDIA_LABEL_DISPLAY_OPTION));
    }
    function setMediaLabelDisplayMode(this: any, b?: any, mode?: any) {
        if (!b)
            return "";
        var normalized: any = normalizeMediaLabelDisplayMode(mode);
        b.options = setConfigOptionValue(b.options, MEDIA_LABEL_DISPLAY_OPTION, normalized === "status" ? "" : normalized);
        b.options = normalizeMediaOptions(b.options, b.sensor);
        return b.options;
    }
    function mediaNumberDisplayMode(this: any, b?: any) {
        return normalizeMediaNumberDisplayMode(configOptionValue(b && b.options, MEDIA_NUMBER_DISPLAY_OPTION));
    }
    function setMediaNumberDisplayMode(this: any, b?: any, mode?: any) {
        if (!b)
            return "";
        var normalized: any = normalizeMediaNumberDisplayMode(mode);
        b.options = setConfigOptionValue(b.options, MEDIA_NUMBER_DISPLAY_OPTION, normalized === "icon" ? "" : normalized);
        b.options = normalizeMediaOptions(b.options, b.sensor);
        return b.options;
    }
    function mediaPlaylistContentId(this: any, b?: any) {
        return configOptionValue(b && b.options, MEDIA_PLAYLIST_CONTENT_ID_OPTION);
    }
    function mediaPlaylistContentType(this: any, b?: any) {
        return configOptionValue(b && b.options, MEDIA_PLAYLIST_CONTENT_TYPE_OPTION) ||
            cardContractOptionDefaultValue("media", MEDIA_PLAYLIST_CONTENT_TYPE_OPTION, "playlist");
    }
    function setMediaPlaylistContentId(this: any, b?: any, value?: any) {
        if (!b)
            return "";
        b.options = setConfigOptionValue(b.options, MEDIA_PLAYLIST_CONTENT_ID_OPTION, value || "");
        b.options = normalizeMediaOptions(b.options, b.sensor);
        return b.options;
    }
    function setMediaPlaylistContentType(this: any, b?: any, value?: any) {
        if (!b)
            return "";
        var defaultType: any = cardContractOptionDefaultValue("media", MEDIA_PLAYLIST_CONTENT_TYPE_OPTION, "playlist");
        value = String(value || "").trim() || defaultType;
        b.options = setConfigOptionValue(b.options, MEDIA_PLAYLIST_CONTENT_TYPE_OPTION, value === defaultType ? "" : value);
        b.options = normalizeMediaOptions(b.options, b.sensor);
        return b.options;
    }
    function mediaPlaylistPlayerSource(this: any, b?: any) {
        return configOptionValue(b && b.options, MEDIA_PLAYLIST_PLAYER_SOURCE_OPTION);
    }
    function setMediaPlaylistPlayerSource(this: any, b?: any, value?: any) {
        if (!b)
            return "";
        b.options = setConfigOptionValue(b.options, MEDIA_PLAYLIST_PLAYER_SOURCE_OPTION, value || "");
        b.options = normalizeMediaOptions(b.options, b.sensor);
        return b.options;
    }
    return {
        mediaBehaviorSpec,
        mediaCoverArtCardsSupported,
        mediaModeOptionValues,
        mediaDefaultMode,
        mediaEditorMode,
        mediaEditorValidMode,
        mediaNowPlayingControlValues,
        mediaNowPlayingControls,
        mediaStateDisplayModeSupported,
        mediaPlaylistSourceDefinitions,
        mediaPlaylistSourceOptions,
        mediaPlaylistSourceDefinition,
        mediaPlaylistContentIdPlaceholder,
        parseMediaPlaylistContentId,
        buildMediaPlaylistContentId,
        mediaPlaylistContentTypeKnown,
        mediaPlaylistContentTypeOptions,
        normalizeMediaVolumeMax,
        normalizeMediaOptions,
        mediaCoverArtDetailsEnabled,
        setMediaCoverArtDetailsEnabled,
        mediaCoverArtSecondaryEntity,
        setMediaCoverArtSecondaryEntity,
        normalizeMediaLabelDisplayMode,
        normalizeMediaNumberDisplayMode,
        mediaVolumeMax,
        setMediaVolumeMax,
        normalizeMediaSpeakerGroupEntity,
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
    };
}

export type ConfigMediaOptionsFeature = ReturnType<typeof createConfigMediaOptionsFeature>;
