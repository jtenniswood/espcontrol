// Media player card: playback buttons, volume, track position, or now-playing details.
registerButtonType("media", {
  label: "Media",
  experimental: "media",
  allowInSubpage: true,
  hideLabel: true,
  labelPlaceholder: "e.g. Living Room Speaker",
  onSelect: function (b) {
    b.entity = "";
    b.sensor = "play_pause";
    b.unit = "";
    b.precision = b.sensor === "play_pause" && b.precision === "state" ? "state" : "";
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
      if (value === "controls") return "play_pause";
      for (var i = 0; i < modes.length; i++) {
        if (modes[i][0] === value) return value;
      }
      return "play_pause";
    }

    function mediaDefaultIcon(value) {
      var mode = validMode(value);
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
      var mode = validMode(value);
      if (mode === "previous") return "Previous";
      if (mode === "next") return "Next";
      if (mode === "volume") return "Volume";
      if (mode === "play_pause") return "Play/Pause";
      return "";
    }

    var rawMode = b.sensor;
    b.sensor = validMode(b.sensor);
    if (rawMode === "controls" && isMediaDefaultIcon(rawMode, b.icon)) b.icon = "Auto";

    var mf = document.createElement("div");
    mf.className = "sp-field";
    mf.appendChild(helpers.fieldLabel("Media Mode", helpers.idPrefix + "media-mode"));
    var modeSelect = document.createElement("select");
    modeSelect.className = "sp-select";
    modeSelect.id = helpers.idPrefix + "media-mode";
    modes.forEach(function (entry) {
      var opt = document.createElement("option");
      opt.value = entry[0];
      opt.textContent = entry[1];
      modeSelect.appendChild(opt);
    });
    modeSelect.value = b.sensor;
    modeSelect.addEventListener("change", function () {
      var oldMode = b.sensor;
      b.sensor = validMode(this.value);
      if (isMediaDefaultIcon(oldMode, b.icon)) {
        b.icon = "Auto";
        helpers.saveField("icon", b.icon);
      }
      if (b.sensor !== "play_pause" && b.precision) {
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
    mf.appendChild(modeSelect);
    panel.appendChild(mf);
  },
  renderSettings: function (panel, b, slot, helpers) {
    function validMode(value) {
      if (value === "controls") return "play_pause";
      if (value === "previous" || value === "next" || value === "volume" ||
          value === "position" || value === "now_playing") return value;
      return "play_pause";
    }

    b.sensor = validMode(b.sensor);
    b.unit = "";
    b.precision = b.sensor === "play_pause" && b.precision === "state" ? "state" : "";
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
    if (b.sensor === "previous" && (!b.icon || b.icon === "Auto")) b.icon = "Skip Previous";
    if (b.sensor === "next" && (!b.icon || b.icon === "Auto")) b.icon = "Skip Next";

    var displayField = document.createElement("div");
    var displaySelect = document.createElement("select");

    function syncDisplayField() {
      if (b.sensor === "play_pause") {
        displayField.style.display = "";
      } else {
        displayField.style.display = "none";
        if (b.precision) {
          b.precision = "";
          helpers.saveField("precision", "");
        }
      }
    }

    displayField.className = "sp-field";
    displayField.appendChild(helpers.fieldLabel("Display", helpers.idPrefix + "media-display"));
    displaySelect.className = "sp-select";
    displaySelect.id = helpers.idPrefix + "media-display";
    [
      ["", "Label"],
      ["state", "State"],
    ].forEach(function (entry) {
      var opt = document.createElement("option");
      opt.value = entry[0];
      opt.textContent = entry[1];
      displaySelect.appendChild(opt);
    });
    displaySelect.value = b.precision === "state" ? "state" : "";
    displaySelect.addEventListener("change", function () {
      b.precision = this.value === "state" ? "state" : "";
      helpers.saveField("precision", b.precision);
      renderButtonSettings();
    });
    displayField.appendChild(displaySelect);
    panel.appendChild(displayField);
    syncDisplayField();

    if (b.sensor !== "now_playing" && b.sensor !== "position" &&
        (b.sensor !== "play_pause" || b.precision !== "state")) {
      var lf = document.createElement("div");
      lf.className = "sp-field";
      lf.appendChild(helpers.fieldLabel("Label", helpers.idPrefix + "label"));
      var labelInp = helpers.textInput(helpers.idPrefix + "label", b.label, "e.g. Living Room Speaker");
      lf.appendChild(labelInp);
      panel.appendChild(lf);
      helpers.bindField(labelInp, "label", true);
    }

    var ef = document.createElement("div");
    ef.className = "sp-field";
    ef.appendChild(helpers.fieldLabel("Media Player Entity", helpers.idPrefix + "entity"));
    var entityInp = helpers.textInput(helpers.idPrefix + "entity", b.entity, "e.g. media_player.living_room");
    ef.appendChild(entityInp);
    panel.appendChild(ef);
    helpers.bindField(entityInp, "entity", true);
    helpers.requireField(entityInp, "Add an entity before saving.");

    if (b.sensor !== "now_playing" && b.sensor !== "position" && b.sensor !== "volume") {
      panel.appendChild(helpers.makeIconPicker(
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
      if (value === "position") return { mode: "position", label: "Media", icon: "progress-clock" };
      if (value === "now_playing") return { mode: "now_playing", label: "Now Playing", icon: "music" };
      return { mode: "play_pause", label: "Play/Pause", icon: "play-pause" };
    }
    var info = modeInfo(b.sensor);
    var mode = info.mode;
    var label = b.label || (mode === "position" ? (b.entity || "Media") : info.label);
    var badge = '<span class="sp-type-badge mdi mdi-speaker"></span>';
    if (mode === "volume") {
      return {
        iconHtml:
          '<span class="sp-btn-icon mdi mdi-' + (b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : info.icon) + '"></span>',
        labelHtml:
          '<span class="sp-btn-label-row"><span class="sp-btn-label">' + helpers.escHtml(label) + '</span>' +
          badge + '</span>',
      };
    }
    if (mode === "position") {
      var trackColor = (typeof state !== "undefined" && state.sensorColor) ? state.sensorColor : "212121";
      return {
        iconHtml:
          '<span class="sp-media-h-slider" style="background:#' + helpers.escHtml(trackColor) + '">' +
          '<span style="width:42%"></span></span>' +
          '<span class="sp-sensor-preview sp-media-position-time">' +
          '<span class="sp-sensor-value">1:31</span></span>',
        labelHtml: "",
      };
    }
    if (mode === "now_playing") {
      return {
        iconHtml:
          '<span class="sp-media-now-title">Midnight City</span>',
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
