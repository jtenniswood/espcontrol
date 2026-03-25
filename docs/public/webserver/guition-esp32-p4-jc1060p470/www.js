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
    Account: "account",
    "Air Conditioner": "air-conditioner",
    "Air Filter": "air-filter",
    "Air Purifier": "air-purifier",
    Alarm: "bell-ring",
    "Alarm Light": "alarm-light",
    Application: "application-outline",
    Battery: "battery",
    "Battery 10%": "battery-10",
    "Battery 20%": "battery-20",
    "Battery 30%": "battery-30",
    "Battery 40%": "battery-40",
    "Battery 50%": "battery-50",
    "Battery 60%": "battery-60",
    "Battery 70%": "battery-70",
    "Battery 80%": "battery-80",
    "Battery 90%": "battery-90",
    "Battery Alert": "battery-alert",
    "Battery Charging": "battery-charging",
    "Battery Charging 100": "battery-charging-100",
    "Battery Charging 70": "battery-charging-70",
    "Battery High": "battery-high",
    "Battery Low": "battery-low",
    "Battery Medium": "battery-medium",
    "Battery Off": "battery-off",
    "Battery Outline": "battery-outline",
    "Battery Unknown": "battery-unknown",
    Bed: "bed",
    Bell: "bell",
    Blinds: "blinds",
    "Blinds Horizontal": "blinds-horizontal",
    "Blinds Horizontal Closed": "blinds-horizontal-closed",
    "Blinds Open": "blinds-open",
    Bluetooth: "bluetooth",
    Broom: "broom",
    Camera: "camera",
    "Car Electric": "car-electric",
    Cast: "cast",
    CCTV: "cctv",
    "Ceiling Fan": "ceiling-fan",
    "Ceiling Light": "ceiling-light",
    Chandelier: "chandelier",
    Clock: "clock-outline",
    "Coffee Maker": "coffee-maker",
    "Current AC": "current-ac",
    "Current DC": "current-dc",
    Curtains: "curtains",
    "Curtains Closed": "curtains-closed",
    "Desk Lamp": "desk-lamp",
    Dishwasher: "dishwasher",
    Dog: "dog",
    Door: "door",
    "Door Open": "door-open",
    Doorbell: "doorbell-video",
    Dryer: "tumble-dryer",
    "EV Charger": "ev-station",
    Fan: "fan",
    Fire: "fire",
    Fireplace: "fireplace",
    Flash: "flash",
    "Floor Lamp": "floor-lamp",
    Fountain: "fountain",
    Fridge: "fridge",
    Gamepad: "gamepad-variant",
    Garage: "garage",
    "Garage Open": "garage-open",
    Gate: "gate",
    "Gate Open": "gate-open",
    Gauge: "gauge",
    "Gauge Empty": "gauge-empty",
    "Gauge Full": "gauge-full",
    "Gauge Low": "gauge-low",
    Headphones: "headphones",
    Heater: "radiator",
    Home: "home",
    "Hot Tub": "hot-tub",
    Humidifier: "air-humidifier",
    Iron: "iron",
    Kettle: "kettle",
    Key: "key-variant",
    Lamp: "lamp",
    LAN: "lan",
    Lawnmower: "robot-mower",
    Leaf: "leaf",
    "LED Strip": "led-strip",
    "LED Strip Variant": "led-strip-variant",
    "Light Switch": "light-switch",
    Lightbulb: "lightbulb",
    "Lightbulb Group": "lightbulb-group",
    "Lightbulb Night": "lightbulb-night",
    "Lightning Bolt": "lightning-bolt",
    Lock: "lock",
    Mailbox: "mailbox",
    "Message Video": "message-video",
    "Meter Electric": "meter-electric",
    "Meter Gas": "meter-gas",
    Microwave: "microwave",
    Monitor: "monitor",
    "Motion Sensor": "motion-sensor",
    "Movie Roll": "movie-roll",
    Music: "music",
    "Outdoor Lamp": "coach-lamp",
    Oven: "stove",
    Package: "package-variant",
    "Package Closed": "package-variant-closed",
    Pool: "pool",
    Power: "power",
    "Power Plug": "power-plug",
    Printer: "printer",
    "Printer 3D": "printer-3d",
    Projector: "projector",
    "Robot Vacuum": "robot-vacuum",
    "Roller Shade": "roller-shade",
    "Roller Shade Closed": "roller-shade-closed",
    Router: "router-wireless",
    "Router Network": "router-network",
    Security: "shield-home",
    Shower: "shower-head",
    "Smoke Detector": "smoke-detector",
    Snowflake: "snowflake",
    "Snowflake Alert": "snowflake-alert",
    "Snowflake Thermometer": "snowflake-thermometer",
    Sofa: "sofa",
    "Solar Panel": "solar-panel",
    "Solar Panel Large": "solar-panel-large",
    "Solar Power": "solar-power",
    "Solar Power Variant": "solar-power-variant",
    Speaker: "speaker",
    Spotlight: "spotlight-beam",
    Sprinkler: "sprinkler",
    "String Lights": "string-lights",
    Sun: "white-balance-sunny",
    Table: "table-furniture",
    Television: "television",
    Thermometer: "thermometer",
    "Thermometer Alert": "thermometer-alert",
    "Thermometer High": "thermometer-high",
    "Thermometer Low": "thermometer-low",
    Thermostat: "thermostat",
    Timer: "timer-outline",
    Toilet: "toilet",
    "Trash Can": "trash-can",
    "Transmission Tower": "transmission-tower",
    "Grid Export": "transmission-tower-export",
    "Grid Import": "transmission-tower-import",
    "Grid Off": "transmission-tower-off",
    "Wall Outlet": "power-socket",
    Washer: "washing-machine",
    Water: "water",
    "Water Heater": "water-boiler",
    "Water Percent": "water-percent",
    "Humidity Alert": "water-percent-alert",
    "Weather Cloudy": "weather-cloudy",
    "Weather Cloudy Alert": "weather-cloudy-alert",
    "Weather Dust": "weather-dust",
    "Weather Fog": "weather-fog",
    "Weather Hail": "weather-hail",
    "Weather Hazy": "weather-hazy",
    "Weather Hurricane": "weather-hurricane",
    "Weather Lightning": "weather-lightning",
    "Weather Lightning Rainy": "weather-lightning-rainy",
    "Weather Night": "weather-night",
    "Weather Night Cloudy": "weather-night-partly-cloudy",
    "Weather Partly Cloudy": "weather-partly-cloudy",
    "Weather Partly Lightning": "weather-partly-lightning",
    "Weather Partly Rainy": "weather-partly-rainy",
    "Weather Partly Snowy": "weather-partly-snowy",
    "Weather Partly Snowy Rainy": "weather-partly-snowy-rainy",
    "Weather Pouring": "weather-pouring",
    "Weather Rainy": "weather-rainy",
    "Weather Snowy": "weather-snowy",
    "Weather Snowy Heavy": "weather-snowy-heavy",
    "Weather Snowy Rainy": "weather-snowy-rainy",
    "Weather Sunny": "weather-sunny",
    "Weather Sunny Alert": "weather-sunny-alert",
    "Weather Sunny Off": "weather-sunny-off",
    "Weather Sunset": "weather-sunset",
    "Weather Sunset Down": "weather-sunset-down",
    "Weather Sunset Up": "weather-sunset-up",
    "Weather Tornado": "weather-tornado",
    "Weather Windy": "weather-windy",
    "Weather Windy Variant": "weather-windy-variant",
    "Wind Power": "wind-power",
    "Wind Turbine": "wind-turbine",
    "Wind Turbine Alert": "wind-turbine-alert",
    "Wind Turbine Check": "wind-turbine-check",
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

  var ICON_OPTIONS = Object.keys(ICON_MAP).sort();

  var CSS =
    "#sp-app{font-family:Roboto,sans-serif;color:#e0e0e0;max-width:900px;margin:0 auto}" +
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
    ".sp-topbar{position:absolute;top:0;left:0;right:0;height:3.2cqw;" +
    "display:flex;align-items:center;padding:0.39cqw;z-index:1}" +
    ".sp-temp{color:#fff;font-size:1.95cqw;white-space:nowrap;opacity:0;transition:opacity .3s}" +
    ".sp-temp.sp-visible{opacity:1}" +
    ".sp-clock{position:absolute;left:50%;transform:translateX(-50%);" +
    "color:#fff;font-size:1.95cqw;white-space:nowrap}" +
    ".sp-main{position:absolute;top:3.9cqw;left:0.49cqw;right:0.49cqw;bottom:0.49cqw;" +
    "display:flex;flex-direction:column;flex-wrap:wrap;align-content:flex-start;gap:0.98cqw}" +

    // Preview buttons
    ".sp-btn{width:19cqw;height:12.5cqw;border-radius:0.78cqw;padding:1.37cqw;" +
    "display:flex;flex-direction:column;justify-content:space-between;" +
    "cursor:pointer;transition:all .2s;box-sizing:border-box;border:2px solid transparent;" +
    "position:relative}" +
    ".sp-btn:hover{filter:brightness(1.15)}" +
    ".sp-btn.sp-selected{border-color:#03a9f4}" +
    ".sp-btn-icon{font-size:4.69cqw;line-height:1;color:#fff}" +
    ".sp-btn-label{font-size:1.8cqw;line-height:1.2;color:#fff;" +
    "white-space:nowrap;overflow:hidden;text-overflow:ellipsis}" +
    ".sp-sensor-badge{position:absolute;top:1cqw;right:1cqw;font-size:1.6cqw;opacity:.5}" +
    ".sp-add-btn{border:2px dashed rgba(255,255,255,.25);background:transparent !important;" +
    "display:flex;align-items:center;justify-content:center;cursor:pointer}" +
    ".sp-add-btn:hover{border-color:#03a9f4}" +
    ".sp-add-icon{font-size:5cqw;color:rgba(255,255,255,.35)}" +
    ".sp-drop-placeholder{border:2px dashed rgba(3,169,244,.5) !important;" +
    "background:rgba(3,169,244,.08) !important;cursor:default;pointer-events:none}" +

    // Hint
    ".sp-hint{text-align:center;font-size:11px;opacity:.4;padding:6px 0 12px}" +

    // Config area
    ".sp-config{padding:0 16px 16px}" +
    ".sp-section-title{font-size:13px;font-weight:500;color:#888;" +
    "margin:16px 0 8px;text-transform:uppercase;letter-spacing:0.5px}" +

    // Settings page panels
    ".sp-panel{background:#1e1e1e;border-radius:8px;padding:16px;margin-top:12px;" +
    "border:1px solid #2a2a2a}" +

    // Form fields
    ".sp-field{margin-bottom:12px}" +
    ".sp-field-label{display:block;font-size:11px;color:#888;margin-top:10px;margin-bottom:4px;" +
    "text-transform:uppercase;letter-spacing:0.3px}" +
    ".sp-input,.sp-select{width:100%;padding:9px 12px;background:#2a2a2a;" +
    "border:1px solid #444;border-radius:6px;color:#e0e0e0;font-size:14px;" +
    "font-family:inherit;box-sizing:border-box;outline:none;transition:border-color .2s;" +
    "margin-bottom:4px}" +
    ".sp-input:focus,.sp-select:focus{border-color:#03a9f4}" +
    ".sp-input--narrow{width:80px}" +
    ".sp-select{appearance:auto}" +

    // Searchable icon picker
    ".sp-icon-picker{position:relative}" +
    ".sp-icon-picker-input{width:100%;padding:9px 12px;padding-left:36px;background:#2a2a2a;" +
    "border:1px solid #444;border-radius:6px;color:#e0e0e0;font-size:14px;" +
    "font-family:inherit;box-sizing:border-box;outline:none;transition:border-color .2s}" +
    ".sp-icon-picker-input:focus{border-color:#03a9f4}" +
    ".sp-icon-picker-input::placeholder{color:#666}" +
    ".sp-icon-picker-preview{position:absolute;left:10px;top:50%;transform:translateY(-50%);" +
    "font-size:18px;color:#aaa;pointer-events:none}" +
    ".sp-icon-picker.sp-open .sp-icon-picker-preview{top:19px}" +
    ".sp-icon-dropdown{display:none;position:absolute;left:0;right:0;top:100%;margin-top:4px;" +
    "background:#2a2a2a;border:1px solid #444;border-radius:6px;max-height:200px;" +
    "overflow-y:auto;z-index:50;box-shadow:0 4px 12px rgba(0,0,0,.4)}" +
    ".sp-icon-picker.sp-open .sp-icon-dropdown{display:block}" +
    ".sp-icon-option{display:flex;align-items:center;gap:10px;padding:8px 12px;" +
    "cursor:pointer;font-size:14px;color:#e0e0e0;transition:background .1s}" +
    ".sp-icon-option:hover,.sp-icon-option.sp-highlighted{background:#3a3a3a}" +
    ".sp-icon-option.sp-active{background:#1a2a3a}" +
    ".sp-icon-option-icon{font-size:20px;width:24px;text-align:center;color:#aaa;flex-shrink:0}" +
    ".sp-icon-option-label{white-space:nowrap;overflow:hidden;text-overflow:ellipsis}" +
    ".sp-icon-option--empty{color:#666}" +
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

    // Color picker
    ".sp-color-row{display:flex;align-items:center;gap:8px}" +
    ".sp-color-swatch{width:40px;height:38px;border-radius:6px;border:1px solid #444;" +
    "cursor:pointer;flex-shrink:0;position:relative;overflow:hidden;transition:border-color .2s}" +
    ".sp-color-swatch:hover{border-color:#03a9f4}" +
    ".sp-color-swatch input{position:absolute;inset:-8px;width:calc(100% + 16px);" +
    "height:calc(100% + 16px);cursor:pointer;opacity:0}" +
    ".sp-color-row .sp-input{flex:1}" +

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

    // Context menu
    ".sp-ctx-menu{position:fixed;z-index:200;background:#1e1e1e;border:1px solid #444;" +
    "border-radius:8px;padding:4px 0;min-width:160px;box-shadow:0 4px 16px rgba(0,0,0,.5);" +
    "font-family:Roboto,sans-serif}" +
    ".sp-ctx-item{display:flex;align-items:center;gap:10px;padding:8px 14px;" +
    "cursor:pointer;font-size:13px;color:#e0e0e0;transition:background .1s;white-space:nowrap}" +
    ".sp-ctx-item:hover{background:#3a3a3a}" +
    ".sp-ctx-item .mdi{font-size:16px;width:18px;text-align:center;color:#aaa}" +
    ".sp-ctx-item.sp-ctx-danger{color:#ef5350}" +
    ".sp-ctx-item.sp-ctx-danger .mdi{color:#ef5350}" +
    ".sp-ctx-divider{height:1px;background:#333;margin:4px 0}" +

    // Connection banner
    ".sp-banner{padding:10px 16px;font-size:13px;text-align:center;display:none}" +
    ".sp-banner.sp-error{display:block;background:#d32f2f;color:#fff}" +
    ".sp-banner.sp-offline{display:block;background:#f57c00;color:#fff}" +
    ".sp-banner.sp-success{display:block;background:#2e7d32;color:#fff}" +

    // Backup buttons
    ".sp-backup-btns{display:flex;gap:8px;margin-top:4px}" +
    ".sp-backup-btn{flex:1;display:flex;align-items:center;justify-content:center;gap:8px;" +
    "padding:10px 16px;border:none;border-radius:6px;font-size:13px;font-weight:500;" +
    "cursor:pointer;font-family:inherit;transition:filter .15s;background:#333;color:#e0e0e0}" +
    ".sp-backup-btn:hover{filter:brightness(1.15)}" +
    ".sp-backup-btn .mdi{font-size:16px}" +

    // Sun info
    ".sp-sun-info{font-size:13px;color:#888;padding:8px 0 2px;display:none}" +
    ".sp-sun-info.sp-visible{display:block}" +

    // Field hint
    ".sp-field-hint{font-size:11px;color:#666;margin-top:2px}" +

    // Firmware
    ".sp-fw-row{display:flex;align-items:center;justify-content:space-between;gap:8px;padding:4px 0}" +
    ".sp-fw-version{font-size:14px;color:#e0e0e0}" +
    ".sp-fw-label{font-size:12px;color:#888}" +
    ".sp-fw-btn{background:#333;color:#e0e0e0;border:none;border-radius:6px;" +
    "padding:8px 16px;font-size:13px;font-weight:500;cursor:pointer;font-family:inherit;" +
    "transition:filter .15s;white-space:nowrap}" +
    ".sp-fw-btn:hover{filter:brightness(1.15)}" +
    ".sp-fw-btn:disabled{opacity:.5;cursor:default}";

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
    brightnessDayVal: 100,
    brightnessNightVal: 75,
    sunrise: "",
    sunset: "",
    firmwareVersion: "",
    autoUpdate: true,
    updateFrequency: "Daily",
    updateFreqOptions: ["Hourly", "Daily", "Weekly", "Monthly"],
  };

  for (var i = 0; i < NUM_SLOTS; i++) {
    state.buttons.push({ entity: "", label: "", icon: "Auto", sensor: "", unit: "" });
  }

  var els = {};
  var dragSrcPos = -1;
  var didDrag = false;
  var previewPlaceholder = null;
  var previewDropIdx = -1;
  var orderReceived = false;
  var migrationTimer = null;
  var _eventSource = null;

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

  function postText(name, value) {
    post("/text/" + encodeURIComponent(name) + "/set?value=" + encodeURIComponent(value));
  }

  function postSelect(name, option) {
    post("/select/" + encodeURIComponent(name) + "/set?option=" + encodeURIComponent(option));
  }

  function postButtonPress(name) {
    post("/button/" + encodeURIComponent(name) + "/press");
  }

  function postSwitch(name, on) {
    post("/switch/" + encodeURIComponent(name) + "/" + (on ? "turn_on" : "turn_off"));
  }

  function postNumber(name, value) {
    post("/number/" + encodeURIComponent(name) + "/set?value=" + encodeURIComponent(value));
  }

  function waitForReboot() {
    if (_eventSource) { _eventSource.close(); _eventSource = null; }
    showBanner("Restarting device\u2026", "offline");
    setTimeout(function () {
      if (els.banner) els.banner.className = "sp-banner";
      els.root.querySelectorAll(".sp-apply-btn").forEach(function (btn) {
        btn.disabled = false;
        btn.textContent = "Apply Configuration";
      });
      connectEvents();
    }, 15000);
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

  function isSettingsFocused() {
    var ae = document.activeElement;
    return ae && els.buttonSettings && els.buttonSettings.contains(ae);
  }

  function bindTextPost(input, postName, opts) {
    input.addEventListener("blur", function () {
      if (opts && opts.onBlur) opts.onBlur(this.value);
      postText(postName, this.value);
      if (opts && opts.rerender) renderPreview();
    });
    input.addEventListener("keydown", function (e) { if (e.key === "Enter") this.blur(); });
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
    setupPreviewDropZone();
    renderPreview();
    renderButtonSettings();
    connectEvents();
    updateClock();
    setInterval(updateClock, 30000);

    document.addEventListener("click", hideContextMenu);
    document.addEventListener("scroll", hideContextMenu, true);
    document.addEventListener("keydown", function (e) {
      if (e.key === "Escape") hideContextMenu();
    });
  }

  // ── Build UI ─────────────────────────────────────────────────────────

  function showBanner(msg, type) {
    if (!els.banner) return;
    els.banner.textContent = msg;
    els.banner.className = "sp-banner sp-" + type;
    if (type === "error" || type === "success") {
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

    // Button settings (shown when a preview button is selected)
    var config = document.createElement("div");
    config.className = "sp-config";
    els.buttonSettings = config;

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
    var onColor = colorField("sp-set-on-color", "FF8C00", function (hex) {
      postText("Button On Color", hex);
    });
    appearPanel.appendChild(onColor);
    els.setOnColor = onColor;

    // Off Color
    appearPanel.appendChild(fieldLabel("Off Color"));
    var offColor = colorField("sp-set-off-color", "313131", function (hex) {
      postText("Button Off Color", hex);
    });
    appearPanel.appendChild(offColor);
    els.setOffColor = offColor;

    config.appendChild(appearPanel);

    // --- Brightness ---
    config.appendChild(sectionTitle("Brightness"));

    var blPanel = document.createElement("div");
    blPanel.className = "sp-panel";

    var daySlider = createRangeSlider("Daytime Brightness", state.brightnessDayVal, "Screen: Daytime Brightness");
    blPanel.appendChild(daySlider.label);
    blPanel.appendChild(daySlider.row);
    els.setDayBrightness = daySlider.range;
    els.setDayBrightnessVal = daySlider.val;

    var nightSlider = createRangeSlider("Nighttime Brightness", state.brightnessNightVal, "Screen: Nighttime Brightness");
    blPanel.appendChild(nightSlider.label);
    blPanel.appendChild(nightSlider.row);
    els.setNightBrightness = nightSlider.range;
    els.setNightBrightnessVal = nightSlider.val;

    // Sun info
    var sunInfo = document.createElement("div");
    sunInfo.className = "sp-sun-info";
    sunInfo.id = "sp-sun-info";
    blPanel.appendChild(sunInfo);
    els.sunInfo = sunInfo;
    updateSunInfo();

    config.appendChild(blPanel);

    // --- Temperature ---
    config.appendChild(sectionTitle("Temperature"));

    var tempPanel = document.createElement("div");
    tempPanel.className = "sp-panel";

    var indoor = createEntityToggleSection("Indoor Temperature", "sp-set-indoor-toggle", state._indoorOn,
      "Indoor Temp Enable", "Indoor Temp Entity", "Indoor Temp Entity", "e.g. sensor.indoor_temperature");
    tempPanel.appendChild(indoor.toggle.row);
    tempPanel.appendChild(indoor.field);
    els.setIndoorToggle = indoor.toggle.input;
    els.setIndoorField = indoor.field;
    els.setIndoorEntity = indoor.input;

    var outdoor = createEntityToggleSection("Outdoor Temperature", "sp-set-outdoor-toggle", state._outdoorOn,
      "Outdoor Temp Enable", "Outdoor Temp Entity", "Outdoor Temp Entity", "e.g. sensor.outdoor_temperature");
    tempPanel.appendChild(outdoor.toggle.row);
    tempPanel.appendChild(outdoor.field);
    els.setOutdoorToggle = outdoor.toggle.input;
    els.setOutdoorField = outdoor.field;
    els.setOutdoorEntity = outdoor.input;

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
    numInp.addEventListener("blur", function () { postNumber("Screensaver Timeout", this.value); });
    numInp.addEventListener("keydown", function (e) { if (e.key === "Enter") this.blur(); });
    numRow.appendChild(numInp);
    numRow.appendChild(numUnit);
    ssPanel.appendChild(numRow);
    els.setSSTimeout = numInp;

    ssPanel.appendChild(fieldLabel("Presence Sensor Entity"));
    var presInp = textInput("sp-set-presence", "", "e.g. binary_sensor.presence");
    ssPanel.appendChild(presInp);
    bindTextPost(presInp, "Presence Sensor Entity", {});
    els.setPresence = presInp;

    config.appendChild(ssPanel);

    // --- Backup ---
    config.appendChild(sectionTitle("Backup"));

    var backupPanel = document.createElement("div");
    backupPanel.className = "sp-panel";

    var backupRow = document.createElement("div");
    backupRow.className = "sp-backup-btns";

    var exportBtn = document.createElement("button");
    exportBtn.className = "sp-backup-btn";
    exportBtn.innerHTML = '<span class="mdi mdi-download"></span>Export';
    exportBtn.addEventListener("click", exportConfig);
    backupRow.appendChild(exportBtn);

    var importBtn = document.createElement("button");
    importBtn.className = "sp-backup-btn";
    importBtn.innerHTML = '<span class="mdi mdi-upload"></span>Import';
    importBtn.addEventListener("click", importConfig);
    backupRow.appendChild(importBtn);

    backupPanel.appendChild(backupRow);
    config.appendChild(backupPanel);

    // --- Firmware ---
    config.appendChild(sectionTitle("Firmware"));

    var fwPanel = document.createElement("div");
    fwPanel.className = "sp-panel";

    var fwVersionRow = document.createElement("div");
    fwVersionRow.className = "sp-fw-row";
    var fwVersionLabel = document.createElement("span");
    fwVersionLabel.className = "sp-fw-version";
    fwVersionLabel.innerHTML = '<span class="sp-fw-label">Installed </span>' + escHtml(state.firmwareVersion || "dev");
    fwVersionRow.appendChild(fwVersionLabel);
    els.fwVersionLabel = fwVersionLabel;

    var fwCheckBtn = document.createElement("button");
    fwCheckBtn.className = "sp-fw-btn";
    fwCheckBtn.textContent = "Check for Update";
    fwCheckBtn.addEventListener("click", function () {
      fwCheckBtn.disabled = true;
      fwCheckBtn.textContent = "Checking\u2026";
      postButtonPress("Firmware: Check for Update");
      setTimeout(function () {
        fwCheckBtn.disabled = false;
        fwCheckBtn.textContent = "Check for Update";
      }, 10000);
    });
    fwVersionRow.appendChild(fwCheckBtn);
    fwPanel.appendChild(fwVersionRow);

    var autoUpdateToggle = toggleRow("Auto Update", "sp-set-auto-update", state.autoUpdate);
    fwPanel.appendChild(autoUpdateToggle.row);
    autoUpdateToggle.input.addEventListener("change", function () {
      postSwitch("Firmware: Auto Update", this.checked);
    });
    els.setAutoUpdate = autoUpdateToggle.input;

    fwPanel.appendChild(fieldLabel("Update Frequency"));
    var freqSelect = document.createElement("select");
    freqSelect.className = "sp-select";
    freqSelect.id = "sp-set-update-freq";
    state.updateFreqOptions.forEach(function (opt) {
      var o = document.createElement("option");
      o.value = opt;
      o.textContent = opt;
      if (opt === state.updateFrequency) o.selected = true;
      freqSelect.appendChild(o);
    });
    freqSelect.addEventListener("change", function () {
      postSelect("Firmware: Update Frequency", this.value);
    });
    fwPanel.appendChild(freqSelect);
    els.setUpdateFreq = freqSelect;

    config.appendChild(fwPanel);

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
    return el;
  }

  function textInput(id, value, placeholder) {
    var el = document.createElement("input");
    el.type = "text";
    el.className = "sp-input";
    el.id = id;
    el.value = value;
    el.placeholder = placeholder || "";
    return el;
  }

  function colorField(id, value, onChange) {
    var row = document.createElement("div");
    row.className = "sp-color-row";

    var swatch = document.createElement("div");
    swatch.className = "sp-color-swatch";
    swatch.style.backgroundColor = "#" + (value.length === 6 ? value : "000000");

    var picker = document.createElement("input");
    picker.type = "color";
    picker.value = "#" + (value.length === 6 ? value : "000000");
    swatch.appendChild(picker);
    row.appendChild(swatch);

    var inp = document.createElement("input");
    inp.type = "text";
    inp.className = "sp-input";
    inp.id = id;
    inp.value = value;
    inp.placeholder = "6-digit hex e.g. FF8C00";
    row.appendChild(inp);

    picker.addEventListener("input", function () {
      var hex = this.value.replace("#", "").toUpperCase();
      inp.value = hex;
      swatch.style.backgroundColor = "#" + hex;
      onChange(hex);
    });

    inp.addEventListener("blur", function () {
      var hex = this.value.replace(/^#/, "").toUpperCase();
      if (/^[0-9A-F]{6}$/i.test(hex)) {
        swatch.style.backgroundColor = "#" + hex;
        picker.value = "#" + hex;
      }
      onChange(hex);
    });
    inp.addEventListener("keydown", function (e) { if (e.key === "Enter") this.blur(); });

    row._syncColor = function (hex) {
      if (document.activeElement !== inp) inp.value = hex;
      swatch.style.backgroundColor = "#" + (hex.length === 6 ? hex : "000000");
      picker.value = "#" + (hex.length === 6 ? hex : "000000");
    };

    return row;
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

  function createRangeSlider(label, initial, postName) {
    var lbl = fieldLabel(label);
    var row = document.createElement("div");
    row.className = "sp-range-row";
    var range = document.createElement("input");
    range.type = "range";
    range.className = "sp-range";
    range.min = "10";
    range.max = "100";
    range.step = "5";
    range.value = String(initial);
    var val = document.createElement("span");
    val.className = "sp-range-val";
    val.textContent = initial + "%";
    range.addEventListener("input", function () { val.textContent = this.value + "%"; });
    range.addEventListener("change", function () { postNumber(postName, this.value); });
    row.appendChild(range);
    row.appendChild(val);
    return { label: lbl, row: row, range: range, val: val };
  }

  function createEntityToggleSection(label, id, checked, switchName, entityLabel, entityPostName, placeholder) {
    var toggle = toggleRow(label, id, checked);
    var field = condField();
    field.appendChild(fieldLabel(entityLabel));
    var inp = textInput("", "", placeholder);
    field.appendChild(inp);
    toggle.input.addEventListener("change", function () { postSwitch(switchName, this.checked); });
    bindTextPost(inp, entityPostName, {});
    return { toggle: toggle, field: field, input: inp };
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
      setTimeout(function () {
        postButtonPress("Apply Configuration");
        waitForReboot();
      }, 600);
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
      var sensorBadge = b.sensor
        ? '<span class="sp-sensor-badge mdi mdi-gauge"></span>'
        : '';
      btn.innerHTML =
        sensorBadge +
        '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>' +
        '<span class="sp-btn-label">' + escHtml(label) + "</span>";
      btn.addEventListener("click", function () {
        if (didDrag) { didDrag = false; return; }
        selectButton(state.selectedSlot === slot ? -1 : slot);
      });
      btn.addEventListener("contextmenu", function (e) {
        showContextMenu(e, slot);
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

  // ── Button settings panel (shown below preview) ─────────────────────

  function renderButtonSettings() {
    var container = els.buttonSettings;
    container.innerHTML = "";

    if (state.selectedSlot < 1) return;

    var slot = state.selectedSlot;
    var b = state.buttons[slot - 1];

    var title = document.createElement("div");
    title.className = "sp-section-title";
    title.textContent = "Button Settings";
    container.appendChild(title);

    var panel = document.createElement("div");
    panel.className = "sp-panel";

    var ef = document.createElement("div");
    ef.className = "sp-field";
    ef.appendChild(fieldLabel("Entity ID"));
    var entityInp = textInput("sp-inp-entity", b.entity, "e.g. light.kitchen");
    ef.appendChild(entityInp);
    panel.appendChild(ef);

    bindTextPost(entityInp, "Button " + slot + " Entity", {
      onBlur: function (v) { state.buttons[slot - 1].entity = v; },
      rerender: true,
    });

    var lf = document.createElement("div");
    lf.className = "sp-field";
    lf.appendChild(fieldLabel("Label"));
    var labelInp = textInput("sp-inp-label", b.label, "Optional custom label");
    lf.appendChild(labelInp);
    panel.appendChild(lf);

    bindTextPost(labelInp, "Button " + slot + " Label", {
      onBlur: function (v) { state.buttons[slot - 1].label = v; },
      rerender: true,
    });

    var icf = document.createElement("div");
    icf.className = "sp-field";
    icf.appendChild(fieldLabel("Icon"));
    var iconPicker = document.createElement("div");
    iconPicker.className = "sp-icon-picker";
    iconPicker.id = "sp-inp-icon-picker";
    iconPicker.innerHTML =
      '<span class="sp-icon-picker-preview mdi mdi-' + (ICON_MAP[b.icon] || "cog") + '"></span>' +
      '<input class="sp-icon-picker-input" id="sp-inp-icon" type="text" ' +
      'placeholder="Search icons\u2026" value="' + escAttr(b.icon) + '" autocomplete="off">' +
      '<div class="sp-icon-dropdown"></div>';
    icf.appendChild(iconPicker);
    panel.appendChild(icf);

    initIconPicker(iconPicker, b.icon, slot);

    var sensorOn = !!b.sensor;
    var sensorToggle = toggleRow("Sensor When On", "sp-inp-sensor-toggle", sensorOn);
    panel.appendChild(sensorToggle.row);
    var sensorCond = condField();
    if (sensorOn) sensorCond.classList.add("sp-visible");
    sensorCond.appendChild(fieldLabel("Sensor Entity"));
    var sensorInp = textInput("sp-inp-sensor", b.sensor, "e.g. sensor.printer_percent_complete");
    sensorCond.appendChild(sensorInp);
    sensorCond.appendChild(fieldLabel("Unit"));
    var unitInp = textInput("sp-inp-unit", b.unit, "e.g. %");
    unitInp.className = "sp-input sp-input--narrow";
    sensorCond.appendChild(unitInp);
    var sensorHint = document.createElement("div");
    sensorHint.className = "sp-field-hint";
    sensorHint.textContent = "Show sensor value instead of icon when on";
    sensorCond.appendChild(sensorHint);
    panel.appendChild(sensorCond);

    sensorToggle.input.addEventListener("change", function () {
      if (this.checked) {
        sensorCond.classList.add("sp-visible");
      } else {
        sensorCond.classList.remove("sp-visible");
        sensorInp.value = "";
        unitInp.value = "";
        state.buttons[slot - 1].sensor = "";
        state.buttons[slot - 1].unit = "";
        postText("Button " + slot + " Sensor", "");
        postText("Button " + slot + " Sensor Unit", "");
        renderPreview();
      }
    });
    bindTextPost(sensorInp, "Button " + slot + " Sensor", {
      onBlur: function (v) { state.buttons[slot - 1].sensor = v; },
      rerender: true,
    });
    bindTextPost(unitInp, "Button " + slot + " Sensor Unit", {
      onBlur: function (v) { state.buttons[slot - 1].unit = v; },
    });

    var btnRow = document.createElement("div");
    btnRow.className = "sp-btn-row";
    var delBtn = document.createElement("button");
    delBtn.className = "sp-action-btn sp-delete-btn";
    delBtn.textContent = "Delete Button";
    delBtn.addEventListener("click", function () {
      deleteButton(slot);
    });
    btnRow.appendChild(delBtn);
    panel.appendChild(btnRow);

    container.appendChild(panel);
  }

  // ── Searchable icon picker ──────────────────────────────────────────

  function initIconPicker(picker, currentIcon, slot) {
    var input = picker.querySelector(".sp-icon-picker-input");
    var dropdown = picker.querySelector(".sp-icon-dropdown");
    var preview = picker.querySelector(".sp-icon-picker-preview");
    var highlighted = -1;

    function iconClass(name) {
      return ICON_MAP[name] || "cog";
    }

    function buildOptions(filter) {
      dropdown.innerHTML = "";
      highlighted = -1;
      var lc = (filter || "").toLowerCase();
      var matches = ICON_OPTIONS.filter(function (opt) {
        return !lc || opt.toLowerCase().indexOf(lc) !== -1;
      });
      if (matches.length === 0) {
        var empty = document.createElement("div");
        empty.className = "sp-icon-option sp-icon-option--empty";
        empty.textContent = "No matches";
        dropdown.appendChild(empty);
        return;
      }
      matches.forEach(function (opt) {
        var row = document.createElement("div");
        row.className = "sp-icon-option" +
          (opt === currentIcon ? " sp-active" : "");
        row.innerHTML =
          '<span class="sp-icon-option-icon mdi mdi-' + iconClass(opt) + '"></span>' +
          '<span class="sp-icon-option-label">' + escHtml(opt) + '</span>';
        row.addEventListener("mousedown", function (e) {
          e.preventDefault();
          selectOption(opt);
        });
        dropdown.appendChild(row);
      });
    }

    function selectOption(opt) {
      currentIcon = opt;
      input.value = opt;
      preview.className = "sp-icon-picker-preview mdi mdi-" + iconClass(opt);
      closePicker();
      state.buttons[slot - 1].icon = opt;
      postSelect("Button " + slot + " Icon", opt);
      renderPreview();
    }

    function openPicker() {
      buildOptions("");
      picker.classList.add("sp-open");
      input.select();
    }

    function closePicker() {
      picker.classList.remove("sp-open");
      input.value = currentIcon;
      highlighted = -1;
    }

    function highlightAt(idx) {
      var items = dropdown.querySelectorAll(".sp-icon-option:not(.sp-icon-option--empty)");
      if (items.length === 0) return;
      items.forEach(function (el) { el.classList.remove("sp-highlighted"); });
      if (idx < 0) idx = items.length - 1;
      if (idx >= items.length) idx = 0;
      highlighted = idx;
      items[highlighted].classList.add("sp-highlighted");
      items[highlighted].scrollIntoView({ block: "nearest" });
    }

    input.addEventListener("focus", function () {
      openPicker();
    });

    input.addEventListener("blur", function () {
      closePicker();
    });

    input.addEventListener("input", function () {
      buildOptions(this.value);
      if (dropdown.querySelector(".sp-icon-option:not(.sp-icon-option--empty)")) {
        highlightAt(0);
      }
    });

    input.addEventListener("keydown", function (e) {
      var items = dropdown.querySelectorAll(".sp-icon-option:not(.sp-icon-option--empty)");
      if (e.key === "ArrowDown") {
        e.preventDefault();
        if (!picker.classList.contains("sp-open")) { openPicker(); return; }
        highlightAt(highlighted + 1);
      } else if (e.key === "ArrowUp") {
        e.preventDefault();
        highlightAt(highlighted - 1);
      } else if (e.key === "Enter") {
        e.preventDefault();
        if (highlighted >= 0 && highlighted < items.length) {
          var label = items[highlighted].querySelector(".sp-icon-option-label");
          if (label) selectOption(label.textContent);
        }
      } else if (e.key === "Escape") {
        e.preventDefault();
        closePicker();
        input.blur();
      }
    });
  }

  // ── Preview drag and drop ─────────────────────────────────────────

  function removePreviewPlaceholder() {
    if (previewPlaceholder && previewPlaceholder.parentNode) {
      previewPlaceholder.parentNode.removeChild(previewPlaceholder);
    }
    previewPlaceholder = null;
    previewDropIdx = -1;
  }

  function setupPreviewDropZone() {
    var container = els.previewMain;

    container.addEventListener("dragover", function (e) {
      if (dragSrcPos < 0 || !previewPlaceholder) return;
      e.preventDefault();
      e.dataTransfer.dropEffect = "move";

      var buttons = container.querySelectorAll(
        ".sp-btn:not(.sp-drop-placeholder):not(.sp-add-btn)"
      );
      var dropIdx = buttons.length;
      for (var i = 0; i < buttons.length; i++) {
        var rect = buttons[i].getBoundingClientRect();
        if (e.clientX < rect.left) { dropIdx = i; break; }
        if (e.clientX <= rect.right) {
          var midY = rect.top + rect.height / 2;
          if (e.clientY < midY) { dropIdx = i; break; }
          if (e.clientY <= rect.bottom) { dropIdx = i + 1; break; }
        }
      }

      if (dropIdx === previewDropIdx) return;
      previewDropIdx = dropIdx;

      var refNodes = Array.from(buttons);
      if (dropIdx < refNodes.length) {
        container.insertBefore(previewPlaceholder, refNodes[dropIdx]);
      } else {
        var addBtn = container.querySelector(".sp-add-btn");
        container.insertBefore(previewPlaceholder, addBtn);
      }
    });

    container.addEventListener("dragleave", function (e) {
      if (!container.contains(e.relatedTarget) && previewPlaceholder && dragSrcPos >= 0) {
        var buttons = container.querySelectorAll(
          ".sp-btn:not(.sp-drop-placeholder):not(.sp-add-btn)"
        );
        var refNodes = Array.from(buttons);
        if (dragSrcPos < refNodes.length) {
          container.insertBefore(previewPlaceholder, refNodes[dragSrcPos]);
        } else {
          var addBtn = container.querySelector(".sp-add-btn");
          container.insertBefore(previewPlaceholder, addBtn);
        }
        previewDropIdx = dragSrcPos;
      }
    });

    container.addEventListener("drop", function (e) {
      e.preventDefault();
      var toIdx = previewDropIdx;
      removePreviewPlaceholder();
      if (dragSrcPos < 0 || toIdx < 0) return;
      if (dragSrcPos === toIdx) {
        renderPreview();
        dragSrcPos = -1;
        return;
      }
      var newOrder = state.order.slice();
      var moved = newOrder.splice(dragSrcPos, 1)[0];
      newOrder.splice(toIdx, 0, moved);
      state.order = newOrder;
      renderPreview();
      renderButtonSettings();
      postText("Button Order", newOrder.join(","));
      dragSrcPos = -1;
    });
  }

  function setupPreviewDrag(btn, idx) {
    btn.addEventListener("dragstart", function (e) {
      dragSrcPos = idx;
      didDrag = true;
      e.dataTransfer.effectAllowed = "move";
      e.dataTransfer.setData("text/plain", String(idx));
      setTimeout(function () {
        previewPlaceholder = document.createElement("div");
        previewPlaceholder.className = "sp-btn sp-drop-placeholder";
        previewDropIdx = idx;
        btn.parentNode.replaceChild(previewPlaceholder, btn);
      }, 0);
    });
    btn.addEventListener("dragend", function () {
      removePreviewPlaceholder();
      if (dragSrcPos >= 0) {
        renderPreview();
        dragSrcPos = -1;
      }
    });
  }

  // ── Button settings panel ───────────────────────────────────────────

  function selectButton(slot) {
    state.selectedSlot = slot;
    renderPreview();
    renderButtonSettings();
  }

  // ── Context menu ────────────────────────────────────────────────────

  var ctxMenu = null;

  function showContextMenu(e, slot) {
    e.preventDefault();
    hideContextMenu();

    ctxMenu = document.createElement("div");
    ctxMenu.className = "sp-ctx-menu";

    var dupItem = document.createElement("div");
    dupItem.className = "sp-ctx-item";
    dupItem.innerHTML = '<span class="mdi mdi-content-copy"></span>Duplicate';
    dupItem.addEventListener("mousedown", function (ev) {
      ev.preventDefault();
      hideContextMenu();
      duplicateButton(slot);
    });
    ctxMenu.appendChild(dupItem);

    var divider = document.createElement("div");
    divider.className = "sp-ctx-divider";
    ctxMenu.appendChild(divider);

    var delItem = document.createElement("div");
    delItem.className = "sp-ctx-item sp-ctx-danger";
    delItem.innerHTML = '<span class="mdi mdi-delete"></span>Delete';
    delItem.addEventListener("mousedown", function (ev) {
      ev.preventDefault();
      hideContextMenu();
      deleteButton(slot);
    });
    ctxMenu.appendChild(delItem);

    document.body.appendChild(ctxMenu);

    var menuW = ctxMenu.offsetWidth;
    var menuH = ctxMenu.offsetHeight;
    var x = e.clientX;
    var y = e.clientY;
    if (x + menuW > window.innerWidth) x = window.innerWidth - menuW - 4;
    if (y + menuH > window.innerHeight) y = window.innerHeight - menuH - 4;
    if (x < 0) x = 4;
    if (y < 0) y = 4;
    ctxMenu.style.left = x + "px";
    ctxMenu.style.top = y + "px";
  }

  function hideContextMenu() {
    if (ctxMenu && ctxMenu.parentNode) {
      ctxMenu.parentNode.removeChild(ctxMenu);
    }
    ctxMenu = null;
  }

  // ── Add / Duplicate / Delete buttons ───────────────────────────────

  function firstFreeSlot() {
    var used = {};
    state.order.forEach(function (s) { used[s] = true; });
    for (var i = 1; i <= NUM_SLOTS; i++) {
      if (!used[i]) return i;
    }
    return -1;
  }

  function addButton() {
    var slot = firstFreeSlot();
    if (slot < 0) return;
    state.order = state.order.concat(slot);
    postText("Button Order", state.order.join(","));
    selectButton(slot);
  }

  function duplicateButton(srcSlot) {
    var newSlot = firstFreeSlot();
    if (newSlot < 0) return;

    var src = state.buttons[srcSlot - 1];
    state.buttons[newSlot - 1] = {
      entity: src.entity,
      label: src.label,
      icon: src.icon,
      sensor: src.sensor,
      unit: src.unit,
    };

    var srcIdx = state.order.indexOf(srcSlot);
    var newOrder = state.order.slice();
    newOrder.splice(srcIdx + 1, 0, newSlot);
    state.order = newOrder;

    postText("Button Order", state.order.join(","));
    postText("Button " + newSlot + " Entity", src.entity);
    postText("Button " + newSlot + " Label", src.label);
    postText("Button " + newSlot + " Sensor", src.sensor);
    postText("Button " + newSlot + " Sensor Unit", src.unit);
    postSelect("Button " + newSlot + " Icon", src.icon || "Auto");
    selectButton(newSlot);
  }

  function deleteButton(slot) {
    state.order = state.order.filter(function (s) { return s !== slot; });
    if (state.selectedSlot === slot) state.selectedSlot = -1;
    renderPreview();
    renderButtonSettings();
    postText("Button Order", state.order.join(","));
    postText("Button " + slot + " Entity", "");
    postText("Button " + slot + " Label", "");
    postText("Button " + slot + " Sensor", "");
    postText("Button " + slot + " Sensor Unit", "");
    postSelect("Button " + slot + " Icon", "Auto");
  }

  // ── Export / Import ─────────────────────────────────────────────────

  function exportConfig() {
    var data = {
      version: 1,
      exported_at: new Date().toISOString(),
      button_order: state.order.join(","),
      button_on_color: state.onColor,
      button_off_color: state.offColor,
      buttons: state.buttons.map(function (b) {
        return {
          entity: b.entity,
          label: b.label,
          icon: b.icon,
          sensor: b.sensor,
          unit: b.unit,
        };
      }),
      settings: {
        indoor_temp_enable: state._indoorOn,
        outdoor_temp_enable: state._outdoorOn,
        indoor_temp_entity: state.indoorEntity,
        outdoor_temp_entity: state.outdoorEntity,
        presence_sensor_entity: state.presenceEntity,
        screensaver_timeout: state.screensaverTimeout,
      },
    };

    var json = JSON.stringify(data, null, 2);
    var blob = new Blob([json], { type: "application/json" });
    var url = URL.createObjectURL(blob);
    var now = new Date();
    var name = "espcontrol-config-" +
      now.getFullYear() + "-" +
      String(now.getMonth() + 1).padStart(2, "0") + "-" +
      String(now.getDate()).padStart(2, "0") + ".json";
    var a = document.createElement("a");
    a.href = url;
    a.download = name;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
  }

  function importConfig() {
    var input = document.createElement("input");
    input.type = "file";
    input.accept = ".json";
    input.style.display = "none";

    input.addEventListener("change", function () {
      if (!input.files || !input.files[0]) return;
      var reader = new FileReader();
      reader.onload = function () {
        var data;
        try { data = JSON.parse(reader.result); } catch (_) {
          showBanner("Invalid file \u2014 could not parse JSON", "error");
          return;
        }

        if (!data.version || !Array.isArray(data.buttons)) {
          showBanner("Invalid config file \u2014 missing required fields", "error");
          return;
        }
        if (data.buttons.length !== NUM_SLOTS) {
          showBanner("Invalid config file \u2014 expected " + NUM_SLOTS + " button slots", "error");
          return;
        }

        postText("Button On Color", data.button_on_color || "FF8C00");
        postText("Button Off Color", data.button_off_color || "313131");

        for (var i = 0; i < NUM_SLOTS; i++) {
          var b = data.buttons[i];
          var n = i + 1;
          postText("Button " + n + " Entity", b.entity || "");
          postText("Button " + n + " Label", b.label || "");
          postText("Button " + n + " Sensor", b.sensor || "");
          postText("Button " + n + " Sensor Unit", b.unit || "");
          postSelect("Button " + n + " Icon", b.icon || "Auto");

          state.buttons[i] = {
            entity: b.entity || "",
            label: b.label || "",
            icon: b.icon || "Auto",
            sensor: b.sensor || "",
            unit: b.unit || "",
          };
        }

        var orderStr = data.button_order || "";
        postText("Button Order", orderStr);
        state.order = parseOrder(orderStr);
        state.onColor = data.button_on_color || "FF8C00";
        state.offColor = data.button_off_color || "313131";

        if (els.setOnColor && els.setOnColor._syncColor) els.setOnColor._syncColor(state.onColor);
        if (els.setOffColor && els.setOffColor._syncColor) els.setOffColor._syncColor(state.offColor);

        if (data.settings) {
          var s = data.settings;

          postSwitch("Indoor Temp Enable", !!s.indoor_temp_enable);
          postSwitch("Outdoor Temp Enable", !!s.outdoor_temp_enable);
          postText("Indoor Temp Entity", s.indoor_temp_entity || "");
          postText("Outdoor Temp Entity", s.outdoor_temp_entity || "");
          postText("Presence Sensor Entity", s.presence_sensor_entity || "");
          postNumber("Screensaver Timeout", s.screensaver_timeout || 300);

          state._indoorOn = !!s.indoor_temp_enable;
          state._outdoorOn = !!s.outdoor_temp_enable;
          state.indoorEntity = s.indoor_temp_entity || "";
          state.outdoorEntity = s.outdoor_temp_entity || "";
          state.presenceEntity = s.presence_sensor_entity || "";
          state.screensaverTimeout = s.screensaver_timeout || 300;

          els.setIndoorToggle.checked = state._indoorOn;
          els.setIndoorField.className = "sp-cond-field" + (state._indoorOn ? " sp-visible" : "");
          syncInput(els.setIndoorEntity, state.indoorEntity);
          els.setOutdoorToggle.checked = state._outdoorOn;
          els.setOutdoorField.className = "sp-cond-field" + (state._outdoorOn ? " sp-visible" : "");
          syncInput(els.setOutdoorEntity, state.outdoorEntity);
          syncInput(els.setPresence, state.presenceEntity);
          syncInput(els.setSSTimeout, String(state.screensaverTimeout));
          updateTempPreview();
        }

        state.selectedSlot = -1;
        renderPreview();
        renderButtonSettings();
        showBanner("Configuration imported successfully", "success");
      };
      reader.readAsText(input.files[0]);
    });

    document.body.appendChild(input);
    input.click();
    document.body.removeChild(input);
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
    if (_eventSource) { _eventSource.close(); _eventSource = null; }
    var source = new EventSource("/events");
    _eventSource = source;

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
      if (source.readyState === 2) {
        source.close();
        _eventSource = null;
        setTimeout(connectEvents, 5000);
      }
    });

    var sseHandlers = {
      "text-button_order": function (val) {
        orderReceived = true;
        state.order = parseOrder(val);
        renderPreview();
        renderButtonSettings();
        if (state.selectedSlot > 0 && state.order.indexOf(state.selectedSlot) === -1)
          state.selectedSlot = -1;
      },
      "text-button_on_color": function (val) {
        state.onColor = val;
        if (els.setOnColor && els.setOnColor._syncColor) els.setOnColor._syncColor(val);
        renderPreview();
      },
      "text-button_off_color": function (val) {
        state.offColor = val;
        if (els.setOffColor && els.setOffColor._syncColor) els.setOffColor._syncColor(val);
        renderPreview();
      },
      "switch-indoor_temp_enable": function (val, d) {
        state._indoorOn = d.value === true || val === "ON";
        els.setIndoorToggle.checked = state._indoorOn;
        els.setIndoorField.className = "sp-cond-field" + (state._indoorOn ? " sp-visible" : "");
        updateTempPreview();
      },
      "switch-outdoor_temp_enable": function (val, d) {
        state._outdoorOn = d.value === true || val === "ON";
        els.setOutdoorToggle.checked = state._outdoorOn;
        els.setOutdoorField.className = "sp-cond-field" + (state._outdoorOn ? " sp-visible" : "");
        updateTempPreview();
      },
      "text-indoor_temp_entity": function (val) {
        state.indoorEntity = val;
        syncInput(els.setIndoorEntity, val);
      },
      "text-outdoor_temp_entity": function (val) {
        state.outdoorEntity = val;
        syncInput(els.setOutdoorEntity, val);
      },
      "number-screensaver_timeout": function (val) {
        state.screensaverTimeout = parseFloat(val) || 300;
        syncInput(els.setSSTimeout, String(state.screensaverTimeout));
      },
      "text-presence_sensor_entity": function (val) {
        state.presenceEntity = val;
        syncInput(els.setPresence, val);
      },
      "number-screen__daytime_brightness": function (val) {
        state.brightnessDayVal = parseFloat(val) || 100;
        if (els.setDayBrightness) {
          els.setDayBrightness.value = state.brightnessDayVal;
          els.setDayBrightnessVal.textContent = Math.round(state.brightnessDayVal) + "%";
        }
      },
      "number-screen__nighttime_brightness": function (val) {
        state.brightnessNightVal = parseFloat(val) || 75;
        if (els.setNightBrightness) {
          els.setNightBrightness.value = state.brightnessNightVal;
          els.setNightBrightnessVal.textContent = Math.round(state.brightnessNightVal) + "%";
        }
      },
      "text_sensor-screen__sunrise": function (val) {
        state.sunrise = val;
        updateSunInfo();
      },
      "text_sensor-screen__sunset": function (val) {
        state.sunset = val;
        updateSunInfo();
      },
      "text_sensor-firmware__version": function (val) {
        state.firmwareVersion = val;
        if (els.fwVersionLabel) {
          els.fwVersionLabel.innerHTML = '<span class="sp-fw-label">Installed </span>' + escHtml(val || "dev");
        }
      },
      "switch-firmware__auto_update": function (val, d) {
        state.autoUpdate = d.value === true || val === "ON";
        if (els.setAutoUpdate) els.setAutoUpdate.checked = state.autoUpdate;
      },
      "select-firmware__update_frequency": function (val, d) {
        state.updateFrequency = val;
        if (d.option) state.updateFrequency = d.option;
        if (els.setUpdateFreq) els.setUpdateFreq.value = state.updateFrequency;
        if (d.options && Array.isArray(d.options)) {
          state.updateFreqOptions = d.options;
        }
      },
    };

    var ssePatterns = [
      {
        re: /^text-button_(\d+)_(entity|label|sensor|sensor_unit)$/,
        fn: function (m, val) {
          var slot = parseInt(m[1], 10);
          var field = m[2] === "sensor_unit" ? "unit" : m[2];
          if (slot >= 1 && slot <= NUM_SLOTS) {
            state.buttons[slot - 1][field] = val;
            renderPreview();
            if (state.selectedSlot === slot && isSettingsFocused()) {
              var idMap = { entity: "sp-inp-entity", label: "sp-inp-label", sensor: "sp-inp-sensor", unit: "sp-inp-unit" };
              syncInput(document.getElementById(idMap[field]), val);
            } else {
              renderButtonSettings();
            }
            scheduleMigration();
          }
        },
      },
      {
        re: /^select-button_(\d+)_icon$/,
        fn: function (m, val) {
          var slot = parseInt(m[1], 10);
          if (slot >= 1 && slot <= NUM_SLOTS) {
            state.buttons[slot - 1].icon = val;
            renderPreview();
            if (state.selectedSlot === slot && isSettingsFocused()) {
              syncInput(document.getElementById("sp-inp-icon"), val);
              var prev = document.querySelector(".sp-icon-picker-preview");
              if (prev) prev.className = "sp-icon-picker-preview mdi mdi-" + (ICON_MAP[val] || "cog");
            } else {
              renderButtonSettings();
            }
          }
        },
      },
    ];

    source.addEventListener("state", function (e) {
      var d;
      try { d = JSON.parse(e.data); } catch (_) { return; }
      var id = d.id;
      var val = d.state != null ? String(d.state) : "";

      if (sseHandlers[id]) { sseHandlers[id](val, d); return; }

      for (var i = 0; i < ssePatterns.length; i++) {
        var m = id.match(ssePatterns[i].re);
        if (m) { ssePatterns[i].fn(m, val, d); return; }
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
        renderButtonSettings();
        postText("Button Order", autoOrder.join(","));
      }
    }, 2000);
  }

  function updateSunInfo() {
    var el = els.sunInfo;
    if (!el) return;
    if (!state.sunrise && !state.sunset) {
      el.classList.remove("sp-visible");
      return;
    }
    el.classList.add("sp-visible");
    var t = "";
    if (state.sunrise) t += "Sunrise: " + escHtml(state.sunrise);
    if (state.sunrise && state.sunset) t += " \u00a0/\u00a0 ";
    if (state.sunset) t += "Sunset: " + escHtml(state.sunset);
    el.innerHTML = t;
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
