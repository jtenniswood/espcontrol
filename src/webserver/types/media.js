// Media player card: playback buttons, volume, track position, or now-playing details.
function mediaEditorMode(value) {
  if (value === "controls") return "play_pause";
  if (value === "previous" || value === "next" || value === "volume" ||
      value === "position" || value === "now_playing" || value === "play_pause") {
    return value;
  }
  return "play_pause";
}

function mediaEditorValidMode(value) {
  return mediaEditorMode(value);
}

function mediaNowPlayingControls(b) {
  if (!b || b.sensor !== "now_playing") return "";
  return b.precision === "progress" || b.precision === "play_pause" ? b.precision : "";
}

function mediaNowPlayingProgressEnabled(b) {
  return mediaNowPlayingControls(b) === "progress";
}

function mediaNowPlayingPlayPauseEnabled(b) {
  return mediaNowPlayingControls(b) === "play_pause";
}

registerButtonType("media", {
  label: "Media",
  allowInSubpage: true,
  hideLabel: true,
  labelPlaceholder: "e.g. Living Room Speaker",
  onSelect: function (b) {
    b.entity = "";
    b.sensor = "play_pause";
    b.unit = "";
    b.precision = (b.sensor === "play_pause" || b.sensor === "position") && b.precision === "state" ? "state" : "";
    b.icon = "Auto";
    b.icon_on = "Auto";
  },
  renderSettingsBeforeLabel: function (panel, b, slot, helpers) {
    var modes = [
      ["play_pause", "Play/Pause Button"],
      ["previous", "Previous Button"],
      ["next", "Next Button"],
      ["volume", "Volume Button"],
      ["position", "Track Position"],
      ["now_playing", "Now Playing"],
    ];

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
      return "";
    }

    var rawMode = b.sensor;
    b.sensor = validMode(b.sensor);
    if (rawMode === "controls" && isMediaDefaultIcon(rawMode, b.icon)) b.icon = "Auto";

    var modeField = helpers.selectField("Media Mode", helpers.idPrefix + "media-mode", modes, b.sensor, function () {
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
      if (b.sensor === "volume") {
        var oldDefaultLabel = mediaActionLabel(oldMode);
        if (!b.label || b.label === oldDefaultLabel || b.label === "Media") {
          b.label = mediaActionLabel(b.sensor);
          helpers.saveField("label", b.label);
        }
        b.icon = "Auto";
        helpers.saveField("icon", b.icon);
      }
      helpers.saveField("sensor", b.sensor);
      renderButtonSettings();
    });
    panel.appendChild(modeField.field);
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
    if (b.sensor === "previous" && (!b.icon || b.icon === "Auto")) b.icon = "Skip Previous";
    if (b.sensor === "next" && (!b.icon || b.icon === "Auto")) b.icon = "Skip Next";

    var entityField = helpers.entityField(
      "Media Player Entity", helpers.idPrefix + "entity", b.entity,
      "e.g. media_player.living_room", ["media_player"], "entity", true,
      "Add an entity before saving.");
    panel.appendChild(entityField.field);

    var displayMode = helpers.segmentControl([
      ["", "Label"],
      ["state", "State"],
    ], b.precision === "state" ? "state" : "", function (value) { setDisplayMode(value); });
    var displayField = helpers.fieldWithControl(
      "Type", helpers.idPrefix + "media-display", displayMode.segment);
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

    if (b.sensor === "now_playing") {
      var controls = helpers.segmentControl([
        ["", "None"],
        ["progress", "Track Position"],
        ["play_pause", "Play/Pause"],
      ], mediaNowPlayingControls(b), function (value) {
        b.precision = value;
        helpers.saveField("precision", b.precision);
        renderButtonSettings();
      });
      controls.segment.classList.add("sp-segment-scroll");
      panel.appendChild(helpers.fieldWithControl(
        "Controls", helpers.idPrefix + "media-controls", controls.segment));
    }

    if (b.sensor === "now_playing") {
      var controlsMode = mediaNowPlayingControls(b);
      if (b.precision !== controlsMode) {
        b.precision = controlsMode;
        helpers.saveField("precision", b.precision);
      }
    }

    if (b.sensor !== "now_playing" &&
        (b.sensor !== "play_pause" || b.precision !== "state") &&
        (b.sensor !== "position" || b.precision !== "state")) {
      panel.appendChild(helpers.textField(
        "Label", helpers.idPrefix + "label", b.label,
        b.sensor === "position" ? "Position" : "e.g. Living Room Speaker",
        "label", true).field);
    }

    if (b.sensor !== "play_pause" && b.sensor !== "now_playing" &&
        b.sensor !== "position" && b.sensor !== "volume") {
      panel.appendChild(helpers.iconPickerField(
        helpers.idPrefix + "icon-picker", helpers.idPrefix + "icon",
        b.icon || "Speaker", function (opt) {
          b.icon = opt || "Speaker";
          helpers.saveField("icon", b.icon);
        }
      ));
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
      return { mode: "play_pause", label: "Play/Pause", icon: "play-pause" };
    }
    var info = modeInfo(mediaEditorValidMode(b.sensor));
    var mode = info.mode;
    var label = (b.label && b.label.trim()) || info.label;
    var badge = '<span class="sp-type-badge mdi mdi-speaker"></span>';
    if (mode === "volume") {
      return {
        iconHtml:
          '<span class="sp-sensor-preview"><span class="sp-sensor-value">42</span></span>',
        labelHtml:
          '<span class="sp-btn-label-row"><span class="sp-btn-label">' + helpers.escHtml(label) + '</span>' +
          badge + '</span>',
      };
    }
    if (mode === "position") {
      var bgColor = (typeof state !== "undefined" && state.offColor) ? state.offColor : "313131";
      var progressColor = "444444";
      var positionLabel = b.precision === "state" ? "Paused" : label;
      return {
        iconHtml:
          '<span class="sp-slider-preview" style="inset:-2px;background:#' + helpers.escHtml(bgColor) + '">' +
          '<span class="sp-slider-track"><span class="sp-slider-fill" style="width:50%;height:100%;background:#' +
          helpers.escHtml(progressColor) + '"></span></span></span>' +
          '<span class="sp-sensor-preview sp-media-position-time">' +
          '<span class="sp-sensor-value">0:00</span></span>',
        labelHtml:
          '<span class="sp-btn-label-row"><span class="sp-btn-label">' + helpers.escHtml(positionLabel) + '</span>' +
          badge + '</span>',
      };
    }
    if (mode === "now_playing") {
      var progressBg = "";
      if (mediaNowPlayingProgressEnabled(b)) {
        var nowBgColor = (typeof state !== "undefined" && state.offColor) ? state.offColor : "313131";
        progressBg =
          '<span class="sp-slider-preview" style="inset:-2px;background:#' + helpers.escHtml(nowBgColor) + '">' +
          '<span class="sp-slider-track"><span class="sp-slider-fill" style="width:50%;height:100%;background:#444444">' +
          '</span></span></span>';
      } else if (mediaNowPlayingPlayPauseEnabled(b)) {
        var playBgColor = (typeof state !== "undefined" && state.offColor) ? state.offColor : "313131";
        progressBg =
          '<span class="sp-slider-preview" style="inset:-2px;background:#' + helpers.escHtml(playBgColor) + '">' +
          '</span>';
      }
      return {
        iconHtml:
          progressBg + '<span class="sp-media-now-title">Midnight City</span>',
        labelHtml:
          '<span class="sp-btn-label-row"><span class="sp-btn-label sp-media-now-artist">M83</span>' +
          badge + '</span>',
      };
    }
    return {
      iconHtml:
        '<span class="sp-btn-icon mdi mdi-' + (b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : info.icon) + '"></span>',
      labelHtml:
        '<span class="sp-btn-label-row"><span class="sp-btn-label">' +
        helpers.escHtml(mode === "play_pause" && b.precision === "state" ? "Playing" : label) + '</span>' +
        badge + '</span>',
    };
  },
});
