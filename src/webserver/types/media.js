// Media player card: playback controls, volume, or track position for media_player entities.
registerButtonType("media", {
  label: "Media",
  experimental: "media",
  allowInSubpage: true,
  labelPlaceholder: "e.g. Living Room Speaker",
  onSelect: function (b) {
    b.entity = "";
    b.sensor = "controls";
    b.unit = "";
    b.precision = "";
    b.icon = "Speaker";
    b.icon_on = "Auto";
  },
  renderSettings: function (panel, b, slot, helpers) {
    var modes = [
      ["controls", "Playback Controls"],
      ["volume", "Volume Slider"],
      ["position", "Track Position"],
    ];

    function validMode(value) {
      for (var i = 0; i < modes.length; i++) {
        if (modes[i][0] === value) return value;
      }
      return "controls";
    }

    b.sensor = validMode(b.sensor);
    b.unit = "";
    b.precision = "";
    b.icon_on = "Auto";

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
      b.sensor = validMode(this.value);
      helpers.saveField("sensor", b.sensor);
    });
    mf.appendChild(modeSelect);
    panel.appendChild(mf);

    var ef = document.createElement("div");
    ef.className = "sp-field";
    ef.appendChild(helpers.fieldLabel("Media Player Entity", helpers.idPrefix + "entity"));
    var entityInp = helpers.textInput(helpers.idPrefix + "entity", b.entity, "e.g. media_player.living_room");
    ef.appendChild(entityInp);
    panel.appendChild(ef);
    helpers.bindField(entityInp, "entity", true);

    panel.appendChild(helpers.makeIconPicker(
      helpers.idPrefix + "icon-picker", helpers.idPrefix + "icon",
      b.icon || "Speaker", function (opt) {
        b.icon = opt || "Speaker";
        helpers.saveField("icon", b.icon);
      }
    ));
  },
  renderPreview: function (b, helpers) {
    var mode = b.sensor === "volume" || b.sensor === "position" ? b.sensor : "controls";
    var label = b.label || b.entity || "Media";
    var badge = '<span class="sp-type-badge mdi mdi-speaker"></span>';
    if (mode === "volume") {
      return {
        iconHtml:
          '<span class="sp-btn-icon mdi mdi-volume-high"></span>' +
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
        '<span class="sp-media-controls-preview">' +
          '<span class="sp-media-control mdi mdi-skip-previous"></span>' +
          '<span class="sp-media-control sp-media-control-primary mdi mdi-play-pause"></span>' +
          '<span class="sp-media-control mdi mdi-skip-next"></span>' +
        '</span>',
      labelHtml:
        '<span class="sp-btn-label-row"><span class="sp-btn-label">' + helpers.escHtml(label) + '</span>' +
        badge + '</span>',
    };
  },
});
