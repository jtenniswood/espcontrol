// =============================================================================
// ESPCONTROL WEB UI - Custom device configuration interface
// =============================================================================
// Replaces the default ESPHome webserver UI with a two-tab layout:
//   Screen  - Live grid preview with drag-and-drop button arrangement
//   Settings - Display, brightness, firmware, and entity configuration
//
// Per-device config, web UI modules, and button type plugins are injected by
// scripts/build.py. Button types stay in src/webserver/types; the larger UI
// sections live in src/webserver/modules.
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

  function isPortraitRotation(value) {
    value = String(value == null ? "0" : value);
    return value === "90" || value === "270";
  }

  function activeLayout() {
    if (isPortraitRotation(state.screenRotation) && CFG.portrait) return CFG.portrait;
    return CFG;
  }

  function screenWidthPercent(screen) {
    var width = screen && screen.width;
    if (typeof width !== "string") return null;
    var match = width.trim().match(/^([0-9]+(?:\.[0-9]+)?)%$/);
    if (!match) return null;
    var pct = parseFloat(match[1]);
    return isFinite(pct) && pct > 0 ? pct : null;
  }

  function previewLayoutScale(layout) {
    var baseWidth = screenWidthPercent(CFG.screen);
    var activeWidth = screenWidthPercent((layout && layout.screen) || CFG.screen);
    if (!baseWidth || !activeWidth) return 1;
    return baseWidth / activeWidth;
  }

  function layoutSection(layout, key) {
    return (layout && layout[key]) || CFG[key] || {};
  }

  function scaledCqw(value, scale) {
    value = parseFloat(value);
    if (!isFinite(value)) value = 0;
    return (value * scale) + "cqw";
  }

  function scaledCqwText(value, scale) {
    return String(value || "").replace(/(-?[0-9]+(?:\.[0-9]+)?)cqw/g, function (_, num) {
      return scaledCqw(num, scale);
    });
  }

  function previewWrapContentWidth() {
    var wrap = document.querySelector(".sp-wrap");
    if (!wrap) return 0;
    var style = window.getComputedStyle ? window.getComputedStyle(wrap) : null;
    var padding = style
      ? (parseFloat(style.paddingLeft) || 0) + (parseFloat(style.paddingRight) || 0)
      : 0;
    return Math.max(0, wrap.clientWidth - padding);
  }

  function previewFontSize(value, scale, layout) {
    value = parseFloat(value);
    if (!isFinite(value)) value = 0;
    layout = layout || activeLayout();
    scale = scale || previewLayoutScale(layout);
    var screen = (layout && layout.screen) || CFG.screen;
    var widthPercent = screenWidthPercent(screen);
    var wrapWidth = previewWrapContentWidth();
    if (!widthPercent || !wrapWidth) return scaledCqw(value, scale);
    return ((value * scale * widthPercent * wrapWidth) / 10000) + "px";
  }

  function syncPreviewGridTop(layout, scale) {
    var grid = layoutSection(layout || activeLayout(), "grid");
    scale = scale || previewLayoutScale(layout || activeLayout());
    var compactTop = grid.compactTop != null ? grid.compactTop : grid.bottom;
    var gridTop = state.clockBarOn ? grid.top : compactTop;
    document.documentElement.style.setProperty("--grid-top", scaledCqw(gridTop, scale));
  }

  function syncPreviewStyleVars(layout, scale) {
    var r = document.documentElement.style;
    var topbar = layoutSection(layout, "topbar");
    var grid = layoutSection(layout, "grid");
    var btn = layoutSection(layout, "btn");
    var sensorBadge = layoutSection(layout, "sensorBadge");
    var emptyCell = layoutSection(layout, "emptyCell");
    var subpageBadge = layoutSection(layout, "subpageBadge");

    r.setProperty("--topbar-h", scaledCqw(topbar.height, scale));
    r.setProperty("--topbar-pad", scaledCqwText(topbar.padding, scale));
    r.setProperty("--topbar-fs", previewFontSize(topbar.fontSize, scale, layout));
    if (topbar.clockFontSize) r.setProperty("--clock-fs", previewFontSize(topbar.clockFontSize, scale, layout));
    else r.removeProperty("--clock-fs");

    syncPreviewGridTop(layout, scale);
    r.setProperty("--grid-left", scaledCqw(grid.left, scale));
    r.setProperty("--grid-right", scaledCqw(grid.right, scale));
    r.setProperty("--grid-bottom", scaledCqw(grid.bottom, scale));
    r.setProperty("--grid-gap", scaledCqw(grid.gap, scale));

    r.setProperty("--btn-r", scaledCqw(btn.radius, scale));
    r.setProperty("--btn-pad", scaledCqw(btn.padding, scale));
    r.setProperty("--btn-icon", previewFontSize(btn.iconSize, scale, layout));
    r.setProperty("--btn-label", previewFontSize(btn.labelSize, scale, layout));
    r.setProperty("--btn-lines", String(btn.labelLines || 1));
    r.setProperty("--btn-lines-dbl", String(btn.labelLinesDouble || btn.labelLines || 1));

    r.setProperty("--sensor-top", scaledCqw(sensorBadge.top, scale));
    r.setProperty("--sensor-right", scaledCqw(sensorBadge.right, scale));
    r.setProperty("--sensor-fs", previewFontSize(sensorBadge.fontSize, scale, layout));
    r.setProperty("--empty-r", scaledCqw(emptyCell.radius, scale));
    r.setProperty("--subpage-bottom", scaledCqw(subpageBadge.bottom, scale));
    r.setProperty("--subpage-right", scaledCqw(subpageBadge.right, scale));
    r.setProperty("--subpage-fs", previewFontSize(subpageBadge.fontSize, scale, layout));
    r.setProperty("--add-icon", previewFontSize(2.8, scale, layout));
  }

  function syncPreviewOrientation() {
    var layout = activeLayout();
    var screen = layout.screen || CFG.screen;
    var scale = previewLayoutScale(layout);
    GRID_COLS = layout.cols || CFG.cols;
    GRID_ROWS = layout.rows || Math.ceil(NUM_SLOTS / GRID_COLS);

    var r = document.documentElement.style;
    r.setProperty("--screen-w", screen.width || CFG.screen.width);
    r.setProperty("--screen-aspect", screen.aspect || CFG.screen.aspect);
    r.setProperty("--grid-cols", "repeat(" + GRID_COLS + "," + CFG.grid.fr + ")");
    r.setProperty("--grid-rows", "repeat(" + GRID_ROWS + "," + CFG.grid.fr + ")");
    syncPreviewStyleVars(layout, scale);
    var largeSensorUnitOffsetPercent = typeof CFG.largeSensorUnitOffsetPercent === "number"
      ? CFG.largeSensorUnitOffsetPercent : -10;
    r.setProperty("--large-sensor-unit-offset-y",
      "calc(var(--btn-icon) * 2.5 * " + (largeSensorUnitOffsetPercent / 100) + ")");

    if (state.grid && state.grid.length) {
      clearSpans(state.grid, NUM_SLOTS);
      applySpans(state.grid, state.sizes, NUM_SLOTS);
    }
    if (state.editingSubpage) {
      var sp = getSubpage(state.editingSubpage);
      clearSpans(sp.grid, NUM_SLOTS);
      applySpans(sp.grid, sp.sizes, NUM_SLOTS);
    }
  }

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
    "Weather Night Cloudy": "weather-night-partly-cloudy",
  };
  var ICON_NAMES = [
    "Account", "Air Conditioner", "Air Filter", "Air Purifier", "Air Purifier Off", "Alarm",
    "Alarm Light", "Application", "Arrow Down", "Arrow Up", "Battery", "Battery 10%",
    "Battery 20%", "Battery 30%", "Battery 40%", "Battery 50%", "Battery 60%", "Battery 70%",
    "Battery 80%", "Battery 90%", "Battery Alert", "Battery Charging", "Battery Charging 100", "Battery Charging 70",
    "Battery High", "Battery Low", "Battery Medium", "Battery Off", "Battery Outline", "Battery Unknown",
    "Bed", "Bell", "Blinds", "Blinds Horizontal", "Blinds Horizontal Closed", "Blinds Open",
    "Bluetooth", "Broom", "Camera", "Car Electric", "Cast", "CCTV",
    "Ceiling Fan", "Ceiling Light", "Ceiling Lights Multiple", "Chandelier", "Check", "Chevron Down",
    "Chevron Up", "Clock", "Coffee Maker", "Current AC", "Current DC", "Curtains",
    "Curtains Closed", "Delete", "Delete Empty", "Delete Outline", "Desk Lamp", "Dishwasher",
    "Dog", "Downlight", "Door", "Door Open", "Doorbell", "Dots Horizontal",
    "Dryer", "EV Charger", "Fan", "Fire", "Fire Off", "Fireplace",
    "Flash", "Floor Lamp", "Fountain", "Fridge", "Gamepad", "Garage",
    "Garage Open", "Garage Open Variant", "Garage Variant", "Gate", "Gate Open", "Gesture Tap",
    "Gauge", "Gauge Empty", "Gauge Full", "Gauge Low", "Grid Export", "Grid Import",
    "Grid Off", "Headphones", "Radiator", "Radiator Off", "Home", "Heat Pump",
    "Heat Wave", "Heating Coil", "HVAC", "HVAC Off", "Hot Tub", "Humidifier",
    "Humidity Alert", "Iron", "Kettle", "Key", "Lamp", "Lamp Outline",
    "LAN", "Lawnmower", "Leaf", "LED Strip", "LED Strip Variant", "LED Strip Variant Off",
    "Light Switch", "Lightbulb", "Lightbulb Group", "Lightbulb Group Outline", "Lightbulb Night", "Lightbulb Night Outline",
    "Lightbulb Off", "Lightbulb On Outline", "Lightbulb Spot", "Lightbulb Spot Off", "Lightbulb Variant", "Lightbulb Variant Outline",
    "Lightbulb Outline", "Lightning Bolt", "Lock", "Lock Open", "Lock Open Outline", "Lock Outline",
    "Mailbox", "Message Video", "Medication", "Medication Outline", "Meter Electric", "Meter Gas",
    "Microsoft Xbox", "Microwave", "Minus", "Monitor", "Motion Sensor", "Movie Roll",
    "Music", "Outdoor Lamp", "Oven", "Package", "Package Closed", "Pill",
    "Pill Multiple", "Plus", "Play Pause", "Pool", "Power", "Power Plug",
    "Progress Clock", "Printer", "Printer 3D", "Projector", "Projector Off", "Recycle",
    "Robot Vacuum", "Roller Shade", "Roller Shade Closed", "Router", "Router Network", "Security",
    "Shower", "Skip Next", "Skip Previous", "Smoke Detector", "Snowflake", "Snowflake Alert",
    "Snowflake Thermometer", "Sofa", "Solar Panel", "Solar Panel Large", "Solar Power", "Solar Power Variant",
    "Speaker", "Spotlight", "Sprinkler", "Stop", "String Lights", "String Lights Off",
    "Power Socket UK", "Power Socket EU", "Power Socket US", "Sun", "Table", "Television",
    "Television Off", "Thermometer", "Thermometer Alert", "Thermometer High", "Thermometer Low", "Thermometer Off",
    "Thermostat", "Thermostat Box", "Home-Thermostat", "Thermostat Auto", "Thermometer Water", "Timer",
    "Toilet", "Transmission Tower", "Trash Can", "Trash Can Outline", "Volume High", "Wall Outlet",
    "Wall Sconce", "Washing Machine", "Washing Machine Off", "Water", "Water Boiler", "Water Boiler Off",
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
      pickerKey: null,
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

  // __WEB_MODULES_START__
  // __WEB_MODULES_END__
})();
