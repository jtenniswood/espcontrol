// =============================================================================
// ESPCONTROL WEB UI - Custom device configuration interface
// =============================================================================
// Replaces the default ESPHome webserver UI with a two-tab layout:
//   Screen  - Live grid preview with drag-and-drop button arrangement
//   Settings - Display, brightness, firmware, and entity configuration
//
// Per-device config (grid size, styling) is injected between __DEVICE_CONFIG__
// markers by scripts/build.py. Button type plugins (switch, sensor, weather,
// action, calendar, timezone, slider, cover, garage, lock, media, push, subpage) are injected between __BUTTON_TYPES__ markers.
// Icon data is generated between GENERATED:ICONS / GENERATED:DOMAIN_ICONS.
// =============================================================================

// Custom UI: two-page layout (Screen / Settings)
(function () {
  // __DEVICE_CONFIG_START__
  var DEVICE_ID = "guition-esp32-p4-jc1060p470";
  var CFG = {"slots":20,"cols":5,"rows":4,"dragMode":"swap","dragAnimation":true,"screen":{"width":"67%","aspect":"1024/600"},"topbar":{"height":3.2,"padding":"0.39cqw","fontSize":1.95},"grid":{"top":4.4,"left":0.49,"right":0.49,"bottom":0.49,"gap":0.98,"fr":"1fr"},"btn":{"radius":0.78,"padding":1.37,"iconSize":4.69,"labelSize":1.8},"emptyCell":{"radius":0.78},"sensorBadge":{"top":1,"right":1,"fontSize":1.6},"subpageBadge":{"bottom":1,"right":1,"fontSize":2}};
  // __DEVICE_CONFIG_END__
  var NUM_SLOTS = CFG.slots;
  var GRID_COLS = CFG.cols;
  var GRID_ROWS = CFG.rows;

  // --- GENERATED:ICONS START ---
  var ICON_EXCEPTIONS = {
    Auto: "cog",
    Alarm: "bell-ring",
    Application: "application-outline",
    "Ceiling Lights Multiple": "ceiling-light-multiple",
    Clock: "clock-outline",
    Downlight: "light-recessed",
    Doorbell: "doorbell-video",
    Dryer: "tumble-dryer",
    "EV Charger": "ev-station",
    Gamepad: "gamepad-variant",
    "Grid Export": "transmission-tower-export",
    "Grid Import": "transmission-tower-import",
    "Grid Off": "transmission-tower-off",
    Humidifier: "air-humidifier",
    "Humidity Alert": "water-percent-alert",
    Key: "key-variant",
    Lawnmower: "robot-mower",
    Oven: "stove",
    Package: "package-variant",
    "Package Closed": "package-variant-closed",
    Router: "router-wireless",
    Security: "shield-home",
    Shower: "shower-head",
    Spotlight: "spotlight-beam",
    Sun: "white-balance-sunny",
    Table: "table-furniture",
    "Home-Thermostat": "home-thermometer",
    Timer: "timer-outline",
    "Wall Outlet": "power-socket",
    Washer: "washing-machine",
    "Water Heater": "water-boiler",
    "Weather Night Cloudy": "weather-night-partly-cloudy",
  };
  var ICON_NAMES = [
    "Account", "Air Conditioner", "Air Filter", "Air Purifier", "Alarm", "Alarm Light",
    "Application", "Battery", "Battery 10%", "Battery 20%", "Battery 30%", "Battery 40%",
    "Battery 50%", "Battery 60%", "Battery 70%", "Battery 80%", "Battery 90%", "Battery Alert",
    "Battery Charging", "Battery Charging 100", "Battery Charging 70", "Battery High", "Battery Low", "Battery Medium",
    "Battery Off", "Battery Outline", "Battery Unknown", "Bed", "Bell", "Blinds",
    "Blinds Horizontal", "Blinds Horizontal Closed", "Blinds Open", "Bluetooth", "Broom", "Camera",
    "Car Electric", "Cast", "CCTV", "Ceiling Fan", "Ceiling Light", "Ceiling Lights Multiple",
    "Chandelier", "Clock", "Coffee Maker", "Current AC", "Current DC", "Curtains",
    "Curtains Closed", "Desk Lamp", "Dishwasher", "Dog", "Downlight", "Door",
    "Door Open", "Doorbell", "Dots Horizontal", "Dryer", "EV Charger", "Fan",
    "Fire", "Fireplace", "Flash", "Floor Lamp", "Fountain", "Fridge",
    "Gamepad", "Garage", "Garage Open", "Garage Open Variant", "Garage Variant", "Gate",
    "Gate Open", "Gesture Tap", "Gauge", "Gauge Empty", "Gauge Full", "Gauge Low",
    "Grid Export", "Grid Import", "Grid Off", "Headphones", "Radiator", "Radiator Off",
    "Home", "Heat Pump", "HVAC", "HVAC Off", "Hot Tub", "Humidifier",
    "Humidity Alert", "Iron", "Kettle", "Key", "Lamp", "Lamp Outline",
    "LAN", "Lawnmower", "Leaf", "LED Strip", "LED Strip Variant", "LED Strip Variant Off",
    "Light Switch", "Lightbulb", "Lightbulb Group", "Lightbulb Group Outline", "Lightbulb Night", "Lightbulb Night Outline",
    "Lightbulb Off", "Lightbulb On Outline", "Lightbulb Spot", "Lightbulb Spot Off", "Lightbulb Variant", "Lightbulb Variant Outline",
    "Lightbulb Outline", "Lightning Bolt", "Lock", "Lock Open", "Lock Open Outline", "Lock Outline",
    "Mailbox", "Message Video", "Medication", "Medication Outline", "Meter Electric", "Meter Gas",
    "Microsoft Xbox", "Microwave", "Minus", "Monitor", "Motion Sensor", "Movie Roll",
    "Music", "Outdoor Lamp", "Oven", "Package", "Package Closed", "Pill",
    "Pill Multiple", "Plus", "Play Pause", "Pool", "Power", "Power Plug",
    "Progress Clock", "Printer", "Printer 3D", "Projector", "Projector Off", "Robot Vacuum",
    "Roller Shade", "Roller Shade Closed", "Router", "Router Network", "Security", "Shower",
    "Skip Next", "Skip Previous", "Smoke Detector", "Snowflake", "Snowflake Alert", "Snowflake Thermometer",
    "Sofa", "Solar Panel", "Solar Panel Large", "Solar Power", "Solar Power Variant", "Speaker",
    "Spotlight", "Sprinkler", "Stop", "String Lights", "String Lights Off", "Power Socket UK",
    "Power Socket EU", "Power Socket US", "Sun", "Table", "Television", "Television Off",
    "Thermometer", "Thermometer Alert", "Thermometer High", "Thermometer Low", "Thermostat", "Home-Thermostat",
    "Thermostat Auto", "Thermometer Water", "Timer", "Toilet", "Transmission Tower", "Trash Can",
    "Volume High", "Wall Outlet", "Wall Sconce", "Washer", "Water", "Water Heater",
    "Water Percent", "Water Alert", "Weather Cloudy", "Weather Cloudy Alert", "Weather Dust", "Weather Fog",
    "Weather Hail", "Weather Hazy", "Weather Hurricane", "Weather Lightning", "Weather Lightning Rainy", "Weather Night",
    "Weather Night Cloudy", "Weather Partly Cloudy", "Weather Partly Lightning", "Weather Partly Rainy", "Weather Partly Snowy", "Weather Partly Snowy Rainy",
    "Weather Pouring", "Weather Rainy", "Weather Snowy", "Weather Snowy Heavy", "Weather Snowy Rainy", "Weather Sunny",
    "Weather Sunny Alert", "Weather Sunny Off", "Weather Sunset", "Weather Sunset Down", "Weather Sunset Up", "Weather Tornado",
    "Weather Windy", "Weather Windy Variant", "Wind Power", "Wind Turbine", "Wind Turbine Alert", "Wind Turbine Check",
    "Window Closed", "Window Open", "Window Shutter", "Window Shutter Open",
  ];
  // --- GENERATED:ICONS END ---

  // Convert an icon display name to its MDI CSS class slug (e.g. "Lightbulb" → "lightbulb")
  function iconSlug(name) {
    return ICON_EXCEPTIONS[name] || name.toLowerCase().replace(/[^a-z0-9]/g, function (ch) {
      return ch === " " ? "-" : "";
    }) || "cog";
  }

  var ICON_OPTIONS = ["Auto"].concat(ICON_NAMES).sort();

  var DOMAIN_ICONS = {
    // --- GENERATED:DOMAIN_ICONS START ---
    light: "lightbulb",
    switch: "power-plug",
    fan: "fan",
    lock: "lock",
    cover: "blinds-horizontal",
    climate: "air-conditioner",
    media_player: "speaker",
    camera: "camera",
    button: "gesture-tap",
    binary_sensor: "motion-sensor",
    // --- GENERATED:DOMAIN_ICONS END ---
  };

  // ── Button type plugin registry ──────────────────────────────────────
  var BUTTON_TYPES = {};
  function registerButtonType(key, def) {
    BUTTON_TYPES[key] = Object.assign({
      key: key,
      label: key || "Toggle",
      allowInSubpage: false,
      hideLabel: false,
      labelPlaceholder: null,
      isAvailable: null,
      onSelect: null,
      renderSettingsBeforeLabel: null,
      renderSettings: null,
      renderPreview: null,
      contextMenuItems: null,
      experimental: null,
    }, def);
  }
  function developerExperimentalUrlFlag() {
    var value = "";
    try {
      value = new URLSearchParams(window.location.search).get("developer") || "";
    } catch (e) {}
    return value.trim().toLowerCase() === "experimental";
  }

  function isExperimentalEnabled(key) {
    return !!state.developerExperimentalFeatures;
  }

  function subpageStateDisplayMode(b) {
    if (!b || !b.sensor) return "off";
    if (b.sensor === "indicator") return "icon";
    return b.precision === "text" ? "text" : "numeric";
  }
  // __BUTTON_TYPES_START__
  // __BUTTON_TYPES_END__

  var CSS =
    ":root{" +
    "--bg:#1b1b1f;--surface:#202127;--surface2:#2e2e32;--border:#3c3f44;" +
    "--text:#dfdfd6;--text2:#98989f;--text3:#6a6a71;--accent:#5c73e7;--accent-hover:#a8b1ff;" +
    "--accent-soft:rgba(100,108,255,.16);--success:#30a46c;--danger:#f14158;" +
    "--radius:12px;--action-r:9999px;--gap:16px;" +
    "--shadow-1:0 1px 2px rgba(0,0,0,.2),0 1px 2px rgba(0,0,0,.24);" +
    "--shadow-2:0 3px 12px rgba(0,0,0,.28),0 1px 4px rgba(0,0,0,.2);" +
    "--shadow-3:0 12px 32px rgba(0,0,0,.35),0 2px 6px rgba(0,0,0,.24)}" +

    "#sp-app{font-family:'Inter',-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;" +
    "color:var(--text);max-width:960px;margin:0 auto;-webkit-font-smoothing:antialiased;" +
    "font-optical-sizing:auto}" +
    "body{background:var(--bg);margin:0}" +
    "esp-app{display:none !important}" +

    ".sp-header{display:flex;align-items:center;background:var(--bg);" +
    "border-bottom:1px solid var(--border);position:sticky;top:0;z-index:100;height:56px;padding:0 20px}" +
    ".sp-brand{font-size:1rem;font-weight:600;color:var(--text);margin-right:auto;" +
    "white-space:nowrap;letter-spacing:-.01em}" +
    ".sp-nav{display:flex;align-items:center;height:100%}" +
    ".sp-tab{padding:0 16px;height:100%;display:flex;align-items:center;color:var(--text2);cursor:pointer;" +
    "font-size:.875rem;font-weight:500;border-bottom:2px solid transparent;text-decoration:none;transition:color .2s}" +
    ".sp-tab:hover{color:var(--text)}" +
    ".sp-tab.active{color:var(--accent);border-bottom-color:var(--accent)}" +
    ".sp-tab-docs{position:relative;gap:6px;margin-left:8px;padding-left:24px}" +
    ".sp-tab-docs::before{content:'';position:absolute;left:0;top:12px;bottom:12px;width:1px;background:var(--border)}" +
    ".sp-tab-docs .mdi{font-size:16px;line-height:1;opacity:.7}" +

    ".sp-page{display:none}.sp-page.active{display:block}" +

    ".sp-support-btn{position:fixed;right:28px;bottom:28px;z-index:150;display:inline-block;line-height:0}" +
    ".sp-support-btn img{height:60px;display:block;border-radius:999px}" +

    ".fade-in{animation:fadeIn .3s ease}" +
    "@keyframes fadeIn{from{opacity:0;transform:translateY(8px)}to{opacity:1;transform:translateY(0)}}" +

    ".sp-wrap{display:flex;justify-content:center;padding:16px var(--gap) 4px;user-select:none}" +
    ".sp-screen{width:var(--screen-w);aspect-ratio:var(--screen-aspect);background:#000;" +
    "border-radius:var(--radius);position:relative;overflow:hidden;" +
    "box-shadow:0 2px 20px rgba(0,0,0,.35);border:2px solid var(--surface);" +
    "container-type:inline-size;font-family:Roboto,sans-serif;user-select:none}" +
    ".sp-topbar{position:absolute;top:0;left:0;right:0;height:var(--topbar-h);" +
    "display:flex;align-items:center;padding:var(--topbar-pad);z-index:1}" +
    ".sp-topbar.sp-hidden{display:none}" +
    ".sp-temp{color:#fff;font-size:var(--topbar-fs);white-space:nowrap;opacity:0;transition:opacity .3s;margin-left:1cqw}" +
    ".sp-temp.sp-visible{opacity:1}" +
    ".sp-clock{position:absolute;left:50%;transform:translateX(-50%);" +
    "color:#fff;font-size:var(--clock-fs,var(--topbar-fs));white-space:nowrap}" +
    ".sp-main{position:absolute;top:var(--grid-top);left:var(--grid-left);right:var(--grid-right);bottom:var(--grid-bottom);" +
    "display:grid;grid-template-columns:var(--grid-cols);grid-template-rows:var(--grid-rows);gap:var(--grid-gap);overflow:hidden}" +

    ".sp-btn{border-radius:var(--btn-r);padding:var(--btn-pad);" +
    "display:flex;flex-direction:column;justify-content:space-between;" +
    "cursor:pointer;transition:all .2s;box-sizing:border-box;border:2px solid transparent;" +
    "position:relative;overflow:hidden;min-width:0}" +
    ".sp-btn:hover{filter:brightness(1.15)}" +
    ".sp-drag-active .sp-btn:hover{filter:none}" +
    ".sp-btn.sp-selected{border-color:var(--accent)}" +
    ".sp-btn-icon{font-size:var(--btn-icon);line-height:1;color:#fff}" +
    ".sp-btn-label{font-size:var(--btn-label);line-height:1.2;color:#fff;" +
    "display:-webkit-box;-webkit-box-orient:vertical;-webkit-line-clamp:var(--btn-lines);" +
    "overflow:hidden;word-break:break-word;min-height:0}" +
    ".sp-sensor-badge{position:absolute;top:var(--sensor-top);right:var(--sensor-right);font-size:var(--sensor-fs);opacity:.5}" +
    ".sp-sensor-preview{display:flex;align-items:baseline;gap:0;color:#fff}" +
    ".sp-forecast-preview{white-space:nowrap;gap:0}" +
    ".sp-sensor-value{font-size:var(--btn-icon);line-height:1;font-weight:300}" +
    ".sp-sensor-unit{font-size:var(--btn-label);line-height:1;color:#fff}" +
    ".sp-slider-preview{position:absolute;inset:0;border-radius:var(--r);overflow:hidden;pointer-events:none}" +
    ".sp-slider-track{width:100%;height:100%;position:relative}" +
    ".sp-slider-fill{position:absolute;left:0;bottom:0;width:100%;height:80%;background:#fff;" +
    "border-radius:var(--r)}" +
    ".sp-media-h-slider{position:absolute;left:8%;right:8%;bottom:10%;height:7.5%;border-radius:999px;" +
    "background:#212121;overflow:hidden;pointer-events:none}" +
    ".sp-media-h-slider span{display:block;width:62%;height:100%;background:#fff;border-radius:999px}" +
    ".sp-media-position-time{z-index:1}" +
    ".sp-media-now-title{font-size:var(--media-title);line-height:1;color:#fff;font-weight:300;z-index:1;" +
    "display:-webkit-box;-webkit-box-orient:vertical;-webkit-line-clamp:2;overflow:hidden;word-break:break-word}" +
    ".sp-media-now-artist{font-size:var(--btn-label);line-height:1.2;color:#fff}" +
    ".sp-btn-double{grid-row:span 2}" +
    ".sp-btn-double .sp-btn-label{-webkit-line-clamp:var(--btn-lines-dbl)}" +
    ".sp-btn-double .sp-btn-label-row .sp-btn-label{-webkit-line-clamp:var(--btn-lines-dbl)}" +
    ".sp-btn-double .sp-media-now-title{-webkit-line-clamp:2}" +
    ".sp-btn-wide{grid-column:span 2}" +
    ".sp-btn-wide .sp-media-now-title{-webkit-line-clamp:var(--btn-lines-dbl)}" +
    ".sp-btn-big{grid-row:span 2;grid-column:span 2}" +
    ".sp-btn-big .sp-btn-label{-webkit-line-clamp:var(--btn-lines-dbl)}" +
    ".sp-btn-big .sp-btn-label-row .sp-btn-label{-webkit-line-clamp:var(--btn-lines-dbl)}" +
    ".sp-btn-big .sp-media-now-title{-webkit-line-clamp:var(--btn-lines-dbl)}" +
    ".sp-empty-cell{border:2px dashed rgba(255,255,255,.15);background:transparent;" +
    "border-radius:var(--empty-r);display:flex;align-items:center;justify-content:center;" +
    "cursor:pointer;transition:border-color .2s,background-color .2s}" +
    ".sp-empty-cell:hover{border-color:var(--accent)}" +
    ".sp-drag-active .sp-empty-cell:hover{border-color:rgba(255,255,255,.15)}" +
    ".sp-add-pill{display:inline-flex;align-items:center;justify-content:center;min-width:8cqw;" +
    "height:4.6cqw;padding:0 1.8cqw;border-radius:9999px;border:1px solid rgba(255,255,255,.18);" +
    "background:rgba(255,255,255,.06);box-shadow:var(--shadow-1)}" +
    ".sp-empty-cell:hover .sp-add-pill{border-color:rgba(168,177,255,.45);background:rgba(92,115,231,.18)}" +
    ".sp-empty-cell .sp-add-icon{font-size:2.8cqw;color:rgba(255,255,255,.82)}" +
    ".sp-empty-cell.sp-drop-placeholder{border:2px dashed rgba(92,156,245,.5) !important;" +
    "background:rgba(92,156,245,.08) !important;cursor:default;pointer-events:none}" +
    ".sp-btn.sp-drop-placeholder{box-shadow:0 0 0 2px rgba(92,156,245,.6),0 0 12px rgba(92,156,245,.25) !important;" +
    "background:rgba(92,156,245,.08) !important}" +
    (CFG.dragAnimation ? ".sp-btn.sp-dragging{opacity:.4;transform:scale(.95)}" +
    ".sp-empty-cell.sp-drop-placeholder{border-color:rgba(92,156,245,.5)}" : "") +

    ".sp-hint{text-align:center;font-size:.7rem;color:var(--text3);padding:8px 0 12px;user-select:none}" +
    ".sp-selection-bar{display:none;align-items:center;justify-content:space-between;gap:12px;" +
    "margin-left:calc(50% - 50vw);margin-right:calc(50% - 50vw);box-sizing:border-box;" +
    "padding:14px max(var(--gap),calc((100vw - 960px)/2 + var(--gap)));background:var(--surface);" +
    "color:var(--text);font-family:'Inter',-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;user-select:none}" +
    ".sp-selection-bar.sp-visible{display:flex}" +
    ".sp-selection-label{font-size:.85rem;color:var(--text2);margin-right:auto}" +
    ".sp-selection-actions{display:flex;align-items:center;gap:8px;flex-shrink:0}" +
    ".sp-selection-btn{display:inline-flex;align-items:center;justify-content:center;gap:8px;" +
    "border:1px solid var(--border);border-radius:var(--action-r);background:var(--surface2);" +
    "color:var(--text);padding:8px 12px;font-size:.8rem;font-weight:500;cursor:pointer;" +
    "font-family:inherit;transition:all .2s;min-height:34px}" +
    ".sp-selection-btn:hover{background:var(--border);border-color:#4a4d54}" +
    ".sp-selection-btn-primary{background:var(--accent);border-color:var(--accent);color:#fff}" +
    ".sp-selection-btn-primary:hover{background:var(--accent-hover);border-color:var(--accent-hover)}" +
    ".sp-selection-btn .mdi{font-size:16px;line-height:1}" +

    ".sp-config{padding:var(--gap) var(--gap) var(--gap)}" +

    ".sp-settings-overlay{display:none;position:fixed;inset:0;background:rgba(0,0,0,.6);" +
    "z-index:100;align-items:center;justify-content:center;" +
    "backdrop-filter:blur(4px);-webkit-backdrop-filter:blur(4px)}" +
    ".sp-settings-overlay.sp-visible{display:flex}" +
    ".sp-settings-modal{position:relative;background:var(--bg);border:1px solid var(--border);" +
    "border-radius:var(--radius);width:90%;max-width:420px;max-height:80vh;" +
    "overflow-y:auto;box-shadow:var(--shadow-3);margin:40px}" +
    "@media(max-width:600px){.sp-settings-modal{width:100%;max-width:none;max-height:none;" +
    "height:100%;margin:0;border-radius:0;border:none}}" +
    ".sp-settings-close{position:absolute;top:8px;right:12px;background:none;border:none;" +
    "color:var(--text2);font-size:1.4rem;cursor:pointer;z-index:1;line-height:1;padding:4px}" +
    ".sp-settings-close:hover{color:var(--text)}" +

    ".sp-section-title{font-size:.8rem;font-weight:600;color:var(--text2);" +
    "margin:var(--gap) 0 8px;letter-spacing:-.01em}" +
    ".sp-settings-modal .sp-section-title{font-size:1.05rem;color:var(--text);" +
    "margin:0 0 20px;letter-spacing:-.01em}" +
    ".sp-settings-modal .sp-panel{background:none;border:none;padding:0;margin:0}" +

    ".card{background:var(--surface);border:1px solid var(--border);" +
    "border-radius:var(--radius);padding:24px;margin-bottom:var(--gap);transition:border-color .2s}" +
    ".card:hover{border-color:#4a4d54}" +
    ".card h3{font-size:.875rem;font-weight:600;margin-bottom:14px;color:var(--text);" +
    "letter-spacing:-.01em}" +
    ".card-header{display:flex;justify-content:space-between;align-items:center;" +
    "cursor:pointer;user-select:none;" +
    "margin:-24px -24px 0 -24px;padding:24px 24px 0 24px}" +
    ".card-header h3{margin:0}" +
    ".card-body{padding-top:20px}" +
    ".card-chevron{display:inline-flex;align-items:center;justify-content:center;" +
    "width:24px;height:24px;color:var(--text3);transition:transform .25s ease;flex-shrink:0}" +
    ".card-chevron svg{width:100%;height:100%}" +
    ".card.collapsed .card-chevron{transform:rotate(-90deg)}" +
    ".card.collapsed .card-body{display:none}" +
    ".card-header-right{display:flex;align-items:center;gap:8px}" +
    ".sp-card-badge{display:inline-flex;align-items:center;gap:7px;min-height:24px;" +
    "padding:0 12px 0 10px;border-radius:999px;background:rgba(48,164,108,.16);" +
    "color:#30a46c;font-size:.68rem;font-weight:400;text-transform:uppercase;letter-spacing:.04em;line-height:1}" +
    ".sp-card-badge-dot{width:7px;height:7px;border-radius:999px;background:#30a46c;flex-shrink:0}" +
    ".card:not(.collapsed) .sp-card-badge{display:none}" +
    ".sp-card-badge.sp-hidden{display:none}" +

    ".sp-panel{background:var(--surface);border-radius:var(--radius);padding:24px;" +
    "margin-bottom:var(--gap);border:1px solid var(--border)}" +

    ".sp-field{margin-bottom:28px}.sp-field:last-child{margin-bottom:0}" +
    ".sp-field-stack{display:grid;gap:10px}" +
    ".sp-field-stack.sp-hidden{display:none}" +
    ".sp-field-label{display:block;font-size:.8rem;font-weight:500;color:var(--text2);margin-bottom:8px}" +
    ".sp-input,.sp-select{width:100%;padding:10px 12px;background:var(--surface2);" +
    "border:1px solid var(--border);border-radius:8px;color:var(--text);font-size:.875rem;" +
    "font-family:inherit;box-sizing:border-box;outline:none;" +
    "transition:border-color .25s,box-shadow .25s}" +
    ".sp-input:focus,.sp-select:focus{border-color:var(--accent);" +
    "box-shadow:0 0 0 3px var(--accent-soft)}" +
    ".sp-input.sp-input-error,.sp-select.sp-input-error{border-color:var(--danger);" +
    "box-shadow:0 0 0 3px rgba(241,65,88,.16)}" +
    ".sp-input--narrow{width:80px}" +
    ".sp-select{appearance:none;-webkit-appearance:none;" +
    "background-image:url(\"data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='12' height='12' viewBox='0 0 12 12'%3E%3Cpath fill='%2398989f' d='M6 8L1 3h10z'/%3E%3C/svg%3E\");" +
    "background-repeat:no-repeat;background-position:right 12px center;padding-right:32px}" +
    "select option{background:var(--surface);color:var(--text)}" +
    ".sp-field-error{font-size:.75rem;color:#f66f81;margin-top:6px;line-height:1.35}" +

    ".sp-icon-picker{position:relative}" +
    ".sp-icon-picker-input{width:100%;padding:10px 12px;padding-left:36px;background:var(--surface2);" +
    "border:1px solid var(--border);border-radius:8px;color:var(--text);font-size:.875rem;" +
    "font-family:inherit;box-sizing:border-box;outline:none;" +
    "transition:border-color .25s,box-shadow .25s}" +
    ".sp-icon-picker-input:focus{border-color:var(--accent);" +
    "box-shadow:0 0 0 3px var(--accent-soft)}" +
    ".sp-icon-picker-input::placeholder{color:var(--text3)}" +
    ".sp-icon-picker-preview{position:absolute;left:10px;top:50%;transform:translateY(-50%);" +
    "font-size:18px;color:var(--text2);pointer-events:none}" +
    ".sp-icon-picker.sp-open .sp-icon-picker-preview{top:19px}" +
    ".sp-icon-dropdown{display:none;position:absolute;left:0;right:0;top:100%;margin-top:4px;" +
    "background:var(--surface2);border:1px solid var(--border);border-radius:8px;max-height:200px;" +
    "overflow-y:auto;z-index:50;box-shadow:var(--shadow-3)}" +
    ".sp-icon-picker.sp-open .sp-icon-dropdown{display:block}" +
    ".sp-icon-option{display:flex;align-items:center;gap:10px;padding:8px 12px;" +
    "cursor:pointer;font-size:.875rem;color:var(--text);transition:background .15s}" +
    ".sp-icon-option:hover,.sp-icon-option.sp-highlighted{background:var(--accent-soft)}" +
    ".sp-icon-option.sp-active{background:var(--accent-soft)}" +
    ".sp-icon-option-icon{font-size:20px;width:24px;text-align:center;color:var(--text2);flex-shrink:0}" +
    ".sp-icon-option-label{white-space:nowrap;overflow:hidden;text-overflow:ellipsis}" +
    ".sp-icon-option--empty{color:var(--text3)}" +
    ".sp-btn-row{display:flex;gap:8px;margin-top:16px}" +
    ".sp-action-btn{padding:10px 20px;border:none;border-radius:var(--action-r);font-size:.875rem;" +
    "font-weight:500;cursor:pointer;font-family:inherit;" +
    "transition:background .25s,opacity .25s,box-shadow .25s}" +
    ".sp-action-btn:active{opacity:.85}" +
    ".sp-delete-btn{background:var(--surface2);color:var(--text);display:inline-flex;align-items:center;gap:6px}" +
    ".sp-delete-btn:hover{background:var(--border);color:var(--text)}" +
    ".sp-save-btn{background:var(--accent);color:#fff}" +
    ".sp-save-btn:hover{background:var(--accent-hover);box-shadow:var(--shadow-1)}" +
    ".sp-edit-subpage-btn{background:var(--accent);color:#fff}" +
    ".sp-edit-subpage-btn:hover{background:var(--accent-hover);box-shadow:var(--shadow-1)}" +
    ".sp-btn-row--save{margin-top:24px;justify-content:flex-end}" +
    ".sp-btn-row--save.sp-has-delete{justify-content:space-between}" +
    ".sp-btn-group-right{display:flex;gap:8px}" +

    ".sp-toggle-row{display:flex;align-items:center;justify-content:space-between;" +
    "min-height:36px;margin-bottom:14px}" +
    ".sp-toggle-row:last-child{margin-bottom:0}" +
    ".sp-cond-field+.sp-toggle-row{margin-top:16px}" +
    ".sp-toggle-row span{font-size:.875rem}" +
    ".sp-toggle{position:relative;width:44px;height:24px;flex-shrink:0}" +
    ".sp-toggle input{opacity:0;width:0;height:0;position:absolute}" +
    ".sp-toggle-track{position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;" +
    "background:var(--surface2);border-radius:12px;transition:background .25s,border-color .25s,box-shadow .25s;" +
    "border:1px solid var(--border)}" +
    ".sp-toggle-track:before{content:'';position:absolute;height:18px;width:18px;" +
    "left:2px;top:2px;background:#fff;border-radius:50%;transition:transform .25s;" +
    "box-shadow:0 1px 3px rgba(0,0,0,.3)}" +
    ".sp-toggle input:checked+.sp-toggle-track{background:var(--accent);border-color:var(--accent)}" +
    ".sp-toggle input:checked+.sp-toggle-track:before{transform:translateX(20px)}" +

    ".sp-segment{display:flex;border-radius:var(--action-r);overflow:hidden;border:1px solid var(--border);margin-bottom:14px}" +
    ".sp-segment button{flex:1;padding:8px 0;background:var(--surface2);color:var(--text2);" +
    "border:none;font-size:.8rem;font-weight:500;cursor:pointer;transition:all .25s;font-family:inherit}" +
    ".sp-segment button:hover{color:var(--text)}" +
    ".sp-segment button.active{background:var(--accent);color:#fff}" +
    ".sp-screensaver-mode{margin-bottom:24px}" +

    ".sp-cond-field{padding:0 0 4px;display:none}" +
    ".sp-cond-field.sp-visible{display:block}" +
    ".sp-schedule-times.sp-hidden{display:none}" +

    ".sp-range-row{display:flex;align-items:center;gap:12px;margin-bottom:16px}" +
    ".sp-range-row:last-child{margin-bottom:0}" +
    ".sp-range{flex:1;height:4px;-webkit-appearance:none;appearance:none;background:var(--surface2);" +
    "border-radius:2px;outline:none}" +
    ".sp-range::-webkit-slider-thumb{-webkit-appearance:none;width:18px;height:18px;" +
    "border-radius:50%;background:var(--accent);cursor:pointer;" +
    "box-shadow:0 1px 4px rgba(0,0,0,.3)}" +
    ".sp-range::-moz-range-thumb{width:18px;height:18px;border-radius:50%;" +
    "background:var(--accent);cursor:pointer;border:none;" +
    "box-shadow:0 1px 4px rgba(0,0,0,.3)}" +
    ".sp-range-val{min-width:42px;text-align:right;font-size:.8rem;color:var(--text2);" +
    "font-variant-numeric:tabular-nums}" +

    ".sp-color-row{display:flex;align-items:center;gap:8px;margin-bottom:16px}" +
    ".sp-color-row:last-child{margin-bottom:0}" +
    ".sp-color-swatch{width:40px;height:38px;border-radius:8px;border:1px solid var(--border);" +
    "cursor:pointer;flex-shrink:0;position:relative;overflow:hidden;transition:border-color .25s}" +
    ".sp-color-swatch:hover{border-color:var(--accent)}" +
    ".sp-color-swatch input{position:absolute;inset:-8px;width:calc(100% + 16px);" +
    "height:calc(100% + 16px);cursor:pointer;opacity:0}" +
    ".sp-color-row .sp-input{flex:1}" +

    ".sp-number-row{display:flex;align-items:center;gap:8px;margin-bottom:16px}" +
    ".sp-number-row:last-child{margin-bottom:0}" +
    ".sp-number{width:80px;padding:10px 12px;background:var(--surface2);border:1px solid var(--border);" +
    "border-radius:8px;color:var(--text);font-size:.875rem;font-family:inherit;text-align:center;" +
    "outline:none;box-sizing:border-box;transition:border-color .25s,box-shadow .25s}" +
    ".sp-number:focus{border-color:var(--accent);box-shadow:0 0 0 3px var(--accent-soft)}" +
    ".sp-number-unit{font-size:.85rem;color:var(--text2)}" +

    ".sp-apply-bar{padding:var(--gap);text-align:center}" +
    ".sp-apply-btn{background:var(--accent);color:#fff;border:none;border-radius:var(--action-r);" +
    "padding:12px 28px;font-size:.875rem;font-weight:600;cursor:pointer;" +
    "font-family:inherit;transition:background .25s,opacity .25s,box-shadow .25s}" +
    ".sp-apply-btn:hover{background:var(--accent-hover);box-shadow:var(--shadow-2)}" +
    ".sp-apply-btn:active{opacity:.85}" +
    ".sp-apply-btn:disabled{opacity:.4;cursor:not-allowed}" +
    ".sp-apply-note{font-size:.75rem;color:var(--text3);margin-top:8px}" +

    ".sp-empty{text-align:center;padding:24px;color:var(--text3);font-size:.85rem}" +

    ".sp-ctx-menu{position:fixed;z-index:200;background:var(--surface);border:1px solid var(--border);" +
    "border-radius:var(--radius);padding:4px 0;min-width:160px;box-shadow:var(--shadow-3);" +
    "font-family:'Inter',-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif}" +
    ".sp-ctx-item{display:flex;align-items:center;gap:10px;padding:8px 14px;" +
    "cursor:pointer;font-size:.8rem;color:var(--text);transition:background .15s;white-space:nowrap}" +
    ".sp-ctx-item:hover{background:var(--accent-soft)}" +
    ".sp-ctx-item .mdi{font-size:16px;width:18px;text-align:center;color:var(--text2)}" +
    ".sp-ctx-item.sp-ctx-danger{color:var(--danger)}" +
    ".sp-ctx-item.sp-ctx-danger .mdi{color:var(--danger)}" +
    ".sp-ctx-divider{height:1px;background:var(--border);margin:4px 0}" +
    ".sp-ctx-sub{position:relative}" +
    ".sp-ctx-sub::after{content:'\\F0142';font-family:'Material Design Icons';position:absolute;right:10px;font-size:14px;opacity:.5}" +
    ".sp-ctx-submenu{display:none;position:absolute;top:-4px;left:100%;background:var(--surface);" +
    "border:1px solid var(--border);border-radius:var(--radius);padding:4px 0;min-width:120px;" +
    "box-shadow:var(--shadow-3);z-index:201}" +
    ".sp-ctx-sub:hover>.sp-ctx-submenu{display:block}" +
    ".sp-ctx-check{font-size:14px;width:18px;text-align:center;color:var(--accent)}" +

    ".sp-banner{padding:12px var(--gap);font-size:.8rem;font-weight:500;text-align:center;display:none}" +
    ".sp-banner.sp-error{display:block;background:rgba(244,63,94,.16);color:#f66f81;border-bottom:1px solid rgba(244,63,94,.25)}" +
    ".sp-banner.sp-offline{display:block;background:var(--accent);color:#fff;border-bottom:none}" +
    ".sp-banner.sp-success{display:block;background:rgba(16,185,129,.16);color:#3dd68c;border-bottom:1px solid rgba(16,185,129,.25)}" +
    ".sp-banner.sp-warning{display:block;background:rgba(234,179,8,.16);color:#f9b44e;border-bottom:1px solid rgba(234,179,8,.25)}" +

    ".sp-backup-btns{display:flex;gap:8px}" +
    ".sp-backup-btn{flex:1;display:flex;align-items:center;justify-content:center;gap:8px;" +
    "padding:10px 16px;border:1px solid var(--border);border-radius:var(--action-r);font-size:.8rem;font-weight:500;" +
    "cursor:pointer;font-family:inherit;transition:all .25s;background:var(--surface2);" +
    "color:var(--text)}" +
    ".sp-backup-btn:hover{background:var(--border);border-color:#4a4d54}" +
    ".sp-backup-btn .mdi{font-size:16px}" +

    ".sp-sun-info{font-size:.8rem;color:var(--text2);padding:8px 12px;margin-top:12px;background:var(--surface2);" +
    "border-radius:8px;text-align:center;display:none}" +
    ".sp-sun-info.sp-visible{display:block}" +

    ".sp-field-hint{font-size:.75rem;color:var(--text2);margin-top:6px;margin-bottom:16px}" +

    ".sp-fw-row{display:flex;align-items:center;justify-content:space-between;gap:8px;min-height:36px;margin-bottom:12px}" +
    ".sp-fw-version{font-size:.875rem;color:var(--text)}" +
    ".sp-fw-label{font-size:inherit;color:var(--text2)}" +
    ".sp-fw-actions{display:flex;align-items:center;justify-content:flex-end;gap:12px;margin-left:auto}" +
    ".sp-fw-inline-status{display:none;font-size:.8rem;color:#3dd68c;white-space:nowrap}" +
    ".sp-fw-inline-status.sp-visible{display:inline}" +
    ".sp-fw-status{font-size:.8rem;color:var(--text2);line-height:1.4;margin:-4px 0 12px 0}" +
    ".sp-fw-status.sp-update-available{color:#3dd68c}" +
    ".sp-fw-status.sp-update-installing{color:#f9b44e}" +
    ".sp-fw-status a{color:inherit;text-decoration:underline;text-underline-offset:2px}" +
    ".sp-fw-btn{background:var(--surface2);color:var(--text);border:1px solid var(--border);" +
    "border-radius:var(--action-r);padding:8px 14px;font-size:.8rem;font-weight:500;cursor:pointer;" +
    "font-family:inherit;transition:all .25s;white-space:nowrap}" +
    ".sp-fw-btn:hover{background:var(--border);border-color:#4a4d54}" +
    ".sp-fw-btn:disabled{opacity:.4;cursor:not-allowed}" +
    ".sp-fw-btn.sp-fw-btn-busy{background:rgba(35,37,43,.5);color:#8a8d94;" +
    "border-color:rgba(74,77,84,.45);border-radius:999px;padding:8px 28px;opacity:1}" +
    ".sp-fw-btn.sp-fw-btn-busy:hover{background:rgba(35,37,43,.5);border-color:rgba(74,77,84,.45)}" +

    ".sp-btn-label-row{display:flex;align-items:flex-end;width:100%;overflow:hidden}" +
    ".sp-btn-label-row .sp-btn-label{flex:1;min-width:0}" +
    ".sp-subpage-badge{font-size:var(--btn-label);line-height:1.2;opacity:.5;flex-shrink:0;" +
    "cursor:pointer;padding:2px 0 2px 4px;border-radius:4px;transition:opacity .15s}" +
    ".sp-subpage-badge:hover{opacity:1}" +
    ".sp-type-badge{font-size:var(--btn-label);line-height:1.2;opacity:.35;flex-shrink:0;" +
    "padding:2px 0 2px 4px;pointer-events:none}" +

    "@media(max-width:768px){" +
    ":root{--gap:12px}" +
    "#sp-app{max-width:100%}" +
    ".sp-header{padding:0 12px;height:48px}" +
    ".sp-brand{font-size:.875rem}" +
    ".sp-tab{padding:0 12px;font-size:.8rem}" +
    ".sp-tab-docs{margin-left:4px;padding-left:18px}" +
    ".card{padding:16px}" +
    ".card-header{margin:-16px -16px 0 -16px;padding:16px 16px 0 16px}" +
    ".card-body{padding-top:14px}" +
    ".sp-config{padding:var(--gap)}" +
    "}" +

    "@media(max-width:480px){" +
    ":root{--gap:10px}" +
    ".sp-header{padding:0 10px}" +
    ".sp-tab{padding:0 10px;font-size:.75rem}" +
    ".sp-tab-docs{margin-left:2px;padding-left:16px;gap:4px}" +
    ".sp-color-row{flex-wrap:wrap}" +
    ".sp-backup-btns{flex-direction:column}" +
    ".sp-fw-row{flex-direction:column;align-items:flex-start;gap:12px}" +
    "}";

  // ── State ──────────────────────────────────────────────────────────────

  var NTP_SERVER_DEFAULTS = ["0.pool.ntp.org", "1.pool.ntp.org", "2.pool.ntp.org"];

  var state = {
    grid: [],
    sizes: {},
    buttons: [],
    onColor: "FF8C00",
    offColor: "313131",
    sensorColor: "212121",
    selectedSlots: [],
    lastClickedSlot: -1,
    activeTab: "screen",
    _indoorOn: false,
    _outdoorOn: false,
    _indoorVal: null,
    _outdoorVal: null,
    indoorEntity: "",
    outdoorEntity: "",
    temperatureUnit: "Auto",
    clockBarOn: false,
    temperatureDegreeSymbolOn: true,
    presenceEntity: "",
    screensaverMode: "disabled",
    _screensaverModeReceived: false,
    clockScreensaverOn: false,
    clockBrightnessDay: 35,
    clockBrightnessNight: 35,
    clockBrightnessSplitReceived: false,
    screensaverTimeout: 300,
    screensaverTimeoutMin: 60,
    screensaverTimeoutMax: 3600,
    screensaverTimeoutLimitsLoaded: false,
    homeScreenTimeout: 60,
    brightnessDayVal: 100,
    brightnessNightVal: 75,
    scheduleEnabled: false,
    scheduleOnHour: 6,
    scheduleOffHour: 23,
    scheduleMode: "screen_off",
    scheduleWakeTimeout: 60,
    scheduleWakeBrightness: 10,
    scheduleDimmedBrightness: 10,
    scheduleClockBrightness: 10,
    timezone: "UTC (GMT+0)",
    timezoneOptions: [],
    clockFormat: "24h",
    clockFormatOptions: ["12h", "24h"],
    customNtpServers: false,
    ntpServer1: NTP_SERVER_DEFAULTS[0],
    ntpServer2: NTP_SERVER_DEFAULTS[1],
    ntpServer3: NTP_SERVER_DEFAULTS[2],
    screenRotation: "0",
    screenRotationOptions: (CFG.features && CFG.features.screenRotationOptions) || ["0", "90", "180", "270"],
    sunrise: "",
    sunset: "",
    firmwareVersion: "",
    firmwareLatestVersion: "",
    firmwareUpdateState: "",
    firmwareReleaseUrl: "",
    firmwareChecking: false,
    firmwareInstallTargetVersion: "",
    autoUpdate: true,
    updateFrequency: "Daily",
    updateFreqOptions: ["Hourly", "Daily", "Weekly", "Monthly"],
    developerExperimentalFeatures: false,
    bluetoothProxyEnabled: false,
    subpages: {},
    subpageRaw: {},
    subpageSavePending: {},
    editingSubpage: null,
    subpageSelectedSlots: [],
    subpageLastClicked: -1,
    clipboard: null,
    settingsDraft: null,
    entityPostPaths: {},
  };

  for (var i = 0; i < NUM_SLOTS; i++) {
    state.grid.push(0);
    state.buttons.push({ entity: "", label: "", icon: "Auto", icon_on: "Auto", sensor: "", unit: "", type: "", precision: "" });
  }

  function getActiveScreensaverMode() {
    if (state.screensaverMode === "sensor") return "sensor";
    if (state.screensaverMode === "timer") return "timer";
    return "disabled";
  }

  function normalizeScreenRotation(value) {
    value = String(value == null ? "" : value);
    return state.screenRotationOptions.indexOf(value) !== -1 ? value : "0";
  }

  function displayScreenRotation(value) {
    var labels = CFG.features && CFG.features.screenRotationDisplayLabels;
    value = String(value == null ? "" : value);
    if (labels && Object.prototype.hasOwnProperty.call(labels, value)) return labels[value];
    var offset = (CFG.features && parseInt(CFG.features.screenRotationDisplayOffset, 10)) || 0;
    var n = parseInt(value, 10);
    if (!isFinite(n)) return value;
    return String((n + offset + 360) % 360);
  }

  function normalizeTemperatureUnit(value) {
    var unit = String(value == null ? "" : value).trim().toLowerCase();
    if (unit === "f" || unit === "\u00B0f" || unit === "fahrenheit") return "\u00B0F";
    if (unit === "c" || unit === "\u00B0c" || unit === "celsius" || unit === "centigrade") return "\u00B0C";
    return "Auto";
  }

  function timezonePrefersFahrenheit(timezone) {
    var tz = getTzId(timezone || state.timezone);
    var fahrenheitZones = {
      "America/Adak": true,
      "America/Anchorage": true,
      "America/Boise": true,
      "America/Chicago": true,
      "America/Denver": true,
      "America/Detroit": true,
      "America/Juneau": true,
      "America/Los_Angeles": true,
      "America/New_York": true,
      "America/Phoenix": true,
      "America/Puerto_Rico": true,
      "Pacific/Guam": true,
      "Pacific/Honolulu": true,
      "Pacific/Pago_Pago": true,
    };
    return !!fahrenheitZones[tz];
  }

  function temperatureUnitSymbol() {
    var unit = normalizeTemperatureUnit(state.temperatureUnit);
    if (unit === "\u00B0F") return "\u00B0F";
    if (unit === "\u00B0C") return "\u00B0C";
    return timezonePrefersFahrenheit(state.timezone) ? "\u00B0F" : "\u00B0C";
  }

  function clockBarTemperatureUnitSymbol() {
    return state.temperatureDegreeSymbolOn ? "\u00B0" : "";
  }

  function appendScreenRotationOption(select, opt) {
    var o = document.createElement("option");
    o.value = opt;
    o.textContent = displayScreenRotation(opt) + " deg";
    select.appendChild(o);
  }

  function normalizeHour(value, fallback) {
    var n = parseInt(value, 10);
    if (!isFinite(n)) return fallback;
    if (n < 0) return 0;
    if (n > 23) return 23;
    return n;
  }

  function normalizeScheduleWakeTimeout(value) {
    var n = parseFloat(value);
    if (!isFinite(n) || n <= 0) return 60;
    if (n < 10) return 10;
    if (n > 3600) return 3600;
    return Math.round(n);
  }

  function normalizeScheduleWakeBrightness(value) {
    var n = parseFloat(value);
    if (!isFinite(n) || n <= 0) return 10;
    if (n < 10) return 10;
    if (n > 100) return 100;
    return Math.round(n);
  }

  function normalizeScheduleClockBrightness(value) {
    var n = parseFloat(value);
    if (!isFinite(n) || n <= 0) return 10;
    if (n < 1) return 1;
    if (n > 100) return 100;
    return Math.round(n);
  }

  function normalizeScheduleDimmedBrightness(value) {
    var n = parseFloat(value);
    if (!isFinite(n) || n <= 0) return 10;
    if (n < 1) return 1;
    if (n > 100) return 100;
    return Math.round(n);
  }

  function normalizeScheduleMode(value) {
    var v = String(value || "").toLowerCase().replace(/[\s-]+/g, "_");
    if (v === "screen_dimmed" || v === "dimmed" || v === "always_on" || v === "always") {
      return "screen_dimmed";
    }
    if (v === "clock") return "clock";
    return "screen_off";
  }

  function scheduleModeOption(value) {
    var mode = normalizeScheduleMode(value);
    if (mode === "screen_dimmed") return "Screen Dimmed";
    if (mode === "clock") return "Clock";
    return "Screen off";
  }

  function normalizeClockBrightness(value, fallback) {
    var n = parseFloat(value);
    if (!isFinite(n) || n <= 0) return fallback;
    if (n < 1) return 1;
    if (n > 100) return 100;
    return Math.round(n);
  }

  function normalizeNtpServer(value, fallback) {
    var v = String(value == null ? "" : value).trim();
    return v || fallback;
  }

  function hasCustomNtpServers() {
    return normalizeNtpServer(state.ntpServer1, NTP_SERVER_DEFAULTS[0]) !== NTP_SERVER_DEFAULTS[0] ||
      normalizeNtpServer(state.ntpServer2, NTP_SERVER_DEFAULTS[1]) !== NTP_SERVER_DEFAULTS[1] ||
      normalizeNtpServer(state.ntpServer3, NTP_SERVER_DEFAULTS[2]) !== NTP_SERVER_DEFAULTS[2];
  }

  function resetNtpServersToDefaults() {
    state.ntpServer1 = NTP_SERVER_DEFAULTS[0];
    state.ntpServer2 = NTP_SERVER_DEFAULTS[1];
    state.ntpServer3 = NTP_SERVER_DEFAULTS[2];
  }

  function formatDuration(seconds) {
    seconds = normalizeScheduleWakeTimeout(seconds);
    if (seconds < 60) return seconds + " second" + (seconds === 1 ? "" : "s");
    if (seconds % 60 === 0) {
      var minutes = seconds / 60;
      return minutes + " minute" + (minutes === 1 ? "" : "s");
    }
    return seconds + " seconds";
  }

  var SCREENSAVER_TIMEOUT_OPTIONS = [
    { label: "10 seconds", value: 10 },
    { label: "30 seconds", value: 30 },
    { label: "1 minute", value: 60 },
    { label: "5 minutes", value: 300 },
    { label: "10 minutes", value: 600 },
    { label: "15 minutes", value: 900 },
    { label: "20 minutes", value: 1200 },
    { label: "30 minutes", value: 1800 },
    { label: "45 minutes", value: 2700 },
    { label: "1 hour", value: 3600 },
  ];

  function readNumberMeta(d, keys, fallback) {
    for (var i = 0; i < keys.length; i++) {
      if (d[keys[i]] == null) continue;
      var n = parseFloat(d[keys[i]]);
      if (isFinite(n)) return n;
    }
    return fallback;
  }

  function syncScreensaverTimeoutLimits(d) {
    state.screensaverTimeoutMin = readNumberMeta(d, ["min", "min_value"], state.screensaverTimeoutMin);
    state.screensaverTimeoutMax = readNumberMeta(d, ["max", "max_value"], state.screensaverTimeoutMax);
    state.screensaverTimeoutLimitsLoaded = true;
  }

  function screensaverTimeoutSupported(value) {
    var n = parseFloat(value);
    if (!isFinite(n)) return false;
    if (!state.screensaverTimeoutLimitsLoaded) {
      return n > 0 && n <= state.screensaverTimeoutMax;
    }
    return n >= state.screensaverTimeoutMin && n <= state.screensaverTimeoutMax;
  }

  function syncScreensaverTimeoutUi() {
    var select = els.setSSTimeout;
    if (!select) return;
    var current = String(state.screensaverTimeout);
    select.innerHTML = "";
    SCREENSAVER_TIMEOUT_OPTIONS.forEach(function (opt) {
      if (!screensaverTimeoutSupported(opt.value)) return;
      var o = document.createElement("option");
      o.value = opt.value;
      o.textContent = opt.label;
      select.appendChild(o);
    });
    if (screensaverTimeoutSupported(state.screensaverTimeout)) {
      setSelectValue(select, state.screensaverTimeout, formatDuration(state.screensaverTimeout));
      select.value = current;
    }
  }

  function applyScreensaverTimeoutState(d) {
    if (!d) return;
    syncScreensaverTimeoutLimits(d);
    var n = parseFloat(d.value != null ? d.value : d.state);
    if (!isFinite(n)) return;
    state.screensaverTimeout = n;
    syncScreensaverTimeoutUi();
  }

  function setSelectValue(select, value, label) {
    if (!select) return;
    value = String(value);
    var found = false;
    for (var i = 0; i < select.options.length; i++) {
      if (select.options[i].value === value) {
        found = true;
        break;
      }
    }
    if (!found) {
      var o = document.createElement("option");
      o.value = value;
      o.textContent = label || value;
      select.appendChild(o);
    }
    select.value = value;
  }

  function formatHour(hour) {
    hour = normalizeHour(hour, 0);
    var suffix = hour < 12 ? "AM" : "PM";
    var h = hour % 12;
    if (h === 0) h = 12;
    return h + ":00 " + suffix;
  }

  function syncScreenScheduleUi() {
    state.scheduleOnHour = normalizeHour(state.scheduleOnHour, 6);
    state.scheduleOffHour = normalizeHour(state.scheduleOffHour, 23);
    state.scheduleMode = normalizeScheduleMode(state.scheduleMode);
    state.scheduleWakeTimeout = normalizeScheduleWakeTimeout(state.scheduleWakeTimeout);
    state.scheduleWakeBrightness = normalizeScheduleWakeBrightness(state.scheduleWakeBrightness);
    state.scheduleDimmedBrightness = normalizeScheduleDimmedBrightness(state.scheduleDimmedBrightness);
    state.scheduleClockBrightness = normalizeScheduleClockBrightness(state.scheduleClockBrightness);
    if (els.setScheduleToggle) els.setScheduleToggle.checked = !!state.scheduleEnabled;
    if (els.setScheduleOnHour) els.setScheduleOnHour.value = String(state.scheduleOnHour);
    if (els.setScheduleOffHour) els.setScheduleOffHour.value = String(state.scheduleOffHour);
    if (els.setScheduleMode) {
      setSelectValue(els.setScheduleMode, state.scheduleMode, scheduleModeOption(state.scheduleMode));
    }
    setSelectValue(els.setScheduleWakeTimeout, state.scheduleWakeTimeout, formatDuration(state.scheduleWakeTimeout));
    if (els.setScheduleWakeBrightness) {
      els.setScheduleWakeBrightness.value = state.scheduleWakeBrightness;
      els.setScheduleWakeBrightnessVal.textContent = Math.round(state.scheduleWakeBrightness) + "%";
    }
    if (els.setScheduleDimmedBrightness) {
      els.setScheduleDimmedBrightness.value = state.scheduleDimmedBrightness;
      els.setScheduleDimmedBrightnessVal.textContent = Math.round(state.scheduleDimmedBrightness) + "%";
    }
    if (els.setScheduleClockBrightness) {
      els.setScheduleClockBrightness.value = state.scheduleClockBrightness;
      els.setScheduleClockBrightnessVal.textContent = Math.round(state.scheduleClockBrightness) + "%";
    }
    if (els.setScheduleOffOptions) {
      els.setScheduleOffOptions.className =
        "sp-cond-field" + (state.scheduleMode === "screen_off" ? " sp-visible" : "");
    }
    if (els.setScheduleDimmedOptions) {
      els.setScheduleDimmedOptions.className =
        "sp-cond-field" + (state.scheduleMode === "screen_dimmed" ? " sp-visible" : "");
    }
    if (els.setScheduleClockOptions) {
      els.setScheduleClockOptions.className =
        "sp-cond-field" + (state.scheduleMode === "clock" ? " sp-visible" : "");
    }
    if (els.setScheduleTimes) {
      els.setScheduleTimes.className = "sp-schedule-times" + (state.scheduleEnabled ? "" : " sp-hidden");
    }
    if (els.setScheduleBadge) {
      els.setScheduleBadge.className = "sp-card-badge" + (state.scheduleEnabled ? "" : " sp-hidden");
    }
  }

  function syncTemperatureUi() {
    if (els.setIndoorToggle) els.setIndoorToggle.checked = !!state._indoorOn;
    if (els.setIndoorField) {
      els.setIndoorField.className = "sp-cond-field" + (state._indoorOn ? " sp-visible" : "");
    }
    if (els.setOutdoorToggle) els.setOutdoorToggle.checked = !!state._outdoorOn;
    if (els.setOutdoorField) {
      els.setOutdoorField.className = "sp-cond-field" + (state._outdoorOn ? " sp-visible" : "");
    }
  }

  function syncNtpServerUi() {
    if (els.setCustomNtpServersToggle) {
      els.setCustomNtpServersToggle.checked = !!state.customNtpServers;
    }
    if (els.setNtpServerFields) {
      els.setNtpServerFields.className =
        "sp-field-stack" + (state.customNtpServers ? "" : " sp-hidden");
    }
    syncInput(els.setNtpServer1, state.ntpServer1);
    syncInput(els.setNtpServer2, state.ntpServer2);
    syncInput(els.setNtpServer3, state.ntpServer3);
  }

  function syncClockBarUi() {
    var compactTop = CFG.grid.compactTop != null ? CFG.grid.compactTop : CFG.grid.bottom;
    var gridTop = state.clockBarOn ? CFG.grid.top : compactTop;
    document.documentElement.style.setProperty("--grid-top", gridTop + "cqw");
    if (els.topbar) els.topbar.className = "sp-topbar" + (state.clockBarOn ? "" : " sp-hidden");
    if (els.setClockBarToggle) els.setClockBarToggle.checked = !!state.clockBarOn;
    if (els.setClockBarBadge) {
      els.setClockBarBadge.className = "sp-card-badge" + (state.clockBarOn ? "" : " sp-hidden");
    }
    if (els.setTemperatureDegreeSymbolToggle) {
      els.setTemperatureDegreeSymbolToggle.checked = !!state.temperatureDegreeSymbolOn;
    }
    updateTempPreview();
  }

  function syncIdleUi() {
    state.homeScreenTimeout = parseFloat(state.homeScreenTimeout) || 0;
    if (els.setHSTimeout) els.setHSTimeout.value = String(state.homeScreenTimeout);
    if (els.setIdleBadge) {
      els.setIdleBadge.className = "sp-card-badge" +
        (state.homeScreenTimeout > 0 ? "" : " sp-hidden");
    }
  }

  function syncBluetoothProxyUi() {
    if (els.setBluetoothProxy) els.setBluetoothProxy.checked = !!state.bluetoothProxyEnabled;
    if (els.setBluetoothProxyBadge) {
      els.setBluetoothProxyBadge.className =
        "sp-card-badge" + (state.bluetoothProxyEnabled ? "" : " sp-hidden");
    }
  }

  var els = {};
  var dragSrcPos = -1;
  var didDrag = false;
  var previewPlaceholder = null;
  var previewDropIdx = -1;
  var dragRafPending = CFG.dragAnimation ? false : null;
  var dragSrcEl = CFG.dragAnimation ? null : null;
  var dragIsSubpage = false;
  var dragEnterCount = 0;
  var orderReceived = false;
  var migrationTimer = null;
  var sliderMigrationTimer = null;
  var pendingSliderSubpageMigrations = {};
  var _eventSource = null;
  var firmwareInstallRefreshTimer = null;
  var firmwareInstallRefreshUntil = 0;

  // ── Utilities ──────────────────────────────────────────────────────────

  function escHtml(s) {
    return String(s).replace(/&/g, "&amp;").replace(/</g, "&lt;")
      .replace(/>/g, "&gt;").replace(/"/g, "&quot;");
  }

  function renderFirmwareVersion() {
    if (!els.fwVersionLabel) return;
    els.fwVersionLabel.innerHTML = '<span class="sp-fw-label">Installed </span>' +
      escHtml(state.firmwareVersion || "Dev");
  }

  function isSpecificFirmwareVersion(version) {
    version = String(version == null ? "" : version).trim().toLowerCase();
    return !!version && version !== "dev" && version !== "0.0.0";
  }

  function setFirmwareVersion(version) {
    version = String(version == null ? "" : version).trim();
    if (!version) return;
    if (isSpecificFirmwareVersion(state.firmwareVersion) && !isSpecificFirmwareVersion(version)) return;
    state.firmwareVersion = isSpecificFirmwareVersion(version) ? version : "Dev";
    renderFirmwareVersion();
    renderFirmwareUpdateStatus();
    stopFirmwareInstallRefreshIfComplete();
  }

  function displayFirmwareVersion(version) {
    return isSpecificFirmwareVersion(version) ? String(version).trim() : "Dev";
  }

  function firmwareUpdateAvailable() {
    return state.firmwareUpdateState === "UPDATE AVAILABLE" &&
      isSpecificFirmwareVersion(state.firmwareLatestVersion);
  }

  function renderFirmwareUpdateStatus() {
    if (!els.fwStatus) return;
    var cls = "sp-fw-status";
    var status = "";
    var inlineStatus = "";
    if (state.firmwareUpdateState === "INSTALLING") {
      status = "Installing update\u2026";
      cls += " sp-update-installing";
    } else if (firmwareUpdateAvailable()) {
      status = "Latest public version: " + escHtml(state.firmwareLatestVersion);
      if (state.firmwareReleaseUrl) {
        status += ' <a href="' + escAttr(state.firmwareReleaseUrl) + '" target="_blank" rel="noopener">release notes</a>';
      }
      cls += " sp-update-available";
    } else if (state.firmwareUpdateState === "NO UPDATE") {
      inlineStatus = "Up to date";
    } else if (state.firmwareChecking) {
      status = "Checking public firmware\u2026";
    }
    els.fwStatus.className = cls;
    els.fwStatus.innerHTML = status;
    if (els.fwInlineStatus) {
      els.fwInlineStatus.className = "sp-fw-inline-status" + (inlineStatus ? " sp-visible" : "");
      els.fwInlineStatus.textContent = inlineStatus;
    }
    if (els.fwCheckBtn) {
      var isBusy = state.firmwareUpdateState === "INSTALLING" || state.firmwareChecking;
      els.fwCheckBtn.className = "sp-fw-btn" + (isBusy ? " sp-fw-btn-busy" : "");
      if (state.firmwareUpdateState === "INSTALLING") {
        els.fwCheckBtn.disabled = true;
        els.fwCheckBtn.textContent = "Installing\u2026";
      } else if (firmwareUpdateAvailable()) {
        els.fwCheckBtn.disabled = false;
        els.fwCheckBtn.textContent = "Install Update";
      } else {
        els.fwCheckBtn.disabled = state.firmwareChecking;
        els.fwCheckBtn.textContent = state.firmwareChecking ? "Checking\u2026" : "Check for Update";
      }
    }
  }

  function setFirmwareUpdateInfo(d) {
    var latest = d.latest_version || d.value || "";
    var updateState = String(d.state || state.firmwareUpdateState || "").trim().toUpperCase();
    if (d.current_version) setFirmwareVersion(d.current_version);
    if (latest) state.firmwareLatestVersion = String(latest).trim();
    if (state.firmwareInstallTargetVersion &&
        updateState === "UPDATE AVAILABLE" &&
        Date.now() < firmwareInstallRefreshUntil) {
      updateState = "INSTALLING";
    }
    state.firmwareUpdateState = updateState;
    state.firmwareReleaseUrl = d.release_url || state.firmwareReleaseUrl || "";
    if (state.firmwareUpdateState) state.firmwareChecking = false;
    if (state.firmwareUpdateState === "INSTALLING") {
      startFirmwareInstallRefresh();
    } else {
      stopFirmwareInstallRefreshIfComplete();
    }
    renderFirmwareUpdateStatus();
  }

  function firmwareVersionMatches(version, expected) {
    return String(version == null ? "" : version).trim() ===
      String(expected == null ? "" : expected).trim();
  }

  function stopFirmwareInstallRefresh() {
    if (firmwareInstallRefreshTimer) clearTimeout(firmwareInstallRefreshTimer);
    firmwareInstallRefreshTimer = null;
    firmwareInstallRefreshUntil = 0;
    state.firmwareInstallTargetVersion = "";
  }

  function stopFirmwareInstallRefreshIfComplete() {
    var target = state.firmwareInstallTargetVersion;
    if (!target || state.firmwareUpdateState !== "NO UPDATE") return false;
    if (isSpecificFirmwareVersion(target) && !firmwareVersionMatches(state.firmwareVersion, target)) {
      setFirmwareVersion(target);
    }
    stopFirmwareInstallRefresh();
    return true;
  }

  function pollFirmwareInstallRefresh() {
    firmwareInstallRefreshTimer = null;
    refreshFirmwareVersion();
    if (stopFirmwareInstallRefreshIfComplete()) return;
    if (Date.now() >= firmwareInstallRefreshUntil) {
      stopFirmwareInstallRefresh();
      return;
    }
    firmwareInstallRefreshTimer = setTimeout(pollFirmwareInstallRefresh, 5000);
  }

  function startFirmwareInstallRefresh() {
    if (!state.firmwareInstallTargetVersion && isSpecificFirmwareVersion(state.firmwareLatestVersion)) {
      state.firmwareInstallTargetVersion = state.firmwareLatestVersion;
    }
    firmwareInstallRefreshUntil = Date.now() + 180000;
    if (firmwareInstallRefreshTimer) clearTimeout(firmwareInstallRefreshTimer);
    firmwareInstallRefreshTimer = setTimeout(pollFirmwareInstallRefresh, 5000);
  }

  function isFirmwareVersionEvent(id, d) {
    id = String(id || "").toLowerCase();
    var nameId = String(d.name_id || "").toLowerCase();
    var domain = String(d.domain || "").toLowerCase();
    var name = String(d.name || "").toLowerCase();
    return nameId === "text_sensor/firmware: version" ||
      (domain === "text_sensor" && name === "firmware: version") ||
      (id.indexOf("text_sensor-") === 0 && id.indexOf("firmware") !== -1 && id.indexOf("version") !== -1);
  }

  function isFirmwareUpdateEvent(id, d) {
    id = String(id || "").toLowerCase();
    var nameId = String(d.name_id || "").toLowerCase();
    var domain = String(d.domain || "").toLowerCase();
    var name = String(d.name || "").toLowerCase();
    return nameId === "update/firmware: update" ||
      (domain === "update" && name === "firmware: update") ||
      (id.indexOf("update-") === 0 && id.indexOf("firmware") !== -1 && id.indexOf("update") !== -1);
  }

  function escAttr(s) {
    return s.replace(/&/g, "&amp;").replace(/"/g, "&quot;")
      .replace(/</g, "&lt;").replace(/>/g, "&gt;");
  }

  function isSettingsFocused() {
    var ae = document.activeElement;
    return ae && els.buttonSettings && els.buttonSettings.contains(ae);
  }

  function isSettingsOpen() {
    return !!(els.settingsOverlay && els.settingsOverlay.classList.contains("sp-visible"));
  }

  // ── Context abstraction ────────────────────────────────────────────────

  function ctx() {
    if (state.editingSubpage) {
      var sp = getSubpage(state.editingSubpage);
      return {
        grid: sp.grid, sizes: sp.sizes, buttons: sp.buttons,
        maxSlots: NUM_SLOTS, selected: state.subpageSelectedSlots,
        isSub: true,
        setSelected: function (s) { state.subpageSelectedSlots = s; },
        setLastClicked: function (s) { state.subpageLastClicked = s; },
        getLastClicked: function () { return state.subpageLastClicked; },
        save: function () { saveSubpageConfig(state.editingSubpage); },
      };
    }
    return {
      grid: state.grid, sizes: state.sizes, buttons: state.buttons,
      maxSlots: NUM_SLOTS, selected: state.selectedSlots,
      isSub: false,
      setSelected: function (s) { state.selectedSlots = s; },
      setLastClicked: function (s) { state.lastClickedSlot = s; },
      getLastClicked: function () { return state.lastClickedSlot; },
      save: function () { postText("Button Order", serializeGrid(state.grid)); },
    };
  }

  // ── Grid helpers ───────────────────────────────────────────────────────

  function parseOrder(str) {
    var grid = [];
    for (var i = 0; i < NUM_SLOTS; i++) grid.push(0);
    if (!str || !str.trim()) return grid;
    var parts = str.split(",");
    for (var i = 0; i < parts.length && i < NUM_SLOTS; i++) {
      var s = parts[i].trim();
      if (!s) continue;
      var last = s.charAt(s.length - 1);
      var dbl = last === "d";
      var wide = last === "w";
      var big = last === "b";
      var n = parseInt(s, 10);
      if (n >= 1 && n <= NUM_SLOTS && !isNaN(n)) {
        grid[i] = n;
        if (big) state.sizes[n] = 4;
        else if (dbl) state.sizes[n] = 2;
        else if (wide) state.sizes[n] = 3;
      }
    }
    applySpans(grid, state.sizes, NUM_SLOTS);
    return grid;
  }

  function applySpans(grid, sizes, maxSlots) {
    for (var i = 0; i < maxSlots; i++) {
      if ((grid[i] > 0 || grid[i] === -2) && sizes[grid[i]] === 4) {
        var below = i + GRID_COLS;
        var right = i + 1;
        var diag = i + GRID_COLS + 1;
        if (below >= maxSlots || right >= maxSlots || right % GRID_COLS === 0 || diag >= maxSlots) {
          delete sizes[grid[i]];
          continue;
        }
        var toRes = [below, right, diag];
        var ok = true;
        for (var ci = 0; ci < toRes.length; ci++) {
          if (grid[toRes[ci]] > 0 || grid[toRes[ci]] === -2) {
            var displaced = grid[toRes[ci]];
            var placed = false;
            for (var j = 0; j < maxSlots; j++) {
              if (grid[j] === 0) { grid[j] = displaced; placed = true; break; }
            }
            if (!placed) { ok = false; break; }
            grid[toRes[ci]] = 0;
          }
        }
        if (!ok) { delete sizes[grid[i]]; continue; }
        for (var ci = 0; ci < toRes.length; ci++) grid[toRes[ci]] = -1;
        continue;
      }
      if ((grid[i] > 0 || grid[i] === -2) && sizes[grid[i]] === 2) {
        var below = i + GRID_COLS;
        if (below >= maxSlots) continue;
        if (grid[below] > 0 || grid[below] === -2) {
          var displaced = grid[below];
          var placed = false;
          for (var j = 0; j < maxSlots; j++) {
            if (grid[j] === 0) { grid[j] = displaced; placed = true; break; }
          }
          if (!placed) {
            delete sizes[grid[i]];
            continue;
          }
        }
        grid[below] = -1;
      }
      if ((grid[i] > 0 || grid[i] === -2) && sizes[grid[i]] === 3) {
        var right = i + 1;
        if (right >= maxSlots || right % GRID_COLS === 0) {
          delete sizes[grid[i]];
          continue;
        }
        if (grid[right] > 0 || grid[right] === -2) {
          var displaced = grid[right];
          var placed = false;
          for (var j = 0; j < maxSlots; j++) {
            if (grid[j] === 0) { grid[j] = displaced; placed = true; break; }
          }
          if (!placed) {
            delete sizes[grid[i]];
            continue;
          }
        }
        grid[right] = -1;
      }
    }
  }

  function serializeGrid(grid) {
    var last = -1;
    for (var i = grid.length - 1; i >= 0; i--) {
      if (grid[i] > 0) { last = i; break; }
    }
    if (last < 0) return "";
    return grid.slice(0, last + 1).map(function (slot) {
      if (slot <= 0) return "";
      var sz = state.sizes[slot];
      return slot + (sz === 4 ? "b" : sz === 2 ? "d" : sz === 3 ? "w" : "");
    }).join(",");
  }

  function applyImportedButtonOrder(orderStr, importedSizes) {
    state.sizes = importedSizes || {};
    state.grid = parseOrder(orderStr);
  }

  function clearSpans(grid, maxSlots) {
    for (var i = 0; i < maxSlots; i++) {
      if (grid[i] === -1) grid[i] = 0;
    }
  }


  function resolveIcon(b) {
    var sel = b.icon || "Auto";
    if (sel === "Auto" && b.entity) {
      var domain = b.entity.split(".")[0];
      return DOMAIN_ICONS[domain] || "cog";
    }
    return iconSlug(sel);
  }

  function btnDisplayName(b) {
    return b.label || b.entity || "Configure";
  }

  // ── POST queue ─────────────────────────────────────────────────────────

  var _postQueue = Promise.resolve();

  function uniquePush(list, value) {
    if (value && list.indexOf(value) === -1) list.push(value);
  }

  function esphomeObjectId(value) {
    return String(value || "").replace(/./g, function (ch) {
      if (ch === " ") return "_";
      var lower = ch.toLowerCase();
      if ((lower >= "a" && lower <= "z") || (ch >= "0" && ch <= "9") || ch === "-" || ch === "_") return lower;
      return "_";
    });
  }

  function parseEntityId(value) {
    var id = String(value || "");
    if (!id) return null;
    if (id.indexOf("/") !== -1) {
      var parts = id.split("/");
      if (parts.length < 2 || !parts[0] || !parts[parts.length - 1]) return null;
      return {
        raw: id,
        domain: parts[0],
        name: parts[parts.length - 1],
        objectId: esphomeObjectId(parts[parts.length - 1]),
        path: "/" + parts.map(encodeURIComponent).join("/"),
      };
    }
    var dash = id.indexOf("-");
    if (dash <= 0) return null;
    return {
      raw: id,
      domain: id.substring(0, dash),
      objectId: id.substring(dash + 1),
      path: "/" + encodeURIComponent(id.substring(0, dash)) + "/" + encodeURIComponent(id.substring(dash + 1)),
    };
  }

  function entityStateKeys(data) {
    var keys = [];
    [data && data.id, data && data.name_id].forEach(function (id) {
      var parsed = parseEntityId(id);
      uniquePush(keys, id);
      if (parsed && parsed.domain && parsed.objectId) uniquePush(keys, parsed.domain + "-" + parsed.objectId);
      if (parsed && parsed.domain && parsed.name) uniquePush(keys, parsed.domain + ":" + parsed.name);
    });
    return keys;
  }

  function rememberEntityPostPath(data) {
    var preferred = parseEntityId(data && data.name_id) || parseEntityId(data && data.id);
    if (!preferred || !preferred.path) return;
    entityStateKeys(data).forEach(function (key) {
      state.entityPostPaths[key] = preferred.path;
    });
    if (preferred.domain && preferred.name) state.entityPostPaths[preferred.domain + ":" + preferred.name] = preferred.path;
    if (preferred.domain && preferred.objectId) state.entityPostPaths[preferred.domain + ":" + preferred.objectId] = preferred.path;
  }

  function rememberedPostUrls(domain, name, objectIds, action) {
    var urls = [];
    var keys = [domain + ":" + name, domain + "-" + esphomeObjectId(name)];
    objectIds.forEach(function (objectId) {
      keys.push(domain + ":" + objectId);
      keys.push(domain + "-" + objectId);
    });
    keys.forEach(function (key) {
      if (state.entityPostPaths[key]) uniquePush(urls, state.entityPostPaths[key] + "/" + action);
    });
    return urls;
  }

  function post(url, fallbackUrl, errorMessage) {
    var urls = Array.isArray(url) ? url.slice() : [url];
    if (fallbackUrl) urls.push(fallbackUrl);
    _postQueue = _postQueue.then(function () {
      var index = 0;
      function tryNext() {
        return fetch(urls[index], { method: "POST" }).then(function (r) {
          if (r.ok || index >= urls.length - 1) {
            if (!r.ok) showBanner(errorMessage || ("Request failed: " + r.status), "error");
            return r;
          }
          index++;
          return tryNext();
        });
      }
      return tryNext().catch(function () {
        showBanner("Cannot reach device \u2014 is it connected?", "error");
      });
    });
    return _postQueue;
  }

  function postText(name, value) {
    post("/text/" + encodeURIComponent(name) + "/set?value=" + encodeURIComponent(value));
  }

  function saveButtonConfig(slot) {
    var b = state.buttons[slot - 1];
    postText("Button " + slot + " Config", serializeButtonConfig(b));
  }

  function saveSubpageEntity(slot) {
    var sp = state.subpages[slot];
    var full = sp ? serializeSubpageConfig(sp) : "";
    var chunks = ["", "", "", ""];
    var rest = full;
    for (var ci = 0; ci < chunks.length && rest; ci++) {
      if (rest.length <= 255) {
        chunks[ci] = rest;
        rest = "";
        break;
      }
      var splitAt = rest.lastIndexOf("|", 255);
      if (splitAt <= 0) splitAt = 255;
      chunks[ci] = rest.substring(0, splitAt);
      rest = rest.substring(splitAt);
    }
    if (rest) {
      showBanner("Subpage is too large to save. Shorten labels or entity IDs.", "error");
      return;
    }
    state.subpageSavePending[slot] = full;
    postText("Subpage " + slot + " Config", chunks[0]);
    postText("Subpage " + slot + " Config Ext", chunks[1]);
    postText("Subpage " + slot + " Config Ext 2", chunks[2]);
    postText("Subpage " + slot + " Config Ext 3", chunks[3]);
  }

  function scheduleSliderSubpageMigration(slot) {
    pendingSliderSubpageMigrations[slot] = true;
    clearTimeout(sliderMigrationTimer);
    sliderMigrationTimer = setTimeout(function () {
      var pending = pendingSliderSubpageMigrations;
      pendingSliderSubpageMigrations = {};
      for (var key in pending) {
        if (state.subpages[key]) saveSubpageEntity(key);
      }
    }, 5000);
  }

  function postSelect(name, option) {
    post("/select/" + encodeURIComponent(name) + "/set?option=" + encodeURIComponent(option));
  }

  function postButtonPress(name) {
    post("/button/" + encodeURIComponent(name) + "/press");
  }

  function postUpdateInstall(name) {
    post("/update/" + encodeURIComponent(name) + "/install");
  }

  function postSwitch(name, on) {
    post("/switch/" + encodeURIComponent(name) + "/" + (on ? "turn_on" : "turn_off"));
  }

  function postNumber(name, value) {
    post("/number/" + encodeURIComponent(name) + "/set?value=" + encodeURIComponent(value));
  }

  function postWithObjectId(domain, name, objectId, action, errorMessage) {
    postWithObjectIds(domain, name, [objectId], action, errorMessage);
  }

  function postWithObjectIds(domain, name, objectIds, action, errorMessage) {
    var urls = rememberedPostUrls(domain, name, objectIds, action);
    uniquePush(urls, "/" + domain + "/" + encodeURIComponent(name) + "/" + action);
    objectIds.forEach(function (objectId) {
      uniquePush(urls, "/" + domain + "/" + encodeURIComponent(objectId) + "/" + action);
    });
    post(urls, null, errorMessage);
  }

  function postNumberWithObjectId(name, objectId, value, errorMessage) {
    postWithObjectId("number", name, objectId, "set?value=" + encodeURIComponent(value), errorMessage);
  }

  function postNumberWithObjectIds(name, objectIds, value, errorMessage) {
    postWithObjectIds("number", name, objectIds, "set?value=" + encodeURIComponent(value), errorMessage);
  }

  function postSelectWithObjectId(name, objectId, option, errorMessage) {
    postWithObjectId("select", name, objectId, "set?option=" + encodeURIComponent(option), errorMessage);
  }

  function postScreensaverTimeout(value) {
    if (!screensaverTimeoutSupported(value)) {
      showBanner("Update the device firmware before using shorter screensaver timers.", "error");
      syncScreensaverTimeoutUi();
      return;
    }
    postNumberWithObjectIds("Screensaver Timeout", ["screensaver_timeout"], value);
  }

  function postClockBrightnessDay(value) {
    postNumberWithObjectIds("Screen Saver: Daytime Clock Brightness", [
      "screen_saver__daytime_clock_brightness",
      "screen_saver__clock_brightness",
    ], value);
  }

  function postClockBrightnessNight(value) {
    postNumberWithObjectIds("Screen Saver: Nighttime Clock Brightness", [
      "screen_saver__nighttime_clock_brightness",
      "screen_saver__clock_brightness",
    ], value);
  }

  function postSwitchWithObjectId(name, objectId, on, errorMessage) {
    postWithObjectId("switch", name, objectId, on ? "turn_on" : "turn_off", errorMessage);
  }

  function postSwitchWithObjectIds(name, objectIds, on, errorMessage) {
    postWithObjectIds("switch", name, objectIds, on ? "turn_on" : "turn_off", errorMessage);
  }

  var CLOCK_BAR_UNAVAILABLE =
    "Clock bar setting is not available on this firmware. Update the device firmware, then reload this page.";

  function postClockBar(on) {
    postSwitchWithObjectIds("Screen: Clock Bar", [
      "screen__clock_bar",
      "screen_clock_bar",
      "clock_bar_enabled",
    ], on, CLOCK_BAR_UNAVAILABLE);
  }

  var TEMPERATURE_DEGREE_SYMBOL_UNAVAILABLE =
    "Temperature degree symbol setting is not available on this firmware. Update the device firmware, then reload this page.";

  function postTemperatureDegreeSymbol(on) {
    postSwitchWithObjectIds("Screen: Temperature Degree Symbol", [
      "screen__temperature_degree_symbol",
      "screen_temperature_degree_symbol",
      "temperature_degree_symbol_enabled",
    ], on, TEMPERATURE_DEGREE_SYMBOL_UNAVAILABLE);
  }

  var SCREEN_SCHEDULE_UNAVAILABLE =
    "Screen schedule is not available on this firmware. Update the device firmware, then reload this page.";
  var SCREEN_SCHEDULE_WAKE_TIMEOUT_UNAVAILABLE =
    "The schedule wake timeout setting is not available on this firmware. Update the device firmware, then reload this page.";
  var SCREEN_SCHEDULE_WAKE_BRIGHTNESS_UNAVAILABLE =
    "The schedule wake brightness setting is not available on this firmware. Update the device firmware, then reload this page.";
  var SCREEN_SCHEDULE_MODE_UNAVAILABLE =
    "The schedule mode setting is not available on this firmware. Update the device firmware, then reload this page.";
  var SCREEN_SCHEDULE_DIMMED_BRIGHTNESS_UNAVAILABLE =
    "The schedule dimmed brightness setting is not available on this firmware. Update the device firmware, then reload this page.";
  var SCREEN_SCHEDULE_CLOCK_BRIGHTNESS_UNAVAILABLE =
    "The schedule clock brightness setting is not available on this firmware. Update the device firmware, then reload this page.";

  function postScreenScheduleEnabled(on) {
    postSwitchWithObjectId("Screen: Schedule Enabled", "screen__schedule_enabled", on, SCREEN_SCHEDULE_UNAVAILABLE);
  }

  function postScreenScheduleOnHour(value) {
    postNumberWithObjectId("Screen: Schedule On Hour", "screen__schedule_on_hour", value, SCREEN_SCHEDULE_UNAVAILABLE);
  }

  function postScreenScheduleOffHour(value) {
    postNumberWithObjectId("Screen: Schedule Off Hour", "screen__schedule_off_hour", value, SCREEN_SCHEDULE_UNAVAILABLE);
  }

  function postScreenScheduleMode(value) {
    postSelectWithObjectId(
      "Screen: Schedule Mode",
      "screen__schedule_mode",
      scheduleModeOption(value),
      SCREEN_SCHEDULE_MODE_UNAVAILABLE
    );
  }

  function postScreenScheduleWakeTimeout(value) {
    postNumberWithObjectIds("Screen: Schedule Wake Timeout", [
      "screen__schedule_wake_timeout",
      "screen_schedule_wake_timeout",
      "schedule_wake_timeout",
    ], value, SCREEN_SCHEDULE_WAKE_TIMEOUT_UNAVAILABLE);
  }

  function postScreenScheduleWakeBrightness(value) {
    postNumberWithObjectIds("Screen: Schedule Wake Brightness", [
      "screen__schedule_wake_brightness",
      "screen_schedule_wake_brightness",
      "schedule_wake_brightness",
    ], value, SCREEN_SCHEDULE_WAKE_BRIGHTNESS_UNAVAILABLE);
  }

  function postScreenScheduleDimmedBrightness(value) {
    postNumberWithObjectIds("Screen: Schedule Dimmed Brightness", [
      "screen__schedule_dimmed_brightness",
      "screen_schedule_dimmed_brightness",
      "schedule_dimmed_brightness",
    ], value, SCREEN_SCHEDULE_DIMMED_BRIGHTNESS_UNAVAILABLE);
  }

  function postScreenScheduleClockBrightness(value) {
    postNumberWithObjectIds("Screen: Schedule Clock Brightness", [
      "screen__schedule_clock_brightness",
      "screen_schedule_clock_brightness",
      "schedule_clock_brightness",
    ], value, SCREEN_SCHEDULE_CLOCK_BRIGHTNESS_UNAVAILABLE);
  }

  var BLUETOOTH_PROXY_UNAVAILABLE =
    "Bluetooth proxy is not available on this firmware. Update the device firmware, then reload this page.";

  function postBluetoothProxy(on) {
    postSwitchWithObjectIds("Bluetooth Proxy", [
      "bluetooth_proxy",
      "bluetooth_proxy_enabled",
    ], on, BLUETOOTH_PROXY_UNAVAILABLE);
  }

  function getJsonQuietly(path, callback) {
    return fetch(path, { cache: "no-store" }).then(function (r) {
      if (!r.ok) return null;
      return r.json();
    }).then(function (data) {
      if (data && callback) callback(data);
      return data;
    }).catch(function () {});
  }

  function entityDetailPath(domain, name) {
    return "/" + encodeURIComponent(domain) + "/" + encodeURIComponent(name) + "?detail=all";
  }

  function eventStreamEnabled() {
    try {
      return new URLSearchParams(window.location.search).get("events") === "1";
    } catch (_) {
      return false;
    }
  }

  function cardStateEntities() {
    var items = [
      ["text", "Button Order"],
      ["text", "Button On Color"],
      ["text", "Button Off Color"],
      ["text", "Sensor Card Color"],
    ];

    for (var i = 1; i <= NUM_SLOTS; i++) {
      items.push(["text", "Button " + i + " Config"]);
    }

    return items;
  }

  function settingsStateEntities() {
    var items = [
      ["switch", "Indoor Temp Enable"],
      ["switch", "Outdoor Temp Enable"],
      ["switch", "Screen: Clock Bar"],
      ["switch", "Screen: Temperature Degree Symbol"],
      ["select", "Screen: Temperature Unit"],
      ["text", "Indoor Temp Entity"],
      ["text", "Outdoor Temp Entity"],
      ["text", "Screensaver Mode"],
      ["text", "Presence Sensor Entity"],
      ["number", "Screen Saver: Daytime Clock Brightness"],
      ["number", "Screen Saver: Nighttime Clock Brightness"],
      ["number", "Screen Saver: Clock Brightness"],
      ["number", "Screensaver Timeout"],
      ["number", "Home Screen Timeout"],
      ["switch", "Screen Saver: Clock"],
      ["select", "Screen: Timezone"],
      ["select", "Screen: Clock Format"],
      ["text", "Screen: NTP Server 1"],
      ["text", "Screen: NTP Server 2"],
      ["text", "Screen: NTP Server 3"],
      ["text_sensor", "Screen: Sunrise"],
      ["text_sensor", "Screen: Sunset"],
      ["switch", "Screen: Schedule Enabled"],
      ["select", "Screen: Schedule Mode"],
      ["number", "Screen: Schedule On Hour"],
      ["number", "Screen: Schedule Off Hour"],
      ["number", "Screen: Schedule Wake Timeout"],
      ["number", "Screen: Schedule Wake Brightness"],
      ["number", "Screen: Schedule Dimmed Brightness"],
      ["number", "Screen: Schedule Clock Brightness"],
      ["number", "Screen: Daytime Brightness"],
      ["number", "Screen: Nighttime Brightness"],
      ["text_sensor", "Firmware: Version"],
      ["update", "Firmware: Update"],
      ["switch", "Firmware: Auto Update"],
      ["select", "Firmware: Update Frequency"],
      ["switch", "Developer: Experimental Features"],
      ["switch", "Bluetooth Proxy"],
    ];

    if (CFG.features && CFG.features.screenRotation) {
      items.push(["select", "Screen: Rotation"]);
    }

    return items;
  }

  function subpageStateEntities() {
    var items = [];
    for (var i = 1; i <= NUM_SLOTS; i++) {
      items.push(["text", "Subpage " + i + " Config"]);
      items.push(["text", "Subpage " + i + " Config Ext"]);
      items.push(["text", "Subpage " + i + " Config Ext 2"]);
      items.push(["text", "Subpage " + i + " Config Ext 3"]);
    }

    return items;
  }

  function loadStateItems(items, handleState, concurrency) {
    var index = 0;
    var active = 0;
    var loadedCount = 0;
    var limit = Math.max(1, concurrency || 1);

    return new Promise(function (resolve) {
      function done() {
        active--;
        run();
      }

      function run() {
        if (index >= items.length && active === 0) {
          resolve(loadedCount);
          return;
        }

        while (active < limit && index < items.length) {
          var item = items[index++];
          active++;
          getJsonQuietly(entityDetailPath(item[0], item[1])).then(function (data) {
            if (data) {
              loadedCount++;
              handleState(data);
            }
          }).then(done, done);
        }
      }

      run();
    });
  }

  function loadInitialState(handleState) {
    loadStateItems(cardStateEntities(), handleState, 4).then(function (loadedCount) {
      if (loadedCount === 0) {
        showBanner("Reconnecting to device\u2026", "offline");
        setTimeout(connectEvents, 5000);
        return;
      }
      clearTimeout(migrationTimer);
      migrationTimer = setTimeout(scheduleMigration, 5000);
      clearTimeout(sliderMigrationTimer);
      pendingSliderSubpageMigrations = {};

      loadStateItems(settingsStateEntities(), handleState, 2).then(function () {
        loadStateItems(subpageStateEntities(), handleState, 2);
      });
    });
  }

  function refreshFirmwareVersion() {
    getJsonQuietly("/text_sensor/" + encodeURIComponent("Firmware: Version") + "?detail=all", function (d) {
      setFirmwareVersion(d.state || d.value);
    });
    getJsonQuietly("/update/" + encodeURIComponent("Firmware: Update") + "?detail=all", function (d) {
      setFirmwareUpdateInfo(d);
    });
  }

  function refreshScreensaverTimeout() {
    getJsonQuietly("/number/" + encodeURIComponent("Screensaver Timeout") + "?detail=all", applyScreensaverTimeoutState)
      .then(function (data) {
        if (!data) {
          getJsonQuietly("/number/" + encodeURIComponent("screensaver_timeout") + "?detail=all", applyScreensaverTimeoutState);
        }
      });
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

  // ── Subpage helpers ────────────────────────────────────────────────────

  function normalizeButtonConfig(b) {
    if (b && b.type === "slider" && b.sensor) {
      b.sensor = "";
    }
    if (b && b.type === "weather_forecast") {
      b.type = "weather";
      b.precision = "tomorrow";
    }
    if (b && b.type === "text_sensor") {
      b.type = "sensor";
      b.precision = "text";
      b.entity = "";
      b.label = "";
      b.unit = "";
      b.icon_on = "Auto";
      if (!b.icon) b.icon = "Auto";
    }
    if (b && b.type === "media") {
      if (b.sensor === "controls") {
        if (!b.icon || b.icon === "Speaker") b.icon = "Auto";
        b.sensor = "play_pause";
      } else if (!b.sensor) {
        b.sensor = "play_pause";
      }
      if (["play_pause", "previous", "next", "volume", "position", "now_playing"].indexOf(b.sensor) < 0) {
        b.sensor = "play_pause";
      }
      if (b.sensor === "previous" && b.label === "Skip Previous") b.label = "Previous";
      if (b.sensor === "next" && b.label === "Skip Next") b.label = "Next";
      if (b.sensor === "volume") {
        if (!b.label || b.label === "Media") b.label = "Volume";
        b.icon = "Auto";
      }
    }
    return b;
  }

  function trimConfigFields(fields) {
    while (fields.length > 1 && !fields[fields.length - 1]) fields.pop();
    return fields;
  }

  function buttonConfigFields(b) {
    var type = b && b.type || "";
    var sensor = type === "slider" ? "" : (b && b.sensor || "");
    var unit = b && b.unit || "";
    var precision = b && b.precision || "";
    if (!type && !sensor) {
      unit = "";
      precision = "";
    }
    return trimConfigFields([
      b && b.entity || "",
      b && b.label || "",
      b && b.icon || "Auto",
      b && b.icon_on || "Auto",
      sensor,
      unit,
      type,
      precision,
    ]);
  }

  function encodeConfigField(value) {
    return String(value || "").replace(/[%,;|:]/g, function (ch) {
      var hex = ch.charCodeAt(0).toString(16).toUpperCase();
      return "%" + (hex.length < 2 ? "0" : "") + hex;
    });
  }

  function decodeConfigField(value) {
    return String(value || "").replace(/%([0-9a-fA-F]{2})/g, function (_, hex) {
      return String.fromCharCode(parseInt(hex, 16));
    });
  }

  function legacyButtonConfigSafe(fields) {
    return fields.join(";").charAt(0) !== "~" && fields.every(function (field) {
      return String(field || "").indexOf(";") < 0;
    });
  }

  function serializeButtonConfig(b) {
    var fields = buttonConfigFields(b || {});
    if (legacyButtonConfigSafe(fields)) return fields.join(";");
    return "~" + fields.map(encodeConfigField).join(",");
  }

  function parseRawButtonConfig(str) {
    var compact = str && str.charAt(0) === "~";
    var parts = compact ? str.substring(1).split(",") : (str || "").split(";");
    if (compact) {
      parts = parts.map(decodeConfigField);
    }
    return {
      entity: parts[0] || "",
      label: parts[1] || "",
      icon: parts[2] || "Auto",
      icon_on: parts[3] || "Auto",
      sensor: parts[4] || "",
      unit: parts[5] || "",
      type: parts[6] || "",
      precision: parts[7] || "",
    };
  }

  function parseButtonConfig(str) {
    return normalizeButtonConfig(parseRawButtonConfig(str));
  }

  function hasLegacySliderDirection(b) {
    return !!(b && b.type === "slider" && b.sensor);
  }

  function buttonConfigHasLegacySliderDirection(str) {
    return hasLegacySliderDirection(parseRawButtonConfig(str || ""));
  }

  function parseSubpageConfig(str, raw) {
    if (str && str.charAt(0) === "~") return parseCompactSubpageConfig(str, raw);
    if (!str || !str.trim()) return { order: [], buttons: [] };
    var parts = str.split("|");
    var order = [];
    if (parts[0]) {
      var op = parts[0].split(",");
      for (var i = 0; i < op.length; i++) {
        var s = op[i].trim();
        order.push(s);
      }
    }
    var buttons = [];
    for (var i = 1; i < parts.length; i++) {
      var f = parts[i].split(":");
      var button = {
        entity: f[0] || "",
        label: f[1] || "",
        icon: f[2] || "Auto",
        icon_on: f[3] || "Auto",
        sensor: f[4] || "",
        unit: f[5] || "",
        type: f[6] || "",
        precision: f[7] || "",
      };
      buttons.push(raw ? button : normalizeButtonConfig(button));
    }
    return { order: order, buttons: buttons };
  }

  function subpageTypeCode(type) {
    var map = {
      action: "A",
      calendar: "D",
      timezone: "T",
      climate: "H",
      sensor: "S",
      weather: "W",
      weather_forecast: "F",
      slider: "L",
      cover: "C",
      light_temperature: "N",
      garage: "R",
      lock: "K",
      media: "M",
      push: "P",
      internal: "I",
      subpage: "G",
    };
    return map[type || ""] || (type || "");
  }

  function subpageTypeFromCode(code) {
    var map = {
      A: "action",
      D: "calendar",
      T: "timezone",
      H: "climate",
      S: "sensor",
      W: "weather",
      F: "weather_forecast",
      L: "slider",
      C: "cover",
      N: "light_temperature",
      R: "garage",
      K: "lock",
      M: "media",
      P: "push",
      I: "internal",
      G: "subpage",
    };
    return map[code || ""] || (code || "");
  }

  function encodeSubpageField(value) {
    return encodeConfigField(value);
  }

  function decodeSubpageField(value) {
    return decodeConfigField(value);
  }

  function parseCompactSubpageConfig(str, raw) {
    if (!str || str.length < 2) return { order: [], buttons: [] };
    var parts = str.substring(1).split("|");
    var order = [];
    if (parts[0]) {
      var op = parts[0].split(",");
      for (var i = 0; i < op.length; i++) order.push(op[i].trim());
    }
    var buttons = [];
    for (var i = 1; i < parts.length; i++) {
      var f = parts[i].split(",");
      var button = {
        type: subpageTypeFromCode(f[0] || ""),
        entity: decodeSubpageField(f[1]),
        label: decodeSubpageField(f[2]),
        icon: decodeSubpageField(f[3]) || "Auto",
        icon_on: decodeSubpageField(f[4]) || "Auto",
        sensor: decodeSubpageField(f[5]),
        unit: decodeSubpageField(f[6]),
        precision: decodeSubpageField(f[7]),
      };
      buttons.push(raw ? button : normalizeButtonConfig(button));
    }
    return { order: order, buttons: buttons };
  }

  function subpageConfigHasLegacySliderDirection(str) {
    var sp = parseSubpageConfig(str, true);
    for (var i = 0; i < sp.buttons.length; i++) {
      if (hasLegacySliderDirection(sp.buttons[i])) return true;
    }
    return false;
  }

  function serializeSubpageConfig(sp) {
    var legacy = legacySubpageConfigSafe(sp) ? serializeLegacySubpageConfig(sp) : "";
    var compact = serializeCompactSubpageConfig(sp);
    if (!compact) return legacy;
    if (!legacy) return compact;
    return compact.length < legacy.length ? compact : legacy;
  }

  function legacySubpageConfigSafe(sp) {
    if (!sp || !sp.buttons) return true;
    for (var i = 0; i < sp.buttons.length; i++) {
      var b = sp.buttons[i];
      var fields = [b.entity || "", b.label || "", b.icon || "Auto", b.icon_on || "Auto", b.sensor || "", b.unit || "", b.type || "", b.precision || ""];
      for (var j = 0; j < fields.length; j++) {
        if (String(fields[j] || "").indexOf("|") >= 0 || String(fields[j] || "").indexOf(":") >= 0) {
          return false;
        }
      }
    }
    return true;
  }

  function serializeLegacySubpageConfig(sp) {
    if (!sp || !sp.buttons || sp.buttons.length === 0) return "";
    var out = sp.order.join(",");
    for (var i = 0; i < sp.buttons.length; i++) {
      var b = sp.buttons[i];
      var sensor = b.type === "slider" ? "" : (b.sensor || "");
      var fields = [b.entity || "", b.label || "", b.icon || "Auto", b.icon_on || "Auto", sensor, b.unit || "", b.type || "", b.precision || ""];
      while (fields.length > 1 && !fields[fields.length - 1]) fields.pop();
      if (fields.length > 1 && fields[fields.length - 1] === "Auto") {
        while (fields.length > 1 && (fields[fields.length - 1] === "Auto" || !fields[fields.length - 1])) fields.pop();
      }
      out += "|" + fields.join(":");
    }
    return out;
  }

  function serializeCompactSubpageConfig(sp) {
    if (!sp || !sp.buttons || sp.buttons.length === 0) return "";
    var out = "~" + sp.order.join(",");
    for (var i = 0; i < sp.buttons.length; i++) {
      var b = sp.buttons[i];
      var sensor = b.type === "slider" ? "" : (b.sensor || "");
      var fields = [
        subpageTypeCode(b.type || ""),
        encodeSubpageField(b.entity),
        encodeSubpageField(b.label),
        b.icon && b.icon !== "Auto" ? encodeSubpageField(b.icon) : "",
        b.icon_on && b.icon_on !== "Auto" ? encodeSubpageField(b.icon_on) : "",
        encodeSubpageField(sensor),
        encodeSubpageField(b.unit),
        encodeSubpageField(b.precision),
      ];
      while (fields.length > 1 && !fields[fields.length - 1]) fields.pop();
      out += "|" + fields.join(",");
    }
    return out;
  }

  function applySubpageRaw(slot) {
    var raw = state.subpageRaw[slot];
    var combined = (raw ? raw.main : "") + (raw ? raw.ext : "") +
      (raw ? raw.ext2 : "") + (raw ? raw.ext3 : "");
    var pending = state.subpageSavePending[slot];
    if (pending) {
      if (combined !== pending) {
        if (state.editingSubpage === slot) scheduleRender();
        return;
      }
      delete state.subpageSavePending[slot];
    }
    var local = state.subpages[slot];
    var localHasData = local && (
      (local.buttons && local.buttons.length > 0) ||
      (local.order && local.order.length > 0)
    );
    if (state.editingSubpage === slot && localHasData) {
      var localSerialized = serializeSubpageConfig(local);
      if (combined !== localSerialized) {
        scheduleRender();
        return;
      }
    }
    if (combined) {
      var migrateSliderDirection = subpageConfigHasLegacySliderDirection(combined);
      var sp = parseSubpageConfig(combined);
      sp.sizes = sp.sizes || {};
      buildSubpageGrid(sp);
      state.subpages[slot] = sp;
      if (migrateSliderDirection) scheduleSliderSubpageMigration(slot);
    } else {
      delete state.subpages[slot];
    }
    if (state.editingSubpage === slot) {
      scheduleRender();
    }
  }

  function getSubpage(homeSlot) {
    if (!state.subpages[homeSlot]) {
      state.subpages[homeSlot] = { order: [], buttons: [], grid: [], sizes: {} };
    }
    return state.subpages[homeSlot];
  }

  function buildSubpageGrid(sp) {
    var grid = [];
    for (var i = 0; i < NUM_SLOTS; i++) grid.push(0);
    sp.sizes = sp.sizes || {};
    if (sp.order.length > 0) {
      var hasBack = false;
      for (var i = 0; i < sp.order.length; i++) {
        var t = sp.order[i];
        if (t === "B" || t === "Bd" || t === "Bw" || t === "Bb") { hasBack = true; break; }
      }
      if (hasBack) {
        for (var i = 0; i < sp.order.length && i < NUM_SLOTS; i++) {
          var s = sp.order[i];
          if (!s) continue;
          if (s === "B" || s === "Bd" || s === "Bw" || s === "Bb") {
            grid[i] = -2;
            if (s === "Bb") sp.sizes[-2] = 4;
            else if (s === "Bd") sp.sizes[-2] = 2;
            else if (s === "Bw") sp.sizes[-2] = 3;
            else delete sp.sizes[-2];
            continue;
          }
          var last = s.charAt(s.length - 1);
          var dbl = last === "d";
          var wide = last === "w";
          var big = last === "b";
          var n = parseInt(s, 10);
          if (n >= 1 && n <= sp.buttons.length && !isNaN(n)) {
            grid[i] = n;
            if (big) sp.sizes[n] = 4;
            else if (dbl) sp.sizes[n] = 2;
            else if (wide) sp.sizes[n] = 3;
          }
        }
      } else {
        grid[0] = -2;
        delete sp.sizes[-2];
        for (var i = 0; i < sp.order.length && i + 1 < NUM_SLOTS; i++) {
          var s = sp.order[i];
          if (!s) continue;
          var last = s.charAt(s.length - 1);
          var dbl = last === "d";
          var wide = last === "w";
          var big = last === "b";
          var n = parseInt(s, 10);
          if (n >= 1 && n <= sp.buttons.length && !isNaN(n)) {
            grid[i + 1] = n;
            if (big) sp.sizes[n] = 4;
            else if (dbl) sp.sizes[n] = 2;
            else if (wide) sp.sizes[n] = 3;
          }
        }
      }
    } else {
      grid[0] = -2;
      delete sp.sizes[-2];
    }
    applySpans(grid, sp.sizes, NUM_SLOTS);
    sp.grid = grid;
    return grid;
  }

  function serializeSubpageGrid(sp) {
    var grid = sp.grid;
    var last = -1;
    for (var i = grid.length - 1; i >= 0; i--) {
      if (grid[i] > 0 || grid[i] === -2) { last = i; break; }
    }
    if (last < 0) return [];
    var order = [];
    for (var i = 0; i <= last; i++) {
      if (grid[i] === -2) {
        var bsz = sp.sizes[-2];
        order.push(bsz === 4 ? "Bb" : bsz === 2 ? "Bd" : bsz === 3 ? "Bw" : "B");
      } else if (grid[i] <= 0) {
        order.push("");
      } else {
        var ssz = sp.sizes[grid[i]];
        order.push(grid[i] + (ssz === 4 ? "b" : ssz === 2 ? "d" : ssz === 3 ? "w" : ""));
      }
    }
    return order;
  }

  function enterSubpage(homeSlot) {
    state.editingSubpage = homeSlot;
    state.subpageSelectedSlots = [];
    state.subpageLastClicked = -1;
    var sp = getSubpage(homeSlot);
    buildSubpageGrid(sp);
    renderPreview();
    renderButtonSettings();
  }

  function exitSubpage() {
    state.editingSubpage = null;
    state.subpageSelectedSlots = [];
    state.subpageLastClicked = -1;
    renderPreview();
    renderButtonSettings();
  }

  function saveSubpageConfig(homeSlot) {
    var sp = getSubpage(homeSlot);
    sp.order = serializeSubpageGrid(sp);
    saveSubpageEntity(homeSlot);
  }

  function subpageFirstFreeSlot(sp) {
    var used = {};
    sp.grid.forEach(function (s) { if (s > 0) used[s] = true; });
    for (var i = 1; i <= sp.buttons.length + 1; i++) {
      if (!used[i]) return i;
    }
    return sp.buttons.length + 1;
  }

  function bindTextPost(input, postName, opts) {
    input.addEventListener("blur", function () {
      if (opts && opts.onBlur) opts.onBlur(this.value);
      postText(postName, this.value);
      if (opts && opts.rerender) renderPreview();
    });
    input.addEventListener("keydown", function (e) { if (e.key === "Enter") this.blur(); });
  }

  // ── Init ───────────────────────────────────────────────────────────────

  var FAVICON_SVG = '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><path fill="#5c73e7" d="M12,3L2,12H5V20H19V12H22L12,3M12,8.5C14.34,8.5 16.46,9.43 18,10.94L16.8,12.12C15.58,10.91 13.88,10.17 12,10.17C10.12,10.17 8.42,10.91 7.2,12.12L6,10.94C7.54,9.43 9.66,8.5 12,8.5M12,11.83C13.4,11.83 14.67,12.39 15.6,13.3L14.4,14.47C13.79,13.87 12.94,13.5 12,13.5C11.06,13.5 10.21,13.87 9.6,14.47L8.4,13.3C9.33,12.39 10.6,11.83 12,11.83M12,15.17C12.94,15.17 13.7,15.91 13.7,16.83C13.7,17.75 12.94,18.5 12,18.5C11.06,18.5 10.3,17.75 10.3,16.83C10.3,15.91 11.06,15.17 12,15.17Z"/></svg>';

  function setFavicon() {
    var link = document.querySelector('link[rel="icon"]') || document.createElement("link");
    link.rel = "icon";
    link.type = "image/svg+xml";
    link.href = "data:image/svg+xml," + encodeURIComponent(FAVICON_SVG);
    if (!link.parentNode) document.head.appendChild(link);
  }

  function addSupportButton() {
    if (document.querySelector(".sp-support-btn")) return;
    var link = document.createElement("a");
    link.className = "sp-support-btn";
    link.href = "https://www.buymeacoffee.com/jtenniswood";
    link.target = "_blank";
    link.rel = "noopener";
    link.innerHTML = '<img src="https://cdn.buymeacoffee.com/buttons/v2/default-yellow.png" alt="Buy Me A Coffee" height="60" style="border-radius:999px;">';
    document.body.appendChild(link);
  }

  function init() {
    setFavicon();

    // Set CSS custom properties from device config
    var r = document.documentElement.style;
    r.setProperty("--screen-w", CFG.screen.width);
    r.setProperty("--screen-aspect", CFG.screen.aspect);
    r.setProperty("--topbar-h", CFG.topbar.height + "cqw");
    r.setProperty("--topbar-pad", CFG.topbar.padding);
    r.setProperty("--topbar-fs", CFG.topbar.fontSize + "cqw");
    if (CFG.topbar.clockFontSize) r.setProperty("--clock-fs", CFG.topbar.clockFontSize + "cqw");
    r.setProperty("--grid-top", CFG.grid.top + "cqw");
    r.setProperty("--grid-left", CFG.grid.left + "cqw");
    r.setProperty("--grid-right", CFG.grid.right + "cqw");
    r.setProperty("--grid-bottom", CFG.grid.bottom + "cqw");
    r.setProperty("--grid-gap", CFG.grid.gap + "cqw");
    r.setProperty("--grid-cols", "repeat(" + GRID_COLS + "," + CFG.grid.fr + ")");
    r.setProperty("--grid-rows", "repeat(" + GRID_ROWS + "," + CFG.grid.fr + ")");
    r.setProperty("--btn-r", CFG.btn.radius + "cqw");
    r.setProperty("--btn-pad", CFG.btn.padding + "cqw");
    r.setProperty("--btn-icon", CFG.btn.iconSize + "cqw");
    r.setProperty("--btn-label", CFG.btn.labelSize + "cqw");
    r.setProperty("--media-title", (CFG.btn.mediaTitleSize || (CFG.btn.iconSize * 0.75)) + "cqw");
    r.setProperty("--btn-lines", String(CFG.btn.labelLines || 1));
    r.setProperty("--btn-lines-dbl", String(CFG.btn.labelLinesDouble || CFG.btn.labelLines || 1));
    r.setProperty("--sensor-top", CFG.sensorBadge.top + "cqw");
    r.setProperty("--sensor-right", CFG.sensorBadge.right + "cqw");
    r.setProperty("--sensor-fs", CFG.sensorBadge.fontSize + "cqw");
    r.setProperty("--empty-r", CFG.emptyCell.radius + "cqw");
    r.setProperty("--subpage-bottom", CFG.subpageBadge.bottom + "cqw");
    r.setProperty("--subpage-right", CFG.subpageBadge.right + "cqw");
    r.setProperty("--subpage-fs", CFG.subpageBadge.fontSize + "cqw");

    var style = document.createElement("style");
    style.textContent = CSS;
    document.head.appendChild(style);

    var mdi = document.createElement("link");
    mdi.rel = "stylesheet";
    mdi.href = "https://cdn.jsdelivr.net/npm/@mdi/font@7.4.47/css/materialdesignicons.min.css";
    document.head.appendChild(mdi);

    var fonts = document.createElement("link");
    fonts.rel = "stylesheet";
    fonts.href = "https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700&family=Roboto:wght@300;400;500&display=swap";
    document.head.appendChild(fonts);

    buildUI();
    addSupportButton();
    syncClockBarUi();
    setupPreviewEvents();
    renderPreview();
    renderButtonSettings();
    connectEvents();
    updateClock();

    document.addEventListener("click", hideContextMenu);
    document.addEventListener("mousedown", handleDocumentSelectionMouseDown);
    document.addEventListener("scroll", hideContextMenu, true);
    document.addEventListener("keydown", function (e) {
      if (e.key === "Escape") hideContextMenu();
    });
  }

  // ── Build UI ───────────────────────────────────────────────────────────

  function showBanner(msg, type) {
    if (!els.banner) return;
    els.banner.textContent = msg;
    els.banner.className = "sp-banner sp-" + type;
    if (type === "error" || type === "success" || type === "warning") {
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

    var brand = document.createElement("div");
    brand.className = "sp-brand";
    brand.textContent = "EspControl";
    header.appendChild(brand);

    var nav = document.createElement("nav");
    nav.className = "sp-nav";
    nav.setAttribute("aria-label", "Primary");

    var tabs = [
      { id: "screen", label: "Screen" },
      { id: "settings", label: "Settings" },
    ];

    tabs.forEach(function (t) {
      var tab = document.createElement("div");
      tab.className = "sp-tab";
      tab.setAttribute("role", "tab");
      tab.setAttribute("aria-selected", "false");
      tab.textContent = t.label;
      tab.addEventListener("click", function () { switchTab(t.id); });
      nav.appendChild(tab);
      els["tab_" + t.id] = tab;
    });

    var docsLink = document.createElement("a");
    docsLink.className = "sp-tab sp-tab-docs";
    docsLink.href = "https://jtenniswood.github.io/espcontrol/";
    docsLink.target = "_blank";
    docsLink.rel = "noopener";
    docsLink.innerHTML = 'Docs <span class="mdi mdi-arrow-top-right"></span>';
    nav.appendChild(docsLink);

    header.appendChild(nav);
    parent.appendChild(header);
  }

  function buildScreenPage(parent) {
    var page = document.createElement("div");
    page.id = "sp-screen";
    page.className = "sp-page";

    var selectionBar = document.createElement("div");
    selectionBar.className = "sp-selection-bar";
    els.selectionBar = selectionBar;
    page.appendChild(selectionBar);

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

    els.topbar = wrap.querySelector(".sp-topbar");
    els.temp = wrap.querySelector(".sp-temp");
    els.clock = wrap.querySelector(".sp-clock");
    els.previewMain = wrap.querySelector(".sp-main");
    els.previewMain.setAttribute("role", "grid");
    els.previewMain.setAttribute("aria-label", "Button grid");

    var hint = document.createElement("div");
    hint.className = "sp-hint";
    hint.textContent = "tap to select \u2022 shift/ctrl+tap to multi-select \u2022 right click to manage";
    els.previewHint = hint;
    page.appendChild(hint);

    var overlay = document.createElement("div");
    overlay.className = "sp-settings-overlay";
    var modal = document.createElement("div");
    modal.className = "sp-settings-modal";
    var closeBtn = document.createElement("button");
    closeBtn.className = "sp-settings-close";
    closeBtn.innerHTML = "&times;";
    closeBtn.addEventListener("click", closeSettings);
    modal.appendChild(closeBtn);
    var config = document.createElement("div");
    config.className = "sp-config";
    els.buttonSettings = config;
    modal.appendChild(config);
    overlay.appendChild(modal);
    overlay.addEventListener("click", function (e) {
      if (e.target === overlay) closeSettings();
    });
    page.appendChild(overlay);
    els.settingsOverlay = overlay;

    page.appendChild(buildApplyBar());

    parent.appendChild(page);
    els.screenPage = page;
  }

  // ── Settings Page ──────────────────────────────────────────────────────

  function buildSettingsPage(parent) {
    var page = document.createElement("div");
    page.id = "sp-settings";
    page.className = "sp-page";

    var config = document.createElement("div");
    config.className = "sp-config fade-in";

    var appearBody = document.createElement("div");

    appearBody.appendChild(fieldLabel("Primary"));
    var onColor = colorField("sp-set-on-color", "FF8C00", function (hex) {
      postText("Button On Color", hex);
    });
    appearBody.appendChild(onColor);
    els.setOnColor = onColor;

    appearBody.appendChild(fieldLabel("Secondary"));
    var offColor = colorField("sp-set-off-color", "313131", function (hex) {
      postText("Button Off Color", hex);
    });
    appearBody.appendChild(offColor);
    els.setOffColor = offColor;

    appearBody.appendChild(fieldLabel("Tertiary"));
    var sensorColor = colorField("sp-set-sensor-color", "212121", function (hex) {
      postText("Sensor Card Color", hex);
    });
    appearBody.appendChild(sensorColor);
    els.setSensorColor = sensorColor;

    config.appendChild(makeCollapsibleCard("Appearance", appearBody, true));

    var blBody = document.createElement("div");

    var daySlider = createRangeSlider("Daytime Brightness", state.brightnessDayVal, "Screen: Daytime Brightness");
    blBody.appendChild(daySlider.wrap);
    els.setDayBrightness = daySlider.range;
    els.setDayBrightnessVal = daySlider.val;

    var nightSlider = createRangeSlider("Nighttime Brightness", state.brightnessNightVal, "Screen: Nighttime Brightness");
    blBody.appendChild(nightSlider.wrap);
    els.setNightBrightness = nightSlider.range;
    els.setNightBrightnessVal = nightSlider.val;

    var sunInfo = document.createElement("div");
    sunInfo.className = "sp-sun-info";
    sunInfo.id = "sp-sun-info";
    blBody.appendChild(sunInfo);
    els.sunInfo = sunInfo;
    updateSunInfo();

    config.appendChild(makeCollapsibleCard("Backlight", blBody, true));

    var scheduleBody = document.createElement("div");
    var scheduleToggle = toggleRow("Night Schedule", "sp-set-schedule-enabled", state.scheduleEnabled);
    scheduleBody.appendChild(scheduleToggle.row);
    els.setScheduleToggle = scheduleToggle.input;

    var scheduleTimes = document.createElement("div");
    scheduleTimes.className = "sp-schedule-times";

    var onHour = createHourSelect("Daytime", "sp-set-schedule-on-hour", state.scheduleOnHour, function (hour) {
      state.scheduleOnHour = hour;
      postScreenScheduleOnHour(hour);
      syncScreenScheduleUi();
    });
    scheduleTimes.appendChild(onHour.wrap);
    els.setScheduleOnHour = onHour.select;

    var offHour = createHourSelect("Night Time", "sp-set-schedule-off-hour", state.scheduleOffHour, function (hour) {
      state.scheduleOffHour = hour;
      postScreenScheduleOffHour(hour);
      syncScreenScheduleUi();
    });
    scheduleTimes.appendChild(offHour.wrap);
    els.setScheduleOffHour = offHour.select;

    var scheduleModeField = document.createElement("div");
    scheduleModeField.className = "sp-field";
    scheduleModeField.appendChild(fieldLabel("At Night Time", "sp-set-schedule-mode"));
    var scheduleModeSelect = document.createElement("select");
    scheduleModeSelect.className = "sp-select";
    scheduleModeSelect.id = "sp-set-schedule-mode";
    [
      { value: "screen_off", label: "Screen Off" },
      { value: "screen_dimmed", label: "Screen Dimmed" },
      { value: "clock", label: "Clock" },
    ].forEach(function (opt) {
      var option = document.createElement("option");
      option.value = opt.value;
      option.textContent = opt.label;
      scheduleModeSelect.appendChild(option);
    });
    scheduleModeSelect.addEventListener("change", function () {
      state.scheduleMode = normalizeScheduleMode(this.value);
      postScreenScheduleMode(state.scheduleMode);
      syncScreenScheduleUi();
    });
    scheduleModeField.appendChild(scheduleModeSelect);
    scheduleTimes.appendChild(scheduleModeField);
    els.setScheduleMode = scheduleModeSelect;

    var offScreenOptions = condField();
    var wakeTimeoutField = document.createElement("div");
    wakeTimeoutField.className = "sp-field";
    wakeTimeoutField.appendChild(fieldLabel("When Woken, Idle Time to Screen Off", "sp-set-schedule-wake-timeout"));
    var wakeTimeoutSelect = document.createElement("select");
    wakeTimeoutSelect.className = "sp-select";
    wakeTimeoutSelect.id = "sp-set-schedule-wake-timeout";
    var wakeTimeoutOptions = [
      { label: "10 seconds", value: 10 },
      { label: "30 seconds", value: 30 },
      { label: "1 minute", value: 60 },
      { label: "2 minutes", value: 120 },
      { label: "5 minutes", value: 300 },
      { label: "10 minutes", value: 600 },
      { label: "30 minutes", value: 1800 },
      { label: "1 hour", value: 3600 },
    ];
    wakeTimeoutOptions.forEach(function (opt) {
      var o = document.createElement("option");
      o.value = opt.value;
      o.textContent = opt.label;
      wakeTimeoutSelect.appendChild(o);
    });
    wakeTimeoutSelect.addEventListener("change", function () {
      state.scheduleWakeTimeout = normalizeScheduleWakeTimeout(this.value);
      postScreenScheduleWakeTimeout(state.scheduleWakeTimeout);
      syncScreenScheduleUi();
    });
    wakeTimeoutField.appendChild(wakeTimeoutSelect);
    offScreenOptions.appendChild(wakeTimeoutField);
    els.setScheduleWakeTimeout = wakeTimeoutSelect;

    var wakeBrightnessSlider = createRangeSlider(
      "When Woken, Screen Brightness",
      state.scheduleWakeBrightness,
      postScreenScheduleWakeBrightness
    );
    wakeBrightnessSlider.range.id = "sp-set-schedule-wake-brightness";
    wakeBrightnessSlider.range.addEventListener("change", function () {
      state.scheduleWakeBrightness = normalizeScheduleWakeBrightness(this.value);
      syncScreenScheduleUi();
    });
    offScreenOptions.appendChild(wakeBrightnessSlider.wrap);
    els.setScheduleWakeBrightness = wakeBrightnessSlider.range;
    els.setScheduleWakeBrightnessVal = wakeBrightnessSlider.val;
    scheduleTimes.appendChild(offScreenOptions);
    els.setScheduleOffOptions = offScreenOptions;

    var dimmedOptions = condField();
    var dimmedBrightnessSlider = createRangeSlider(
      "Dimmed Screen Brightness",
      state.scheduleDimmedBrightness,
      postScreenScheduleDimmedBrightness
    );
    dimmedBrightnessSlider.range.id = "sp-set-schedule-dimmed-brightness";
    dimmedBrightnessSlider.range.min = "1";
    dimmedBrightnessSlider.range.step = "1";
    dimmedBrightnessSlider.range.addEventListener("input", function () {
      state.scheduleDimmedBrightness = normalizeScheduleDimmedBrightness(this.value);
      syncScreenScheduleUi();
    });
    dimmedOptions.appendChild(dimmedBrightnessSlider.wrap);
    scheduleTimes.appendChild(dimmedOptions);
    els.setScheduleDimmedOptions = dimmedOptions;
    els.setScheduleDimmedBrightness = dimmedBrightnessSlider.range;
    els.setScheduleDimmedBrightnessVal = dimmedBrightnessSlider.val;

    var clockOptions = condField();
    var clockBrightnessSlider = createRangeSlider(
      "Clock Brightness",
      state.scheduleClockBrightness,
      postScreenScheduleClockBrightness
    );
    clockBrightnessSlider.range.id = "sp-set-schedule-clock-brightness";
    clockBrightnessSlider.range.min = "1";
    clockBrightnessSlider.range.step = "1";
    clockBrightnessSlider.range.addEventListener("input", function () {
      state.scheduleClockBrightness = normalizeScheduleClockBrightness(this.value);
      syncScreenScheduleUi();
    });
    clockOptions.appendChild(clockBrightnessSlider.wrap);
    scheduleTimes.appendChild(clockOptions);
    els.setScheduleClockOptions = clockOptions;
    els.setScheduleClockBrightness = clockBrightnessSlider.range;
    els.setScheduleClockBrightnessVal = clockBrightnessSlider.val;

    scheduleBody.appendChild(scheduleTimes);
    els.setScheduleTimes = scheduleTimes;

    scheduleToggle.input.addEventListener("change", function () {
      state.scheduleEnabled = this.checked;
      postScreenScheduleEnabled(state.scheduleEnabled);
      syncScreenScheduleUi();
    });

    var scheduleBadge = document.createElement("span");
    scheduleBadge.setAttribute("aria-label", "Schedule on");
    scheduleBadge.innerHTML = '<span class="sp-card-badge-dot"></span><span>ON</span>';
    els.setScheduleBadge = scheduleBadge;
    syncScreenScheduleUi();
    var scheduleCard = makeCollapsibleCard("Night Schedule", scheduleBody, true, scheduleBadge);
    config.appendChild(scheduleCard);

    var clockBody = document.createElement("div");

    var tzField = document.createElement("div");
    tzField.className = "sp-field";
    tzField.appendChild(fieldLabel("Timezone", "sp-set-timezone"));
    var tzSelect = document.createElement("select");
    tzSelect.className = "sp-select";
    tzSelect.id = "sp-set-timezone";
    if (state.timezoneOptions.length) {
      state.timezoneOptions.forEach(function (opt) {
        appendTimezoneOption(tzSelect, opt);
      });
    }
    tzSelect.value = state.timezone;
    tzSelect.addEventListener("change", function () {
      state.timezone = this.value;
      postSelect("Screen: Timezone", this.value);
      if (normalizeTemperatureUnit(state.temperatureUnit) === "Auto") {
        updateTempPreview();
        renderPreview();
      }
      updateClock();
    });
    tzField.appendChild(tzSelect);
    clockBody.appendChild(tzField);
    els.setTimezone = tzSelect;

    var cfField = document.createElement("div");
    cfField.className = "sp-field";
    cfField.appendChild(fieldLabel("Clock Format", "sp-set-clock-format"));
    var cfSelect = document.createElement("select");
    cfSelect.className = "sp-select";
    cfSelect.id = "sp-set-clock-format";
    state.clockFormatOptions.forEach(function (opt) {
      var o = document.createElement("option");
      o.value = opt;
      o.textContent = opt === "12h" ? "12-hour" : "24-hour";
      cfSelect.appendChild(o);
    });
    cfSelect.value = state.clockFormat;
    cfSelect.addEventListener("change", function () {
      postSelect("Screen: Clock Format", this.value);
    });
    cfField.appendChild(cfSelect);
    clockBody.appendChild(cfField);
    els.setClockFormat = cfSelect;

    var ntpField = document.createElement("div");
    ntpField.className = "sp-field";
    ntpField.appendChild(fieldLabel("NTP Servers", "sp-set-custom-ntp-servers"));
    state.customNtpServers = state.customNtpServers || hasCustomNtpServers();
    var customNtpServers = toggleRow("Custom NTP Servers", "sp-set-custom-ntp-servers", state.customNtpServers);
    ntpField.appendChild(customNtpServers.row);
    els.setCustomNtpServersToggle = customNtpServers.input;
    customNtpServers.input.addEventListener("change", function () {
      state.customNtpServers = this.checked;
      if (!state.customNtpServers) {
        resetNtpServersToDefaults();
        postText("Screen: NTP Server 1", state.ntpServer1);
        postText("Screen: NTP Server 2", state.ntpServer2);
        postText("Screen: NTP Server 3", state.ntpServer3);
      }
      syncNtpServerUi();
    });

    var ntpList = document.createElement("div");
    ntpList.className = "sp-field-stack";
    els.setNtpServerFields = ntpList;

    function addNtpServerInput(id, stateKey, postName, placeholder, ariaLabel) {
      var input = textInput(id, state[stateKey], placeholder);
      input.setAttribute("aria-label", ariaLabel);
      input.addEventListener("blur", function () {
        var value = this.value.trim();
        this.value = value;
        state[stateKey] = value;
        state.customNtpServers = true;
        syncNtpServerUi();
        postText(postName, value);
      });
      input.addEventListener("keydown", function (e) {
        if (e.key === "Enter") this.blur();
      });
      ntpList.appendChild(input);
      return input;
    }

    els.setNtpServer1 = addNtpServerInput(
      "sp-set-ntp-server-1", "ntpServer1",
      "Screen: NTP Server 1", NTP_SERVER_DEFAULTS[0], "NTP Server 1");
    els.setNtpServer2 = addNtpServerInput(
      "sp-set-ntp-server-2", "ntpServer2",
      "Screen: NTP Server 2", NTP_SERVER_DEFAULTS[1], "NTP Server 2");
    els.setNtpServer3 = addNtpServerInput(
      "sp-set-ntp-server-3", "ntpServer3",
      "Screen: NTP Server 3", NTP_SERVER_DEFAULTS[2], "NTP Server 3");

    ntpField.appendChild(ntpList);
    syncNtpServerUi();
    clockBody.appendChild(ntpField);

    var timeSettingsCard = makeCollapsibleCard("Time Settings", clockBody, true);

    var clockBarBody = document.createElement("div");

    var clockBar = toggleRow("Show Clock Bar", "sp-set-clock-bar", state.clockBarOn);
    clockBarBody.appendChild(clockBar.row);
    els.setClockBarToggle = clockBar.input;
    clockBar.input.addEventListener("change", function () {
      state.clockBarOn = this.checked;
      syncClockBarUi();
      postClockBar(state.clockBarOn);
    });

    var outdoor = createEntityToggleSection("Outdoor Temperature", "sp-set-outdoor-toggle", state._outdoorOn,
      "Outdoor Temp Enable", "Outdoor Temp Entity", "Outdoor Temp Entity", "sensor.outdoor_temperature");
    clockBarBody.appendChild(outdoor.toggle.row);
    clockBarBody.appendChild(outdoor.field);
    els.setOutdoorToggle = outdoor.toggle.input;
    els.setOutdoorField = outdoor.field;
    els.setOutdoorEntity = outdoor.input;
    outdoor.toggle.input.addEventListener("change", function () {
      state._outdoorOn = this.checked;
      syncTemperatureUi();
      updateTempPreview();
    });

    var indoor = createEntityToggleSection("Indoor Temperature", "sp-set-indoor-toggle", state._indoorOn,
      "Indoor Temp Enable", "Indoor Temp Entity", "Indoor Temp Entity", "sensor.indoor_temperature");
    clockBarBody.appendChild(indoor.toggle.row);
    clockBarBody.appendChild(indoor.field);
    els.setIndoorToggle = indoor.toggle.input;
    els.setIndoorField = indoor.field;
    els.setIndoorEntity = indoor.input;
    indoor.toggle.input.addEventListener("change", function () {
      state._indoorOn = this.checked;
      syncTemperatureUi();
      updateTempPreview();
    });

    var degreeSymbol = toggleRow("Show Degree Symbol", "sp-set-temperature-degree-symbol", state.temperatureDegreeSymbolOn);
    clockBarBody.appendChild(degreeSymbol.row);
    els.setTemperatureDegreeSymbolToggle = degreeSymbol.input;
    degreeSymbol.input.addEventListener("change", function () {
      state.temperatureDegreeSymbolOn = this.checked;
      syncClockBarUi();
      postTemperatureDegreeSymbol(state.temperatureDegreeSymbolOn);
    });

    var clockBarBadge = document.createElement("span");
    clockBarBadge.setAttribute("aria-label", "Clock bar on");
    clockBarBadge.innerHTML = '<span class="sp-card-badge-dot"></span><span>ON</span>';
    els.setClockBarBadge = clockBarBadge;
    syncClockBarUi();
    syncTemperatureUi();
    config.appendChild(makeCollapsibleCard("Clock Bar", clockBarBody, true, clockBarBadge));

    if (CFG.features && CFG.features.screenRotation) {
      var rotationBody = document.createElement("div");
      var rotField = document.createElement("div");
      rotField.className = "sp-field";
      rotField.appendChild(fieldLabel("Rotation", "sp-set-screen-rotation"));
      var rotSelect = document.createElement("select");
      rotSelect.className = "sp-select";
      rotSelect.id = "sp-set-screen-rotation";
      state.screenRotationOptions.forEach(function (opt) {
        appendScreenRotationOption(rotSelect, opt);
      });
      rotSelect.value = state.screenRotation;
      rotSelect.addEventListener("change", function () {
        state.screenRotation = normalizeScreenRotation(this.value);
        postSelect("Screen: Rotation", this.value);
      });
      rotField.appendChild(rotSelect);
      rotationBody.appendChild(rotField);
      config.appendChild(makeCollapsibleCard("Rotation", rotationBody, true));
      els.setScreenRotation = rotSelect;
    }

    var tempBody = document.createElement("div");

    var unitField = document.createElement("div");
    unitField.className = "sp-field";
    unitField.appendChild(fieldLabel("Temperature Unit", "sp-set-temperature-unit"));
    var unitSelect = document.createElement("select");
    unitSelect.className = "sp-select";
    unitSelect.id = "sp-set-temperature-unit";
    [
      ["Auto", "Auto (from timezone)"],
      ["\u00B0C", "Centigrade (\u00B0C)"],
      ["\u00B0F", "Fahrenheit (\u00B0F)"],
    ].forEach(function (opt) {
      var o = document.createElement("option");
      o.value = opt[0];
      o.textContent = opt[1];
      unitSelect.appendChild(o);
    });
    unitSelect.value = normalizeTemperatureUnit(state.temperatureUnit);
    unitSelect.addEventListener("change", function () {
      state.temperatureUnit = normalizeTemperatureUnit(this.value);
      postSelect("Screen: Temperature Unit", state.temperatureUnit);
      updateTempPreview();
      renderPreview();
    });
    unitField.appendChild(unitSelect);
    tempBody.appendChild(unitField);
    els.setTemperatureUnit = unitSelect;

    syncTemperatureUi();
    config.appendChild(makeCollapsibleCard("Temperature", tempBody, true));

    var ssBody = document.createElement("div");
    var ssMode = getActiveScreensaverMode();

    ssBody.appendChild(fieldLabel("Mode"));
    var segment = document.createElement("div");
    segment.className = "sp-segment sp-screensaver-mode";
    var disabledBtn = document.createElement("button");
    disabledBtn.textContent = "Disabled";
    disabledBtn.type = "button";
    var timerBtn = document.createElement("button");
    timerBtn.textContent = "Timer";
    timerBtn.type = "button";
    var sensorBtn = document.createElement("button");
    sensorBtn.textContent = "Sensor";
    sensorBtn.type = "button";
    segment.appendChild(disabledBtn);
    segment.appendChild(timerBtn);
    segment.appendChild(sensorBtn);
    ssBody.appendChild(segment);

    var timerPanel = document.createElement("div");

    var timeoutField = document.createElement("div");
    timeoutField.className = "sp-field";
    timeoutField.appendChild(fieldLabel("Timeout"));
    var timeoutSelect = document.createElement("select");
    timeoutSelect.className = "sp-select";
    timeoutSelect.id = "sp-set-ss-timeout";
    timeoutSelect.addEventListener("change", function () {
      var n = parseFloat(this.value);
      if (isFinite(n)) state.screensaverTimeout = n;
      postScreensaverTimeout(this.value);
    });
    timeoutField.appendChild(timeoutSelect);
    timerPanel.appendChild(timeoutField);

    var timerClockControls = createScreensaverThenControls("sp-set-clock-mode");
    timerPanel.appendChild(timerClockControls.clockField);
    timerPanel.appendChild(timerClockControls.brightnessField);
    els.setClockSelect = timerClockControls.clockSelect;
    els.setClockField = timerClockControls.clockField;
    els.setClockBrightnessDay = timerClockControls.clockBrightnessDay;
    els.setClockBrightnessDayVal = timerClockControls.clockBrightnessDayVal;
    els.setClockBrightnessNight = timerClockControls.clockBrightnessNight;
    els.setClockBrightnessNightVal = timerClockControls.clockBrightnessNightVal;
    els.setClockBrightnessField = timerClockControls.brightnessField;

    ssBody.appendChild(timerPanel);
    els.setSSTimeout = timeoutSelect;
    syncScreensaverTimeoutUi();

    var sensorPanel = document.createElement("div");
    var presenceField = document.createElement("div");
    presenceField.className = "sp-field";
    presenceField.appendChild(fieldLabel("Presence Entity", "sp-set-presence"));
    var presInp = textInput("sp-set-presence", "", "Presence sensor entity");
    presenceField.appendChild(presInp);
    sensorPanel.appendChild(presenceField);
    bindTextPost(presInp, "Presence Sensor Entity", {});
    var sensorClockControls = createScreensaverThenControls("sp-set-sensor-clock-mode");
    sensorPanel.appendChild(sensorClockControls.clockField);
    sensorPanel.appendChild(sensorClockControls.brightnessField);
    ssBody.appendChild(sensorPanel);
    els.setPresence = presInp;
    els.setSensorClockSelect = sensorClockControls.clockSelect;
    els.setSensorClockField = sensorClockControls.clockField;
    els.setSensorClockBrightnessDay = sensorClockControls.clockBrightnessDay;
    els.setSensorClockBrightnessDayVal = sensorClockControls.clockBrightnessDayVal;
    els.setSensorClockBrightnessNight = sensorClockControls.clockBrightnessNight;
    els.setSensorClockBrightnessNightVal = sensorClockControls.clockBrightnessNightVal;
    els.setSensorClockBrightnessField = sensorClockControls.brightnessField;
    syncClockScreensaverControls();

    var ssBadge = document.createElement("span");
    ssBadge.setAttribute("aria-label", "Screensaver on");
    ssBadge.innerHTML = '<span class="sp-card-badge-dot"></span><span>ON</span>';
    els.setScreensaverBadge = ssBadge;

    function setSsMode(mode) {
      ssMode = mode;
      disabledBtn.className = mode === "disabled" ? "active" : "";
      timerBtn.className = mode === "timer" ? "active" : "";
      sensorBtn.className = mode === "sensor" ? "active" : "";
      timerPanel.style.display = mode === "timer" ? "" : "none";
      sensorPanel.style.display = mode === "sensor" ? "" : "none";
      if (els.setScreensaverBadge) {
        els.setScreensaverBadge.className = "sp-card-badge" + (mode === "disabled" ? " sp-hidden" : "");
      }
    }
    disabledBtn.addEventListener("click", function () {
      setSsMode("disabled");
      state.screensaverMode = "disabled";
      postText("Screensaver Mode", "disabled");
    });
    timerBtn.addEventListener("click", function () {
      setSsMode("timer");
      state.screensaverMode = "timer";
      postText("Screensaver Mode", "timer");
    });
    sensorBtn.addEventListener("click", function () {
      setSsMode("sensor");
      state.screensaverMode = "sensor";
      postText("Screensaver Mode", "sensor");
    });
    els.setSsMode = setSsMode;
    setSsMode(ssMode);

    config.insertBefore(makeCollapsibleCard("Screensaver", ssBody, true, ssBadge), scheduleCard);

    var idleBody = document.createElement("div");
    idleBody.appendChild(fieldLabel("Return Home After"));
    var hsSelect = document.createElement("select");
    hsSelect.className = "sp-select";
    hsSelect.id = "sp-set-hs-timeout";
    var hsOptions = [
      { label: "Disabled", value: 0 },
      { label: "10 seconds", value: 10 },
      { label: "20 seconds", value: 20 },
      { label: "30 seconds", value: 30 },
      { label: "1 minute", value: 60 },
      { label: "2 minutes", value: 120 },
      { label: "5 minutes", value: 300 },
    ];
    hsOptions.forEach(function (opt) {
      var o = document.createElement("option");
      o.value = opt.value;
      o.textContent = opt.label;
      if (opt.value === state.homeScreenTimeout) o.selected = true;
      hsSelect.appendChild(o);
    });
    hsSelect.addEventListener("change", function () {
      state.homeScreenTimeout = parseFloat(this.value) || 0;
      syncIdleUi();
      postNumber("Home Screen Timeout", this.value);
    });
    idleBody.appendChild(hsSelect);
    els.setHSTimeout = hsSelect;
    var idleBadge = document.createElement("span");
    idleBadge.setAttribute("aria-label", "Idle on");
    idleBadge.innerHTML = '<span class="sp-card-badge-dot"></span><span>ON</span>';
    els.setIdleBadge = idleBadge;
    syncIdleUi();
    config.appendChild(makeCollapsibleCard("Idle", idleBody, true, idleBadge));

    var backupBody = document.createElement("div");

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

    backupBody.appendChild(backupRow);
    config.appendChild(timeSettingsCard);
    config.appendChild(makeCollapsibleCard("Backup", backupBody, true));

    var bluetoothBody = document.createElement("div");
    var bluetoothToggle = toggleRow("Bluetooth Proxy", "sp-set-bluetooth-proxy", state.bluetoothProxyEnabled);
    bluetoothBody.appendChild(bluetoothToggle.row);
    bluetoothToggle.input.addEventListener("change", function () {
      state.bluetoothProxyEnabled = this.checked;
      syncBluetoothProxyUi();
      postBluetoothProxy(state.bluetoothProxyEnabled);
    });
    els.setBluetoothProxy = bluetoothToggle.input;
    var bluetoothBadge = document.createElement("span");
    bluetoothBadge.setAttribute("aria-label", "Bluetooth proxy on");
    bluetoothBadge.innerHTML = '<span class="sp-card-badge-dot"></span><span>ON</span>';
    els.setBluetoothProxyBadge = bluetoothBadge;
    syncBluetoothProxyUi();
    var bluetoothCard = makeCollapsibleCard("Bluetooth Proxy", bluetoothBody, true, bluetoothBadge);
    config.appendChild(bluetoothCard);

    var fwBody = document.createElement("div");

    var fwVersionRow = document.createElement("div");
    fwVersionRow.className = "sp-fw-row";
    var fwVersionLabel = document.createElement("span");
    fwVersionLabel.className = "sp-fw-version";
    fwVersionRow.appendChild(fwVersionLabel);
    els.fwVersionLabel = fwVersionLabel;
    renderFirmwareVersion();
    refreshFirmwareVersion();

    var fwActions = document.createElement("div");
    fwActions.className = "sp-fw-actions";
    var fwInlineStatus = document.createElement("span");
    fwInlineStatus.className = "sp-fw-inline-status";
    fwActions.appendChild(fwInlineStatus);
    els.fwInlineStatus = fwInlineStatus;

    var fwCheckBtn = document.createElement("button");
    fwCheckBtn.className = "sp-fw-btn";
    fwCheckBtn.textContent = "Check for Update";
    fwCheckBtn.addEventListener("click", function () {
      if (firmwareUpdateAvailable()) {
        state.firmwareInstallTargetVersion = state.firmwareLatestVersion;
        state.firmwareUpdateState = "INSTALLING";
        renderFirmwareUpdateStatus();
        postUpdateInstall("Firmware: Update");
        startFirmwareInstallRefresh();
        return;
      }
      state.firmwareChecking = true;
      renderFirmwareUpdateStatus();
      postButtonPress("Firmware: Check for Update");
      setTimeout(function () {
        state.firmwareChecking = false;
        refreshFirmwareVersion();
        renderFirmwareUpdateStatus();
      }, 10000);
    });
    fwActions.appendChild(fwCheckBtn);
    fwVersionRow.appendChild(fwActions);
    els.fwCheckBtn = fwCheckBtn;
    fwBody.appendChild(fwVersionRow);

    var fwStatus = document.createElement("div");
    fwStatus.className = "sp-fw-status";
    fwBody.appendChild(fwStatus);
    els.fwStatus = fwStatus;
    renderFirmwareUpdateStatus();

    var autoUpdateToggle = toggleRow("Auto Update", "sp-set-auto-update", state.autoUpdate);
    fwBody.appendChild(autoUpdateToggle.row);
    autoUpdateToggle.input.addEventListener("change", function () {
      postSwitch("Firmware: Auto Update", this.checked);
      if (els.updateFreqWrap) els.updateFreqWrap.style.display = this.checked ? "" : "none";
    });
    els.setAutoUpdate = autoUpdateToggle.input;

    var freqWrap = document.createElement("div");
    freqWrap.style.display = state.autoUpdate ? "" : "none";
    var freqSelect = document.createElement("select");
    freqSelect.className = "sp-select";
    freqSelect.id = "sp-set-update-freq";
    state.updateFreqOptions.forEach(function (opt) {
      var o = document.createElement("option");
      o.value = opt;
      o.textContent = opt;
      freqSelect.appendChild(o);
    });
    freqSelect.value = state.updateFrequency;
    freqSelect.addEventListener("change", function () {
      postSelect("Firmware: Update Frequency", this.value);
    });
    freqWrap.appendChild(freqSelect);
    fwBody.appendChild(freqWrap);
    els.updateFreqWrap = freqWrap;
    els.setUpdateFreq = freqSelect;

    config.appendChild(makeCollapsibleCard("Firmware", fwBody, true));

    if (developerExperimentalUrlFlag()) {
      var devBody = document.createElement("div");
      var experimentalToggle = toggleRow(
        "Developer/Experimental Features",
        "sp-set-developer-experimental-features",
        state.developerExperimentalFeatures
      );
      devBody.appendChild(experimentalToggle.row);
      experimentalToggle.input.addEventListener("change", function () {
        state.developerExperimentalFeatures = this.checked;
        postSwitch("Developer: Experimental Features", state.developerExperimentalFeatures);
        scheduleRender();
      });
      els.setDeveloperExperimentalFeatures = experimentalToggle.input;
      config.appendChild(makeCollapsibleCard("Developer", devBody, true));
    }

    page.appendChild(config);
    page.appendChild(buildApplyBar());

    parent.appendChild(page);
    els.settingsPage = page;
  }

  // ── Settings helpers ───────────────────────────────────────────────────

  function makeCollapsibleCard(title, bodyElement, defaultCollapsed, badgeElement) {
    var card = document.createElement("div");
    card.className = "card";
    var header = document.createElement("div");
    header.className = "card-header";
    var h3 = document.createElement("h3");
    h3.textContent = title;
    var rightWrap = document.createElement("div");
    rightWrap.className = "card-header-right";
    var chevron = document.createElement("span");
    chevron.className = "card-chevron";
    chevron.innerHTML = '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><path d="M6 9l6 6 6-6"/></svg>';
    if (badgeElement) rightWrap.appendChild(badgeElement);
    rightWrap.appendChild(chevron);
    header.appendChild(h3);
    header.appendChild(rightWrap);
    var body = document.createElement("div");
    body.className = "card-body";
    body.appendChild(bodyElement);
    card.appendChild(header);
    card.appendChild(body);
    if (defaultCollapsed) card.classList.add("collapsed");
    header.onclick = function () { card.classList.toggle("collapsed"); };
    return card;
  }

  function fieldLabel(text, forId) {
    var el = document.createElement("label");
    el.className = "sp-field-label";
    el.textContent = text;
    if (forId) el.htmlFor = forId;
    return el;
  }

  function textInput(id, value, placeholder) {
    var el = document.createElement("input");
    el.type = "text";
    el.className = "sp-input";
    if (id) el.id = id;
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
    var wrap = document.createElement("div");
    wrap.className = "sp-field";
    wrap.appendChild(fieldLabel(label));
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
    range.addEventListener("change", function () {
      if (typeof postName === "function") postName(this.value);
      else if (postName) postNumber(postName, this.value);
    });
    row.appendChild(range);
    row.appendChild(val);
    wrap.appendChild(row);
    return { wrap: wrap, range: range, val: val };
  }

  function syncClockScreensaverControls() {
    var mode = state.clockScreensaverOn ? "clock" : "off";
    var dayBrightness = Math.round(state.clockBrightnessDay) + "%";
    var nightBrightness = Math.round(state.clockBrightnessNight) + "%";
    var display = state.clockScreensaverOn ? "" : "none";

    if (els.setClockSelect) els.setClockSelect.value = mode;
    if (els.setSensorClockSelect) els.setSensorClockSelect.value = mode;
    syncOptionalClockBrightness(els.setClockBrightnessField, els.setClockField, display);
    syncOptionalClockBrightness(els.setSensorClockBrightnessField, els.setSensorClockField, display);
    if (els.setClockBrightnessDay) {
      els.setClockBrightnessDay.value = state.clockBrightnessDay;
      els.setClockBrightnessDayVal.textContent = dayBrightness;
    }
    if (els.setClockBrightnessNight) {
      els.setClockBrightnessNight.value = state.clockBrightnessNight;
      els.setClockBrightnessNightVal.textContent = nightBrightness;
    }
    if (els.setSensorClockBrightnessDay) {
      els.setSensorClockBrightnessDay.value = state.clockBrightnessDay;
      els.setSensorClockBrightnessDayVal.textContent = dayBrightness;
    }
    if (els.setSensorClockBrightnessNight) {
      els.setSensorClockBrightnessNight.value = state.clockBrightnessNight;
      els.setSensorClockBrightnessNightVal.textContent = nightBrightness;
    }
  }

  function syncOptionalClockBrightness(field, previousField, display) {
    if (field) field.style.display = display;
    if (previousField) previousField.style.marginBottom = display === "none" ? "20px" : "";
  }

  function createScreensaverThenControls(selectId) {
    var clockField = document.createElement("div");
    clockField.className = "sp-field";
    clockField.appendChild(fieldLabel("Then", selectId));
    var clockSelect = document.createElement("select");
    clockSelect.className = "sp-select";
    clockSelect.id = selectId;
    var clockOff = document.createElement("option");
    clockOff.value = "off";
    clockOff.textContent = "Display Off";
    var clockOn = document.createElement("option");
    clockOn.value = "clock";
    clockOn.textContent = "Clock";
    clockSelect.appendChild(clockOff);
    clockSelect.appendChild(clockOn);
    clockSelect.value = state.clockScreensaverOn ? "clock" : "off";
    clockSelect.addEventListener("change", function () {
      state.clockScreensaverOn = this.value === "clock";
      syncClockScreensaverControls();
      postSwitch("Screen Saver: Clock", state.clockScreensaverOn);
    });
    clockField.appendChild(clockSelect);

    var clockBrightnessField = document.createElement("div");
    clockBrightnessField.style.display = state.clockScreensaverOn ? "" : "none";
    var daySlider = createRangeSlider("Daytime Clock Brightness", state.clockBrightnessDay, postClockBrightnessDay);
    daySlider.range.min = "1";
    daySlider.range.step = "1";
    daySlider.range.addEventListener("input", function () {
      state.clockBrightnessDay = normalizeClockBrightness(this.value, 35);
      syncClockScreensaverControls();
    });
    clockBrightnessField.appendChild(daySlider.wrap);
    var nightSlider = createRangeSlider("Nighttime Clock Brightness", state.clockBrightnessNight, postClockBrightnessNight);
    nightSlider.range.min = "1";
    nightSlider.range.step = "1";
    nightSlider.range.addEventListener("input", function () {
      state.clockBrightnessNight = normalizeClockBrightness(this.value, state.clockBrightnessDay);
      syncClockScreensaverControls();
    });
    clockBrightnessField.appendChild(nightSlider.wrap);

    return {
      clockField: clockField,
      clockSelect: clockSelect,
      brightnessField: clockBrightnessField,
      clockBrightnessDay: daySlider.range,
      clockBrightnessDayVal: daySlider.val,
      clockBrightnessNight: nightSlider.range,
      clockBrightnessNightVal: nightSlider.val,
    };
  }

  function createHourSelect(label, id, initial, onChange) {
    var wrap = document.createElement("div");
    wrap.className = "sp-field";
    wrap.appendChild(fieldLabel(label, id));
    var select = document.createElement("select");
    select.className = "sp-select";
    select.id = id;
    for (var h = 0; h < 24; h++) {
      var o = document.createElement("option");
      o.value = String(h);
      o.textContent = formatHour(h);
      select.appendChild(o);
    }
    select.value = String(normalizeHour(initial, 0));
    select.addEventListener("change", function () {
      onChange(normalizeHour(this.value, 0));
    });
    wrap.appendChild(select);
    return { wrap: wrap, select: select };
  }

  function createEntityToggleSection(label, id, checked, switchName, entityLabel, entityPostName, placeholder) {
    var toggle = toggleRow(label, id, checked);
    var field = condField();
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

  function switchTab(tab) {
    state.activeTab = tab;
    ["screen", "settings"].forEach(function (t) {
      els["tab_" + t].className = "sp-tab" + (tab === t ? " active" : "");
      els["tab_" + t].setAttribute("aria-selected", tab === t ? "true" : "false");
    });
    els.screenPage.className = "sp-page" + (tab === "screen" ? " active" : "");
    els.settingsPage.className = "sp-page" + (tab === "settings" ? " active" : "");
  }

  // ── Preview rendering (unified) ────────────────────────────────────────

  function previewHtmlValue(typePreview, key, fallback) {
    return typePreview && Object.prototype.hasOwnProperty.call(typePreview, key)
      ? typePreview[key]
      : fallback;
  }

  function renderPreview() {
    var main = els.previewMain;
    main.innerHTML = "";
    var c = ctx();

    updatePreviewHint(c);

    for (var pos = 0; pos < c.maxSlots; pos++) {
      var slot = c.grid[pos];
      if (slot === -1) continue;

      if (slot === -2) {
        var backBtn = document.createElement("div");
        var bkSz = c.sizes[-2];
        backBtn.className = "sp-btn sp-back-btn" + (bkSz === 4 ? " sp-btn-big" : bkSz === 2 ? " sp-btn-double" : bkSz === 3 ? " sp-btn-wide" : "");
        backBtn.innerHTML =
          '<span class="sp-btn-icon mdi mdi-chevron-left"></span>' +
          '<span class="sp-btn-label">Back</span>';
        backBtn.style.backgroundColor = "#" + (state.offColor.length === 6 ? state.offColor : "313131");
        backBtn.style.cursor = "pointer";
        backBtn.setAttribute("data-pos", pos);
        backBtn.draggable = true;
        main.appendChild(backBtn);
      } else if (slot > 0) {
        var bIdx = slot - 1;
        if (c.isSub && bIdx >= c.buttons.length) continue;
        var b = c.buttons[bIdx];
        var iconName = resolveIcon(b);
        var label = b.label || b.entity || "Configure";
        var color = (b.type === "sensor" || b.type === "weather" || b.type === "weather_forecast" || b.type === "calendar" || b.type === "timezone")
          ? state.sensorColor : state.offColor;
        var previewTypeDef = BUTTON_TYPES[b.type || ""] || null;
        if (previewTypeDef && c.isSub && !previewTypeDef.allowInSubpage) previewTypeDef = null;
        if (previewTypeDef && previewTypeDef.experimental && !isExperimentalEnabled(previewTypeDef.experimental)) {
          previewTypeDef = null;
          iconName = "cog";
          label = "Configure";
        }
        var typePreview = previewTypeDef && previewTypeDef.renderPreview
          ? previewTypeDef.renderPreview(b, { escHtml: escHtml })
          : null;

        var btn = document.createElement("div");
        var slotSz = c.sizes[slot];
        btn.className = "sp-btn" +
          (slotSz === 4 ? " sp-btn-big" : slotSz === 2 ? " sp-btn-double" : slotSz === 3 ? " sp-btn-wide" : "") +
          (c.selected.indexOf(slot) !== -1 ? " sp-selected" : "");
        btn.style.backgroundColor = "#" + (color.length === 6 ? color : "313131");
        btn.draggable = true;
        btn.setAttribute("data-pos", pos);
        btn.setAttribute("data-slot", slot);
        var hasWhenOn = !typePreview && (b.sensor || (b.icon_on && b.icon_on !== "Auto"));
        var badgeIcon = b.sensor ? "gauge" : "swap-horizontal";
        var sensorBadge = hasWhenOn
          ? '<span class="sp-sensor-badge mdi mdi-' + badgeIcon + '"></span>'
          : '';
        var labelHtml = previewHtmlValue(typePreview, "labelHtml",
          '<span class="sp-btn-label">' + escHtml(label) + '</span>');
        var iconHtml = previewHtmlValue(typePreview, "iconHtml",
          '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>');
        btn.innerHTML =
          sensorBadge +
          iconHtml +
          labelHtml;
        main.appendChild(btn);
      } else {
        var empty = document.createElement("div");
        empty.className = "sp-empty-cell";
        empty.setAttribute("data-pos", pos);
        empty.innerHTML = '<span class="sp-add-pill"><span class="sp-add-icon mdi mdi-plus"></span></span>';
        main.appendChild(empty);
      }
    }
    renderSelectionBar(c);
  }

  // ── Button settings panel (unified) ────────────────────────────────────

  function hideSettingsOverlay() {
    if (els.settingsOverlay) els.settingsOverlay.classList.remove("sp-visible");
  }

  function updatePreviewHint(c) {
    if (!els.previewHint) return;
    c = c || ctx();
    els.previewHint.style.display = "";
    if (c.selected.length > 1) {
      els.previewHint.textContent = c.selected.length + " buttons selected \u2022 right click to copy, cut, or delete";
    } else {
      els.previewHint.textContent = "tap to select \u2022 shift/ctrl+tap to multi-select \u2022 right click to manage";
    }
  }

  function renderSelectionBar(c) {
    if (!els.selectionBar) return;
    c = c || ctx();
    els.selectionBar.innerHTML = "";
    if (!c.selected.length) {
      els.selectionBar.className = "sp-selection-bar";
      return;
    }

    els.selectionBar.className = "sp-selection-bar sp-visible";

    var label = document.createElement("span");
    label.className = "sp-selection-label";
    label.textContent = c.selected.length === 1 ? "1 card selected" : c.selected.length + " cards selected";
    els.selectionBar.appendChild(label);

    var actions = document.createElement("div");
    actions.className = "sp-selection-actions";

    if (c.selected.length === 1) {
      var editBtn = document.createElement("button");
      editBtn.type = "button";
      editBtn.className = "sp-selection-btn sp-selection-btn-primary";
      editBtn.innerHTML = '<span class="mdi mdi-pencil"></span>Edit';
      editBtn.addEventListener("click", function (e) {
        e.preventDefault();
        e.stopPropagation();
        openSelectedCardSettings();
      });
      actions.appendChild(editBtn);
    }

    var menuBtn = document.createElement("button");
    menuBtn.type = "button";
    menuBtn.className = "sp-selection-btn";
    menuBtn.setAttribute("aria-label", "Card actions");
    menuBtn.innerHTML = '<span class="mdi mdi-dots-horizontal"></span>';
    menuBtn.addEventListener("click", function (e) {
      e.preventDefault();
      e.stopPropagation();
      showSelectionMenu(e);
    });
    actions.appendChild(menuBtn);
    els.selectionBar.appendChild(actions);
  }

  function closeSettings() {
    hideSettingsOverlay();
    _settingsDeferred = false;
    state.settingsDraft = null;
    ctx().setSelected([]);
    renderPreview();
  }

  function clearCardSelection() {
    var c = ctx();
    if (!c.selected.length && c.getLastClicked() < 0) return;
    c.setSelected([]);
    c.setLastClicked(-1);
    hideSettingsOverlay();
    renderPreview();
    renderButtonSettings();
  }

  function isSelectionControlTarget(target) {
    return !!(
      (els.previewMain && els.previewMain.contains(target)) ||
      (els.selectionBar && els.selectionBar.contains(target)) ||
      (els.settingsOverlay && els.settingsOverlay.contains(target)) ||
      (ctxMenu && ctxMenu.contains(target)) ||
      (target.closest && target.closest(".sp-ctx-menu"))
    );
  }

  function handleDocumentSelectionMouseDown(e) {
    if (e.button !== 0) return;
    if (isSelectionControlTarget(e.target)) return;
    clearCardSelection();
  }

  function openSelectedCardSettings() {
    var c = ctx();
    if (c.selected.length !== 1) return;
    renderButtonSettings(true);
  }

  function openCardSettings(slot) {
    var c = ctx();
    if (slot > 0 && c.selected.indexOf(slot) === -1) {
      c.setSelected([slot]);
      c.setLastClicked(slot);
      renderPreview();
    }
    renderButtonSettings(true);
  }

  function renderButtonSettings(forceOpen) {
    var container = els.buttonSettings;
    container.innerHTML = "";
    var c = ctx();

    if (c.selected.length === 0) {
      hideSettingsOverlay();
      return;
    }

    if (c.selected.length > 1) {
      hideSettingsOverlay();
      return;
    }

    if (!forceOpen && !isSettingsOpen()) {
      hideSettingsOverlay();
      return;
    }

    if (els.settingsOverlay) els.settingsOverlay.classList.add("sp-visible");

    var slot = c.selected[0];
    var bIdx = slot - 1;
    if (bIdx < 0 || bIdx >= c.buttons.length) return;
    var liveButton = c.buttons[bIdx];
    var draftKey = (c.isSub ? "sub:" + state.editingSubpage : "main") + ":" + slot;

    function cloneButtonConfig(src) {
      return {
        entity: src.entity || "",
        label: src.label || "",
        icon: src.icon || "Auto",
        icon_on: src.icon_on || "Auto",
        sensor: src.sensor || "",
        unit: src.unit || "",
        type: src.type || "",
        precision: src.precision || "",
        _whenOnActive: src._whenOnActive,
        _whenOnMode: src._whenOnMode,
      };
    }

    function copyButtonConfig(target, src) {
      target.entity = src.entity || "";
      target.label = src.label || "";
      target.icon = src.icon || "Auto";
      target.icon_on = src.icon_on || "Auto";
      target.sensor = src.sensor || "";
      target.unit = src.unit || "";
      target.type = src.type || "";
      target.precision = src.precision || "";
      target._whenOnActive = src._whenOnActive;
      target._whenOnMode = src._whenOnMode;
      normalizeButtonConfig(target);
    }

    if (!state.settingsDraft || state.settingsDraft.key !== draftKey) {
      state.settingsDraft = {
        key: draftKey,
        slot: slot,
        homeSlot: state.editingSubpage,
        isSub: c.isSub,
        dirty: false,
        button: cloneButtonConfig(liveButton),
      };
    }
    var b = state.settingsDraft.button;

    var title = document.createElement("div");
    title.className = "sp-section-title";
    title.textContent = "Settings";
    container.appendChild(title);

    var panel = document.createElement("div");
    panel.className = "sp-panel";

    var idPrefix = c.isSub ? "sp-sp-inp-" : "sp-inp-";
    var requiredFields = [];

    function markDraftDirty() {
      if (state.settingsDraft && state.settingsDraft.key === draftKey) {
        state.settingsDraft.dirty = true;
      }
    }

    function saveField(field, val) {
      markDraftDirty();
    }

    function fieldContainer(input) {
      return input && input.closest ? input.closest(".sp-field") : null;
    }

    function clearFieldError(input) {
      if (!input) return;
      input.classList.remove("sp-input-error");
      input.removeAttribute("aria-invalid");
      input.removeAttribute("aria-describedby");
      var field = fieldContainer(input);
      if (field) field.classList.remove("sp-field-invalid");
      var existing = field ? field.querySelector(".sp-field-error") : null;
      if (existing && existing.parentNode) existing.parentNode.removeChild(existing);
    }

    function showFieldError(input, message) {
      if (!input) return;
      var field = fieldContainer(input);
      input.classList.add("sp-input-error");
      input.setAttribute("aria-invalid", "true");
      if (field) field.classList.add("sp-field-invalid");
      var existing = field ? field.querySelector(".sp-field-error") : null;
      if (!existing && field) {
        existing = document.createElement("div");
        existing.className = "sp-field-error";
        existing.id = (input.id || "sp-field") + "-error";
        field.appendChild(existing);
      }
      if (existing) {
        existing.textContent = message || "Add an entity before saving.";
        input.setAttribute("aria-describedby", existing.id);
      }
    }

    function requireField(input, message, isActive) {
      if (!input) return;
      requiredFields.push({
        input: input,
        message: message || "Add an entity before saving.",
        isActive: isActive || function () { return true; },
      });
      function maybeClearError() {
        if (!isActive || isActive()) {
          if (String(input.value || "").trim()) clearFieldError(input);
        } else {
          clearFieldError(input);
        }
      }
      input.addEventListener("input", maybeClearError);
      input.addEventListener("change", maybeClearError);
    }

    function validateSettingsDraft() {
      var firstInvalid = null;
      for (var i = 0; i < requiredFields.length; i++) {
        var rule = requiredFields[i];
        if (rule.isActive && !rule.isActive()) {
          clearFieldError(rule.input);
          continue;
        }
        if (String(rule.input.value || "").trim()) {
          clearFieldError(rule.input);
          continue;
        }
        if (!firstInvalid) firstInvalid = rule.input;
        showFieldError(rule.input, rule.message);
      }
      if (!firstInvalid) return true;
      firstInvalid.focus();
      return false;
    }

    function applySettingsDraft() {
      if (!state.settingsDraft || state.settingsDraft.key !== draftKey) return;
      copyButtonConfig(liveButton, state.settingsDraft.button);
      state.settingsDraft = null;
      if (c.isSub) {
        saveSubpageConfig(state.editingSubpage);
      } else {
        saveButtonConfig(slot);
      }
      renderPreview();
    }

    function bindField(input, field, rerender) {
      function syncValue() {
        if (b[field] === input.value) return;
        b[field] = input.value;
        markDraftDirty();
      }
      input.addEventListener("input", function () {
        syncValue();
      });
      input.addEventListener("change", syncValue);
      input.addEventListener("blur", syncValue);
      input.addEventListener("keydown", function (e) {
        if (e.key === "Enter") {
          syncValue();
          this.blur();
        }
      });
    }

    function makeIconPicker(pickerId, inputId, currentVal, onSelect, labelText) {
      var icf = document.createElement("div");
      icf.className = "sp-field";
      icf.appendChild(fieldLabel(labelText || "Icon", inputId));
      var picker = document.createElement("div");
      picker.className = "sp-icon-picker";
      if (pickerId) picker.id = pickerId;
      picker.innerHTML =
        '<span class="sp-icon-picker-preview mdi mdi-' + iconSlug(currentVal) + '"></span>' +
        '<input class="sp-icon-picker-input"' + (inputId ? ' id="' + inputId + '"' : '') +
        ' type="text" placeholder="Search icons\u2026" value="' + escAttr(currentVal) + '" autocomplete="off">' +
        '<div class="sp-icon-dropdown"></div>';
      icf.appendChild(picker);
      initIconPicker(picker, currentVal, onSelect);
      return icf;
    }

    var typeDef = BUTTON_TYPES[b.type || ""] || BUTTON_TYPES[""];
    if (typeDef && typeDef.experimental && !isExperimentalEnabled(typeDef.experimental)) {
      typeDef = BUTTON_TYPES[""];
    }
    {
      var typeOpts = [];
      for (var k in BUTTON_TYPES) {
        var td = BUTTON_TYPES[k];
        if (c.isSub && !td.allowInSubpage) continue;
        if (td.isAvailable && !td.isAvailable({ isSub: c.isSub }) && (b.type || "") !== td.key) continue;
        if (td.experimental && !isExperimentalEnabled(td.experimental)) continue;
        typeOpts.push([td.key, td.label]);
      }
      typeOpts.sort(function (a, b) {
        return a[1].localeCompare(b[1]);
      });
      var tf = document.createElement("div");
      tf.className = "sp-field";
      tf.appendChild(fieldLabel("Type", "sp-inp-type"));
      var typeSelect = document.createElement("select");
      typeSelect.className = "sp-select";
      typeSelect.id = "sp-inp-type";
      typeOpts.forEach(function (o) {
        var opt = document.createElement("option");
        opt.value = o[0];
        opt.textContent = o[1];
        if ((b.type || "") === o[0]) opt.selected = true;
        typeSelect.appendChild(opt);
      });
      typeSelect.addEventListener("change", function () {
        var newType = this.value;
        b.type = newType;
        var td = BUTTON_TYPES[newType];
        if (td && td.onSelect) td.onSelect(b);
        saveField("type", newType);
        renderButtonSettings();
      });
      tf.appendChild(typeSelect);
      panel.appendChild(tf);
    }

    var typeHelpers = {
      makeIconPicker: makeIconPicker,
      fieldLabel: fieldLabel,
      textInput: textInput,
      bindField: bindField,
      saveField: saveField,
      requireField: requireField,
      clearFieldError: clearFieldError,
      toggleRow: toggleRow,
      idPrefix: idPrefix,
    };

    if (typeDef && typeDef.renderSettingsBeforeLabel && (!c.isSub || typeDef.allowInSubpage)) {
      typeDef.renderSettingsBeforeLabel(panel, b, slot, typeHelpers);
    }

    if (!typeDef || !typeDef.hideLabel) {
      var lf = document.createElement("div");
      lf.className = "sp-field";
      lf.appendChild(fieldLabel("Label", idPrefix + "label"));
      var labelPlaceholder = (typeDef && typeDef.labelPlaceholder) || "e.g. Kitchen";
      var labelInp = textInput(idPrefix + "label", b.label, labelPlaceholder);
      lf.appendChild(labelInp);
      panel.appendChild(lf);
      bindField(labelInp, "label", true);
    }

    if (typeDef && typeDef.renderSettings && (!c.isSub || typeDef.allowInSubpage)) {
      typeDef.renderSettings(panel, b, slot, typeHelpers);
    } else {
      // Toggle fallback: entity, icons, sensor data
      var ef = document.createElement("div");
      ef.className = "sp-field";
      ef.appendChild(fieldLabel("Entity ID", idPrefix + "entity"));
      var entityInp = textInput(idPrefix + "entity", b.entity, "e.g. light.kitchen");
      ef.appendChild(entityInp);
      panel.appendChild(ef);
      bindField(entityInp, "entity", true);
      requireField(entityInp, "Add an entity before saving.");

      panel.appendChild(makeIconPicker(idPrefix + "icon-picker", idPrefix + "icon", b.icon || "Auto", function (opt) {
        b.icon = opt;
        saveField("icon", opt);
      }, "Off Icon"));

      // When-on section
      var hasIconOn = b.icon_on && b.icon_on !== "Auto";
      var hasSensor = !!b.sensor;
      var whenOnEnabled = hasIconOn || hasSensor || !!b._whenOnActive;
      var whenOnMode = b._whenOnMode || (hasSensor ? "sensor" : "icon");

      var whenOnToggle = toggleRow("Show sensor data when on", idPrefix + "whenon-toggle", whenOnEnabled);
      panel.appendChild(whenOnToggle.row);

      var whenOnCond = condField();
      if (whenOnEnabled) whenOnCond.classList.add("sp-visible");

      var seg = document.createElement("div");
      seg.className = "sp-segment";
      var btnIcon = document.createElement("button");
      btnIcon.type = "button";
      btnIcon.textContent = "On Icon";
      if (whenOnMode === "icon") btnIcon.classList.add("active");
      var btnSensor = document.createElement("button");
      btnSensor.type = "button";
      btnSensor.textContent = "Numeric";
      if (whenOnMode === "sensor") btnSensor.classList.add("active");
      seg.appendChild(btnIcon);
      seg.appendChild(btnSensor);
      whenOnCond.appendChild(seg);

      // Icon-on section
      var iconOnSection = condField();
      if (whenOnMode === "icon") iconOnSection.classList.add("sp-visible");
      var ionLabel = fieldLabel("On Icon", idPrefix + "icon-on");
      iconOnSection.appendChild(ionLabel);
      var iconOnVal = hasIconOn ? b.icon_on : "Auto";
      var iconOnPicker = document.createElement("div");
      iconOnPicker.className = "sp-icon-picker";
      iconOnPicker.id = idPrefix + "icon-on-picker";
      iconOnPicker.innerHTML =
        '<span class="sp-icon-picker-preview mdi mdi-' + iconSlug(iconOnVal) + '"></span>' +
        '<input class="sp-icon-picker-input" id="' + idPrefix + 'icon-on" type="text" ' +
        'placeholder="Search icons\u2026" value="' + escAttr(iconOnVal) + '" autocomplete="off">' +
        '<div class="sp-icon-dropdown"></div>';
      iconOnSection.appendChild(iconOnPicker);
      whenOnCond.appendChild(iconOnSection);

      initIconPicker(iconOnPicker, iconOnVal, function (opt) {
        b.icon_on = opt;
        saveField("icon_on", opt);
      });

      // Sensor section
      var sensorSection = condField();
      if (whenOnMode === "sensor") sensorSection.classList.add("sp-visible");

      var sf = document.createElement("div");
      sf.className = "sp-field";
      sf.appendChild(fieldLabel("Sensor Entity", idPrefix + "sensor"));
      var sensorInp = textInput(idPrefix + "sensor", b.sensor, "e.g. sensor.printer_percent_complete");
      sf.appendChild(sensorInp);
      sensorSection.appendChild(sf);

      var uf = document.createElement("div");
      uf.className = "sp-field";
      uf.appendChild(fieldLabel("Unit", idPrefix + "unit"));
      var unitInp = textInput(idPrefix + "unit", b.unit, "e.g. %");
      unitInp.className = "sp-input";
      uf.appendChild(unitInp);
      sensorSection.appendChild(uf);

      var pf = document.createElement("div");
      pf.className = "sp-field";
      pf.appendChild(fieldLabel("Unit Precision", idPrefix + "precision"));
      var precisionSelect = document.createElement("select");
      precisionSelect.className = "sp-select";
      precisionSelect.id = idPrefix + "precision";
      var precOpts = [["0", "10"], ["1", "10.2"], ["2", "10.21"]];
      for (var pi = 0; pi < precOpts.length; pi++) {
        var opt = document.createElement("option");
        opt.value = precOpts[pi][0];
        opt.textContent = precOpts[pi][1];
        precisionSelect.appendChild(opt);
      }
      precisionSelect.value = b.precision || "0";
      precisionSelect.addEventListener("change", function () {
        b.precision = this.value === "0" ? "" : this.value;
        saveField("precision", b.precision);
      });
      pf.appendChild(precisionSelect);
      sensorSection.appendChild(pf);
      whenOnCond.appendChild(sensorSection);

      panel.appendChild(whenOnCond);

      bindField(sensorInp, "sensor", true);
      bindField(unitInp, "unit", false);

      function setWhenOnMode(mode) {
        whenOnMode = mode;
        b._whenOnActive = true;
        b._whenOnMode = mode;
        btnIcon.classList.toggle("active", mode === "icon");
        btnSensor.classList.toggle("active", mode === "sensor");
        iconOnSection.classList.toggle("sp-visible", mode === "icon");
        sensorSection.classList.toggle("sp-visible", mode === "sensor");
        if (mode === "icon") {
          sensorInp.value = "";
          unitInp.value = "";
          precisionSelect.value = "0";
          b.sensor = "";
          b.unit = "";
          b.precision = "";
          saveField("sensor", "");
          saveField("unit", "");
          saveField("precision", "");
        } else {
          b.icon_on = "Auto";
          saveField("icon_on", "Auto");
          var ionPreview = iconOnPicker.querySelector(".sp-icon-picker-preview");
          if (ionPreview) ionPreview.className = "sp-icon-picker-preview mdi mdi-cog";
          var ionInput = iconOnPicker.querySelector(".sp-icon-picker-input");
          if (ionInput) ionInput.value = "Auto";
        }
      }

      btnIcon.addEventListener("click", function () { setWhenOnMode("icon"); });
      btnSensor.addEventListener("click", function () { setWhenOnMode("sensor"); });

      whenOnToggle.input.addEventListener("change", function () {
        if (this.checked) {
          b._whenOnActive = true;
          whenOnCond.classList.add("sp-visible");
        } else {
          b._whenOnActive = false;
          b._whenOnMode = null;
          whenOnCond.classList.remove("sp-visible");
          sensorInp.value = "";
          unitInp.value = "";
          precisionSelect.value = "0";
          b.sensor = "";
          b.unit = "";
          b.precision = "";
          b.icon_on = "Auto";
          saveField("sensor", "");
          saveField("unit", "");
          saveField("precision", "");
          saveField("icon_on", "Auto");
        }
      });
    }

    var saveRow = document.createElement("div");
    saveRow.className = "sp-btn-row sp-btn-row--save";

    var delBtn = document.createElement("button");
    delBtn.className = "sp-action-btn sp-delete-btn";
    delBtn.innerHTML = '<span class="mdi mdi-trash-can-outline"></span>';
    delBtn.addEventListener("click", function () {
      state.settingsDraft = null;
      deleteSlot(slot);
    });
    saveRow.appendChild(delBtn);
    saveRow.classList.add("sp-has-delete");

    var rightGroup = document.createElement("div");
    rightGroup.className = "sp-btn-group-right";
    var editSubBtn = panel.querySelector(".sp-edit-subpage-btn");
    if (editSubBtn) rightGroup.appendChild(editSubBtn);
    var saveBtn = document.createElement("button");
    saveBtn.className = "sp-action-btn sp-save-btn";
    saveBtn.textContent = "Save";
    saveBtn.addEventListener("click", function () {
      if (!validateSettingsDraft()) return;
      applySettingsDraft();
      closeSettings();
    });
    rightGroup.appendChild(saveBtn);
    saveRow.appendChild(rightGroup);
    panel.appendChild(saveRow);

    container.appendChild(panel);
  }

  // ── Render debouncing ──────────────────────────────────────────────────

  var _renderPending = false;
  function scheduleRender() {
    if (_renderPending) return;
    _renderPending = true;
    requestAnimationFrame(function () {
      _renderPending = false;
      renderPreview();
      if (isSettingsOpen() || isSettingsFocused()) {
        _settingsDeferred = true;
      } else {
        renderButtonSettings();
      }
    });
  }

  var _settingsDeferred = false;
  document.addEventListener("focusout", function (e) {
    if (!_settingsDeferred) return;
    if (e.relatedTarget && els.buttonSettings && els.buttonSettings.contains(e.relatedTarget)) return;
    requestAnimationFrame(function () {
      if (isSettingsOpen()) return;
      if (!isSettingsFocused()) {
        _settingsDeferred = false;
        renderButtonSettings();
      }
    });
  });

  document.addEventListener("keydown", function (e) {
    if (e.key === "Escape" && els.settingsOverlay &&
        els.settingsOverlay.classList.contains("sp-visible")) {
      closeSettings();
    }
  });

  // ── Icon picker (optimized) ────────────────────────────────────────────

  function initIconPicker(picker, currentIcon, onSelect) {
    var input = picker.querySelector(".sp-icon-picker-input");
    var dropdown = picker.querySelector(".sp-icon-dropdown");
    var preview = picker.querySelector(".sp-icon-picker-preview");
    var highlighted = -1;
    var optionEls = null;
    var emptyEl = null;

    function ensureBuilt() {
      if (optionEls) return;
      optionEls = [];
      var frag = document.createDocumentFragment();
      ICON_OPTIONS.forEach(function (opt) {
        var row = document.createElement("div");
        row.className = "sp-icon-option" + (opt === currentIcon ? " sp-active" : "");
        row.innerHTML =
          '<span class="sp-icon-option-icon mdi mdi-' + iconSlug(opt) + '"></span>' +
          '<span class="sp-icon-option-label">' + escHtml(opt) + '</span>';
        row._lcName = opt.toLowerCase();
        row._optName = opt;
        row.addEventListener("mousedown", function (e) {
          e.preventDefault();
          selectOpt(opt);
        });
        frag.appendChild(row);
        optionEls.push(row);
      });
      emptyEl = document.createElement("div");
      emptyEl.className = "sp-icon-option sp-icon-option--empty";
      emptyEl.textContent = "No matches";
      emptyEl.style.display = "none";
      frag.appendChild(emptyEl);
      dropdown.appendChild(frag);
    }

    function filterOpts(filter) {
      ensureBuilt();
      highlighted = -1;
      var lc = (filter || "").toLowerCase();
      var hasMatch = false;
      for (var i = 0; i < optionEls.length; i++) {
        var match = !lc || optionEls[i]._lcName.indexOf(lc) !== -1;
        optionEls[i].style.display = match ? "" : "none";
        optionEls[i].classList.remove("sp-highlighted");
        if (match) hasMatch = true;
      }
      emptyEl.style.display = hasMatch ? "none" : "";
    }

    function setPickerIcon(opt) {
      currentIcon = opt;
      input.value = opt;
      preview.className = "sp-icon-picker-preview mdi mdi-" + iconSlug(opt);
      if (optionEls) {
        for (var i = 0; i < optionEls.length; i++) {
          optionEls[i].classList.toggle("sp-active", optionEls[i]._optName === opt);
        }
      }
    }

    function selectOpt(opt) {
      setPickerIcon(opt);
      closePicker();
      onSelect(opt);
    }
    picker._setIcon = setPickerIcon;

    function openPicker() {
      if (currentIcon === "Auto") {
        input.value = "";
        filterOpts("");
      } else {
        filterOpts(currentIcon);
        setTimeout(function () { input.select(); }, 0);
      }
      picker.classList.add("sp-open");
    }

    function closePicker() {
      picker.classList.remove("sp-open");
      input.value = currentIcon;
      highlighted = -1;
    }

    function getVisible() {
      var vis = [];
      if (optionEls) {
        for (var i = 0; i < optionEls.length; i++) {
          if (optionEls[i].style.display !== "none") vis.push(optionEls[i]);
        }
      }
      return vis;
    }

    function highlightAt(idx) {
      var visible = getVisible();
      if (visible.length === 0) return;
      if (optionEls) optionEls.forEach(function (el) { el.classList.remove("sp-highlighted"); });
      if (idx < 0) idx = visible.length - 1;
      if (idx >= visible.length) idx = 0;
      highlighted = idx;
      visible[highlighted].classList.add("sp-highlighted");
      visible[highlighted].scrollIntoView({ block: "nearest" });
    }

    input.addEventListener("focus", openPicker);
    input.addEventListener("blur", closePicker);

    input.addEventListener("input", function () {
      filterOpts(this.value);
      var vis = getVisible();
      if (vis.length > 0) highlightAt(0);
    });

    input.addEventListener("keydown", function (e) {
      if (e.key === "ArrowDown") {
        e.preventDefault();
        if (!picker.classList.contains("sp-open")) { openPicker(); return; }
        highlightAt(highlighted + 1);
      } else if (e.key === "ArrowUp") {
        e.preventDefault();
        highlightAt(highlighted - 1);
      } else if (e.key === "Enter") {
        e.preventDefault();
        var visible = getVisible();
        if (highlighted >= 0 && highlighted < visible.length) {
          selectOpt(visible[highlighted]._optName);
        }
      } else if (e.key === "Tab") {
        var visible = getVisible();
        if (picker.classList.contains("sp-open") && highlighted >= 0 && highlighted < visible.length) {
          selectOpt(visible[highlighted]._optName);
        }
      } else if (e.key === "Escape") {
        e.preventDefault();
        closePicker();
        input.blur();
      }
    });
  }

  // ── Preview event delegation & drag ────────────────────────────────────

  function resolveSpanPos(pos) {
    var c = ctx();
    if (c.grid[pos] === -1) {
      var above = pos - GRID_COLS;
      if (above >= 0 && (c.grid[above] > 0 || c.grid[above] === -2) && (c.sizes[c.grid[above]] === 2 || c.sizes[c.grid[above]] === 4)) return above;
      var left = pos - 1;
      if (pos % GRID_COLS !== 0 && left >= 0 && (c.grid[left] > 0 || c.grid[left] === -2) && (c.sizes[c.grid[left]] === 3 || c.sizes[c.grid[left]] === 4)) return left;
      var diag = pos - GRID_COLS - 1;
      if (pos % GRID_COLS !== 0 && diag >= 0 && (c.grid[diag] > 0 || c.grid[diag] === -2) && c.sizes[c.grid[diag]] === 4) return diag;
    }
    return pos;
  }

  function getCellFromEvent(e, container) {
    if (CFG.dragMode === "swap") {
      var rect = container.getBoundingClientRect();
      var col = Math.floor((e.clientX - rect.left) / (rect.width / GRID_COLS));
      var row = Math.floor((e.clientY - rect.top) / (rect.height / GRID_ROWS));
      col = Math.max(0, Math.min(col, GRID_COLS - 1));
      row = Math.max(0, Math.min(row, GRID_ROWS - 1));
      return resolveSpanPos(row * GRID_COLS + col);
    }
    var x = e.clientX, y = e.clientY;
    var children = container.children;
    var skip = 0;
    var bestDist = Infinity, bestPos = -1;
    for (var i = skip; i < children.length; i++) {
      var r = children[i].getBoundingClientRect();
      var pos = parseInt(children[i].getAttribute("data-pos"), 10);
      if (isNaN(pos)) continue;
      if (x >= r.left && x <= r.right && y >= r.top && y <= r.bottom) return pos;
      var cx = (r.left + r.right) / 2, cy = (r.top + r.bottom) / 2;
      var d = (x - cx) * (x - cx) + (y - cy) * (y - cy);
      if (d < bestDist) { bestDist = d; bestPos = pos; }
    }
    return bestPos;
  }

  function moveToCell(fromPos, toPos) {
    var c = ctx();
    toPos = resolveSpanPos(toPos);
    if (toPos >= c.maxSlots || c.grid[toPos] === -1) return;
    var grid = c.grid.slice();
    var movingSlot = grid[fromPos];
    clearSpans(grid, c.maxSlots);
    var targetSlot = grid[toPos];
    grid[toPos] = movingSlot;
    grid[fromPos] = targetSlot;
    applySpans(grid, c.sizes, c.maxSlots);
    if (c.sizes[movingSlot] === 4 && (toPos + GRID_COLS >= c.maxSlots || toPos + 1 >= c.maxSlots || (toPos + 1) % GRID_COLS === 0 || toPos + GRID_COLS + 1 >= c.maxSlots)) {
      delete c.sizes[movingSlot];
    }
    if (c.sizes[movingSlot] === 2 && toPos + GRID_COLS >= c.maxSlots) {
      delete c.sizes[movingSlot];
    }
    if (c.sizes[movingSlot] === 3 && (toPos + 1 >= c.maxSlots || (toPos + 1) % GRID_COLS === 0)) {
      delete c.sizes[movingSlot];
    }
    if (c.isSub) {
      getSubpage(state.editingSubpage).grid = grid;
    } else {
      state.grid = grid;
    }
  }


  function canPlaceSlotAt(grid, pos, size, maxSlots) {
    if (pos < 0 || pos >= maxSlots || grid[pos] !== 0) return false;
    if (size === 2 || size === 4) {
      var below = pos + GRID_COLS;
      if (below >= maxSlots || grid[below] !== 0) return false;
    }
    if (size === 3 || size === 4) {
      var right = pos + 1;
      if (right >= maxSlots || right % GRID_COLS === 0 || grid[right] !== 0) return false;
    }
    if (size === 4) {
      var diag = pos + GRID_COLS + 1;
      if (diag >= maxSlots || grid[diag] !== 0) return false;
    }
    return true;
  }

  function findPlacementCell(grid, start, size, maxSlots) {
    for (var i = 0; i < maxSlots; i++) {
      var candidate = (start + i) % maxSlots;
      if (canPlaceSlotAt(grid, candidate, size, maxSlots)) return candidate;
    }
    return -1;
  }

  function placeSlotAt(grid, slot, pos, size) {
    grid[pos] = slot;
    if (size === 2 || size === 4) grid[pos + GRID_COLS] = -1;
    if (size === 3 || size === 4) grid[pos + 1] = -1;
    if (size === 4) grid[pos + GRID_COLS + 1] = -1;
  }

  function placeOrderedGridEntries(entries, sizes, maxSlots) {
    var grid = [];
    for (var i = 0; i < maxSlots; i++) grid.push(0);

    for (var j = 0; j < entries.length && j < maxSlots; j++) {
      var slot = entries[j];
      if (!(slot > 0 || slot === -2)) continue;

      var targetSize = sizes[slot] || 1;
      var place = j;
      if (!canPlaceSlotAt(grid, place, targetSize, maxSlots)) {
        place = findPlacementCell(grid, place, targetSize, maxSlots);
      }
      if (place < 0 && targetSize !== 1) {
        targetSize = 1;
        place = canPlaceSlotAt(grid, j, targetSize, maxSlots)
          ? j
          : findPlacementCell(grid, j, targetSize, maxSlots);
      }
      if (place < 0) continue;

      if (targetSize === 1) delete sizes[slot]; else sizes[slot] = targetSize;
      placeSlotAt(grid, slot, place, targetSize);
    }

    return grid;
  }

  function moveSelectedToCell(fromPos, toPos) {
    var c = ctx();
    toPos = resolveSpanPos(toPos);
    if (toPos < 0 || toPos >= c.maxSlots) return false;

    var sourceEntries = c.grid.slice();
    clearSpans(sourceEntries, c.maxSlots);

    var movingSlot = sourceEntries[fromPos];
    if (c.selected.length <= 1 || c.selected.indexOf(movingSlot) === -1) return false;

    var movingSlots = c.selected.slice();
    var targetSlot = sourceEntries[toPos];
    if (targetSlot > 0 && c.selected.indexOf(targetSlot) !== -1) return true;

    var entries = [];
    for (var i = 0; i < c.maxSlots; i++) {
      var entry = sourceEntries[i];
      if (entry > 0 && c.selected.indexOf(entry) !== -1) continue;
      entries.push(entry);
    }
    while (entries.length < c.maxSlots) entries.push(0);

    var insertPos;
    if (targetSlot > 0 || targetSlot === -2) {
      insertPos = entries.indexOf(targetSlot);
      insertPos = insertPos < 0 ? toPos : insertPos + 1;
    } else {
      insertPos = toPos;
      for (var r = 0; r < toPos; r++) {
        if (sourceEntries[r] > 0 && c.selected.indexOf(sourceEntries[r]) !== -1) insertPos--;
      }
    }
    insertPos = Math.max(0, Math.min(insertPos, entries.length));
    entries.splice.apply(entries, [insertPos, 0].concat(movingSlots));
    entries = entries.slice(0, c.maxSlots);

    var grid = placeOrderedGridEntries(entries, c.sizes, c.maxSlots);

    if (c.isSub) {
      getSubpage(state.editingSubpage).grid = grid;
    } else {
      state.grid = grid;
    }
    return true;
  }

  function clearPlaceholder() {
    if (previewPlaceholder) {
      previewPlaceholder.classList.remove("sp-drop-placeholder");
      previewPlaceholder = null;
    }
  }

  function clearTextSelection() {
    var selection = window.getSelection && window.getSelection();
    if (selection && selection.removeAllRanges) selection.removeAllRanges();
  }

  function setupPreviewEvents() {
    var container = els.previewMain;
    var pendingCellIdx = -1;

    container.addEventListener("mousedown", function (e) {
      if (!e.target.closest("[data-pos]")) return;
      if (e.shiftKey || e.ctrlKey || e.metaKey) e.preventDefault();
    });

    // Click delegation
    container.addEventListener("click", function (e) {
      if (e.target.closest(".sp-subpage-badge")) {
        var btnEl = e.target.closest("[data-slot]");
        if (btnEl) {
          enterSubpage(parseInt(btnEl.getAttribute("data-slot"), 10));
          return;
        }
      }
      var target = e.target.closest("[data-pos]");
      if (!target) return;
      var pos = parseInt(target.getAttribute("data-pos"), 10);
      var c = ctx();
      var slot = c.grid[pos];
      if (slot > 0) {
        handleBtnClick(e, slot, pos);
      } else if (slot === -2) {
        if (didDrag) { didDrag = false; return; }
        exitSubpage();
      } else if (slot === 0) {
        if (state.clipboard) {
          e.preventDefault();
          e.stopPropagation();
          showEmptySlotMenu(e, pos);
        } else {
          addSlot(pos);
        }
      }
    });

    // Context menu delegation
    container.addEventListener("contextmenu", function (e) {
      var target = e.target.closest("[data-pos]");
      if (!target) return;
      e.preventDefault();
      var pos = parseInt(target.getAttribute("data-pos"), 10);
      var c = ctx();
      var slot = c.grid[pos];
      if (slot > 0) {
        showContextMenu(e, slot);
      } else if (slot === -2) {
        showBackContextMenu(e);
      } else if (slot === 0) {
        showEmptySlotMenu(e, pos);
      }
    });

    // Drag delegation
    container.addEventListener("dragstart", function (e) {
      var target = e.target.closest(".sp-btn") || e.target.closest(".sp-back-btn");
      if (!target) return;
      var pos = parseInt(target.getAttribute("data-pos"), 10);
      dragSrcPos = pos;
      if (CFG.dragAnimation) dragSrcEl = target;
      dragIsSubpage = !!state.editingSubpage;
      didDrag = true;
      dragEnterCount = 0;
      container.classList.add("sp-drag-active");
      e.dataTransfer.effectAllowed = "move";
      e.dataTransfer.setData("text/plain", String(pos));
      if (CFG.dragAnimation) {
        requestAnimationFrame(function () { target.classList.add("sp-dragging"); });
      }
    });

    container.addEventListener("dragend", function () {
      dragSrcPos = -1;
      previewDropIdx = -1;
      dragIsSubpage = false;
      dragEnterCount = 0;
      clearPlaceholder();
      if (dragSrcEl) { dragSrcEl.classList.remove("sp-dragging"); dragSrcEl = null; }
      setTimeout(function () { container.classList.remove("sp-drag-active"); }, 50);
    });

    // Drop zone
    function updatePlaceholder(cellIdx) {
      if (cellIdx === previewDropIdx) return;
      previewDropIdx = cellIdx;
      clearPlaceholder();
      var target = container.querySelector('[data-pos="' + cellIdx + '"]');
      if (target) {
        previewPlaceholder = target;
        previewPlaceholder.classList.add("sp-drop-placeholder");
      }
    }

    container.addEventListener("dragenter", function (e) {
      if (dragSrcPos < 0) return;
      e.preventDefault();
      dragEnterCount++;
    });

    container.addEventListener("dragover", function (e) {
      if (dragSrcPos < 0) return;
      e.preventDefault();
      e.dataTransfer.dropEffect = "move";
      if (CFG.dragAnimation) {
        pendingCellIdx = getCellFromEvent(e, container);
        if (dragRafPending) return;
        dragRafPending = true;
        requestAnimationFrame(function () {
          dragRafPending = false;
          if (dragSrcPos < 0) return;
          updatePlaceholder(pendingCellIdx);
        });
      } else {
        updatePlaceholder(getCellFromEvent(e, container));
      }
    });

    container.addEventListener("dragleave", function () {
      dragEnterCount--;
      if (dragEnterCount <= 0) {
        dragEnterCount = 0;
        previewDropIdx = -1;
        clearPlaceholder();
      }
    });

    container.addEventListener("drop", function (e) {
      e.preventDefault();
      dragEnterCount = 0;
      var toPos = previewDropIdx;
      previewDropIdx = -1;
      clearPlaceholder();
      if (dragSrcEl) { dragSrcEl.classList.remove("sp-dragging"); dragSrcEl = null; }
      var c = ctx();
      if (dragSrcPos < 0 || toPos < 0 || toPos >= c.maxSlots) { dragSrcPos = -1; dragIsSubpage = false; return; }
      if (dragSrcPos === toPos) { dragSrcPos = -1; dragIsSubpage = false; return; }
      if (!moveSelectedToCell(dragSrcPos, toPos)) moveToCell(dragSrcPos, toPos);
      renderPreview();
      renderButtonSettings();
      c.save();
      dragSrcPos = -1;
      dragIsSubpage = false;
    });
  }

  function handleBtnClick(e, slot, pos) {
    if (didDrag) { didDrag = false; return; }
    var c = ctx();
    if (e.shiftKey || e.ctrlKey || e.metaKey) e.preventDefault();

    if (e.shiftKey && c.getLastClicked() > 0) {
      var anchorPos = c.grid.indexOf(c.getLastClicked());
      if (anchorPos !== -1) {
        var from = Math.min(anchorPos, pos);
        var to = Math.max(anchorPos, pos);
        var newSel = [];
        for (var i = from; i <= to; i++) {
          if (c.grid[i] > 0) newSel.push(c.grid[i]);
        }
        c.setSelected(newSel);
        renderPreview();
        hideSettingsOverlay();
        clearTextSelection();
        return;
      }
    }

    if (e.ctrlKey || e.metaKey) {
      var idx = c.selected.indexOf(slot);
      if (idx !== -1) {
        c.selected.splice(idx, 1);
      } else {
        c.selected.push(slot);
        c.setLastClicked(slot);
      }
      renderPreview();
      hideSettingsOverlay();
      clearTextSelection();
      return;
    }

    if (c.selected.length === 1 && c.selected[0] === slot) {
      c.setSelected([]);
      c.setLastClicked(-1);
    } else {
      c.setSelected([slot]);
      c.setLastClicked(slot);
    }
    renderPreview();
    renderButtonSettings();
  }

  function selectButton(slot) {
    if (slot < 1) {
      state.selectedSlots = [];
    } else {
      state.selectedSlots = [slot];
      state.lastClickedSlot = slot;
    }
    renderPreview();
    renderButtonSettings();
  }

  // ── Button management (unified) ────────────────────────────────────────

  function firstFreeSlot() {
    var used = {};
    state.grid.forEach(function (s) { if (s > 0) used[s] = true; });
    for (var i = 1; i <= NUM_SLOTS; i++) {
      if (!used[i]) return i;
    }
    return -1;
  }

  function firstFreeCell(afterPos) {
    var start = afterPos != null ? afterPos : 0;
    for (var i = 0; i < NUM_SLOTS; i++) {
      var candidate = (start + i) % NUM_SLOTS;
      if (state.grid[candidate] === 0) return candidate;
    }
    return -1;
  }

  function emptyButtonConfig(type) {
    return { entity: "", label: "", icon: "Auto", icon_on: "Auto", sensor: "", unit: "", type: type || "", precision: "" };
  }

  function addSlot(pos) {
    var c = ctx();
    if (c.isSub) {
      var sp = getSubpage(state.editingSubpage);
      var newSlot = subpageFirstFreeSlot(sp);
      while (sp.buttons.length < newSlot) {
        sp.buttons.push(emptyButtonConfig());
      }
      sp.grid[pos] = newSlot;
      sp.order = serializeSubpageGrid(sp);
      saveSubpageConfig(state.editingSubpage);
      state.subpageSelectedSlots = [newSlot];
      state.subpageLastClicked = newSlot;
      renderPreview();
      renderButtonSettings(true);
    } else {
      var slot = firstFreeSlot();
      if (slot < 0) return;
      state.grid[pos] = slot;
      postText("Button Order", serializeGrid(state.grid));
      state.selectedSlots = [slot];
      state.lastClickedSlot = slot;
      renderPreview();
      renderButtonSettings(true);
    }
  }

  function addSubpageSlot(pos) {
    var c = ctx();
    if (c.isSub) return;
    var slot = firstFreeSlot();
    if (slot < 0) return;
    state.buttons[slot - 1] = emptyButtonConfig("subpage");
    state.grid[pos] = slot;
    state.subpages[slot] = { order: [], buttons: [], grid: [], sizes: {} };
    buildSubpageGrid(state.subpages[slot]);
    postText("Button Order", serializeGrid(state.grid));
    saveButtonConfig(slot);
    saveSubpageEntity(slot);
    selectButton(slot);
  }

  function duplicateButton(srcSlot) {
    var newSlot = firstFreeSlot();
    if (newSlot < 0) return;

    var src = state.buttons[srcSlot - 1];
    state.buttons[newSlot - 1] = {
      entity: src.entity, label: src.label, icon: src.icon,
      icon_on: src.icon_on, sensor: src.sensor, unit: src.unit,
      type: src.type || "", precision: src.precision || "",
    };

    var srcSz = state.sizes[srcSlot];
    if (srcSz) state.sizes[newSlot] = srcSz;

    var srcPos = state.grid.indexOf(srcSlot);
    var newPos = firstFreeCell(srcPos + 1);
    if (newPos < 0) return;
    state.grid[newPos] = newSlot;
    if (state.sizes[newSlot] === 2 || state.sizes[newSlot] === 4) {
      var belowNew = newPos + GRID_COLS;
      if (belowNew < NUM_SLOTS && state.grid[belowNew] === 0) state.grid[belowNew] = -1;
    }
    if (state.sizes[newSlot] === 3 || state.sizes[newSlot] === 4) {
      var rightNew = newPos + 1;
      if (rightNew < NUM_SLOTS && rightNew % GRID_COLS !== 0 && state.grid[rightNew] === 0) state.grid[rightNew] = -1;
    }
    if (state.sizes[newSlot] === 4) {
      var diagNew = newPos + GRID_COLS + 1;
      if (diagNew < NUM_SLOTS && (newPos + 1) % GRID_COLS !== 0 && state.grid[diagNew] === 0) state.grid[diagNew] = -1;
    }

    if (src.type === "subpage" && state.subpages[srcSlot]) {
      var spJson = serializeSubpageConfig(state.subpages[srcSlot]);
      var spCopy = parseSubpageConfig(spJson);
      spCopy.sizes = {};
      buildSubpageGrid(spCopy);
      state.subpages[newSlot] = spCopy;
    }
    postText("Button Order", serializeGrid(state.grid));
    saveButtonConfig(newSlot);
    saveSubpageEntity(newSlot);
    state.selectedSlots = [newSlot];
    state.lastClickedSlot = newSlot;
    renderPreview();
  }

  function duplicateSubpageButton(srcSlot) {
    var homeSlot = state.editingSubpage;
    var sp = getSubpage(homeSlot);
    var newSlot = subpageFirstFreeSlot(sp);
    while (sp.buttons.length < newSlot) {
      sp.buttons.push(emptyButtonConfig());
    }

    var src = sp.buttons[srcSlot - 1];
    sp.buttons[newSlot - 1] = {
      entity: src.entity, label: src.label, icon: src.icon,
      icon_on: src.icon_on, sensor: src.sensor, unit: src.unit,
      type: src.type || "", precision: src.precision || "",
    };

    var srcSz = sp.sizes[srcSlot];
    if (srcSz) sp.sizes[newSlot] = srcSz;

    var srcPos = sp.grid.indexOf(srcSlot);
    var newPos = -1;
    for (var c = srcPos + 1; c < NUM_SLOTS; c++) {
      if (sp.grid[c] === 0) { newPos = c; break; }
    }
    if (newPos < 0) return;
    sp.grid[newPos] = newSlot;
    if (sp.sizes[newSlot] === 2 || sp.sizes[newSlot] === 4) {
      var below = newPos + GRID_COLS;
      if (below < NUM_SLOTS && sp.grid[below] === 0) sp.grid[below] = -1;
    }
    if (sp.sizes[newSlot] === 3 || sp.sizes[newSlot] === 4) {
      var right = newPos + 1;
      if (right < NUM_SLOTS && right % GRID_COLS !== 0 && sp.grid[right] === 0) sp.grid[right] = -1;
    }
    if (sp.sizes[newSlot] === 4) {
      var diag = newPos + GRID_COLS + 1;
      if (diag < NUM_SLOTS && (newPos + 1) % GRID_COLS !== 0 && sp.grid[diag] === 0) sp.grid[diag] = -1;
    }

    sp.order = serializeSubpageGrid(sp);
    saveSubpageConfig(homeSlot);
    state.subpageSelectedSlots = [newSlot];
    state.subpageLastClicked = newSlot;
    renderPreview();
  }

  function deleteSlot(slot) {
    var c = ctx();
    for (var i = 0; i < c.maxSlots; i++) {
      if (c.grid[i] === slot) {
        c.grid[i] = 0;
        if ((c.sizes[slot] === 2 || c.sizes[slot] === 4) && i + GRID_COLS < c.maxSlots && c.grid[i + GRID_COLS] === -1) {
          c.grid[i + GRID_COLS] = 0;
        }
        if ((c.sizes[slot] === 3 || c.sizes[slot] === 4) && i + 1 < c.maxSlots && c.grid[i + 1] === -1) {
          c.grid[i + 1] = 0;
        }
        if (c.sizes[slot] === 4 && i + GRID_COLS + 1 < c.maxSlots && c.grid[i + GRID_COLS + 1] === -1) {
          c.grid[i + GRID_COLS + 1] = 0;
        }
        break;
      }
    }
    delete c.sizes[slot];

    var selIdx = c.selected.indexOf(slot);
    if (selIdx !== -1) c.selected.splice(selIdx, 1);

    if (c.isSub) {
      var sp = getSubpage(state.editingSubpage);
      if (slot >= 1 && slot <= sp.buttons.length) {
        sp.buttons[slot - 1] = { entity: "", label: "", icon: "Auto", icon_on: "Auto", sensor: "", unit: "", type: "", precision: "" };
      }
      sp.order = serializeSubpageGrid(sp);
      state.subpageLastClicked = -1;
      saveSubpageConfig(state.editingSubpage);
    } else {
      postText("Button Order", serializeGrid(state.grid));
      state.buttons[slot - 1] = { entity: "", label: "", icon: "Auto", icon_on: "Auto", sensor: "", unit: "", type: "", precision: "" };
      delete state.subpages[slot];
      saveButtonConfig(slot);
      saveSubpageEntity(slot);
    }

    renderPreview();
    renderButtonSettings();
  }

  function deleteButtons(slots) {
    var c = ctx();
    for (var i = 0; i < c.maxSlots; i++) {
      if (slots.indexOf(c.grid[i]) !== -1) {
        if ((c.sizes[c.grid[i]] === 2 || c.sizes[c.grid[i]] === 4) && i + GRID_COLS < c.maxSlots && c.grid[i + GRID_COLS] === -1) {
          c.grid[i + GRID_COLS] = 0;
        }
        if ((c.sizes[c.grid[i]] === 3 || c.sizes[c.grid[i]] === 4) && i + 1 < c.maxSlots && c.grid[i + 1] === -1) {
          c.grid[i + 1] = 0;
        }
        if (c.sizes[c.grid[i]] === 4 && i + GRID_COLS + 1 < c.maxSlots && c.grid[i + GRID_COLS + 1] === -1) {
          c.grid[i + GRID_COLS + 1] = 0;
        }
        c.grid[i] = 0;
      }
    }
    slots.forEach(function (slot) { delete c.sizes[slot]; });
    c.setSelected([]);
    c.setLastClicked(-1);
    if (c.isSub) {
      var sp = getSubpage(state.editingSubpage);
      slots.forEach(function (slot) {
        if (slot >= 1 && slot <= sp.buttons.length) {
          sp.buttons[slot - 1] = { entity: "", label: "", icon: "Auto", icon_on: "Auto", sensor: "", unit: "", type: "", precision: "" };
        }
      });
      sp.order = serializeSubpageGrid(sp);
      saveSubpageConfig(state.editingSubpage);
    } else {
      slots.forEach(function (slot) {
        state.buttons[slot - 1] = { entity: "", label: "", icon: "Auto", icon_on: "Auto", sensor: "", unit: "", type: "", precision: "" };
        delete state.subpages[slot];
        saveButtonConfig(slot);
        saveSubpageEntity(slot);
      });
      postText("Button Order", serializeGrid(state.grid));
    }
    renderPreview();
    renderButtonSettings();
  }

  // ── Context menu (unified) ─────────────────────────────────────────────

  var ctxMenu = null;

  function positionMenu(menu, e) {
    var w = menu.offsetWidth, h = menu.offsetHeight;
    var x = Math.max(4, Math.min(e.clientX, window.innerWidth - w - 4));
    var y = Math.max(4, Math.min(e.clientY, window.innerHeight - h - 4));
    menu.style.left = x + "px";
    menu.style.top = y + "px";
  }

  function addCtxItem(icon, text, handler, danger) {
    var item = document.createElement("div");
    item.className = "sp-ctx-item" + (danger ? " sp-ctx-danger" : "");
    item.innerHTML = '<span class="mdi mdi-' + icon + '"></span>' + escHtml(text);
    item.addEventListener("mousedown", function (ev) {
      ev.preventDefault();
      ev.stopPropagation();
      hideContextMenu();
      handler();
    });
    ctxMenu.appendChild(item);
  }

  function addCtxDivider() {
    var div = document.createElement("div");
    div.className = "sp-ctx-divider";
    ctxMenu.appendChild(div);
  }

  function addCtxSubmenu(icon, text, buildFn) {
    var wrapper = document.createElement("div");
    wrapper.className = "sp-ctx-item sp-ctx-sub";
    wrapper.innerHTML = '<span class="mdi mdi-' + icon + '"></span>' + escHtml(text);
    var sub = document.createElement("div");
    sub.className = "sp-ctx-submenu";
    buildFn(sub);
    wrapper.appendChild(sub);
    wrapper.addEventListener("mouseenter", function () {
      sub.style.left = "100%"; sub.style.right = "auto";
      var r = sub.getBoundingClientRect();
      if (r.right > window.innerWidth - 4) { sub.style.left = "auto"; sub.style.right = "100%"; }
    });
    wrapper.addEventListener("mousedown", function (ev) { ev.preventDefault(); ev.stopPropagation(); });
    ctxMenu.appendChild(wrapper);
  }

  function addSubItem(container, icon, text, handler, active) {
    var item = document.createElement("div");
    item.className = "sp-ctx-item";
    item.innerHTML = (active ? '<span class="sp-ctx-check mdi mdi-check"></span>' : '<span style="width:18px"></span>') + escHtml(text);
    item.addEventListener("mousedown", function (ev) {
      ev.preventDefault();
      ev.stopPropagation();
      hideContextMenu();
      handler();
    });
    container.appendChild(item);
  }

  function resizeSlot(slot, targetSz) {
    var c = ctx();
    var slotPos = slot === -2 ? c.grid.indexOf(-2) : c.grid.indexOf(slot);
    if (slotPos < 0) return;
    var curSz = c.sizes[slot] || 1;
    if (curSz === targetSz) return;

    var curD = curSz === 2 || curSz === 4;
    var curW = curSz === 3 || curSz === 4;
    if (curD) { var bp = slotPos + GRID_COLS; if (bp < c.maxSlots && c.grid[bp] === -1) c.grid[bp] = 0; }
    if (curW) { var rp = slotPos + 1; if (rp < c.maxSlots && c.grid[rp] === -1) c.grid[rp] = 0; }
    if (curSz === 4) { var dp = slotPos + GRID_COLS + 1; if (dp < c.maxSlots && c.grid[dp] === -1) c.grid[dp] = 0; }

    var wantD = targetSz === 2 || targetSz === 4;
    var wantW = targetSz === 3 || targetSz === 4;
    var need = [];
    if (wantD) need.push(slotPos + GRID_COLS);
    if (wantW) need.push(slotPos + 1);
    if (wantD && wantW) need.push(slotPos + GRID_COLS + 1);

    for (var i = 0; i < need.length; i++) {
      var p = need[i];
      if (p >= c.maxSlots) { if (targetSz > 1) delete c.sizes[slot]; return; }
      if (wantW && (slotPos + 1) % GRID_COLS === 0) { if (targetSz > 1) delete c.sizes[slot]; return; }
      if (c.grid[p] > 0 || c.grid[p] === -2) {
        if (c.isSub && c.grid[p] > 0) return;
        var displaced = c.grid[p];
        c.grid[p] = 0;
        if (c.isSub) {
          for (var j = 0; j < c.maxSlots; j++) { if (c.grid[j] === 0 && need.indexOf(j) === -1) { c.grid[j] = displaced; break; } }
        } else {
          var fc = firstFreeCell(p + 1);
          if (fc >= 0) c.grid[fc] = displaced;
        }
      }
    }
    for (var i = 0; i < need.length; i++) c.grid[need[i]] = -1;

    if (targetSz === 1) delete c.sizes[slot]; else c.sizes[slot] = targetSz;

    if (c.isSub) {
      var sp = getSubpage(state.editingSubpage);
      sp.order = serializeSubpageGrid(sp);
      saveSubpageConfig(state.editingSubpage);
    } else {
      postText("Button Order", serializeGrid(state.grid));
    }
    renderPreview();
    renderButtonSettings();
  }


  function addBulkCardMenuItems(slots) {
    addCtxItem("clipboard-outline", "Copy " + slots.length + " Cards", function () { copyButtons(slots); });
    addCtxItem("content-cut", "Cut " + slots.length + " Cards", function () { cutButtons(slots); });
    addCtxItem("delete", "Delete " + slots.length + " Cards", function () { deleteButtons(slots); }, true);
  }

  function addSingleCardMenuItems(slot) {
    var c = ctx();
    var b = c.buttons[slot - 1];
    addCtxItem("pencil", "Edit Card", function () { openCardSettings(slot); });

    var ctxTypeDef = BUTTON_TYPES[(b && b.type) || ""];
    if (ctxTypeDef && ctxTypeDef.contextMenuItems && (!c.isSub || ctxTypeDef.allowInSubpage)) {
      ctxTypeDef.contextMenuItems(slot, b, { addCtxItem: addCtxItem });
    }

    var sz = c.sizes[slot] || 1;
    addCtxSubmenu("arrow-expand-all", "Size", function (sub) {
      addSubItem(sub, "", "Single", function () { resizeSlot(slot, 1); }, sz === 1);
      addSubItem(sub, "", "Tall", function () { resizeSlot(slot, 2); }, sz === 2);
      addSubItem(sub, "", "Wide", function () { resizeSlot(slot, 3); }, sz === 3);
      addSubItem(sub, "", "Large", function () { resizeSlot(slot, 4); }, sz === 4);
    });

    addCtxDivider();
    addCtxItem("content-copy", "Duplicate", function () {
      if (c.isSub) { duplicateSubpageButton(slot); } else { duplicateButton(slot); }
    });

    addCtxItem("clipboard-outline", "Copy", function () { copySlot(slot); });
    addCtxItem("content-cut", "Cut", function () { cutSlot(slot); });
    addCtxItem("delete", "Delete", function () { deleteSlot(slot); }, true);
  }

  function showSelectionMenu(e) {
    hideContextMenu();
    var c = ctx();
    if (!c.selected.length) return;

    ctxMenu = document.createElement("div");
    ctxMenu.className = "sp-ctx-menu";
    if (c.selected.length > 1) {
      addBulkCardMenuItems(c.selected.slice());
    } else {
      addSingleCardMenuItems(c.selected[0]);
    }
    document.body.appendChild(ctxMenu);
    positionMenu(ctxMenu, e);
  }

  function showContextMenu(e, slot) {
    hideContextMenu();
    var c = ctx();

    if (c.selected.indexOf(slot) === -1) {
      if (c.selected.length > 1) {
        c.selected.push(slot);
      } else {
        c.setSelected([slot]);
        c.setLastClicked(slot);
      }
      renderPreview();
      renderButtonSettings();
      c = ctx();
    }

    ctxMenu = document.createElement("div");
    ctxMenu.className = "sp-ctx-menu";

    if (c.selected.length > 1 && c.selected.indexOf(slot) !== -1) {
      addBulkCardMenuItems(c.selected.slice());
    } else {
      addSingleCardMenuItems(slot);
    }

    document.body.appendChild(ctxMenu);
    positionMenu(ctxMenu, e);
  }

  function showBackContextMenu(e) {
    hideContextMenu();
    ctxMenu = document.createElement("div");
    ctxMenu.className = "sp-ctx-menu";
    var sp = getSubpage(state.editingSubpage);
    var bkSz = sp.sizes[-2] || 1;
    addCtxSubmenu("arrow-expand-all", "Size", function (sub) {
      addSubItem(sub, "", "Single", function () { resizeSlot(-2, 1); }, bkSz === 1);
      addSubItem(sub, "", "Tall", function () { resizeSlot(-2, 2); }, bkSz === 2);
      addSubItem(sub, "", "Wide", function () { resizeSlot(-2, 3); }, bkSz === 3);
      addSubItem(sub, "", "Large", function () { resizeSlot(-2, 4); }, bkSz === 4);
    });
    document.body.appendChild(ctxMenu);
    positionMenu(ctxMenu, e);
  }

  function showEmptySlotMenu(e, pos) {
    hideContextMenu();
    ctxMenu = document.createElement("div");
    ctxMenu.className = "sp-ctx-menu";
    var c = ctx();
    if (state.clipboard) {
      var count = state.clipboard.buttons.length;
      addCtxItem("content-paste", count > 1 ? "Paste " + count + " Cards" : "Paste", function () {
        if (c.isSub) {
          pasteSubpageButton(pos);
        } else {
          pasteButton(pos);
        }
      });
      addCtxDivider();
    }
    addCtxItem("plus", "Create Card", function () { addSlot(pos); });
    if (!c.isSub) {
      addCtxItem("folder-plus", "Create Subpage", function () { addSubpageSlot(pos); });
    }
    document.body.appendChild(ctxMenu);
    positionMenu(ctxMenu, e);
  }

  function hideContextMenu() {
    if (ctxMenu && ctxMenu.parentNode) {
      ctxMenu.parentNode.removeChild(ctxMenu);
    }
    ctxMenu = null;
  }

  // ── Cut / Paste ────────────────────────────────────────────────────────

  function buildClipboardEntry(slot) {
    var c = ctx();
    var src = c.buttons[slot - 1];
    var entry = {
      entity: src.entity, label: src.label, icon: src.icon,
      icon_on: src.icon_on, sensor: src.sensor, unit: src.unit,
      type: src.type || "", precision: src.precision || "",
      subpageConfig: null, size: c.sizes[slot] || 1,
    };
    if (!c.isSub && src.type === "subpage" && state.subpages[slot]) {
      entry.subpageConfig = serializeSubpageConfig(state.subpages[slot]);
    }
    return entry;
  }

  function copySlot(slot) {
    state.clipboard = { buttons: [buildClipboardEntry(slot)] };
  }

  function copyButtons(slots) {
    var entries = [];
    slots.forEach(function (slot) { entries.push(buildClipboardEntry(slot)); });
    state.clipboard = { buttons: entries };
  }

  function cutSlot(slot) {
    copySlot(slot);
    deleteSlot(slot);
  }

  function cutButtons(slots) {
    copyButtons(slots);
    deleteButtons(slots);
  }

  function pasteButton(pos) {
    if (!state.clipboard) return;
    var entries = state.clipboard.buttons;
    var lastSlot = -1;
    for (var i = 0; i < entries.length; i++) {
      var newSlot = firstFreeSlot();
      if (newSlot < 0) break;
      var cell = firstFreeCell(pos);
      if (cell < 0) break;
      var e = entries[i];
      state.buttons[newSlot - 1] = {
        entity: e.entity, label: e.label, icon: e.icon,
        icon_on: e.icon_on, sensor: e.sensor, unit: e.unit,
        type: e.type || "", precision: e.precision || "",
      };
      if (e.size === 4) state.sizes[newSlot] = 4;
      else if (e.size === 2) state.sizes[newSlot] = 2;
      else if (e.size === 3) state.sizes[newSlot] = 3;
      state.grid[cell] = newSlot;
      if (e.size === 2 || e.size === 4) {
        var below = cell + GRID_COLS;
        if (below < NUM_SLOTS && state.grid[below] === 0) state.grid[below] = -1;
      }
      if (e.size === 3 || e.size === 4) {
        var right = cell + 1;
        if (right < NUM_SLOTS && right % GRID_COLS !== 0 && state.grid[right] === 0) state.grid[right] = -1;
      }
      if (e.size === 4) {
        var diag = cell + GRID_COLS + 1;
        if (diag < NUM_SLOTS && (cell + 1) % GRID_COLS !== 0 && state.grid[diag] === 0) state.grid[diag] = -1;
      }
      if (e.subpageConfig) {
        var spCopy = parseSubpageConfig(e.subpageConfig);
        spCopy.sizes = {};
        buildSubpageGrid(spCopy);
        state.subpages[newSlot] = spCopy;
      }
      saveButtonConfig(newSlot);
      saveSubpageEntity(newSlot);
      lastSlot = newSlot;
    }
    postText("Button Order", serializeGrid(state.grid));
    state.clipboard = null;
    state.selectedSlots = [];
    renderPreview();
  }

  function pasteSubpageButton(pos) {
    if (!state.clipboard) return;
    var homeSlot = state.editingSubpage;
    var sp = getSubpage(homeSlot);
    var maxPos = NUM_SLOTS;
    var entries = state.clipboard.buttons;
    var lastSlot = -1;
    for (var i = 0; i < entries.length; i++) {
      var cell = -1;
      for (var c = pos; c < maxPos; c++) {
        if (sp.grid[c] === 0) { cell = c; break; }
      }
      if (cell < 0) break;
      var newSlot = subpageFirstFreeSlot(sp);
      while (sp.buttons.length < newSlot) {
        sp.buttons.push(emptyButtonConfig());
      }
      var e = entries[i];
      sp.buttons[newSlot - 1] = {
        entity: e.entity, label: e.label, icon: e.icon,
        icon_on: e.icon_on, sensor: e.sensor, unit: e.unit,
        type: e.type || "", precision: e.precision || "",
      };
      if (e.size === 4) sp.sizes[newSlot] = 4;
      else if (e.size === 2) sp.sizes[newSlot] = 2;
      else if (e.size === 3) sp.sizes[newSlot] = 3;
      sp.grid[cell] = newSlot;
      if (e.size === 2 || e.size === 4) {
        var below = cell + GRID_COLS;
        if (below < maxPos && sp.grid[below] === 0) sp.grid[below] = -1;
      }
      if (e.size === 3 || e.size === 4) {
        var right = cell + 1;
        if (right < maxPos && right % GRID_COLS !== 0 && sp.grid[right] === 0) sp.grid[right] = -1;
      }
      if (e.size === 4) {
        var diag = cell + GRID_COLS + 1;
        if (diag < maxPos && (cell + 1) % GRID_COLS !== 0 && sp.grid[diag] === 0) sp.grid[diag] = -1;
      }
      lastSlot = newSlot;
    }
    sp.order = serializeSubpageGrid(sp);
    state.clipboard = null;
    saveSubpageConfig(homeSlot);
    state.subpageSelectedSlots = [];
    renderPreview();
  }

  // ── Export / Import ────────────────────────────────────────────────────

  function exportConfig() {
    var data = {
      version: 1,
      device: DEVICE_ID,
      exported_at: new Date().toISOString(),
      button_order: serializeGrid(state.grid),
      button_on_color: state.onColor,
      button_off_color: state.offColor,
      sensor_card_color: state.sensorColor,
      buttons: state.buttons.map(function (b) {
        return {
          entity: b.entity, label: b.label, icon: b.icon,
          icon_on: b.icon_on, sensor: b.sensor, unit: b.unit,
          type: b.type || "", precision: b.precision || "",
        };
      }),
      subpages: (function () {
        var sp = {};
        for (var k in state.subpages) {
          if (state.subpages[k] && state.subpages[k].buttons && state.subpages[k].buttons.length > 0) {
            sp[k] = serializeSubpageConfig(state.subpages[k]);
          }
        }
        return sp;
      })(),
      settings: {
        indoor_temp_enable: state._indoorOn,
        outdoor_temp_enable: state._outdoorOn,
        indoor_temp_entity: state.indoorEntity,
        outdoor_temp_entity: state.outdoorEntity,
        temperature_unit: normalizeTemperatureUnit(state.temperatureUnit),
        clock_bar: state.clockBarOn,
        temperature_degree_symbol: state.temperatureDegreeSymbolOn,
        timezone: state.timezone,
        clock_format: state.clockFormat,
        ntp_server_1: state.ntpServer1,
        ntp_server_2: state.ntpServer2,
        ntp_server_3: state.ntpServer3,
        screensaver_mode: getActiveScreensaverMode(),
        presence_sensor_entity: state.presenceEntity,
        clock_screensaver: state.clockScreensaverOn,
        clock_brightness: state.clockBrightnessDay,
        clock_brightness_day: state.clockBrightnessDay,
        clock_brightness_night: state.clockBrightnessNight,
        screensaver_timeout: state.screensaverTimeout,
        home_screen_timeout: state.homeScreenTimeout,
        screen_rotation: state.screenRotation,
        developer_experimental_features: state.developerExperimentalFeatures,
        bluetooth_proxy: state.bluetoothProxyEnabled,
      },
      screen: {
        brightness_day: Math.round(state.brightnessDayVal),
        brightness_night: Math.round(state.brightnessNightVal),
        schedule_enabled: !!state.scheduleEnabled,
        schedule_on_hour: normalizeHour(state.scheduleOnHour, 6),
        schedule_off_hour: normalizeHour(state.scheduleOffHour, 23),
        schedule_mode: normalizeScheduleMode(state.scheduleMode),
        schedule_wake_timeout: normalizeScheduleWakeTimeout(state.scheduleWakeTimeout),
        schedule_wake_brightness: normalizeScheduleWakeBrightness(state.scheduleWakeBrightness),
        schedule_dimmed_brightness: normalizeScheduleDimmedBrightness(state.scheduleDimmedBrightness),
        schedule_clock_brightness: normalizeScheduleClockBrightness(state.scheduleClockBrightness),
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

    function cleanupInput() {
      if (input.parentNode) input.parentNode.removeChild(input);
    }

    input.addEventListener("cancel", cleanupInput);
    input.addEventListener("change", function () {
      if (!input.files || !input.files[0]) {
        cleanupInput();
        return;
      }
      var reader = new FileReader();
      reader.onerror = function () {
        cleanupInput();
        showBanner("Invalid file \u2014 could not read backup", "error");
      };
      reader.onload = function () {
        var data;
        try { data = JSON.parse(reader.result); } catch (_) {
          showBanner("Invalid file \u2014 could not parse JSON", "error");
          cleanupInput();
          return;
        }

        if (!data.version || !Array.isArray(data.buttons)) {
          showBanner("Invalid config file \u2014 missing required fields", "error");
          cleanupInput();
          return;
        }
        if (data.device && data.device !== DEVICE_ID) {
          showBanner("Config was exported from a different panel (" + data.device + ") \u2014 layout may look different", "warning");
        }
        var importedCount = data.buttons.length;
        if (importedCount !== NUM_SLOTS) {
          showBanner("Backup has " + importedCount + " slots, current config has " + NUM_SLOTS + " \u2014 adapting", "warning");
        }

        postText("Button On Color", data.button_on_color || "FF8C00");
        postText("Button Off Color", data.button_off_color || "313131");
        postText("Sensor Card Color", data.sensor_card_color || "212121");

        var empty = { entity: "", label: "", icon: "Auto", icon_on: "Auto", sensor: "", unit: "", type: "", precision: "" };
        var buttons, orderStr, spKeyMap, importedSizes;

        if (importedCount !== NUM_SLOTS) {
          // Grid dimensions differ — remap used buttons into target slots
          var origParts = (data.button_order || "").split(",");
          var usedSlots = [];
          var seen = {};
          for (var j = 0; j < origParts.length; j++) {
            var tok = origParts[j].trim();
            if (!tok) continue;
            var lastCh = tok.charAt(tok.length - 1);
            var dbl = lastCh === "d";
            var wide = lastCh === "w";
            var big = lastCh === "b";
            var num = parseInt(tok, 10);
            if (isNaN(num) || num < 1 || num > importedCount || seen[num]) continue;
            seen[num] = true;
            usedSlots.push({ oldSlot: num, isDouble: dbl || big, isWide: wide || big, isBig: big });
          }
          for (var j = 0; j < importedCount; j++) {
            var sn = j + 1;
            if (seen[sn]) continue;
            var bb = data.buttons[j];
            if (bb.entity || bb.label || bb.type) {
              usedSlots.push({ oldSlot: sn, isDouble: false, isWide: false, isBig: false });
            }
          }

          var limit = Math.min(usedSlots.length, NUM_SLOTS);
          var slotMap = {};
          buttons = [];
          var newSizes = {};
          for (var j = 0; j < limit; j++) {
            var ns = j + 1;
            slotMap[usedSlots[j].oldSlot] = ns;
            buttons.push(data.buttons[usedSlots[j].oldSlot - 1]);
            if (usedSlots[j].isBig) newSizes[ns] = 4;
            else if (usedSlots[j].isDouble) newSizes[ns] = 2;
            else if (usedSlots[j].isWide) newSizes[ns] = 3;
          }
          for (var j = limit; j < NUM_SLOTS; j++) buttons.push(empty);

          var newGrid = [];
          for (var j = 0; j < NUM_SLOTS; j++) newGrid.push(0);
          var pos = 0;
          for (var j = 0; j < limit && pos < NUM_SLOTS; j++) {
            var ns = j + 1;
            var isB = newSizes[ns] === 4;
            var isD = isB || newSizes[ns] === 2;
            var isW = isB || newSizes[ns] === 3;
            var row = Math.floor(pos / GRID_COLS);
            if (isD && row >= GRID_ROWS - 1) {
              isD = false;
              if (isB) { isB = false; newSizes[ns] = isW ? 3 : undefined; if (!newSizes[ns]) delete newSizes[ns]; }
              else delete newSizes[ns];
            }
            var col = pos % GRID_COLS;
            if (isW && col >= GRID_COLS - 1) {
              isW = false;
              if (isB) { isB = false; newSizes[ns] = isD ? 2 : undefined; if (!newSizes[ns]) delete newSizes[ns]; }
              else delete newSizes[ns];
            }
            newGrid[pos] = ns;
            if (isD) {
              var bp = pos + GRID_COLS;
              if (bp < NUM_SLOTS) newGrid[bp] = -1;
            }
            if (isW) {
              var rp = pos + 1;
              if (rp < NUM_SLOTS) newGrid[rp] = -1;
            }
            if (isD && isW) {
              var dp = pos + GRID_COLS + 1;
              if (dp < NUM_SLOTS) newGrid[dp] = -1;
            }
            pos++;
            while (pos < NUM_SLOTS && newGrid[pos] === -1) pos++;
          }

          state.sizes = newSizes;
          importedSizes = newSizes;
          orderStr = serializeGrid(newGrid);

          spKeyMap = {};
          if (data.subpages) {
            for (var k in data.subpages) {
              var oldKey = parseInt(k, 10);
              if (slotMap[oldKey]) spKeyMap[k] = slotMap[oldKey];
            }
          }
        } else {
          buttons = [];
          for (var j = 0; j < NUM_SLOTS; j++) {
            buttons.push(j < importedCount ? data.buttons[j] : empty);
          }
          importedSizes = {};
          orderStr = data.button_order || "";
          spKeyMap = {};
          if (data.subpages) {
            for (var k in data.subpages) {
              var kn = parseInt(k, 10);
              if (kn >= 1 && kn <= NUM_SLOTS) spKeyMap[k] = kn;
            }
          }
        }

        for (var i = 0; i < NUM_SLOTS; i++) {
          var b = buttons[i];
          var n = i + 1;
          state.buttons[i] = normalizeButtonConfig({
            entity: b.entity || "", label: b.label || "",
            icon: b.icon || "Auto", icon_on: b.icon_on || "Auto",
            sensor: b.sensor || "", unit: b.unit || "",
            type: b.type || "", precision: b.precision || "",
          });
          saveButtonConfig(n);
        }

        state.subpages = {};
        state.subpageRaw = {};
        if (data.subpages) {
          for (var k in data.subpages) {
            var newKey = spKeyMap[k];
            if (!newKey) continue;
            var sp = parseSubpageConfig(data.subpages[k]);
            sp.sizes = {};
            buildSubpageGrid(sp);
            state.subpages[String(newKey)] = sp;
            saveSubpageEntity(newKey);
          }
        }

        postText("Button Order", orderStr);
        applyImportedButtonOrder(orderStr, importedSizes);
        state.onColor = data.button_on_color || "FF8C00";
        state.offColor = data.button_off_color || "313131";
        state.sensorColor = data.sensor_card_color || "212121";

        if (els.setOnColor && els.setOnColor._syncColor) els.setOnColor._syncColor(state.onColor);
        if (els.setOffColor && els.setOffColor._syncColor) els.setOffColor._syncColor(state.offColor);
        if (els.setSensorColor && els.setSensorColor._syncColor) els.setSensorColor._syncColor(state.sensorColor);

        if (data.settings) {
          var s = data.settings;

          postSwitch("Indoor Temp Enable", !!s.indoor_temp_enable);
          postSwitch("Outdoor Temp Enable", !!s.outdoor_temp_enable);
          postText("Indoor Temp Entity", s.indoor_temp_entity || "");
          postText("Outdoor Temp Entity", s.outdoor_temp_entity || "");
          postClockBar(s.clock_bar != null ? !!s.clock_bar : false);
          postTemperatureDegreeSymbol(s.temperature_degree_symbol != null ? !!s.temperature_degree_symbol : true);
          var importedTimezone = s.timezone || state.timezone;
          var importedTemperatureUnit = normalizeTemperatureUnit(s.temperature_unit);
          var importedClockFormat =
            state.clockFormatOptions.indexOf(s.clock_format) !== -1
              ? s.clock_format
              : state.clockFormat;
          var hasNtpServer1 = Object.prototype.hasOwnProperty.call(s, "ntp_server_1");
          var hasNtpServer2 = Object.prototype.hasOwnProperty.call(s, "ntp_server_2");
          var hasNtpServer3 = Object.prototype.hasOwnProperty.call(s, "ntp_server_3");
          var hasDeveloperExperimentalFeatures =
            Object.prototype.hasOwnProperty.call(s, "developer_experimental_features");
          var hasBluetoothProxy =
            Object.prototype.hasOwnProperty.call(s, "bluetooth_proxy");
          var importedDeveloperExperimentalFeatures = hasDeveloperExperimentalFeatures
            ? !!s.developer_experimental_features
            : state.developerExperimentalFeatures;
          var importedBluetoothProxy = hasBluetoothProxy
            ? !!s.bluetooth_proxy
            : false;
          var importedNtpServer1 = hasNtpServer1
            ? normalizeNtpServer(s.ntp_server_1, NTP_SERVER_DEFAULTS[0])
            : state.ntpServer1;
          var importedNtpServer2 = hasNtpServer2
            ? normalizeNtpServer(s.ntp_server_2, NTP_SERVER_DEFAULTS[1])
            : state.ntpServer2;
          var importedNtpServer3 = hasNtpServer3
            ? normalizeNtpServer(s.ntp_server_3, NTP_SERVER_DEFAULTS[2])
            : state.ntpServer3;
          if (s.timezone) postSelect("Screen: Timezone", importedTimezone);
          postSelect("Screen: Temperature Unit", importedTemperatureUnit);
          if (s.clock_format) postSelect("Screen: Clock Format", importedClockFormat);
          if (hasNtpServer1) {
            postText("Screen: NTP Server 1", importedNtpServer1);
          }
          if (hasNtpServer2) {
            postText("Screen: NTP Server 2", importedNtpServer2);
          }
          if (hasNtpServer3) {
            postText("Screen: NTP Server 3", importedNtpServer3);
          }
          var importedScreensaverMode = s.screensaver_mode || "disabled";
          if (importedScreensaverMode !== "sensor" &&
              importedScreensaverMode !== "timer" &&
              importedScreensaverMode !== "disabled") {
            importedScreensaverMode = "disabled";
          }
          postText("Screensaver Mode", importedScreensaverMode);
          postText("Presence Sensor Entity", s.presence_sensor_entity || "");
          var importedClockBrightnessDay = normalizeClockBrightness(
            s.clock_brightness_day != null ? s.clock_brightness_day : s.clock_brightness,
            35);
          var importedClockBrightnessNight = normalizeClockBrightness(
            s.clock_brightness_night != null ? s.clock_brightness_night : s.clock_brightness,
            importedClockBrightnessDay);
          postSwitch("Screen Saver: Clock", s.clock_screensaver != null ? !!s.clock_screensaver : false);
          postClockBrightnessDay(importedClockBrightnessDay);
          postClockBrightnessNight(importedClockBrightnessNight);
          postScreensaverTimeout(s.screensaver_timeout || 300);
          postNumber("Home Screen Timeout", s.home_screen_timeout != null ? s.home_screen_timeout : 60);
          var importedScreenRotation = normalizeScreenRotation(s.screen_rotation);
          if (CFG.features && CFG.features.screenRotation) postSelect("Screen: Rotation", importedScreenRotation);
          if (hasDeveloperExperimentalFeatures) {
            postSwitch("Developer: Experimental Features", importedDeveloperExperimentalFeatures);
          }
          if (hasBluetoothProxy || state.bluetoothProxyEnabled) {
            postBluetoothProxy(importedBluetoothProxy);
          }

          state._indoorOn = !!s.indoor_temp_enable;
          state._outdoorOn = !!s.outdoor_temp_enable;
          state.indoorEntity = s.indoor_temp_entity || "";
          state.outdoorEntity = s.outdoor_temp_entity || "";
          state.temperatureUnit = importedTemperatureUnit;
          state.clockBarOn = s.clock_bar != null ? !!s.clock_bar : false;
          state.temperatureDegreeSymbolOn = s.temperature_degree_symbol != null ? !!s.temperature_degree_symbol : true;
          state.timezone = importedTimezone;
          state.clockFormat = importedClockFormat;
          state.ntpServer1 = importedNtpServer1;
          state.ntpServer2 = importedNtpServer2;
          state.ntpServer3 = importedNtpServer3;
          state.customNtpServers = hasCustomNtpServers();
          state.screensaverMode = importedScreensaverMode;
          state._screensaverModeReceived = true;
          state.presenceEntity = s.presence_sensor_entity || "";
          state.clockScreensaverOn = s.clock_screensaver != null ? !!s.clock_screensaver : false;
          state.clockBrightnessDay = importedClockBrightnessDay;
          state.clockBrightnessNight = importedClockBrightnessNight;
          state.screensaverTimeout = s.screensaver_timeout || 300;
          state.homeScreenTimeout = s.home_screen_timeout != null ? s.home_screen_timeout : 60;
          state.screenRotation = importedScreenRotation;
          if (hasDeveloperExperimentalFeatures) {
            state.developerExperimentalFeatures = importedDeveloperExperimentalFeatures;
          }
          state.bluetoothProxyEnabled = importedBluetoothProxy;

          syncTemperatureUi();
          syncClockBarUi();
          syncBluetoothProxyUi();
          syncInput(els.setIndoorEntity, state.indoorEntity);
          syncInput(els.setOutdoorEntity, state.outdoorEntity);
          if (els.setTemperatureUnit) els.setTemperatureUnit.value = state.temperatureUnit;
          syncInput(els.setPresence, state.presenceEntity);
          if (els.setTimezone) els.setTimezone.value = state.timezone;
          if (els.setClockFormat) els.setClockFormat.value = state.clockFormat;
          syncNtpServerUi();
          syncClockScreensaverControls();
          syncScreensaverTimeoutUi();
          syncIdleUi();
          if (els.setScreenRotation) els.setScreenRotation.value = state.screenRotation;
          if (els.setDeveloperExperimentalFeatures) {
            els.setDeveloperExperimentalFeatures.checked = state.developerExperimentalFeatures;
          }
          if (els.setBluetoothProxy) {
            els.setBluetoothProxy.checked = state.bluetoothProxyEnabled;
          }
          if (els.setSsMode) els.setSsMode(getActiveScreensaverMode());
          updateTempPreview();

        }

        var screenSettings = data.screen || (data.settings && data.settings.screen);
        if (screenSettings) {
          state.brightnessDayVal = parseFloat(screenSettings.brightness_day);
          if (!isFinite(state.brightnessDayVal)) state.brightnessDayVal = 100;
          state.brightnessNightVal = parseFloat(screenSettings.brightness_night);
          if (!isFinite(state.brightnessNightVal)) state.brightnessNightVal = 75;
          state.scheduleEnabled = !!screenSettings.schedule_enabled;
          state.scheduleOnHour = normalizeHour(screenSettings.schedule_on_hour, 6);
          state.scheduleOffHour = normalizeHour(screenSettings.schedule_off_hour, 23);
          state.scheduleMode = normalizeScheduleMode(screenSettings.schedule_mode);
          state.scheduleWakeTimeout = normalizeScheduleWakeTimeout(screenSettings.schedule_wake_timeout);
          state.scheduleWakeBrightness = normalizeScheduleWakeBrightness(
            screenSettings.schedule_wake_brightness != null
              ? screenSettings.schedule_wake_brightness
              : state.scheduleWakeBrightness
          );
          state.scheduleDimmedBrightness = normalizeScheduleDimmedBrightness(
            screenSettings.schedule_dimmed_brightness != null
              ? screenSettings.schedule_dimmed_brightness
              : state.scheduleDimmedBrightness
          );
          state.scheduleClockBrightness = normalizeScheduleClockBrightness(
            screenSettings.schedule_clock_brightness != null
              ? screenSettings.schedule_clock_brightness
              : state.scheduleClockBrightness
          );

          postNumber("Screen: Daytime Brightness", state.brightnessDayVal);
          postNumber("Screen: Nighttime Brightness", state.brightnessNightVal);
          postScreenScheduleOnHour(state.scheduleOnHour);
          postScreenScheduleOffHour(state.scheduleOffHour);
          postScreenScheduleMode(state.scheduleMode);
          postScreenScheduleWakeTimeout(state.scheduleWakeTimeout);
          postScreenScheduleWakeBrightness(state.scheduleWakeBrightness);
          postScreenScheduleDimmedBrightness(state.scheduleDimmedBrightness);
          postScreenScheduleClockBrightness(state.scheduleClockBrightness);
          postScreenScheduleEnabled(state.scheduleEnabled);

          if (els.setDayBrightness) {
            els.setDayBrightness.value = state.brightnessDayVal;
            els.setDayBrightnessVal.textContent = Math.round(state.brightnessDayVal) + "%";
          }
          if (els.setNightBrightness) {
            els.setNightBrightness.value = state.brightnessNightVal;
            els.setNightBrightnessVal.textContent = Math.round(state.brightnessNightVal) + "%";
          }
          syncScreenScheduleUi();
        }

        state.selectedSlots = [];
        state.lastClickedSlot = -1;
        renderPreview();
        renderButtonSettings();
        switchTab("screen");
        showBanner("Configuration imported successfully", "success");
        cleanupInput();
      };
      reader.readAsText(input.files[0]);
    });

    document.body.appendChild(input);
    input.click();
  }

  // ── Clock (minute-aligned) ─────────────────────────────────────────────

  function getTzId(tz) {
    var idx = tz.indexOf(" (");
    return idx > 0 ? tz.substring(0, idx) : tz;
  }

  function formatGmtOffset(minutes) {
    var sign = minutes >= 0 ? "+" : "-";
    var abs = Math.abs(minutes);
    var h = Math.floor(abs / 60);
    var m = abs % 60;
    return "GMT" + sign + h + (m ? ":" + String(m).padStart(2, "0") : "");
  }

  function timezoneOffsetMinutes(tzId, date) {
    try {
      var parts = new Intl.DateTimeFormat("en-US", {
        timeZone: tzId,
        hourCycle: "h23",
        year: "numeric",
        month: "2-digit",
        day: "2-digit",
        hour: "2-digit",
        minute: "2-digit",
        second: "2-digit"
      }).formatToParts(date);
      var values = {};
      for (var i = 0; i < parts.length; i++) {
        if (parts[i].type !== "literal") values[parts[i].type] = parts[i].value;
      }
      var localAsUtc = Date.UTC(
        Number(values.year),
        Number(values.month) - 1,
        Number(values.day),
        Number(values.hour),
        Number(values.minute),
        Number(values.second)
      );
      return Math.round((localAsUtc - date.getTime()) / 60000);
    } catch (_) {
      return null;
    }
  }

  function formatTimezoneOption(opt) {
    var tzId = getTzId(opt);
    var offset = timezoneOffsetMinutes(tzId, new Date());
    if (offset == null || !isFinite(offset)) return opt;
    return tzId + " (" + formatGmtOffset(offset) + ")";
  }

  function appendTimezoneOption(select, opt) {
    var o = document.createElement("option");
    o.value = opt;
    o.textContent = formatTimezoneOption(opt);
    select.appendChild(o);
  }

  function updateClock() {
    if (!els.clock) return;
    var now = new Date();
    var tzId = getTzId(state.timezone);
    try {
      var parts = new Intl.DateTimeFormat("en-US", {
        timeZone: tzId, hour: "numeric", minute: "2-digit",
        hour12: state.clockFormat === "12h"
      }).formatToParts(now);
      var h = "", m = "";
      for (var i = 0; i < parts.length; i++) {
        if (parts[i].type === "hour") h = parts[i].value;
        else if (parts[i].type === "minute") m = parts[i].value;
      }
      els.clock.textContent = (state.clockFormat === "24h"
        ? h.padStart(2, "0") : h) + ":" + m;
    } catch (_) {
      var hr = now.getUTCHours();
      var mn = String(now.getUTCMinutes()).padStart(2, "0");
      els.clock.textContent = String(hr).padStart(2, "0") + ":" + mn;
    }
    var msToNext = (60 - now.getSeconds()) * 1000 - now.getMilliseconds();
    setTimeout(updateClock, msToNext + 50);
  }

  // ── SSE ────────────────────────────────────────────────────────────────

  function connectEvents() {
    if (_eventSource) { _eventSource.close(); _eventSource = null; }

    function markConnected() {
      state.selectedSlots = [];
      state.lastClickedSlot = -1;
      state.editingSubpage = null;
      state.subpageSelectedSlots = [];
      state.subpageLastClicked = -1;
      orderReceived = false;
      if (els.banner) els.banner.className = "sp-banner";
      els.root.querySelectorAll(".sp-apply-btn").forEach(function (btn) {
        btn.disabled = false;
        btn.textContent = "Apply Configuration";
      });
      clearTimeout(migrationTimer);
      migrationTimer = setTimeout(scheduleMigration, 5000);
      clearTimeout(sliderMigrationTimer);
      pendingSliderSubpageMigrations = {};
      refreshFirmwareVersion();
      refreshScreensaverTimeout();
    }

    function handleDisconnected(source) {
      showBanner("Reconnecting to device\u2026", "offline");
      if (source.readyState === 2) {
        source.close();
        _eventSource = null;
        setTimeout(connectEvents, 5000);
      }
    }

    var sseHandlers = {
      "text-button_order": function (val) {
        orderReceived = !!(val && val.trim());
        state.sizes = {};
        state.grid = parseOrder(val);
        state.selectedSlots = state.selectedSlots.filter(function (s) {
          return state.grid.indexOf(s) !== -1;
        });
        scheduleRender();
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
      "text-sensor_card_color": function (val) {
        state.sensorColor = val;
        if (els.setSensorColor && els.setSensorColor._syncColor) els.setSensorColor._syncColor(val);
        renderPreview();
      },
      "switch-indoor_temp_enable": function (val, d) {
        state._indoorOn = d.value === true || val === "ON";
        syncTemperatureUi();
        updateTempPreview();
      },
      "switch-outdoor_temp_enable": function (val, d) {
        state._outdoorOn = d.value === true || val === "ON";
        syncTemperatureUi();
        updateTempPreview();
      },
      "switch-screen__clock_bar": function (val, d) {
        state.clockBarOn = d.value === true || val === "ON";
        syncClockBarUi();
      },
      "switch-screen_clock_bar": function (val, d) {
        state.clockBarOn = d.value === true || val === "ON";
        syncClockBarUi();
      },
      "switch-clock_bar_enabled": function (val, d) {
        state.clockBarOn = d.value === true || val === "ON";
        syncClockBarUi();
      },
      "switch-screen__temperature_degree_symbol": function (val, d) {
        state.temperatureDegreeSymbolOn = d.value === true || val === "ON";
        syncClockBarUi();
      },
      "switch-screen_temperature_degree_symbol": function (val, d) {
        state.temperatureDegreeSymbolOn = d.value === true || val === "ON";
        syncClockBarUi();
      },
      "switch-temperature_degree_symbol_enabled": function (val, d) {
        state.temperatureDegreeSymbolOn = d.value === true || val === "ON";
        syncClockBarUi();
      },
      "text-indoor_temp_entity": function (val) {
        state.indoorEntity = val;
        syncInput(els.setIndoorEntity, val);
      },
      "text-outdoor_temp_entity": function (val) {
        state.outdoorEntity = val;
        syncInput(els.setOutdoorEntity, val);
      },
      "select-screen__temperature_unit": function (val, d) {
        state.temperatureUnit = normalizeTemperatureUnit(d.value || val);
        if (els.setTemperatureUnit) els.setTemperatureUnit.value = state.temperatureUnit;
        updateTempPreview();
        renderPreview();
      },
      "number-screensaver_timeout": function (val, d) {
        applyScreensaverTimeoutState(d);
      },
      "number-screen_saver__timeout": function (val, d) {
        applyScreensaverTimeoutState(d);
      },
      "number-screen_saver_timeout": function (val, d) {
        applyScreensaverTimeoutState(d);
      },
      "number-home_screen_timeout": function (val) {
        state.homeScreenTimeout = parseFloat(val) || 0;
        syncIdleUi();
      },
      "switch-screen_saver__clock": function (val, d) {
        state.clockScreensaverOn = d.value === true || val === "ON";
        syncClockScreensaverControls();
      },
      "number-screen_saver__clock_brightness": function (val) {
        if (state.clockBrightnessSplitReceived) return;
        var brightness = normalizeClockBrightness(val, 35);
        state.clockBrightnessDay = brightness;
        state.clockBrightnessNight = brightness;
        syncClockScreensaverControls();
      },
      "number-screen_saver__daytime_clock_brightness": function (val) {
        state.clockBrightnessSplitReceived = true;
        state.clockBrightnessDay = normalizeClockBrightness(val, 35);
        syncClockScreensaverControls();
      },
      "number-screen_saver__nighttime_clock_brightness": function (val) {
        state.clockBrightnessSplitReceived = true;
        state.clockBrightnessNight = normalizeClockBrightness(val, state.clockBrightnessDay);
        syncClockScreensaverControls();
      },
      "text-presence_sensor_entity": function (val) {
        state.presenceEntity = val;
        syncInput(els.setPresence, val);
        if (state.screensaverMode === "") {
          if (els.setSsMode) els.setSsMode(getActiveScreensaverMode());
        }
      },
      "text-screensaver_mode": function (val) {
        state._screensaverModeReceived = true;
        state.screensaverMode = val === "sensor" || val === "timer" || val === "disabled" ? val : "disabled";
        if (els.setSsMode) els.setSsMode(getActiveScreensaverMode());
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
      "switch-screen__schedule_enabled": function (val, d) {
        state.scheduleEnabled = d.value === true || val === "ON";
        syncScreenScheduleUi();
      },
      "number-screen__schedule_on_hour": function (val) {
        state.scheduleOnHour = normalizeHour(val, 6);
        syncScreenScheduleUi();
      },
      "number-screen__schedule_off_hour": function (val) {
        state.scheduleOffHour = normalizeHour(val, 23);
        syncScreenScheduleUi();
      },
      "select-screen__schedule_mode": function (val, d) {
        state.scheduleMode = normalizeScheduleMode(d.value || val);
        syncScreenScheduleUi();
      },
      "number-screen__schedule_wake_timeout": function (val) {
        state.scheduleWakeTimeout = normalizeScheduleWakeTimeout(val);
        syncScreenScheduleUi();
      },
      "number-screen_schedule_wake_timeout": function (val) {
        state.scheduleWakeTimeout = normalizeScheduleWakeTimeout(val);
        syncScreenScheduleUi();
      },
      "number-schedule_wake_timeout": function (val) {
        state.scheduleWakeTimeout = normalizeScheduleWakeTimeout(val);
        syncScreenScheduleUi();
      },
      "number-screen__schedule_wake_brightness": function (val) {
        state.scheduleWakeBrightness = normalizeScheduleWakeBrightness(val);
        syncScreenScheduleUi();
      },
      "number-screen_schedule_wake_brightness": function (val) {
        state.scheduleWakeBrightness = normalizeScheduleWakeBrightness(val);
        syncScreenScheduleUi();
      },
      "number-schedule_wake_brightness": function (val) {
        state.scheduleWakeBrightness = normalizeScheduleWakeBrightness(val);
        syncScreenScheduleUi();
      },
      "number-screen__schedule_dimmed_brightness": function (val) {
        state.scheduleDimmedBrightness = normalizeScheduleDimmedBrightness(val);
        syncScreenScheduleUi();
      },
      "number-screen_schedule_dimmed_brightness": function (val) {
        state.scheduleDimmedBrightness = normalizeScheduleDimmedBrightness(val);
        syncScreenScheduleUi();
      },
      "number-schedule_dimmed_brightness": function (val) {
        state.scheduleDimmedBrightness = normalizeScheduleDimmedBrightness(val);
        syncScreenScheduleUi();
      },
      "number-screen__schedule_clock_brightness": function (val) {
        state.scheduleClockBrightness = normalizeScheduleClockBrightness(val);
        syncScreenScheduleUi();
      },
      "number-screen_schedule_clock_brightness": function (val) {
        state.scheduleClockBrightness = normalizeScheduleClockBrightness(val);
        syncScreenScheduleUi();
      },
      "number-schedule_clock_brightness": function (val) {
        state.scheduleClockBrightness = normalizeScheduleClockBrightness(val);
        syncScreenScheduleUi();
      },
      "select-screen__timezone": function (val, d) {
        state.timezone = d.value || val || state.timezone;
        if (d.option && Array.isArray(d.option)) {
          state.timezoneOptions = d.option;
          if (els.setTimezone) {
            els.setTimezone.innerHTML = "";
            d.option.forEach(function (opt) {
              appendTimezoneOption(els.setTimezone, opt);
            });
          }
        }
        if (els.setTimezone) els.setTimezone.value = state.timezone;
        if (normalizeTemperatureUnit(state.temperatureUnit) === "Auto") {
          updateTempPreview();
          renderPreview();
        }
        updateClock();
      },
      "select-screen__clock_format": function (val, d) {
        state.clockFormat = d.value || val || state.clockFormat;
        if (d.option && Array.isArray(d.option)) {
          state.clockFormatOptions = d.option;
          if (els.setClockFormat) {
            els.setClockFormat.innerHTML = "";
            d.option.forEach(function (opt) {
              var o = document.createElement("option");
              o.value = opt;
              o.textContent = opt === "12h" ? "12-hour" : "24-hour";
              els.setClockFormat.appendChild(o);
            });
          }
        }
        if (els.setClockFormat) els.setClockFormat.value = state.clockFormat;
        updateClock();
      },
      "text-screen__ntp_server_1": function (val) {
        state.ntpServer1 = normalizeNtpServer(val, NTP_SERVER_DEFAULTS[0]);
        state.customNtpServers = state.customNtpServers || hasCustomNtpServers();
        syncNtpServerUi();
      },
      "text-screen__ntp_server_2": function (val) {
        state.ntpServer2 = normalizeNtpServer(val, NTP_SERVER_DEFAULTS[1]);
        state.customNtpServers = state.customNtpServers || hasCustomNtpServers();
        syncNtpServerUi();
      },
      "text-screen__ntp_server_3": function (val) {
        state.ntpServer3 = normalizeNtpServer(val, NTP_SERVER_DEFAULTS[2]);
        state.customNtpServers = state.customNtpServers || hasCustomNtpServers();
        syncNtpServerUi();
      },
      "text-ntp_server_1": function (val) {
        state.ntpServer1 = normalizeNtpServer(val, NTP_SERVER_DEFAULTS[0]);
        state.customNtpServers = state.customNtpServers || hasCustomNtpServers();
        syncNtpServerUi();
      },
      "text-ntp_server_2": function (val) {
        state.ntpServer2 = normalizeNtpServer(val, NTP_SERVER_DEFAULTS[1]);
        state.customNtpServers = state.customNtpServers || hasCustomNtpServers();
        syncNtpServerUi();
      },
      "text-ntp_server_3": function (val) {
        state.ntpServer3 = normalizeNtpServer(val, NTP_SERVER_DEFAULTS[2]);
        state.customNtpServers = state.customNtpServers || hasCustomNtpServers();
        syncNtpServerUi();
      },
      "select-screen__rotation": function (val, d) {
        state.screenRotation = normalizeScreenRotation(d.value || val || state.screenRotation);
        if (d.option && Array.isArray(d.option)) {
          state.screenRotationOptions = d.option;
          if (els.setScreenRotation) {
            els.setScreenRotation.innerHTML = "";
            d.option.forEach(function (opt) {
              appendScreenRotationOption(els.setScreenRotation, opt);
            });
          }
        }
        if (els.setScreenRotation) els.setScreenRotation.value = state.screenRotation;
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
        setFirmwareVersion(val);
      },
      "update-firmware__update": function (val, d) {
        setFirmwareUpdateInfo(d);
      },
      "switch-firmware__auto_update": function (val, d) {
        state.autoUpdate = d.value === true || val === "ON";
        if (els.setAutoUpdate) els.setAutoUpdate.checked = state.autoUpdate;
        if (els.updateFreqWrap) els.updateFreqWrap.style.display = state.autoUpdate ? "" : "none";
      },
      "switch-bluetooth_proxy": function (val, d) {
        state.bluetoothProxyEnabled = d.value === true || val === "ON";
        syncBluetoothProxyUi();
      },
      "switch-bluetooth_proxy_enabled": function (val, d) {
        state.bluetoothProxyEnabled = d.value === true || val === "ON";
        syncBluetoothProxyUi();
      },
      "switch-developer__experimental_features": function (val, d) {
        state.developerExperimentalFeatures = d.value === true || val === "ON";
        if (els.setDeveloperExperimentalFeatures) {
          els.setDeveloperExperimentalFeatures.checked = state.developerExperimentalFeatures;
        }
        scheduleRender();
      },
      "switch-developer_experimental_features": function (val, d) {
        state.developerExperimentalFeatures = d.value === true || val === "ON";
        if (els.setDeveloperExperimentalFeatures) {
          els.setDeveloperExperimentalFeatures.checked = state.developerExperimentalFeatures;
        }
        scheduleRender();
      },
      "select-firmware__update_frequency": function (val, d) {
        state.updateFrequency = d.value || val || state.updateFrequency;
        if (els.setUpdateFreq) els.setUpdateFreq.value = state.updateFrequency;
        if (d.option && Array.isArray(d.option)) {
          state.updateFreqOptions = d.option;
        }
      },
    };

    var ssePatterns = [
      {
        re: /^text-button_(\d+)_config$/,
        fn: function (m, val) {
          var slot = parseInt(m[1], 10);
          if (slot < 1 || slot > NUM_SLOTS) return;
          var b = state.buttons[slot - 1];
          var migrateSliderDirection = buttonConfigHasLegacySliderDirection(val || "");
          var parsed = parseButtonConfig(val || "");
          b.entity = parsed.entity;
          b.label = parsed.label;
          b.icon = parsed.icon;
          b.icon_on = parsed.icon_on;
          b.sensor = parsed.sensor;
          b.unit = parsed.unit;
          b.type = parsed.type;
          b.precision = parsed.precision;
          if (migrateSliderDirection) saveButtonConfig(slot);
          scheduleRender();
        },
      },
      {
        re: /^text-subpage_(\d+)_config$/,
        fn: function (m, val) {
          var slot = parseInt(m[1], 10);
          if (slot < 1 || slot > NUM_SLOTS) return;
          if (!state.subpageRaw[slot]) state.subpageRaw[slot] = { main: "", ext: "", ext2: "", ext3: "" };
          state.subpageRaw[slot].main = val || "";
          applySubpageRaw(slot);
        },
      },
      {
        re: /^text-subpage_(\d+)_config_ext$/,
        fn: function (m, val) {
          var slot = parseInt(m[1], 10);
          if (slot < 1 || slot > NUM_SLOTS) return;
          if (!state.subpageRaw[slot]) state.subpageRaw[slot] = { main: "", ext: "", ext2: "", ext3: "" };
          state.subpageRaw[slot].ext = val || "";
          applySubpageRaw(slot);
        },
      },
      {
        re: /^text-subpage_(\d+)_config_ext_2$/,
        fn: function (m, val) {
          var slot = parseInt(m[1], 10);
          if (slot < 1 || slot > NUM_SLOTS) return;
          if (!state.subpageRaw[slot]) state.subpageRaw[slot] = { main: "", ext: "", ext2: "", ext3: "" };
          state.subpageRaw[slot].ext2 = val || "";
          applySubpageRaw(slot);
        },
      },
      {
        re: /^text-subpage_(\d+)_config_ext_3$/,
        fn: function (m, val) {
          var slot = parseInt(m[1], 10);
          if (slot < 1 || slot > NUM_SLOTS) return;
          if (!state.subpageRaw[slot]) state.subpageRaw[slot] = { main: "", ext: "", ext2: "", ext3: "" };
          state.subpageRaw[slot].ext3 = val || "";
          applySubpageRaw(slot);
        },
      },
    ];

    function handleState(d) {
      rememberEntityPostPath(d);
      var keys = entityStateKeys(d);
      var id = keys[0] || d.id;
      var val = d.state != null ? String(d.state) : "";

      for (var ki = 0; ki < keys.length; ki++) {
        if (sseHandlers[keys[ki]]) { sseHandlers[keys[ki]](val, d); return; }
      }
      if (isFirmwareVersionEvent(id, d)) {
        setFirmwareVersion(val);
        return;
      }
      if (isFirmwareUpdateEvent(id, d)) {
        setFirmwareUpdateInfo(d);
        return;
      }

      for (var i = 0; i < ssePatterns.length; i++) {
        for (var pk = 0; pk < keys.length; pk++) {
          var m = keys[pk].match(ssePatterns[i].re);
          if (m) { ssePatterns[i].fn(m, val, d); return; }
        }
      }

      console.log("[state] unhandled:", id, val);
    }

    markConnected();
    if (!eventStreamEnabled()) {
      loadInitialState(handleState);
      return;
    }

    var source = new EventSource("/events");
    _eventSource = source;

    source.addEventListener("open", markConnected);
    source.addEventListener("error", function () {
      handleDisconnected(source);
    });
    source.addEventListener("state", function (e) {
      var d;
      try { d = JSON.parse(e.data); } catch (_) { return; }
      handleState(d);
    });

  }

  function syncInput(el, val) {
    if (el && document.activeElement !== el) el.value = val;
  }

  function gridHasAny() {
    for (var i = 0; i < NUM_SLOTS; i++) { if (state.grid[i] > 0) return true; }
    return false;
  }

  function scheduleMigration() {
    if (orderReceived || gridHasAny()) return;
    clearTimeout(migrationTimer);
    migrationTimer = setTimeout(function () {
      if (orderReceived || gridHasAny()) return;
      var pos = 0;
      for (var i = 0; i < NUM_SLOTS; i++) {
        if (state.buttons[i].entity && pos < NUM_SLOTS) {
          state.grid[pos] = i + 1;
          pos++;
        }
      }
      if (pos > 0) {
        renderPreview();
        renderButtonSettings();
        postText("Button Order", serializeGrid(state.grid));
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
    if (!els.temp) return;
    var show = state.clockBarOn && (state._indoorOn || state._outdoorOn);
    els.temp.className = "sp-temp" + (show ? " sp-visible" : "");
    var unit = clockBarTemperatureUnitSymbol();
    var indoor = state._indoorVal != null ? state._indoorVal : "24";
    var outdoor = state._outdoorVal != null ? state._outdoorVal : "17";
    if (state._indoorOn && state._outdoorOn) {
      els.temp.textContent = outdoor + unit + " / " + indoor + unit;
    } else if (state._outdoorOn) {
      els.temp.textContent = outdoor + unit;
    } else if (state._indoorOn) {
      els.temp.textContent = indoor + unit;
    }
  }

  if (typeof globalThis !== "undefined" && globalThis.__ESPCONTROL_TEST_HOOKS__) {
    globalThis.__ESPCONTROL_TEST_HOOKS__.config = {
      parseButtonConfig: parseButtonConfig,
      serializeButtonConfig: serializeButtonConfig,
      parseSubpageConfig: parseSubpageConfig,
      serializeSubpageConfig: serializeSubpageConfig,
      subpageStateDisplayMode: subpageStateDisplayMode,
      normalizeTemperatureUnit: normalizeTemperatureUnit,
      previewHtmlValue: previewHtmlValue,
      importedButtonOrderFor: function (orderStr, existingSizes) {
        var oldSizes = state.sizes;
        var oldGrid = state.grid;
        state.sizes = existingSizes || {};
        state.grid = [];
        for (var i = 0; i < NUM_SLOTS; i++) state.grid.push(0);
        applyImportedButtonOrder(orderStr, {});
        var sizes = {};
        for (var k in state.sizes) sizes[k] = state.sizes[k];
        var grid = state.grid.slice();
        state.sizes = oldSizes;
        state.grid = oldGrid;
        return { grid: grid, sizes: sizes };
      },
      screensaverTimeoutSupportedFor: function (value, limitsLoaded, min, max) {
        var oldLoaded = state.screensaverTimeoutLimitsLoaded;
        var oldMin = state.screensaverTimeoutMin;
        var oldMax = state.screensaverTimeoutMax;
        state.screensaverTimeoutLimitsLoaded = !!limitsLoaded;
        state.screensaverTimeoutMin = min;
        state.screensaverTimeoutMax = max;
        var supported = screensaverTimeoutSupported(value);
        state.screensaverTimeoutLimitsLoaded = oldLoaded;
        state.screensaverTimeoutMin = oldMin;
        state.screensaverTimeoutMax = oldMax;
        return supported;
      },
      temperatureUnitSymbolFor: function (timezone, unit) {
        var oldTimezone = state.timezone;
        var oldUnit = state.temperatureUnit;
        state.timezone = timezone || oldTimezone;
        state.temperatureUnit = normalizeTemperatureUnit(unit);
        var symbol = temperatureUnitSymbol();
        state.timezone = oldTimezone;
        state.temperatureUnit = oldUnit;
        return symbol;
      },
    };
  }

  // ── Start ──────────────────────────────────────────────────────────────

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", init);
  } else {
    init();
  }
})();
