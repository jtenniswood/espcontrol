// Media player card: playback buttons, volume, or track position for media_player entities.
registerButtonType("media", {
  label: "Media",
  experimental: "media",
  allowInSubpage: true,
  labelPlaceholder: "e.g. Living Room Speaker",
  onSelect: function (b) {
    b.entity = "";
    b.sensor = "play_pause";
    b.unit = "";
    b.precision = "";
    b.icon = "Auto";
    b.icon_on = "Auto";
  },
  renderSettings: function (panel, b, slot, helpers) {
    var modes = [
      ["play_pause", "Play/Pause Button"],
      ["previous", "Previous Button"],
      ["next", "Next Button"],
      ["volume", "Volume Slider"],
      ["position", "Track Position"],
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
      return "Play Pause";
    }

    function isMediaDefaultIcon(value, icon) {
      if (!icon || icon === "Auto") return true;
      if (value === "controls" && icon === "Speaker") return true;
      return icon === mediaDefaultIcon(value);
    }

    var rawMode = b.sensor;
    b.sensor = validMode(b.sensor);
    if (rawMode === "controls" && isMediaDefaultIcon(rawMode, b.icon)) b.icon = "Auto";
    b.unit = "";
    b.precision = "";
    b.icon_on = "Auto";

    var ef = document.createElement("div");
    ef.className = "sp-field";
    ef.appendChild(helpers.fieldLabel("Media Player Entity", helpers.idPrefix + "entity"));
    var entityInp = helpers.textInput(helpers.idPrefix + "entity", b.entity, "e.g. media_player.living_room");
    ef.appendChild(entityInp);
    panel.appendChild(ef);
    helpers.bindField(entityInp, "entity", true);
    helpers.requireField(entityInp, "Add an entity before saving.");

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
      helpers.saveField("sensor", b.sensor);
    });
    mf.appendChild(modeSelect);
    panel.appendChild(mf);

    panel.appendChild(helpers.makeIconPicker(
      helpers.idPrefix + "icon-picker", helpers.idPrefix + "icon",
      b.icon || "Speaker", function (opt) {
        b.icon = opt || "Speaker";
        helpers.saveField("icon", b.icon);
      }
    ));
  },
  renderPreview: function (b, helpers) {
    function modeInfo(value) {
      if (value === "controls") value = "play_pause";
      if (value === "previous") return { mode: "previous", label: "Previous", icon: "skip-previous" };
      if (value === "next") return { mode: "next", label: "Next", icon: "skip-next" };
      if (value === "volume") return { mode: "volume", label: "Media", icon: "volume-high" };
      if (value === "position") return { mode: "position", label: "Media", icon: "progress-clock" };
      return { mode: "play_pause", label: "Play/Pause", icon: "play-pause" };
    }
    var info = modeInfo(b.sensor);
    var mode = info.mode;
    var label = b.label || (mode === "volume" || mode === "position" ? (b.entity || "Media") : info.label);
    var badge = '<span class="sp-type-badge mdi mdi-speaker"></span>';
    if (mode === "volume") {
      return {
        iconHtml:
          '<span class="sp-sensor-preview"><span class="sp-sensor-value">62</span></span>' +
          '<span class="sp-media-h-slider"><span></span></span>',
        labelHtml:
          '<span class="sp-btn-label-row"><span class="sp-btn-label">' + helpers.escHtml(label) + '</span>' +
          badge + '</span>',
      };
    }
    if (mode === "position") {
      return {
        iconHtml:
          '<span class="sp-slider-preview"><span class="sp-slider-track">' +
            '<span class="sp-slider-fill" style="height:42%"></span>' +
          '</span></span>' +
          '<span class="sp-media-position-time">1:31</span>',
        labelHtml:
          '<span class="sp-btn-label-row"><span class="sp-media-position-status">Playing</span>' +
          badge + '</span>',
      };
    }
    return {
      iconHtml:
        '<span class="sp-btn-icon mdi mdi-' + (b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : info.icon) + '"></span>',
      labelHtml:
        '<span class="sp-btn-label-row"><span class="sp-btn-label">' + helpers.escHtml(label) + '</span>' +
        badge + '</span>',
    };
  },
});
