import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installConfigMediaOptionsModule(): GlobalDescriptors {
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
        "normalizeMediaVolumeMax": staticGlobal(normalizeMediaVolumeMax),
        "normalizeMediaOptions": staticGlobal(normalizeMediaOptions),
        "normalizeMediaLabelDisplayMode": staticGlobal(normalizeMediaLabelDisplayMode),
        "normalizeMediaNumberDisplayMode": staticGlobal(normalizeMediaNumberDisplayMode),
        "mediaVolumeMax": staticGlobal(mediaVolumeMax),
        "setMediaVolumeMax": staticGlobal(setMediaVolumeMax),
        "mediaLabelDisplayMode": staticGlobal(mediaLabelDisplayMode),
        "setMediaLabelDisplayMode": staticGlobal(setMediaLabelDisplayMode),
        "mediaNumberDisplayMode": staticGlobal(mediaNumberDisplayMode),
        "setMediaNumberDisplayMode": staticGlobal(setMediaNumberDisplayMode),
        "mediaPlaylistContentId": staticGlobal(mediaPlaylistContentId),
        "mediaPlaylistContentType": staticGlobal(mediaPlaylistContentType),
        "setMediaPlaylistContentId": staticGlobal(setMediaPlaylistContentId),
        "setMediaPlaylistContentType": staticGlobal(setMediaPlaylistContentType),
        "mediaPlaylistPlayerSource": staticGlobal(mediaPlaylistPlayerSource),
        "setMediaPlaylistPlayerSource": staticGlobal(setMediaPlaylistPlayerSource),
    };
}
