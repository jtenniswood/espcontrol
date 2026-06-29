// Media player card: playback buttons, volume, track position, or now-playing details.
function mediaBehaviorSpec() {
  var card = cardContractCard("media");
  return card && card.behavior && card.behavior.media || {};
}

function mediaModeOptionValues() {
  var spec = cardContractOptionSpec("media", "media_mode");
  return spec && spec.values ? spec.values.slice() :
    ["play_pause", "previous", "next", "volume", "position", "now_playing"];
}

function mediaDefaultMode() {
  return mediaBehaviorSpec().defaultMode || "play_pause";
}

function mediaEditorMode(value) {
  value = String(value || "");
  var legacy = mediaBehaviorSpec().legacyModes || {};
  value = legacy[value] || value;
  return mediaModeOptionValues().indexOf(value) >= 0 ? value : mediaDefaultMode();
}

function mediaEditorValidMode(value) {
  return mediaEditorMode(value);
}

function mediaNowPlayingControls(b) {
  if (!b || b.sensor !== "now_playing") return "";
  return mediaNowPlayingControlValues().indexOf(b.precision || "") >= 0 ? b.precision : "";
}

function mediaNowPlayingControlValues() {
  var spec = cardContractOptionSpec("media", "media_now_playing_controls");
  return spec && spec.values ? spec.values.slice() : ["", "progress", "play_pause"];
}

function mediaStateDisplayModeSupported(mode) {
  var modes = mediaBehaviorSpec().stateDisplayModes || ["play_pause", "position"];
  return modes.indexOf(mediaEditorMode(mode)) >= 0;
}

function mediaNowPlayingProgressEnabled(b) {
  return mediaNowPlayingControls(b) === "progress";
}

function mediaNowPlayingPlayPauseEnabled(b) {
  return mediaNowPlayingControls(b) === "play_pause";
}

var MEDIA_CARD_METADATA = {
  mode: {
    label: "Type",
    idSuffix: "media-mode",
    options: [
      ["play_pause", "Play/Pause Button"],
      ["previous", "Previous Button"],
      ["next", "Next Button"],
      ["volume", "Volume Button"],
      ["position", "Track Position"],
      ["now_playing", "Now Playing"],
      ["playlist", "Playlist Button"],
    ],
    value: function (b) {
      return mediaEditorValidMode(b.sensor);
    },
  },
  entity: {
    label: "Entity",
    idSuffix: "entity",
    placeholder: "e.g. media_player.living_room",
    domains: function () { return cardContractDomains("media"); },
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
  largeNumbers: {
    label: "Large Media Numbers",
    idSuffix: "large-media-numbers",
    supported: function (b) {
      var mode = mediaEditorMode(b && b.sensor);
      return mode === "volume" || mode === "position";
    },
  },
  preview: {
    badge: "speaker",
  },
};

function mediaPlaylistContentIdPlaceholder() {
  return "e.g. spotify:playlist:1LG2Lnt9EDQS1DqoE8E2uO";
}

function mediaPlaylistContentTypeKnown(value) {
  return mediaPlaylistContentTypeOptions().some(function (option) { return option[0] === value; });
}

function mediaPlaylistContentTypeOptions() {
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
  label: function () { return cardContractCardLabel("media"); },
  allowInSubpage: function () { return cardContractAllowInSubpage("media"); },
  pickerKey: function () { return cardContractPickerKey("media"); },
  hidden: function () { return cardContractHidden("media"); },
  hideLabel: true,
  labelPlaceholder: "e.g. Living Room Speaker",
  defaultConfig: function () { return cardContractDefaultConfig("media"); },
  cardMetadata: MEDIA_CARD_METADATA,
  onSelect: function (b) {
    b.entity = "";
    b.sensor = "play_pause";
    b.unit = "";
    b.precision = (b.sensor === "play_pause" || b.sensor === "position") && b.precision === "state" ? "state" : "";
    b.icon = "Auto";
    b.icon_on = "Auto";
    b.options = "";
  },
  renderSettingsBeforeLabel: function (panel, b, slot, helpers) {
    function validMode(value) {
      return mediaEditorValidMode(value);
    }

    function mediaDefaultIcon(value) {
      var mode = mediaEditorMode(value);
      if (mode === "previous") return "Skip Previous";
      if (mode === "next") return "Skip Next";
      if (mode === "volume") return "Volume High";
      if (mode === "position") return "Progress Clock";
      if (mode === "now_playing") return "Music";
      if (mode === "playlist") return "Music";
      return "Play Pause";
    }

    function isMediaDefaultIcon(value, icon) {
      if (!icon || icon === "Auto") return true;
      if (value === "controls" && icon === "Speaker") return true;
      return icon === mediaDefaultIcon(value);
    }

    function mediaActionLabel(value) {
      var mode = mediaEditorMode(value);
      if (mode === "previous") return "Previous";
      if (mode === "next") return "Next";
      if (mode === "volume") return "Volume";
      if (mode === "play_pause") return "Play/Pause";
      if (mode === "playlist") return "Playlist";
      return "";
    }

    var rawMode = b.sensor;
    b.sensor = validMode(b.sensor);
    if (rawMode === "controls" && isMediaDefaultIcon(rawMode, b.icon)) b.icon = "Auto";

    helpers.renderCardModeSelector(panel, b, helpers, Object.assign({}, MEDIA_CARD_METADATA, {
      mode: Object.assign({}, MEDIA_CARD_METADATA.mode, {
        onChange: function () {
          var oldMode = b.sensor;
          b.sensor = validMode(this.value);
          if (isMediaDefaultIcon(oldMode, b.icon)) {
            b.icon = "Auto";
            helpers.saveField("icon", b.icon);
          }
          if (b.sensor === "now_playing") {
            b.precision = mediaNowPlayingControls(b);
            helpers.saveField("precision", b.precision);
          } else if (b.sensor === "play_pause" || b.sensor === "position") {
            b.precision = b.precision === "state" ? "state" : "";
            helpers.saveField("precision", b.precision);
          } else if (b.precision) {
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
            var oldPlaylistDefaultLabel = mediaActionLabel(oldMode);
            if (!b.label || b.label === oldPlaylistDefaultLabel || b.label === "Media") {
              b.label = mediaActionLabel(b.sensor);
              helpers.saveField("label", b.label);
            }
            b.icon = mediaDefaultIcon(b.sensor);
            helpers.saveField("icon", b.icon);
          }
          if (b.sensor === "volume") {
            var oldDefaultLabel = mediaActionLabel(oldMode);
            if (!b.label || b.label === oldDefaultLabel || b.label === "Media") {
              b.label = mediaActionLabel(b.sensor);
              helpers.saveField("label", b.label);
            }
            b.icon = "Auto";
            helpers.saveField("icon", b.icon);
          }
          var normalizedOptions = normalizeMediaOptions(b.options, b.sensor);
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
  renderSettings: function (panel, b, slot, helpers) {
    function validMode(value) {
      return mediaEditorValidMode(value);
    }

    b.sensor = validMode(b.sensor);
    b.unit = "";
    b.precision = b.sensor === "now_playing"
      ? mediaNowPlayingControls(b)
      : ((b.sensor === "play_pause" || b.sensor === "position") && b.precision === "state" ? "state" : "");
    b.icon_on = "Auto";
    var normalizedOptions = normalizeMediaOptions(b.options, b.sensor);
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
      if (!b.label || b.label === "Media") b.label = "Volume";
      if (b.icon !== "Auto") {
        b.icon = "Auto";
        helpers.saveField("icon", b.icon);
      }
    }
    if (b.sensor === "play_pause" && b.icon !== "Auto") {
      b.icon = "Auto";
      helpers.saveField("icon", b.icon);
    }
    if (b.sensor === "playlist") {
      if (!b.label || b.label === "Media") b.label = "Playlist";
      if (!b.icon || b.icon === "Auto") {
        b.icon = "Music";
        helpers.saveField("icon", b.icon);
      }
    }
    if (b.sensor === "previous" && (!b.icon || b.icon === "Auto")) b.icon = "Skip Previous";
    if (b.sensor === "next" && (!b.icon || b.icon === "Auto")) b.icon = "Skip Next";

    helpers.renderCardEntityField(panel, b, helpers, b.sensor === "playlist"
      ? {
        entity: Object.assign({}, MEDIA_CARD_METADATA.entity, {
          label: "Speaker Entity",
          requiredMessage: "Add a speaker entity before saving.",
        }),
      }
      : MEDIA_CARD_METADATA);

    var displayMode = helpers.renderCardSegmentControl(panel, b, helpers, {
      segment: Object.assign({}, MEDIA_CARD_METADATA.displayMode, {
        inputId: helpers.idPrefix + "media-display",
        value: function () { return b.precision === "state" ? "state" : ""; },
        onSelect: function (button, cardHelpers, value) { setDisplayMode(value); },
      }),
    });
    var displayField = displayMode.segment.parentNode;
    var labelModeBtn = displayMode.buttons[""];
    var stateModeBtn = displayMode.buttons.state;
    function syncDisplayField() {
      if (b.sensor === "play_pause" || b.sensor === "position") {
        displayField.style.display = "";
      } else {
        displayField.style.display = "none";
        if (b.precision && !mediaNowPlayingControls(b)) {
          b.precision = "";
          helpers.saveField("precision", "");
        }
      }
      labelModeBtn.classList.toggle("active", b.precision !== "state");
      stateModeBtn.classList.toggle("active", b.precision === "state");
    }

    function setDisplayMode(mode) {
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
      var controls = helpers.renderCardSegmentControl(panel, b, helpers, {
        segment: Object.assign({}, MEDIA_CARD_METADATA.nowPlayingControls, {
          inputId: helpers.idPrefix + "media-controls",
          value: function () { return mediaNowPlayingControls(b); },
          onSelect: function (button, cardHelpers, value) {
            button.precision = value;
            cardHelpers.saveField("precision", button.precision);
            renderButtonSettings();
          },
        }),
      });
      controls.segment.classList.add("sp-segment-scroll");
    }

    if (b.sensor === "now_playing") {
      var controlsMode = mediaNowPlayingControls(b);
      if (b.precision !== controlsMode) {
        b.precision = controlsMode;
        helpers.saveField("precision", b.precision);
      }
    }

    if (b.sensor !== "now_playing" &&
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
      var maxField = helpers.renderCardNumberField(panel, b, helpers, {
        label: "Maximum Volume",
        idSuffix: "volume-max",
        min: 1,
        max: 100,
        step: 1,
        placeholder: "100",
        value: function () {
          var maxVolume = mediaVolumeMax(b);
          return maxVolume === "100" ? "" : maxVolume;
        },
      });
      maxField.input.addEventListener("change", function () {
        setMediaVolumeMax(b, maxField.input.value);
        maxField.input.value = mediaVolumeMax(b) === "100" ? "" : mediaVolumeMax(b);
        helpers.saveField("options", b.options);
      });
    }

    if (b.sensor === "playlist") {
      var playlistInfo = document.createElement("div");
      playlistInfo.className = "sp-info-panel";
      playlistInfo.setAttribute("role", "note");
      var playlistInfoIcon = document.createElement("span");
      playlistInfoIcon.className = "mdi mdi-information-outline";
      playlistInfoIcon.setAttribute("aria-hidden", "true");
      var playlistInfoText = document.createElement("span");
      playlistInfoText.appendChild(document.createTextNode("Need help finding the media content ID? "));
      var playlistInfoLink = document.createElement("a");
      playlistInfoLink.href = "https://jtenniswood.github.io/espcontrol/card-types/media/#playlist-button";
      playlistInfoLink.target = "_blank";
      playlistInfoLink.rel = "noopener";
      playlistInfoLink.textContent = "Learn how to configure playlist buttons";
      playlistInfoText.appendChild(playlistInfoLink);
      playlistInfoText.appendChild(document.createTextNode("."));
      playlistInfo.appendChild(playlistInfoIcon);
      playlistInfo.appendChild(playlistInfoText);
      panel.appendChild(playlistInfo);

      var contentIdField = helpers.textField(
        "Media Content ID / URI",
        helpers.idPrefix + "playlist-content-id",
        mediaPlaylistContentId(b),
        mediaPlaylistContentIdPlaceholder(),
        "",
        false);
      panel.appendChild(contentIdField.field);
      helpers.requireField(contentIdField.input, "Add a media content ID before saving.");
      contentIdField.input.addEventListener("change", function () {
        setMediaPlaylistContentId(b, contentIdField.input.value);
        contentIdField.input.value = mediaPlaylistContentId(b);
        helpers.saveField("options", b.options);
      });

      var playlistContentType = mediaPlaylistContentType(b);
      var contentTypeField = helpers.selectField(
        "Media Content Type",
        helpers.idPrefix + "playlist-content-type",
        mediaPlaylistContentTypeOptions(),
        mediaPlaylistContentTypeKnown(playlistContentType) ? playlistContentType : "__custom");
      panel.appendChild(contentTypeField.field);

      var customContentTypeField = helpers.textField(
        "Custom Media Content Type",
        helpers.idPrefix + "playlist-content-type-custom",
        mediaPlaylistContentTypeKnown(playlistContentType) ? "" : playlistContentType,
        "e.g. favorite",
        "",
        false);
      panel.appendChild(customContentTypeField.field);

      function updateCustomContentTypeVisibility() {
        customContentTypeField.field.hidden = contentTypeField.select.value !== "__custom";
      }

      contentTypeField.select.addEventListener("change", function () {
        if (contentTypeField.select.value === "__custom") {
          setMediaPlaylistContentType(b, customContentTypeField.input.value);
        } else {
          setMediaPlaylistContentType(b, contentTypeField.select.value);
        }
        helpers.saveField("options", b.options);
        updateCustomContentTypeVisibility();
      });
      customContentTypeField.input.addEventListener("change", function () {
        setMediaPlaylistContentType(b, customContentTypeField.input.value);
        customContentTypeField.input.value = mediaPlaylistContentTypeKnown(mediaPlaylistContentType(b))
          ? "" : mediaPlaylistContentType(b);
        helpers.saveField("options", b.options);
      });
      updateCustomContentTypeVisibility();

      helpers.renderCardTextField(panel, b, helpers, {
        label: "Label",
        idSuffix: "label",
        field: "label",
        placeholder: "e.g. Morning Playlist",
        rerender: true,
      });
    }

    if (b.sensor !== "play_pause" && b.sensor !== "now_playing" &&
        b.sensor !== "position" && b.sensor !== "volume") {
      helpers.renderCardIconPicker(panel, b, helpers, {
        pickerIdSuffix: "icon-picker",
        idSuffix: "icon",
        field: "icon",
        fallback: "Speaker",
      });
    }
  },
  renderPreview: function (b, helpers) {
    function modeInfo(value) {
      if (value === "controls") value = "play_pause";
      if (value === "previous") return { mode: "previous", label: "Previous", icon: "skip-previous" };
      if (value === "next") return { mode: "next", label: "Next", icon: "skip-next" };
      if (value === "volume") return { mode: "volume", label: "Volume", icon: "volume-high" };
      if (value === "position") return { mode: "position", label: "Position", icon: "progress-clock" };
      if (value === "now_playing") return { mode: "now_playing", label: "Now Playing", icon: "music" };
      if (value === "playlist") return { mode: "playlist", label: "Playlist", icon: "music" };
      return { mode: "play_pause", label: "Play/Pause", icon: "play-pause" };
    }
    var info = modeInfo(mediaEditorValidMode(b.sensor));
    var mode = info.mode;
    var label = (b.label && b.label.trim()) || info.label;
    if (mode === "volume") {
      return {
        iconHtml: cardSensorPreviewHtml(b, helpers, "42", null),
        labelHtml: cardBadgeLabelHtml(helpers, label, MEDIA_CARD_METADATA.preview.badge),
      };
    }
    if (mode === "position") {
      var bgColor = WEB_UI_COLORS.secondary;
      var progressColor = WEB_UI_COLORS.secondary;
      var positionLabel = b.precision === "state" ? "Paused" : label;
      var positionClass = "sp-sensor-preview sp-media-position-time" +
        (cardLargeNumbersActiveForCardSize(b, helpers, MEDIA_CARD_METADATA) ? " sp-sensor-preview-large" : "");
      return {
        iconHtml:
          '<span class="sp-slider-preview" style="inset:-2px;background:#' + helpers.escHtml(bgColor) + '">' +
          '<span class="sp-slider-track"><span class="sp-slider-fill" style="width:50%;height:100%;background:#' +
          helpers.escHtml(progressColor) + '"></span></span></span>' +
          '<span class="' + positionClass + '">' +
          '<span class="sp-sensor-value">0:00</span></span>',
        labelHtml: cardBadgeLabelHtml(helpers, positionLabel, MEDIA_CARD_METADATA.preview.badge),
      };
    }
    if (mode === "now_playing") {
      var progressBg = "";
      if (mediaNowPlayingProgressEnabled(b)) {
        var nowBgColor = WEB_UI_COLORS.secondary;
        progressBg =
          '<span class="sp-slider-preview" style="inset:-2px;background:#' + helpers.escHtml(nowBgColor) + '">' +
          '<span class="sp-slider-track"><span class="sp-slider-fill" style="width:50%;height:100%;background:#' + WEB_UI_COLORS.secondary + '">' +
          '</span></span></span>';
      } else if (mediaNowPlayingPlayPauseEnabled(b)) {
        var playBgColor = WEB_UI_COLORS.secondary;
        progressBg =
          '<span class="sp-slider-preview" style="inset:-2px;background:#' + helpers.escHtml(playBgColor) + '">' +
          '</span>';
      }
      return {
        iconHtml:
          progressBg + '<span class="sp-media-now-title">Midnight City</span>',
        labelHtml:
          '<span class="sp-btn-label-row"><span class="sp-btn-label sp-media-now-artist">M83</span>' +
          '<span class="sp-type-badge mdi mdi-' + MEDIA_CARD_METADATA.preview.badge + '"></span></span>',
      };
    }
    return {
      iconHtml:
        '<span class="sp-btn-icon mdi mdi-' + (b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : info.icon) + '"></span>',
      labelHtml: cardBadgeLabelHtml(helpers, mode === "play_pause" && b.precision === "state" ? "Playing" : label,
        MEDIA_CARD_METADATA.preview.badge),
    };
  },
});
