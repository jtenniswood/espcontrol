// Load the original ESPHome webserver v3 React app (used for API only)
(function () {
  var s = document.createElement("script");
  s.src = "https://oi.esphome.io/v3/www.js";
  document.head.appendChild(s);
})();

// Custom UI: three-page layout (Screen / Settings / Logs)
(function () {
  var NUM_SLOTS = 20;

  var ICON_MAP = {
    Auto: "cog",
    "Air Conditioner": "air-conditioner",
    Alarm: "bell-ring",
    Battery: "battery",
    Bed: "bed",
    "Blinds Closed": "blinds-horizontal-closed",
    "Blinds Open": "blinds-horizontal",
    Camera: "camera",
    "Ceiling Fan": "ceiling-fan",
    "Ceiling Light": "ceiling-light",
    "Coffee Maker": "coffee-maker",
    "Desk Lamp": "desk-lamp",
    Dishwasher: "dishwasher",
    Door: "door",
    Doorbell: "doorbell-video",
    Dryer: "tumble-dryer",
    "EV Charger": "ev-station",
    Fan: "fan",
    Fire: "fire",
    Fireplace: "fireplace",
    "Floor Lamp": "floor-lamp",
    Fridge: "fridge",
    Garage: "garage",
    Gate: "gate",
    Heater: "radiator",
    "Hot Tub": "hot-tub",
    Humidifier: "air-humidifier",
    Lamp: "lamp",
    Lawnmower: "robot-mower",
    "LED Strip": "led-strip",
    Lightbulb: "lightbulb",
    Lock: "lock",
    Microwave: "microwave",
    "Motion Sensor": "motion-sensor",
    Oven: "stove",
    Pool: "pool",
    Power: "power",
    "Power Plug": "power-plug",
    "Robot Vacuum": "robot-vacuum",
    Security: "shield-home",
    "Smoke Detector": "smoke-detector",
    "Solar Panel": "solar-panel",
    Speaker: "speaker",
    Sprinkler: "sprinkler",
    Television: "television",
    Thermometer: "thermometer",
    Washer: "washing-machine",
    Water: "water",
    "Water Heater": "water-boiler",
    Window: "window-open-variant",
  };

  var DOMAIN_ICONS = {
    light: "lightbulb",
    switch: "power-plug",
    fan: "fan",
    lock: "lock",
    cover: "blinds-horizontal",
    climate: "air-conditioner",
    media_player: "speaker",
    camera: "camera",
    binary_sensor: "motion-sensor",
  };

  var ICON_OPTIONS = [
    "Auto", "Air Conditioner", "Alarm", "Battery", "Bed",
    "Blinds Closed", "Blinds Open", "Camera", "Ceiling Fan", "Ceiling Light",
    "Coffee Maker", "Desk Lamp", "Dishwasher", "Door", "Doorbell",
    "Dryer", "EV Charger", "Fan", "Fire", "Fireplace",
    "Floor Lamp", "Fridge", "Garage", "Gate", "Heater",
    "Hot Tub", "Humidifier", "Lamp", "Lawnmower", "LED Strip",
    "Lightbulb", "Lock", "Microwave", "Motion Sensor", "Oven",
    "Pool", "Power", "Power Plug", "Robot Vacuum", "Security",
    "Smoke Detector", "Solar Panel", "Speaker", "Sprinkler", "Television",
    "Thermometer", "Washer", "Water", "Water Heater", "Window",
  ];

  var CSS =
    "#sp-app{font-family:Roboto,sans-serif;color:#e0e0e0;max-width:600px;margin:0 auto}" +
    "esp-app{display:none !important}" +

    // Header
    ".sp-header{display:flex;background:#1a1a1a;border-bottom:1px solid #333;" +
    "position:sticky;top:0;z-index:100;border-radius:0 0 8px 8px;overflow:hidden}" +
    ".sp-tab{flex:1;padding:14px 0;text-align:center;color:#888;cursor:pointer;" +
    "font-size:14px;font-weight:500;border-bottom:2px solid transparent;transition:all .2s}" +
    ".sp-tab:hover{color:#bbb}" +
    ".sp-tab.active{color:#fff;border-bottom-color:#03a9f4}" +

    // Pages
    ".sp-page{display:none}.sp-page.active{display:block}" +

    // Screen preview
    ".sp-wrap{display:flex;justify-content:center;padding:20px 16px 4px}" +
    ".sp-screen{width:100%;aspect-ratio:1024/600;background:#000;" +
    "border-radius:10px;position:relative;overflow:hidden;" +
    "box-shadow:0 2px 20px rgba(0,0,0,.35);border:2px solid #1a1a1a;" +
    "container-type:inline-size;font-family:Roboto,sans-serif;user-select:none}" +
    ".sp-topbar{position:absolute;top:0;left:0;right:0;height:4.1cqw;" +
    "display:flex;align-items:center;padding:0.78cqw;z-index:1}" +
    ".sp-temp{color:#fff;font-size:1.95cqw;white-space:nowrap;opacity:0;transition:opacity .3s}" +
    ".sp-temp.sp-visible{opacity:1}" +
    ".sp-clock{position:absolute;left:50%;transform:translateX(-50%);" +
    "color:#fff;font-size:1.95cqw;white-space:nowrap}" +
    ".sp-main{position:absolute;top:4.1cqw;left:0.49cqw;right:0.49cqw;bottom:0.49cqw;" +
    "display:flex;flex-direction:column;flex-wrap:wrap;align-content:flex-start;gap:0.98cqw;padding:0.49cqw}" +

    // Preview buttons
    ".sp-btn{width:19cqw;height:12.7cqw;border-radius:0.78cqw;padding:1.37cqw;" +
    "display:flex;flex-direction:column;justify-content:space-between;" +
    "cursor:pointer;transition:all .2s;box-sizing:border-box;border:2px solid transparent}" +
    ".sp-btn:hover{filter:brightness(1.15)}" +
    ".sp-btn.sp-selected{border-color:#03a9f4}" +
    ".sp-btn-icon{font-size:4.69cqw;line-height:1;color:#fff}" +
    ".sp-btn-label{font-size:1.8cqw;line-height:1.2;color:#fff;" +
    "white-space:nowrap;overflow:hidden;text-overflow:ellipsis}" +
    ".sp-add-btn{border:2px dashed rgba(255,255,255,.25);background:transparent !important;" +
    "display:flex;align-items:center;justify-content:center;cursor:pointer}" +
    ".sp-add-btn:hover{border-color:#03a9f4}" +
    ".sp-add-icon{font-size:5cqw;color:rgba(255,255,255,.35)}" +
    ".sp-btn.sp-dragging{opacity:.35;cursor:grabbing}" +
    ".sp-btn.sp-drop-before{border-top:2px solid #03a9f4 !important}" +
    ".sp-btn.sp-drop-after{border-bottom:2px solid #03a9f4 !important}" +

    // Hint
    ".sp-hint{text-align:center;font-size:11px;opacity:.4;padding:6px 0 12px}" +

    // Config area
    ".sp-config{padding:0 16px 16px}" +
    ".sp-section-title{font-size:13px;font-weight:500;color:#888;" +
    "margin:16px 0 8px;text-transform:uppercase;letter-spacing:0.5px}" +

    // Button list
    ".sp-list{list-style:none;margin:0;padding:0}" +
    ".sp-list-item{display:flex;align-items:center;flex-wrap:wrap;padding:10px 12px;" +
    "background:#1e1e1e;border-radius:6px;margin-bottom:6px;cursor:pointer;" +
    "border:1px solid #2a2a2a;transition:all .15s}" +
    ".sp-list-item:hover{border-color:#444}" +
    ".sp-list-item.sp-list-selected{border-color:#03a9f4;background:#1a2a3a}" +
    ".sp-list-item.sp-dragging{opacity:.4}" +
    ".sp-list-handle{color:#555;margin-right:10px;cursor:grab;font-size:18px;line-height:1}" +
    ".sp-list-handle:active{cursor:grabbing}" +
    ".sp-list-info{flex:1;min-width:0}" +
    ".sp-list-name{font-size:13px;font-weight:500;white-space:nowrap;" +
    "overflow:hidden;text-overflow:ellipsis}" +
    ".sp-list-entity{font-size:11px;color:#888;white-space:nowrap;" +
    "overflow:hidden;text-overflow:ellipsis;margin-top:2px}" +
    ".sp-list-icon{font-size:20px;margin-right:10px;width:24px;text-align:center;color:#aaa}" +
    ".sp-drop-before{border-top:2px solid #03a9f4 !important}" +
    ".sp-drop-after{border-bottom:2px solid #03a9f4 !important}" +

    // Inline settings (expanded inside selected list item)
    ".sp-list-expand{width:100%;padding-top:12px;cursor:default}" +

    // Settings page panels
    ".sp-panel{background:#1e1e1e;border-radius:8px;padding:16px;margin-top:12px;" +
    "border:1px solid #2a2a2a}" +

    // Form fields
    ".sp-field{margin-bottom:12px}" +
    ".sp-field-label{display:block;font-size:11px;color:#888;margin-bottom:4px;" +
    "text-transform:uppercase;letter-spacing:0.3px}" +
    ".sp-input,.sp-select{width:100%;padding:9px 12px;background:#2a2a2a;" +
    "border:1px solid #444;border-radius:6px;color:#e0e0e0;font-size:14px;" +
    "font-family:inherit;box-sizing:border-box;outline:none;transition:border-color .2s}" +
    ".sp-input:focus,.sp-select:focus{border-color:#03a9f4}" +
    ".sp-select{appearance:auto}" +
    ".sp-btn-row{display:flex;gap:8px;margin-top:16px}" +
    ".sp-action-btn{padding:9px 16px;border:none;border-radius:6px;font-size:13px;" +
    "font-weight:500;cursor:pointer;font-family:inherit;transition:filter .15s}" +
    ".sp-action-btn:hover{filter:brightness(1.15)}" +
    ".sp-delete-btn{background:#d32f2f;color:#fff}" +

    // Toggle switch
    ".sp-toggle-row{display:flex;align-items:center;justify-content:space-between;" +
    "padding:10px 0}" +
    ".sp-toggle-label{font-size:14px}" +
    ".sp-toggle{position:relative;width:44px;height:24px;flex-shrink:0}" +
    ".sp-toggle input{opacity:0;width:0;height:0;position:absolute}" +
    ".sp-toggle-track{position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;" +
    "background:#444;border-radius:24px;transition:.3s}" +
    ".sp-toggle-track:before{content:'';position:absolute;height:18px;width:18px;" +
    "left:3px;bottom:3px;background:#888;border-radius:50%;transition:.3s}" +
    ".sp-toggle input:checked+.sp-toggle-track{background:#03a9f4}" +
    ".sp-toggle input:checked+.sp-toggle-track:before{transform:translateX(20px);background:#fff}" +

    // Conditional field (shown below toggle when enabled)
    ".sp-cond-field{padding:0 0 8px;display:none}" +
    ".sp-cond-field.sp-visible{display:block}" +

    // Range slider
    ".sp-range-row{display:flex;align-items:center;gap:12px;padding:4px 0}" +
    ".sp-range{flex:1;height:6px;-webkit-appearance:none;appearance:none;background:#444;" +
    "border-radius:3px;outline:none}" +
    ".sp-range::-webkit-slider-thumb{-webkit-appearance:none;width:20px;height:20px;" +
    "border-radius:50%;background:#03a9f4;cursor:pointer}" +
    ".sp-range-val{font-size:13px;color:#aaa;min-width:32px;text-align:right}" +

    // Number input
    ".sp-number-row{display:flex;align-items:center;gap:8px}" +
    ".sp-number{width:80px;padding:8px 10px;background:#2a2a2a;border:1px solid #444;" +
    "border-radius:6px;color:#e0e0e0;font-size:14px;font-family:inherit;text-align:center;" +
    "outline:none;box-sizing:border-box}" +
    ".sp-number:focus{border-color:#03a9f4}" +
    ".sp-number-unit{font-size:13px;color:#888}" +

    // Apply bar
    ".sp-apply-bar{padding:16px;text-align:center}" +
    ".sp-apply-btn{background:#03a9f4;color:#fff;border:none;border-radius:6px;" +
    "padding:12px 32px;font-size:14px;font-weight:500;cursor:pointer;" +
    "font-family:inherit;transition:filter .15s}" +
    ".sp-apply-btn:hover{filter:brightness(1.15)}" +
    ".sp-apply-note{font-size:11px;color:#666;margin-top:6px}" +

    // Log viewer
    ".sp-log-toolbar{display:flex;justify-content:flex-end;padding:12px 16px 0}" +
    ".sp-log-clear{background:#333;color:#ccc;border:none;border-radius:4px;" +
    "padding:6px 14px;font-size:12px;cursor:pointer;font-family:inherit}" +
    ".sp-log-clear:hover{background:#444}" +
    ".sp-log-output{margin:8px 16px 16px;padding:12px;background:#0a0a0a;" +
    "border-radius:8px;font-family:'Courier New',monospace;font-size:12px;" +
    "color:#ccc;height:70vh;overflow-y:auto;white-space:pre-wrap;word-break:break-all;" +
    "border:1px solid #1a1a1a}" +
    ".sp-log-line{padding:1px 0}" +
    ".sp-log-error{color:#ef5350}" +
    ".sp-log-warn{color:#ffa726}" +
    ".sp-log-debug{color:#666}" +
    ".sp-log-verbose{color:#555}" +

    // Empty state
    ".sp-empty{text-align:center;padding:24px;color:#666;font-size:13px}" +

    // Connection banner
    ".sp-banner{padding:10px 16px;font-size:13px;text-align:center;display:none}" +
    ".sp-banner.sp-error{display:block;background:#d32f2f;color:#fff}" +
    ".sp-banner.sp-offline{display:block;background:#f57c00;color:#fff}";

  var state = {
    order: [],
    buttons: [],
    onColor: "FF8C00",
    offColor: "313131",
    selectedSlot: -1,
    activeTab: "screen",
    _indoorOn: false,
    _outdoorOn: false,
    indoorEntity: "",
    outdoorEntity: "",
    presenceEntity: "",
    screensaverTimeout: 300,
    backlight: 255,
  };

  for (var i = 0; i < NUM_SLOTS; i++) {
    state.buttons.push({ entity: "", label: "", icon: "Auto" });
  }

  var els = {};
  var dragSrcPos = -1;
  var didDrag = false;
  var orderReceived = false;
  var migrationTimer = null;

  // ── Helpers ──────────────────────────────────────────────────────────

  function parseOrder(str) {
    if (!str || !str.trim()) return [];
    return str
      .split(",")
      .map(function (s) { return parseInt(s.trim(), 10); })
      .filter(function (n) { return n >= 1 && n <= NUM_SLOTS && !isNaN(n); });
  }

  function resolveIcon(slot) {
    var b = state.buttons[slot - 1];
    var sel = b.icon || "Auto";
    if (sel === "Auto" && b.entity) {
      var domain = b.entity.split(".")[0];
      return DOMAIN_ICONS[domain] || "cog";
    }
    return ICON_MAP[sel] || "cog";
  }

  function btnDisplayName(slot) {
    var b = state.buttons[slot - 1];
    return b.label || b.entity || "Configure";
  }

  function post(url) {
    return fetch(url, { method: "POST" }).then(function (r) {
      if (!r.ok) showBanner("Request failed: " + r.status, "error");
      return r;
    }).catch(function () {
      showBanner("Cannot reach device \u2014 is it connected?", "error");
    });
  }

  function postText(id, value) {
    post("/text/" + id + "/set?value=" + encodeURIComponent(value));
  }

  function postSelect(id, option) {
    post("/select/" + id + "/set?option=" + encodeURIComponent(option));
  }

  function postButtonPress(id) {
    post("/button/" + id + "/press");
  }

  function postSwitch(id, on) {
    post("/switch/" + id + "/" + (on ? "turn_on" : "turn_off"));
  }

  function postNumber(id, value) {
    post("/number/" + id + "/set?value=" + encodeURIComponent(value));
  }

  function postLight(id, brightness) {
    post("/light/" + id + "/turn_on?brightness=" + brightness);
  }

  function escHtml(s) {
    var d = document.createElement("div");
    d.textContent = s;
    return d.innerHTML;
  }

  function escAttr(s) {
    return s.replace(/&/g, "&amp;").replace(/"/g, "&quot;")
      .replace(/</g, "&lt;").replace(/>/g, "&gt;");
  }

  function isExpandFocused() {
    var ae = document.activeElement;
    return ae && ae.closest && ae.closest(".sp-list-expand");
  }

  // ── Init ─────────────────────────────────────────────────────────────

  function init() {
    var style = document.createElement("style");
    style.textContent = CSS;
    document.head.appendChild(style);

    var mdi = document.createElement("link");
    mdi.rel = "stylesheet";
    mdi.href = "https://cdn.jsdelivr.net/npm/@mdi/font@7.4.47/css/materialdesignicons.min.css";
    document.head.appendChild(mdi);

    var roboto = document.createElement("link");
    roboto.rel = "stylesheet";
    roboto.href = "https://fonts.googleapis.com/css2?family=Roboto:wght@300;400;500&display=swap";
    document.head.appendChild(roboto);

    buildUI();
    renderPreview();
    renderButtonList();
    connectEvents();
    updateClock();
    setInterval(updateClock, 30000);

  }

  // ── Build UI ─────────────────────────────────────────────────────────

  function showBanner(msg, type) {
    if (!els.banner) return;
    els.banner.textContent = msg;
    els.banner.className = "sp-banner sp-" + type;
    if (type === "error") {
      clearTimeout(els._bannerTimer);
      els._bannerTimer = setTimeout(function () {
        els.banner.className = "sp-banner";
      }, 6000);
    }
  }

  function buildUI() {
    var root = document.createElement("div");
    root.id = "sp-app";

    var banner = document.createElement("div");
    banner.className = "sp-banner";
    root.appendChild(banner);
    els.banner = banner;

    buildHeader(root);
    buildScreenPage(root);
    buildSettingsPage(root);
    buildLogsPage(root);

    var app = document.querySelector("esp-app");
    if (app) {
      app.parentNode.insertBefore(root, app);
    } else {
      document.body.insertBefore(root, document.body.firstChild);
    }
    els.root = root;
    switchTab("screen");
  }

  function buildHeader(parent) {
    var header = document.createElement("div");
    header.className = "sp-header";

    var tabs = [
      { id: "screen", label: "Screen" },
      { id: "settings", label: "Settings" },
      { id: "logs", label: "Logs" },
    ];

    tabs.forEach(function (t) {
      var tab = document.createElement("div");
      tab.className = "sp-tab";
      tab.textContent = t.label;
      tab.addEventListener("click", function () { switchTab(t.id); });
      header.appendChild(tab);
      els["tab_" + t.id] = tab;
    });

    parent.appendChild(header);
  }

  // ── Screen Page ──────────────────────────────────────────────────────

  function buildScreenPage(parent) {
    var page = document.createElement("div");
    page.id = "sp-screen";
    page.className = "sp-page";

    // Preview
    var wrap = document.createElement("div");
    wrap.className = "sp-wrap";
    wrap.innerHTML =
      '<div class="sp-screen">' +
      '<div class="sp-topbar">' +
      '<span class="sp-temp"></span>' +
      '<span class="sp-clock">--:--</span>' +
      "</div>" +
      '<div class="sp-main"></div>' +
      "</div>";
    page.appendChild(wrap);

    els.temp = wrap.querySelector(".sp-temp");
    els.clock = wrap.querySelector(".sp-clock");
    els.previewMain = wrap.querySelector(".sp-main");

    var hint = document.createElement("div");
    hint.className = "sp-hint";
    hint.textContent = "tap a button to configure \u2022 tap + to add";
    page.appendChild(hint);

    // Button list + settings
    var config = document.createElement("div");
    config.className = "sp-config";

    var listTitle = document.createElement("div");
    listTitle.className = "sp-section-title";
    listTitle.textContent = "Buttons";
    config.appendChild(listTitle);

    var list = document.createElement("ul");
    list.className = "sp-list";
    config.appendChild(list);
    els.buttonList = list;

    page.appendChild(config);
    page.appendChild(buildApplyBar());

    parent.appendChild(page);
    els.screenPage = page;
  }

  // ── Settings Page ────────────────────────────────────────────────────

  function buildSettingsPage(parent) {
    var page = document.createElement("div");
    page.id = "sp-settings";
    page.className = "sp-page";

    var config = document.createElement("div");
    config.className = "sp-config";

    // --- Appearance ---
    config.appendChild(sectionTitle("Appearance"));

    var appearPanel = document.createElement("div");
    appearPanel.className = "sp-panel";

    // On Color
    appearPanel.appendChild(fieldLabel("On Color"));
    var onInp = textInput("sp-set-on-color", "FF8C00", "6-digit hex e.g. FF8C00");
    appearPanel.appendChild(onInp);
    onInp.addEventListener("blur", function () { postText("button_on_color", this.value); });
    onInp.addEventListener("keydown", function (e) { if (e.key === "Enter") this.blur(); });
    els.setOnColor = onInp;

    // Off Color
    appearPanel.appendChild(fieldLabel("Off Color"));
    var offInp = textInput("sp-set-off-color", "313131", "6-digit hex e.g. 313131");
    appearPanel.appendChild(offInp);
    offInp.addEventListener("blur", function () { postText("button_off_color", this.value); });
    offInp.addEventListener("keydown", function (e) { if (e.key === "Enter") this.blur(); });
    els.setOffColor = offInp;

    // Backlight
    appearPanel.appendChild(fieldLabel("Display Backlight"));
    var blRow = document.createElement("div");
    blRow.className = "sp-range-row";
    var blRange = document.createElement("input");
    blRange.type = "range";
    blRange.className = "sp-range";
    blRange.min = "0";
    blRange.max = "255";
    blRange.value = "255";
    var blVal = document.createElement("span");
    blVal.className = "sp-range-val";
    blVal.textContent = "255";
    blRange.addEventListener("input", function () { blVal.textContent = this.value; });
    blRange.addEventListener("change", function () { postLight("display_backlight", this.value); });
    blRow.appendChild(blRange);
    blRow.appendChild(blVal);
    appearPanel.appendChild(blRow);
    els.setBacklight = blRange;
    els.setBacklightVal = blVal;

    config.appendChild(appearPanel);

    // --- Temperature ---
    config.appendChild(sectionTitle("Temperature"));

    var tempPanel = document.createElement("div");
    tempPanel.className = "sp-panel";

    // Indoor
    var indoorToggle = toggleRow("Indoor Temperature", "sp-set-indoor-toggle", state._indoorOn);
    tempPanel.appendChild(indoorToggle.row);
    var indoorField = condField();
    indoorField.appendChild(fieldLabel("Indoor Temp Entity"));
    var indoorInp = textInput("sp-set-indoor-entity", "", "e.g. sensor.indoor_temperature");
    indoorField.appendChild(indoorInp);
    tempPanel.appendChild(indoorField);
    indoorToggle.input.addEventListener("change", function () {
      postSwitch("indoor_temp_enable", this.checked);
    });
    indoorInp.addEventListener("blur", function () { postText("indoor_temp_entity", this.value); });
    indoorInp.addEventListener("keydown", function (e) { if (e.key === "Enter") this.blur(); });
    els.setIndoorToggle = indoorToggle.input;
    els.setIndoorField = indoorField;
    els.setIndoorEntity = indoorInp;

    // Outdoor
    var outdoorToggle = toggleRow("Outdoor Temperature", "sp-set-outdoor-toggle", state._outdoorOn);
    tempPanel.appendChild(outdoorToggle.row);
    var outdoorField = condField();
    outdoorField.appendChild(fieldLabel("Outdoor Temp Entity"));
    var outdoorInp = textInput("sp-set-outdoor-entity", "", "e.g. sensor.outdoor_temperature");
    outdoorField.appendChild(outdoorInp);
    tempPanel.appendChild(outdoorField);
    outdoorToggle.input.addEventListener("change", function () {
      postSwitch("outdoor_temp_enable", this.checked);
    });
    outdoorInp.addEventListener("blur", function () { postText("outdoor_temp_entity", this.value); });
    outdoorInp.addEventListener("keydown", function (e) { if (e.key === "Enter") this.blur(); });
    els.setOutdoorToggle = outdoorToggle.input;
    els.setOutdoorField = outdoorField;
    els.setOutdoorEntity = outdoorInp;

    config.appendChild(tempPanel);

    // --- Screensaver ---
    config.appendChild(sectionTitle("Screensaver"));

    var ssPanel = document.createElement("div");
    ssPanel.className = "sp-panel";

    ssPanel.appendChild(fieldLabel("Idle Timeout"));
    var numRow = document.createElement("div");
    numRow.className = "sp-number-row";
    var numInp = document.createElement("input");
    numInp.type = "number";
    numInp.className = "sp-number";
    numInp.id = "sp-set-ss-timeout";
    numInp.min = "30";
    numInp.max = "1800";
    numInp.step = "30";
    numInp.value = "300";
    var numUnit = document.createElement("span");
    numUnit.className = "sp-number-unit";
    numUnit.textContent = "seconds";
    numInp.addEventListener("blur", function () { postNumber("screensaver_timeout", this.value); });
    numInp.addEventListener("keydown", function (e) { if (e.key === "Enter") this.blur(); });
    numRow.appendChild(numInp);
    numRow.appendChild(numUnit);
    ssPanel.appendChild(numRow);
    els.setSSTimeout = numInp;

    ssPanel.appendChild(fieldLabel("Presence Sensor Entity"));
    var presInp = textInput("sp-set-presence", "", "e.g. binary_sensor.presence");
    ssPanel.appendChild(presInp);
    presInp.addEventListener("blur", function () { postText("presence_sensor_entity", this.value); });
    presInp.addEventListener("keydown", function (e) { if (e.key === "Enter") this.blur(); });
    els.setPresence = presInp;

    config.appendChild(ssPanel);

    page.appendChild(config);
    page.appendChild(buildApplyBar());

    parent.appendChild(page);
    els.settingsPage = page;
  }

  // ── Settings page helpers ───────────────────────────────────────────

  function sectionTitle(text) {
    var el = document.createElement("div");
    el.className = "sp-section-title";
    el.textContent = text;
    return el;
  }

  function fieldLabel(text) {
    var el = document.createElement("label");
    el.className = "sp-field-label";
    el.textContent = text;
    el.style.marginTop = "10px";
    el.style.display = "block";
    return el;
  }

  function textInput(id, value, placeholder) {
    var el = document.createElement("input");
    el.type = "text";
    el.className = "sp-input";
    el.id = id;
    el.value = value;
    el.placeholder = placeholder || "";
    el.style.marginBottom = "4px";
    return el;
  }

  function toggleRow(label, id, checked) {
    var row = document.createElement("div");
    row.className = "sp-toggle-row";
    var lbl = document.createElement("span");
    lbl.className = "sp-toggle-label";
    lbl.textContent = label;
    row.appendChild(lbl);
    var toggle = document.createElement("label");
    toggle.className = "sp-toggle";
    var inp = document.createElement("input");
    inp.type = "checkbox";
    inp.id = id;
    inp.checked = !!checked;
    var track = document.createElement("span");
    track.className = "sp-toggle-track";
    toggle.appendChild(inp);
    toggle.appendChild(track);
    row.appendChild(toggle);
    return { row: row, input: inp };
  }

  function condField() {
    var el = document.createElement("div");
    el.className = "sp-cond-field";
    return el;
  }

  function buildApplyBar() {
    var bar = document.createElement("div");
    bar.className = "sp-apply-bar";
    var btn = document.createElement("button");
    btn.className = "sp-apply-btn";
    btn.textContent = "Apply Configuration";
    btn.addEventListener("click", function () {
      if (document.activeElement && document.activeElement.blur) {
        document.activeElement.blur();
      }
      btn.disabled = true;
      btn.textContent = "Restarting\u2026";
      setTimeout(function () { postButtonPress("apply_configuration"); }, 600);
    });
    bar.appendChild(btn);
    var note = document.createElement("div");
    note.className = "sp-apply-note";
    note.textContent = "Restarts the device to apply changes";
    bar.appendChild(note);
    return bar;
  }

  // ── Logs Page ────────────────────────────────────────────────────────

  function buildLogsPage(parent) {
    var page = document.createElement("div");
    page.id = "sp-logs";
    page.className = "sp-page";

    var toolbar = document.createElement("div");
    toolbar.className = "sp-log-toolbar";
    var clearBtn = document.createElement("button");
    clearBtn.className = "sp-log-clear";
    clearBtn.textContent = "Clear";
    clearBtn.addEventListener("click", function () { els.logOutput.innerHTML = ""; });
    toolbar.appendChild(clearBtn);
    page.appendChild(toolbar);

    var output = document.createElement("div");
    output.className = "sp-log-output";
    page.appendChild(output);
    els.logOutput = output;

    parent.appendChild(page);
    els.logsPage = page;
  }

  // ── Tab switching ────────────────────────────────────────────────────

  function switchTab(tab) {
    state.activeTab = tab;
    ["screen", "settings", "logs"].forEach(function (t) {
      els["tab_" + t].className = "sp-tab" + (tab === t ? " active" : "");
    });
    els.screenPage.className = "sp-page" + (tab === "screen" ? " active" : "");
    els.settingsPage.className = "sp-page" + (tab === "settings" ? " active" : "");
    els.logsPage.className = "sp-page" + (tab === "logs" ? " active" : "");
  }

  // ── Preview rendering ────────────────────────────────────────────────

  function renderPreview() {
    var main = els.previewMain;
    main.innerHTML = "";

    state.order.forEach(function (slot, idx) {
      var b = state.buttons[slot - 1];
      var iconName = resolveIcon(slot);
      var label = b.label || b.entity || "Configure";
      var color = state.offColor;

      var btn = document.createElement("div");
      btn.className = "sp-btn" + (state.selectedSlot === slot ? " sp-selected" : "");
      btn.style.backgroundColor = "#" + (color.length === 6 ? color : "313131");
      btn.draggable = true;
      btn.innerHTML =
        '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>' +
        '<span class="sp-btn-label">' + escHtml(label) + "</span>";
      btn.addEventListener("click", function () {
        if (didDrag) { didDrag = false; return; }
        selectButton(slot);
      });
      setupPreviewDrag(btn, idx);
      main.appendChild(btn);
    });

    if (state.order.length < NUM_SLOTS) {
      var add = document.createElement("div");
      add.className = "sp-btn sp-add-btn";
      add.innerHTML = '<span class="sp-add-icon mdi mdi-plus"></span>';
      add.addEventListener("click", addButton);
      main.appendChild(add);
    }
  }

  // ── Button list rendering ───────────────────────────────────────────

  function renderButtonList() {
    var list = els.buttonList;
    list.innerHTML = "";

    if (state.order.length === 0) {
      var empty = document.createElement("li");
      empty.className = "sp-empty";
      empty.textContent = 'No buttons yet \u2014 tap + on the screen to add one.';
      list.appendChild(empty);
      return;
    }

    state.order.forEach(function (slot, pos) {
      var b = state.buttons[slot - 1];
      var iconName = resolveIcon(slot);
      var name = btnDisplayName(slot);
      var isSelected = state.selectedSlot === slot;

      var item = document.createElement("li");
      item.className = "sp-list-item" + (isSelected ? " sp-list-selected" : "");
      item.dataset.pos = pos;
      item.draggable = true;
      item.innerHTML =
        '<span class="sp-list-handle mdi mdi-drag"></span>' +
        '<span class="sp-list-icon mdi mdi-' + iconName + '"></span>' +
        '<div class="sp-list-info">' +
        '<div class="sp-list-name">' + escHtml(name) + "</div>" +
        (b.entity ? '<div class="sp-list-entity">' + escHtml(b.entity) + "</div>" : "") +
        "</div>";

      item.addEventListener("click", function (e) {
        if (e.target.closest(".sp-list-handle") || e.target.closest(".sp-list-expand")) return;
        selectButton(state.selectedSlot === slot ? -1 : slot);
      });

      if (isSelected) {
        var expand = document.createElement("div");
        expand.className = "sp-list-expand";
        expand.innerHTML =
          '<div class="sp-field">' +
          '<label class="sp-field-label">Entity ID</label>' +
          '<input class="sp-input" id="sp-inp-entity" type="text" ' +
          'placeholder="e.g. light.kitchen" value="' + escAttr(b.entity) + '">' +
          "</div>" +
          '<div class="sp-field">' +
          '<label class="sp-field-label">Label</label>' +
          '<input class="sp-input" id="sp-inp-label" type="text" ' +
          'placeholder="Optional custom label" value="' + escAttr(b.label) + '">' +
          "</div>" +
          '<div class="sp-field">' +
          '<label class="sp-field-label">Icon</label>' +
          '<select class="sp-select" id="sp-inp-icon">' +
          ICON_OPTIONS.map(function (opt) {
            return '<option value="' + opt + '"' + (opt === b.icon ? " selected" : "") + ">" + opt + "</option>";
          }).join("") +
          "</select></div>" +
          '<div class="sp-btn-row">' +
          '<button class="sp-action-btn sp-delete-btn">Delete Button</button></div>';

        expand.querySelector("#sp-inp-entity").addEventListener("blur", function () {
          state.buttons[slot - 1].entity = this.value;
          postText("button_" + slot + "_entity", this.value);
          renderPreview();
          updateListItemHeader(item, slot);
        });
        expand.querySelector("#sp-inp-entity").addEventListener("keydown", function (e) {
          if (e.key === "Enter") this.blur();
        });
        expand.querySelector("#sp-inp-label").addEventListener("blur", function () {
          state.buttons[slot - 1].label = this.value;
          postText("button_" + slot + "_label", this.value);
          renderPreview();
          updateListItemHeader(item, slot);
        });
        expand.querySelector("#sp-inp-label").addEventListener("keydown", function (e) {
          if (e.key === "Enter") this.blur();
        });
        expand.querySelector("#sp-inp-icon").addEventListener("change", function () {
          state.buttons[slot - 1].icon = this.value;
          postSelect("button_" + slot + "_icon", this.value);
          renderPreview();
          updateListItemHeader(item, slot);
        });
        expand.querySelector(".sp-delete-btn").addEventListener("click", function () {
          deleteButton(slot);
        });

        item.appendChild(expand);
      }

      setupDrag(item, pos);
      list.appendChild(item);
    });
  }

  function updateListItemHeader(item, slot) {
    var iconEl = item.querySelector(".sp-list-icon");
    var nameEl = item.querySelector(".sp-list-name");
    var entityEl = item.querySelector(".sp-list-entity");
    var b = state.buttons[slot - 1];
    var iconName = resolveIcon(slot);

    iconEl.className = "sp-list-icon mdi mdi-" + iconName;
    nameEl.textContent = btnDisplayName(slot);
    if (b.entity) {
      if (!entityEl) {
        entityEl = document.createElement("div");
        entityEl.className = "sp-list-entity";
        item.querySelector(".sp-list-info").appendChild(entityEl);
      }
      entityEl.textContent = b.entity;
    } else if (entityEl) {
      entityEl.remove();
    }
  }

  // ── Drag and drop ───────────────────────────────────────────────────

  function setupDrag(item, pos) {
    item.addEventListener("dragstart", function (e) {
      dragSrcPos = pos;
      e.dataTransfer.effectAllowed = "move";
      e.dataTransfer.setData("text/plain", String(pos));
      setTimeout(function () { item.classList.add("sp-dragging"); }, 0);
    });
    item.addEventListener("dragend", function () {
      item.classList.remove("sp-dragging");
      clearDropIndicators();
    });
    item.addEventListener("dragover", function (e) {
      e.preventDefault();
      e.dataTransfer.dropEffect = "move";
      clearDropIndicators();
      var rect = item.getBoundingClientRect();
      if (e.clientY < rect.top + rect.height / 2) {
        item.classList.add("sp-drop-before");
      } else {
        item.classList.add("sp-drop-after");
      }
    });
    item.addEventListener("dragleave", function () {
      item.classList.remove("sp-drop-before", "sp-drop-after");
    });
    item.addEventListener("drop", function (e) {
      e.preventDefault();
      clearDropIndicators();
      if (dragSrcPos < 0) return;
      var rect = item.getBoundingClientRect();
      var toPos = e.clientY < rect.top + rect.height / 2 ? pos : pos + 1;
      if (dragSrcPos < toPos) toPos--;
      if (dragSrcPos === toPos) return;
      var newOrder = state.order.slice();
      var moved = newOrder.splice(dragSrcPos, 1)[0];
      newOrder.splice(toPos, 0, moved);
      postText("button_order", newOrder.join(","));
      dragSrcPos = -1;
    });
  }

  function clearDropIndicators() {
    els.buttonList.querySelectorAll(".sp-drop-before,.sp-drop-after")
      .forEach(function (el) { el.classList.remove("sp-drop-before", "sp-drop-after"); });
  }

  // ── Preview drag and drop ─────────────────────────────────────────

  function clearPreviewDropIndicators() {
    els.previewMain.querySelectorAll(".sp-drop-before,.sp-drop-after")
      .forEach(function (el) { el.classList.remove("sp-drop-before", "sp-drop-after"); });
  }

  function setupPreviewDrag(btn, idx) {
    btn.addEventListener("dragstart", function (e) {
      dragSrcPos = idx;
      didDrag = true;
      e.dataTransfer.effectAllowed = "move";
      e.dataTransfer.setData("text/plain", String(idx));
      setTimeout(function () { btn.classList.add("sp-dragging"); }, 0);
    });
    btn.addEventListener("dragend", function () {
      btn.classList.remove("sp-dragging");
      clearPreviewDropIndicators();
    });
    btn.addEventListener("dragover", function (e) {
      e.preventDefault();
      e.dataTransfer.dropEffect = "move";
      clearPreviewDropIndicators();
      var rect = btn.getBoundingClientRect();
      if (e.clientY < rect.top + rect.height / 2) {
        btn.classList.add("sp-drop-before");
      } else {
        btn.classList.add("sp-drop-after");
      }
    });
    btn.addEventListener("dragleave", function () {
      btn.classList.remove("sp-drop-before", "sp-drop-after");
    });
    btn.addEventListener("drop", function (e) {
      e.preventDefault();
      clearPreviewDropIndicators();
      if (dragSrcPos < 0) return;
      var rect = btn.getBoundingClientRect();
      var toIdx = e.clientY < rect.top + rect.height / 2 ? idx : idx + 1;
      if (dragSrcPos < toIdx) toIdx--;
      if (dragSrcPos === toIdx) return;
      var newOrder = state.order.slice();
      var moved = newOrder.splice(dragSrcPos, 1)[0];
      newOrder.splice(toIdx, 0, moved);
      postText("button_order", newOrder.join(","));
      dragSrcPos = -1;
    });
  }

  // ── Button settings panel ───────────────────────────────────────────

  function selectButton(slot) {
    state.selectedSlot = slot;
    renderPreview();
    renderButtonList();
  }

  // ── Add / Delete buttons ────────────────────────────────────────────

  function addButton() {
    var used = {};
    state.order.forEach(function (s) { used[s] = true; });
    var slot = -1;
    for (var i = 1; i <= NUM_SLOTS; i++) {
      if (!used[i]) { slot = i; break; }
    }
    if (slot < 0) return;
    state.order = state.order.concat(slot);
    postText("button_order", state.order.join(","));
    selectButton(slot);
  }

  function deleteButton(slot) {
    state.order = state.order.filter(function (s) { return s !== slot; });
    if (state.selectedSlot === slot) state.selectedSlot = -1;
    renderPreview();
    renderButtonList();
    postText("button_order", state.order.join(","));
    postText("button_" + slot + "_entity", "");
    postText("button_" + slot + "_label", "");
    postSelect("button_" + slot + "_icon", "Auto");
  }

  // ── Clock ───────────────────────────────────────────────────────────

  function updateClock() {
    if (!els.clock) return;
    var now = new Date();
    els.clock.textContent =
      String(now.getHours()).padStart(2, "0") + ":" +
      String(now.getMinutes()).padStart(2, "0");
  }

  // ── SSE ──────────────────────────────────────────────────────────────

  function connectEvents() {
    var source = new EventSource("/events");

    source.addEventListener("open", function () {
      state.selectedSlot = -1;
      orderReceived = false;
      if (els.banner) els.banner.className = "sp-banner";
      els.root.querySelectorAll(".sp-apply-btn").forEach(function (btn) {
        btn.disabled = false;
        btn.textContent = "Apply Configuration";
      });
    });

    source.addEventListener("error", function () {
      showBanner("Reconnecting to device\u2026", "offline");
    });

    source.addEventListener("state", function (e) {
      var d;
      try { d = JSON.parse(e.data); } catch (_) { return; }
      var id = d.id;
      var val = d.state != null ? String(d.state) : "";

      // --- Button order ---
      if (id === "text-button_order") {
        orderReceived = true;
        state.order = parseOrder(val);
        renderPreview();
        renderButtonList();
        if (state.selectedSlot > 0 && state.order.indexOf(state.selectedSlot) === -1)
          state.selectedSlot = -1;
        return;
      }

      // --- Shared colors ---
      if (id === "text-button_on_color") {
        state.onColor = val;
        syncInput(els.setOnColor, val);
        renderPreview();
        return;
      }
      if (id === "text-button_off_color") {
        state.offColor = val;
        syncInput(els.setOffColor, val);
        renderPreview();
        return;
      }

      // --- Per-button entity / label ---
      var textMatch = id.match(/^text-button_(\d+)_(entity|label)$/);
      if (textMatch) {
        var slot = parseInt(textMatch[1], 10);
        var field = textMatch[2];
        if (slot >= 1 && slot <= NUM_SLOTS) {
          state.buttons[slot - 1][field] = val;
          renderPreview();
          if (state.selectedSlot === slot && isExpandFocused()) {
            syncInput(document.getElementById(field === "entity" ? "sp-inp-entity" : "sp-inp-label"), val);
          } else {
            renderButtonList();
          }
          scheduleMigration();
        }
        return;
      }

      // --- Per-button icon ---
      var selMatch = id.match(/^select-button_(\d+)_icon$/);
      if (selMatch) {
        var slot = parseInt(selMatch[1], 10);
        if (slot >= 1 && slot <= NUM_SLOTS) {
          state.buttons[slot - 1].icon = val;
          renderPreview();
          if (state.selectedSlot === slot && isExpandFocused()) {
            syncInput(document.getElementById("sp-inp-icon"), val);
          } else {
            renderButtonList();
          }
        }
        return;
      }

      // --- Temperature switches ---
      if (id === "switch-indoor_temp_enable") {
        state._indoorOn = d.value === true || val === "ON";
        els.setIndoorToggle.checked = state._indoorOn;
        els.setIndoorField.className = "sp-cond-field" + (state._indoorOn ? " sp-visible" : "");
        updateTempPreview();
        return;
      }
      if (id === "switch-outdoor_temp_enable") {
        state._outdoorOn = d.value === true || val === "ON";
        els.setOutdoorToggle.checked = state._outdoorOn;
        els.setOutdoorField.className = "sp-cond-field" + (state._outdoorOn ? " sp-visible" : "");
        updateTempPreview();
        return;
      }

      // --- Temperature entities ---
      if (id === "text-indoor_temp_entity") {
        state.indoorEntity = val;
        syncInput(els.setIndoorEntity, val);
        return;
      }
      if (id === "text-outdoor_temp_entity") {
        state.outdoorEntity = val;
        syncInput(els.setOutdoorEntity, val);
        return;
      }

      // --- Screensaver ---
      if (id === "number-screensaver_timeout") {
        state.screensaverTimeout = parseFloat(val) || 300;
        syncInput(els.setSSTimeout, String(state.screensaverTimeout));
        return;
      }
      if (id === "text-presence_sensor_entity") {
        state.presenceEntity = val;
        syncInput(els.setPresence, val);
        return;
      }

      // --- Backlight ---
      if (id === "light-display_backlight") {
        var br = d.brightness != null ? d.brightness : 255;
        state.backlight = br;
        els.setBacklight.value = br;
        els.setBacklightVal.textContent = br;
        return;
      }

      console.log("[SSE] unhandled:", id, val);
    });

    // Log events
    source.addEventListener("log", function (e) {
      var d;
      try { d = JSON.parse(e.data); } catch (_) { d = { msg: e.data }; }
      appendLog(d.msg || e.data, d.lvl);
    });
  }

  function syncInput(el, val) {
    if (el && document.activeElement !== el) el.value = val;
  }

  function scheduleMigration() {
    if (orderReceived || state.order.length > 0) return;
    clearTimeout(migrationTimer);
    migrationTimer = setTimeout(function () {
      if (orderReceived || state.order.length > 0) return;
      var autoOrder = [];
      for (var i = 0; i < NUM_SLOTS; i++) {
        if (state.buttons[i].entity) autoOrder.push(i + 1);
      }
      if (autoOrder.length > 0) {
        state.order = autoOrder;
        renderPreview();
        renderButtonList();
        postText("button_order", autoOrder.join(","));
      }
    }, 2000);
  }

  function updateTempPreview() {
    var show = state._indoorOn || state._outdoorOn;
    els.temp.className = "sp-temp" + (show ? " sp-visible" : "");
    if (state._indoorOn && state._outdoorOn) {
      els.temp.textContent = "-\u00B0 / -\u00B0";
    } else if (state._indoorOn || state._outdoorOn) {
      els.temp.textContent = "-\u00B0";
    }
  }

  // ── Log viewer ──────────────────────────────────────────────────────

  function appendLog(msg, lvl) {
    if (!els.logOutput) return;
    var line = document.createElement("div");
    line.className = "sp-log-line";
    if (lvl === 1) line.classList.add("sp-log-error");
    else if (lvl === 2) line.classList.add("sp-log-warn");
    else if (lvl === 5) line.classList.add("sp-log-debug");
    else if (lvl >= 6) line.classList.add("sp-log-verbose");
    line.textContent = msg;

    var atBottom = els.logOutput.scrollHeight - els.logOutput.scrollTop - els.logOutput.clientHeight < 40;
    els.logOutput.appendChild(line);
    while (els.logOutput.childNodes.length > 1000)
      els.logOutput.removeChild(els.logOutput.firstChild);
    if (atBottom) els.logOutput.scrollTop = els.logOutput.scrollHeight;
  }

  // ── Start ───────────────────────────────────────────────────────────

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", init);
  } else {
    init();
  }
})();
