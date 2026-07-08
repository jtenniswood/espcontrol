// ── Media Card Options ─────────────────────────────────────────────
// @web-module-requires: config_option_core

function normalizeMediaVolumeMax(value) {
  value = String(value || "").trim();
  var spec = cardContractOptionSpec("media", MEDIA_VOLUME_MAX_OPTION) || {};
  var fallback = cardContractOptionDefaultValue("media", MEDIA_VOLUME_MAX_OPTION, "100");
  if (!value) return fallback;
  var parsed = parseInt(value, 10);
  if (!isFinite(parsed)) return fallback;
  if (typeof spec.min === "number" && parsed < spec.min) parsed = spec.min;
  if (typeof spec.max === "number" && parsed > spec.max) parsed = spec.max;
  return String(parsed);
}

function normalizeMediaOptions(options, mode) {
  mode = mediaEditorMode(mode);
  if (mode === "control_modal") {
    var controlOut = "";
    var labelMode = normalizeMediaLabelDisplayMode(
      configOptionValue(options, MEDIA_LABEL_DISPLAY_OPTION));
    var numberMode = normalizeMediaNumberDisplayMode(
      configOptionValue(options, MEDIA_NUMBER_DISPLAY_OPTION));
    if (labelMode !== "status") {
      controlOut = setConfigOptionValue(controlOut, MEDIA_LABEL_DISPLAY_OPTION, labelMode);
    }
    if (numberMode !== "icon") {
      controlOut = setConfigOptionValue(controlOut, MEDIA_NUMBER_DISPLAY_OPTION, numberMode);
    }
    var controlMaxVolume = normalizeMediaVolumeMax(configOptionValue(options, MEDIA_VOLUME_MAX_OPTION));
    if (controlMaxVolume !== cardContractOptionDefaultValue("media", MEDIA_VOLUME_MAX_OPTION, "100")) {
      controlOut = setConfigOptionValue(controlOut, MEDIA_VOLUME_MAX_OPTION, controlMaxVolume);
    }
    return controlOut;
  }
  if (mode === "playlist") {
    var playlistOut = "";
    var contentId = configOptionValue(options, MEDIA_PLAYLIST_CONTENT_ID_OPTION);
    if (contentId) playlistOut = setConfigOptionValue(playlistOut, MEDIA_PLAYLIST_CONTENT_ID_OPTION, contentId);
    var defaultType = cardContractOptionDefaultValue("media", MEDIA_PLAYLIST_CONTENT_TYPE_OPTION, "playlist");
    var contentType = configOptionValue(options, MEDIA_PLAYLIST_CONTENT_TYPE_OPTION) || defaultType;
    if (contentType !== defaultType) {
      playlistOut = setConfigOptionValue(playlistOut, MEDIA_PLAYLIST_CONTENT_TYPE_OPTION, contentType);
    }
    var playerSource = configOptionValue(options, MEDIA_PLAYLIST_PLAYER_SOURCE_OPTION);
    if (playerSource) playlistOut = setConfigOptionValue(playlistOut, MEDIA_PLAYLIST_PLAYER_SOURCE_OPTION, playerSource);
    return playlistOut;
  }
  var out = "";
  if (mode === "now_playing") {
    return out;
  }
  if (mode === "cover_art") {
    var action = normalizeMediaCoverArtAction(
      configOptionValue(options, MEDIA_COVER_ART_ACTION_OPTION));
    if (action !== "play_pause") {
      out = setConfigOptionValue(out, MEDIA_COVER_ART_ACTION_OPTION, action);
    }
    return out;
  }
  if (mode !== "volume" && mode !== "position") return out;
  var maxVolume = normalizeMediaVolumeMax(configOptionValue(options, MEDIA_VOLUME_MAX_OPTION));
  if (mode === "volume" && maxVolume !== cardContractOptionDefaultValue("media", MEDIA_VOLUME_MAX_OPTION, "100")) {
    out = setConfigOptionValue(out, MEDIA_VOLUME_MAX_OPTION, maxVolume);
  }
  out = copyLargeNumbersOption(out, options);
  return out;
}

function normalizeMediaCoverArtAction(value) {
  value = String(value || "").trim();
  var spec = cardContractOptionSpec("media", MEDIA_COVER_ART_ACTION_OPTION);
  var values = spec && spec.values ? spec.values : ["play_pause", "control_modal"];
  return values.indexOf(value) >= 0 ? value : "play_pause";
}

function mediaCoverArtAction(b) {
  return normalizeMediaCoverArtAction(
    configOptionValue(b && b.options, MEDIA_COVER_ART_ACTION_OPTION));
}

function setMediaCoverArtAction(b, value) {
  if (!b) return "";
  var normalized = normalizeMediaCoverArtAction(value);
  b.options = setConfigOptionValue(
    b.options,
    MEDIA_COVER_ART_ACTION_OPTION,
    normalized === "play_pause" ? "" : normalized
  );
  b.options = normalizeMediaOptions(b.options, b.sensor);
  return b.options;
}

function normalizeMediaLabelDisplayMode(value) {
  value = String(value || "").trim();
  var spec = cardContractOptionSpec("media", MEDIA_LABEL_DISPLAY_OPTION);
  var values = spec && spec.values ? spec.values : ["label", "status"];
  var fallback = cardContractOptionDefaultValue("media", MEDIA_LABEL_DISPLAY_OPTION, "status");
  return values.indexOf(value) >= 0 ? value : fallback;
}

function normalizeMediaNumberDisplayMode(value) {
  value = String(value || "").trim();
  var spec = cardContractOptionSpec("media", MEDIA_NUMBER_DISPLAY_OPTION);
  var values = spec && spec.values ? spec.values : ["icon", "volume"];
  return values.indexOf(value) >= 0 ? value : "icon";
}

function mediaVolumeMax(b) {
  return normalizeMediaVolumeMax(configOptionValue(b && b.options, MEDIA_VOLUME_MAX_OPTION));
}

function setMediaVolumeMax(b, value) {
  if (!b) return "";
  var normalized = normalizeMediaVolumeMax(value);
  b.options = setConfigOptionValue(
    b.options,
    MEDIA_VOLUME_MAX_OPTION,
    normalized === "100" ? "" : normalized
  );
  b.options = normalizeMediaOptions(b.options, b.sensor);
  return b.options;
}

function mediaLabelDisplayMode(b) {
  return normalizeMediaLabelDisplayMode(
    configOptionValue(b && b.options, MEDIA_LABEL_DISPLAY_OPTION));
}

function setMediaLabelDisplayMode(b, mode) {
  if (!b) return "";
  var normalized = normalizeMediaLabelDisplayMode(mode);
  b.options = setConfigOptionValue(
    b.options,
    MEDIA_LABEL_DISPLAY_OPTION,
    normalized === "status" ? "" : normalized
  );
  b.options = normalizeMediaOptions(b.options, b.sensor);
  return b.options;
}

function mediaNumberDisplayMode(b) {
  return normalizeMediaNumberDisplayMode(
    configOptionValue(b && b.options, MEDIA_NUMBER_DISPLAY_OPTION));
}

function setMediaNumberDisplayMode(b, mode) {
  if (!b) return "";
  var normalized = normalizeMediaNumberDisplayMode(mode);
  b.options = setConfigOptionValue(
    b.options,
    MEDIA_NUMBER_DISPLAY_OPTION,
    normalized === "icon" ? "" : normalized
  );
  b.options = normalizeMediaOptions(b.options, b.sensor);
  return b.options;
}

function mediaCoverArtEnabled(b) {
  return configOptionEnabled(b && b.options, MEDIA_COVER_ART_OPTION);
}

function setMediaCoverArtEnabled(b, enabled) {
  if (!b) return "";
  b.options = setConfigOption(b.options, MEDIA_COVER_ART_OPTION, !!enabled);
  b.options = normalizeMediaOptions(b.options, b.sensor);
  return b.options;
}

function mediaPlaylistContentId(b) {
  return configOptionValue(b && b.options, MEDIA_PLAYLIST_CONTENT_ID_OPTION);
}

function mediaPlaylistContentType(b) {
  return configOptionValue(b && b.options, MEDIA_PLAYLIST_CONTENT_TYPE_OPTION) ||
    cardContractOptionDefaultValue("media", MEDIA_PLAYLIST_CONTENT_TYPE_OPTION, "playlist");
}

function setMediaPlaylistContentId(b, value) {
  if (!b) return "";
  b.options = setConfigOptionValue(b.options, MEDIA_PLAYLIST_CONTENT_ID_OPTION, value || "");
  b.options = normalizeMediaOptions(b.options, b.sensor);
  return b.options;
}

function setMediaPlaylistContentType(b, value) {
  if (!b) return "";
  var defaultType = cardContractOptionDefaultValue("media", MEDIA_PLAYLIST_CONTENT_TYPE_OPTION, "playlist");
  value = String(value || "").trim() || defaultType;
  b.options = setConfigOptionValue(
    b.options,
    MEDIA_PLAYLIST_CONTENT_TYPE_OPTION,
    value === defaultType ? "" : value);
  b.options = normalizeMediaOptions(b.options, b.sensor);
  return b.options;
}

function mediaPlaylistPlayerSource(b) {
  return configOptionValue(b && b.options, MEDIA_PLAYLIST_PLAYER_SOURCE_OPTION);
}

function setMediaPlaylistPlayerSource(b, value) {
  if (!b) return "";
  b.options = setConfigOptionValue(b.options, MEDIA_PLAYLIST_PLAYER_SOURCE_OPTION, value || "");
  b.options = normalizeMediaOptions(b.options, b.sensor);
  return b.options;
}
