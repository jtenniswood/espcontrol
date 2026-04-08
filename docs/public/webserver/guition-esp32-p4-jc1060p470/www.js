// Load the original ESPHome webserver v3 React app (used for API only)
(function () {
  var s = document.createElement("script");
  s.src = "https://oi.esphome.io/v3/www.js";
  document.head.appendChild(s);
})();

// Custom UI: three-page layout (Screen / Settings / Logs)
(function () {
  // __DEVICE_CONFIG_START__
  var DEVICE_ID = "guition-esp32-p4-jc1060p470";
  var CFG = {"slots":20,"cols":5,"rows":4,"dragMode":"swap","dragAnimation":true,"screen":{"width":"100%","aspect":"1024/600"},"topbar":{"height":3.2,"padding":"0.39cqw","fontSize":1.95},"grid":{"top":3.9,"left":0.49,"right":0.49,"bottom":0.49,"gap":0.98,"fr":"1fr"},"btn":{"radius":0.78,"padding":1.37,"iconSize":4.69,"labelSize":1.8},"emptyCell":{"radius":0.78},"sensorBadge":{"top":1,"right":1,"fontSize":1.6},"subpageBadge":{"bottom":1,"right":1,"fontSize":2},"backBtn":{"radius":0.78,"padding":1.37,"iconSize":4.69,"labelSize":1.8}};
  // __DEVICE_CONFIG_END__
  var NUM_SLOTS = CFG.slots;
  var GRID_COLS = CFG.cols;
  var GRID_ROWS = CFG.rows;

  var ICON_MAP = {
    Auto: "cog",
    // --- GENERATED:ICONS START ---
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
    "Grid Export": "transmission-tower-export",
    "Grid Import": "transmission-tower-import",
    "Grid Off": "transmission-tower-off",
    Headphones: "headphones",
    Heater: "radiator",
    Home: "home",
    "Hot Tub": "hot-tub",
    Humidifier: "air-humidifier",
    "Humidity Alert": "water-percent-alert",
    Iron: "iron",
    Kettle: "kettle",
    Key: "key-variant",
    Lamp: "lamp",
    LAN: "lan",
    Lawnmower: "robot-mower",
    Leaf: "leaf",
    "LED Strip": "led-strip",
    "LED Strip Variant": "led-strip-variant",
    "LED Strip Variant Off": "led-strip-variant-off",
    "Light Switch": "light-switch",
    Lightbulb: "lightbulb",
    "Lightbulb Group": "lightbulb-group",
    "Lightbulb Group Outline": "lightbulb-group-outline",
    "Lightbulb Night": "lightbulb-night",
    "Lightbulb Night Outline": "lightbulb-night-outline",
    "Lightbulb Off": "lightbulb-off",
    "Lightbulb Outline": "lightbulb-outline",
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
    "Transmission Tower": "transmission-tower",
    "Trash Can": "trash-can",
    "Wall Outlet": "power-socket",
    Washer: "washing-machine",
    Water: "water",
    "Water Heater": "water-boiler",
    "Water Percent": "water-percent",
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
    // --- GENERATED:ICONS END ---
  };

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
    binary_sensor: "motion-sensor",
    // --- GENERATED:DOMAIN_ICONS END ---
  };

  var ICON_OPTIONS = Object.keys(ICON_MAP).sort();

  var CSS =
    // CSS variables
    ":root{" +
    "--bg:#121212;--surface:#1e1e1e;--surface2:#2a2a2a;--border:#333;" +
    "--text:#e0e0e0;--text2:#999;--accent:#5c9cf5;--accent-hover:#7bb3ff;" +
    "--success:#4caf50;--danger:#ef5350;--radius:10px;--gap:16px}" +

    // App shell
    "#sp-app{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;" +
    "color:var(--text);max-width:960px;margin:0 auto;-webkit-font-smoothing:antialiased}" +
    "esp-app{display:none !important}" +

    // Header
    ".sp-header{display:flex;background:var(--surface);border-bottom:1px solid var(--border);" +
    "position:sticky;top:0;z-index:100;border-radius:0 0 var(--radius) var(--radius);overflow:hidden}" +
    ".sp-tab{flex:1;padding:14px 0;text-align:center;color:var(--text2);cursor:pointer;" +
    "font-size:.9rem;font-weight:500;border-bottom:2px solid transparent;transition:all .2s}" +
    ".sp-tab:hover{color:#bbb}" +
    ".sp-tab.active{color:#fff;border-bottom-color:var(--accent)}" +

    // Pages
    ".sp-page{display:none}.sp-page.active{display:block}" +

    // Fade-in animation
    ".fade-in{animation:fadeIn .3s ease}" +
    "@keyframes fadeIn{from{opacity:0;transform:translateY(8px)}to{opacity:1;transform:translateY(0)}}" +

    // Screen preview
    ".sp-wrap{display:flex;justify-content:center;padding:20px var(--gap) 4px}" +
    ".sp-screen{width:" + CFG.screen.width + ";aspect-ratio:" + CFG.screen.aspect + ";background:#000;" +
    "border-radius:var(--radius);position:relative;overflow:hidden;" +
    "box-shadow:0 2px 20px rgba(0,0,0,.35);border:2px solid var(--surface);" +
    "container-type:inline-size;font-family:Roboto,sans-serif;user-select:none}" +
    ".sp-topbar{position:absolute;top:0;left:0;right:0;height:" + CFG.topbar.height + "cqw;" +
    "display:flex;align-items:center;padding:" + CFG.topbar.padding + ";z-index:1}" +
    ".sp-temp{color:#fff;font-size:" + CFG.topbar.fontSize + "cqw;white-space:nowrap;opacity:0;transition:opacity .3s}" +
    ".sp-temp.sp-visible{opacity:1}" +
    ".sp-clock{position:absolute;left:50%;transform:translateX(-50%);" +
    "color:#fff;font-size:" + CFG.topbar.fontSize + "cqw;white-space:nowrap}" +
    ".sp-main{position:absolute;top:" + CFG.grid.top + "cqw;left:" + CFG.grid.left + "cqw;right:" + CFG.grid.right + "cqw;bottom:" + CFG.grid.bottom + "cqw;" +
    "display:grid;grid-template-columns:repeat(" + GRID_COLS + "," + CFG.grid.fr + ");" +
    "grid-template-rows:repeat(" + GRID_ROWS + "," + CFG.grid.fr + ");gap:" + CFG.grid.gap + "cqw;overflow:hidden}" +

    // Preview buttons
    ".sp-btn{border-radius:" + CFG.btn.radius + "cqw;padding:" + CFG.btn.padding + "cqw;" +
    "display:flex;flex-direction:column;justify-content:space-between;" +
    "cursor:pointer;transition:all .2s;box-sizing:border-box;border:2px solid transparent;" +
    "position:relative}" +
    ".sp-btn:hover{filter:brightness(1.15)}" +
    ".sp-btn.sp-selected{border-color:var(--accent)}" +
    ".sp-btn-icon{font-size:" + CFG.btn.iconSize + "cqw;line-height:1;color:#fff}" +
    ".sp-btn-label{font-size:" + CFG.btn.labelSize + "cqw;line-height:1.2;color:#fff;" +
    "white-space:nowrap;overflow:hidden;text-overflow:ellipsis}" +
    ".sp-sensor-badge{position:absolute;top:" + CFG.sensorBadge.top + "cqw;right:" + CFG.sensorBadge.right + "cqw;font-size:" + CFG.sensorBadge.fontSize + "cqw;opacity:.5}" +
    ".sp-btn-double{grid-row:span 2}" +
    ".sp-empty-cell{border:2px dashed rgba(255,255,255,.15);background:transparent;" +
    "border-radius:" + CFG.emptyCell.radius + "cqw;display:flex;align-items:center;justify-content:center;" +
    "cursor:pointer;transition:border-color .2s}" +
    ".sp-empty-cell:hover{border-color:var(--accent)}" +
    ".sp-empty-cell .sp-add-icon{font-size:5cqw;color:rgba(255,255,255,.2)}" +
    ".sp-drop-placeholder{border:2px dashed rgba(92,156,245,.5) !important;" +
    "background:rgba(92,156,245,.08) !important;cursor:default;pointer-events:none}" +
    (CFG.dragAnimation ? ".sp-btn.sp-dragging{opacity:.4;transform:scale(.95)}" +
    ".sp-empty-cell.sp-drop-placeholder{border-color:rgba(92,156,245,.5)}" : "") +

    // Hint
    ".sp-hint{text-align:center;font-size:.75rem;opacity:.4;padding:6px 0 12px}" +

    // Config area
    ".sp-config{padding:var(--gap) var(--gap) var(--gap)}" +
    ".sp-section-title{font-size:.8rem;font-weight:600;color:var(--text2);" +
    "margin:var(--gap) 0 8px;text-transform:uppercase;letter-spacing:.5px}" +

    // Collapsible cards
    ".card{background:var(--surface);border:1px solid var(--border);" +
    "border-radius:var(--radius);padding:20px;margin-bottom:var(--gap)}" +
    ".card h3{font-size:.8rem;font-weight:600;margin-bottom:14px;color:var(--text2);" +
    "text-transform:uppercase;letter-spacing:.5px}" +
    ".card-header{display:flex;justify-content:space-between;align-items:center;" +
    "cursor:pointer;user-select:none;" +
    "margin:-20px -20px 0 -20px;padding:20px 20px 0 20px}" +
    ".card-header h3{margin:0}" +
    ".card-body{padding-top:18px}" +
    ".card-chevron{display:inline-flex;align-items:center;justify-content:center;" +
    "width:24px;height:24px;color:var(--text2);transition:transform .2s;flex-shrink:0}" +
    ".card-chevron svg{width:100%;height:100%}" +
    ".card.collapsed .card-chevron{transform:rotate(-90deg)}" +
    ".card.collapsed .card-body{display:none}" +
    ".card-header-right{display:flex;align-items:center;gap:8px}" +

    // Settings page panels (legacy, used on screen page)
    ".sp-panel{background:var(--surface);border-radius:var(--radius);padding:20px;" +
    "margin-bottom:var(--gap);border:1px solid var(--border)}" +

    // Form fields
    ".sp-field{margin-bottom:28px}.sp-field:last-child{margin-bottom:0}" +
    ".sp-field-label{display:block;font-size:.85rem;color:var(--text2);margin-bottom:8px}" +
    ".sp-input,.sp-select{width:100%;padding:10px 12px;background:var(--surface2);" +
    "border:1px solid var(--border);border-radius:6px;color:var(--text);font-size:.9rem;" +
    "font-family:inherit;box-sizing:border-box;outline:none;transition:border-color .2s}" +
    ".sp-input:focus,.sp-select:focus{border-color:var(--accent)}" +
    ".sp-input--narrow{width:80px}" +
    ".sp-select{appearance:none;-webkit-appearance:none;" +
    "background-image:url(\"data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='12' height='12' viewBox='0 0 12 12'%3E%3Cpath fill='%23999' d='M6 8L1 3h10z'/%3E%3C/svg%3E\");" +
    "background-repeat:no-repeat;background-position:right 12px center;padding-right:32px}" +
    "select option{background:var(--surface);color:var(--text)}" +

    // Searchable icon picker
    ".sp-icon-picker{position:relative}" +
    ".sp-icon-picker-input{width:100%;padding:10px 12px;padding-left:36px;background:var(--surface2);" +
    "border:1px solid var(--border);border-radius:6px;color:var(--text);font-size:.9rem;" +
    "font-family:inherit;box-sizing:border-box;outline:none;transition:border-color .2s}" +
    ".sp-icon-picker-input:focus{border-color:var(--accent)}" +
    ".sp-icon-picker-input::placeholder{color:#666}" +
    ".sp-icon-picker-preview{position:absolute;left:10px;top:50%;transform:translateY(-50%);" +
    "font-size:18px;color:var(--text2);pointer-events:none}" +
    ".sp-icon-picker.sp-open .sp-icon-picker-preview{top:19px}" +
    ".sp-icon-dropdown{display:none;position:absolute;left:0;right:0;top:100%;margin-top:4px;" +
    "background:var(--surface2);border:1px solid var(--border);border-radius:6px;max-height:200px;" +
    "overflow-y:auto;z-index:50;box-shadow:0 4px 12px rgba(0,0,0,.4)}" +
    ".sp-icon-picker.sp-open .sp-icon-dropdown{display:block}" +
    ".sp-icon-option{display:flex;align-items:center;gap:10px;padding:8px 12px;" +
    "cursor:pointer;font-size:.9rem;color:var(--text);transition:background .1s}" +
    ".sp-icon-option:hover,.sp-icon-option.sp-highlighted{background:#3a3a3a}" +
    ".sp-icon-option.sp-active{background:#1a2a3a}" +
    ".sp-icon-option-icon{font-size:20px;width:24px;text-align:center;color:var(--text2);flex-shrink:0}" +
    ".sp-icon-option-label{white-space:nowrap;overflow:hidden;text-overflow:ellipsis}" +
    ".sp-icon-option--empty{color:#666}" +
    ".sp-btn-row{display:flex;gap:8px;margin-top:16px}" +
    ".sp-action-btn{padding:10px 20px;border:none;border-radius:6px;font-size:.9rem;" +
    "font-weight:500;cursor:pointer;font-family:inherit;transition:background .2s,opacity .2s}" +
    ".sp-action-btn:active{opacity:.8}" +
    ".sp-delete-btn{background:var(--danger);color:#fff}" +

    // Toggle switch
    ".sp-toggle-row{display:flex;align-items:center;justify-content:space-between;" +
    "min-height:36px;margin-bottom:14px}" +
    ".sp-toggle-row:last-child{margin-bottom:0}" +
    ".sp-cond-field+.sp-toggle-row{margin-top:16px}" +
    ".sp-toggle-row span{font-size:.9rem}" +
    ".sp-toggle{position:relative;width:44px;height:24px;flex-shrink:0}" +
    ".sp-toggle input{opacity:0;width:0;height:0;position:absolute}" +
    ".sp-toggle-track{position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;" +
    "background:var(--surface2);border-radius:12px;transition:background .2s;border:1px solid var(--border)}" +
    ".sp-toggle-track:before{content:'';position:absolute;height:18px;width:18px;" +
    "left:2px;top:2px;background:#fff;border-radius:50%;transition:transform .2s}" +
    ".sp-toggle input:checked+.sp-toggle-track{background:var(--accent);border-color:var(--accent)}" +
    ".sp-toggle input:checked+.sp-toggle-track:before{transform:translateX(20px)}" +

    // Segment control
    ".sp-segment{display:flex;border-radius:6px;overflow:hidden;border:1px solid var(--border);margin-bottom:14px}" +
    ".sp-segment button{flex:1;padding:8px 0;background:var(--surface2);color:var(--text2);" +
    "border:none;font-size:.85rem;cursor:pointer;transition:background .2s,color .2s;font-family:inherit}" +
    ".sp-segment button.active{background:var(--accent);color:#fff}" +

    // Conditional field (shown below toggle when enabled)
    ".sp-cond-field{padding:0 0 4px;display:none}" +
    ".sp-cond-field.sp-visible{display:block}" +

    // Range slider
    ".sp-range-row{display:flex;align-items:center;gap:12px;margin-bottom:16px}" +
    ".sp-range-row:last-child{margin-bottom:0}" +
    ".sp-range{flex:1;height:4px;-webkit-appearance:none;appearance:none;background:var(--surface2);" +
    "border-radius:2px;outline:none}" +
    ".sp-range::-webkit-slider-thumb{-webkit-appearance:none;width:18px;height:18px;" +
    "border-radius:50%;background:var(--accent);cursor:pointer}" +
    ".sp-range::-moz-range-thumb{width:18px;height:18px;border-radius:50%;" +
    "background:var(--accent);cursor:pointer;border:none}" +
    ".sp-range-val{min-width:42px;text-align:right;font-size:.85rem;color:var(--text2);" +
    "font-variant-numeric:tabular-nums}" +

    // Color picker
    ".sp-color-row{display:flex;align-items:center;gap:8px;margin-bottom:16px}" +
    ".sp-color-row:last-child{margin-bottom:0}" +
    ".sp-color-swatch{width:40px;height:38px;border-radius:6px;border:1px solid var(--border);" +
    "cursor:pointer;flex-shrink:0;position:relative;overflow:hidden;transition:border-color .2s}" +
    ".sp-color-swatch:hover{border-color:var(--accent)}" +
    ".sp-color-swatch input{position:absolute;inset:-8px;width:calc(100% + 16px);" +
    "height:calc(100% + 16px);cursor:pointer;opacity:0}" +
    ".sp-color-row .sp-input{flex:1}" +

    // Number input
    ".sp-number-row{display:flex;align-items:center;gap:8px;margin-bottom:16px}" +
    ".sp-number-row:last-child{margin-bottom:0}" +
    ".sp-number{width:80px;padding:10px 12px;background:var(--surface2);border:1px solid var(--border);" +
    "border-radius:6px;color:var(--text);font-size:.9rem;font-family:inherit;text-align:center;" +
    "outline:none;box-sizing:border-box}" +
    ".sp-number:focus{border-color:var(--accent)}" +
    ".sp-number-unit{font-size:.85rem;color:var(--text2)}" +

    // Apply bar
    ".sp-apply-bar{padding:var(--gap);text-align:center}" +
    ".sp-apply-btn{background:var(--accent);color:#fff;border:none;border-radius:6px;" +
    "padding:10px 20px;font-size:.9rem;font-weight:500;cursor:pointer;" +
    "font-family:inherit;transition:background .2s,opacity .2s}" +
    ".sp-apply-btn:hover{background:var(--accent-hover)}" +
    ".sp-apply-btn:active{opacity:.8}" +
    ".sp-apply-btn:disabled{opacity:.4;cursor:not-allowed}" +
    ".sp-apply-note{font-size:.75rem;color:var(--text2);margin-top:6px}" +

    // Log viewer
    ".sp-log-toolbar{display:flex;justify-content:flex-end;padding:12px var(--gap) 0}" +
    ".sp-log-clear{background:var(--surface2);color:var(--text);border:1px solid var(--border);" +
    "border-radius:6px;padding:8px 14px;font-size:.8rem;cursor:pointer;font-family:inherit}" +
    ".sp-log-clear:hover{background:var(--border)}" +
    ".sp-log-output{margin:8px var(--gap) var(--gap);padding:12px;background:#0d0d0d;" +
    "border:1px solid var(--border);border-radius:6px;" +
    "font-family:'SF Mono',SFMono-Regular,Menlo,Consolas,monospace;" +
    "font-size:.75rem;line-height:1.5;color:#b0b0b0;overflow-x:auto;overflow-y:auto;" +
    "max-height:70vh;white-space:pre;word-break:break-all}" +
    ".sp-log-line{padding:1px 0;border-left:3px solid transparent;padding-left:6px}" +
    ".sp-log-error{color:var(--danger);border-left-color:var(--danger);background:rgba(239,83,80,.08)}" +
    ".sp-log-warn{color:#fdd835;border-left-color:#fdd835;background:rgba(255,167,38,.06)}" +
    ".sp-log-info{color:var(--success)}" +
    ".sp-log-config{color:#ce93d8}" +
    ".sp-log-debug{color:#aaa}" +
    ".sp-log-verbose{color:#666}" +

    // Empty state
    ".sp-empty{text-align:center;padding:24px;color:#666;font-size:.85rem}" +

    // Context menu
    ".sp-ctx-menu{position:fixed;z-index:200;background:var(--surface);border:1px solid var(--border);" +
    "border-radius:var(--radius);padding:4px 0;min-width:160px;box-shadow:0 4px 16px rgba(0,0,0,.5);" +
    "font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif}" +
    ".sp-ctx-item{display:flex;align-items:center;gap:10px;padding:8px 14px;" +
    "cursor:pointer;font-size:.85rem;color:var(--text);transition:background .1s;white-space:nowrap}" +
    ".sp-ctx-item:hover{background:#3a3a3a}" +
    ".sp-ctx-item .mdi{font-size:16px;width:18px;text-align:center;color:var(--text2)}" +
    ".sp-ctx-item.sp-ctx-danger{color:var(--danger)}" +
    ".sp-ctx-item.sp-ctx-danger .mdi{color:var(--danger)}" +
    ".sp-ctx-divider{height:1px;background:var(--border);margin:4px 0}" +

    // Connection banner
    ".sp-banner{padding:10px var(--gap);font-size:.85rem;text-align:center;display:none}" +
    ".sp-banner.sp-error{display:block;background:var(--danger);color:#fff}" +
    ".sp-banner.sp-offline{display:block;background:#1976d2;color:#fff}" +
    ".sp-banner.sp-success{display:block;background:var(--success);color:#fff}" +
    ".sp-banner.sp-warning{display:block;background:#e6a700;color:#000}" +

    // Backup buttons
    ".sp-backup-btns{display:flex;gap:8px}" +
    ".sp-backup-btn{flex:1;display:flex;align-items:center;justify-content:center;gap:8px;" +
    "padding:10px 16px;border:1px solid var(--border);border-radius:6px;font-size:.85rem;font-weight:500;" +
    "cursor:pointer;font-family:inherit;transition:background .2s;background:var(--surface2);" +
    "color:var(--text)}" +
    ".sp-backup-btn:hover{background:var(--border)}" +
    ".sp-backup-btn .mdi{font-size:16px}" +

    // Sun info
    ".sp-sun-info{font-size:.8rem;color:var(--text2);padding:8px 12px;margin-top:12px;background:var(--surface2);" +
    "border-radius:6px;text-align:center;display:none}" +
    ".sp-sun-info.sp-visible{display:block}" +

    // Field hint
    ".sp-field-hint{font-size:.75rem;color:var(--text2);margin-top:6px}" +

    // Firmware
    ".sp-fw-row{display:flex;align-items:center;justify-content:space-between;gap:8px;min-height:36px;margin-bottom:12px}" +
    ".sp-fw-version{font-size:.9rem;color:var(--text)}" +
    ".sp-fw-label{font-size:.8rem;color:var(--text2)}" +
    ".sp-fw-btn{background:var(--surface2);color:var(--text);border:1px solid var(--border);" +
    "border-radius:6px;padding:8px 14px;font-size:.8rem;font-weight:500;cursor:pointer;" +
    "font-family:inherit;transition:background .2s;white-space:nowrap}" +
    ".sp-fw-btn:hover{background:var(--border)}" +
    ".sp-fw-btn:disabled{opacity:.4;cursor:not-allowed}" +

    // Subpage back button in preview
    ".sp-back-btn{border-radius:" + CFG.backBtn.radius + "cqw;padding:" + CFG.backBtn.padding + "cqw;display:flex;flex-direction:column;" +
    "justify-content:space-between;box-sizing:border-box;border:2px solid transparent;" +
    "position:relative;background:#222;opacity:.6}" +
    ".sp-back-btn .sp-btn-icon{font-size:" + CFG.backBtn.iconSize + "cqw;line-height:1;color:#fff}" +
    ".sp-back-btn .sp-btn-label{font-size:" + CFG.backBtn.labelSize + "cqw;line-height:1.2;color:#fff}" +

    // Subpage chevron overlay on home buttons
    ".sp-subpage-badge{position:absolute;bottom:" + CFG.subpageBadge.bottom + "cqw;right:" + CFG.subpageBadge.right + "cqw;font-size:" + CFG.subpageBadge.fontSize + "cqw;opacity:.5}";

  var state = {
    grid: [],
    sizes: {},
    buttons: [],
    onColor: "FF8C00",
    offColor: "313131",
    selectedSlots: [],
    lastClickedSlot: -1,
    activeTab: "screen",
    _indoorOn: false,
    _outdoorOn: false,
    _indoorVal: null,
    _outdoorVal: null,
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
    subpages: {},
    editingSubpage: null,
    subpageSelectedSlots: [],
    subpageLastClicked: -1,
    clipboard: null,
  };

  for (var i = 0; i < NUM_SLOTS; i++) {
    state.grid.push(0);
    state.buttons.push({ entity: "", label: "", icon: "Auto", icon_on: "Auto", sensor: "", unit: "", type: "" });
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
  var _eventSource = null;

  // ── Helpers ──────────────────────────────────────────────────────────

  function parseOrder(str) {
    var grid = [];
    for (var i = 0; i < NUM_SLOTS; i++) grid.push(0);
    state.sizes = {};
    if (!str || !str.trim()) return grid;
    var parts = str.split(",");
    for (var i = 0; i < parts.length && i < NUM_SLOTS; i++) {
      var s = parts[i].trim();
      if (!s) continue;
      var dbl = s.charAt(s.length - 1) === "d";
      var n = parseInt(s, 10);
      if (n >= 1 && n <= NUM_SLOTS && !isNaN(n)) {
        grid[i] = n;
        if (dbl) state.sizes[n] = 2;
      }
    }
    applySpans(grid);
    return grid;
  }

  function applySpans(grid) {
    for (var i = 0; i < NUM_SLOTS; i++) {
      if (grid[i] > 0 && state.sizes[grid[i]] === 2) {
        var below = i + GRID_COLS;
        if (below < NUM_SLOTS) grid[below] = -1;
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
      return slot + (state.sizes[slot] === 2 ? "d" : "");
    }).join(",");
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

  // ── Subpage helpers ──────────────────────────────────────────────────

  function parseSubpageConfig(str) {
    if (!str || !str.trim()) return { order: [], buttons: [] };
    var parts = str.split("|");
    var order = [];
    if (parts[0]) {
      var op = parts[0].split(",");
      for (var i = 0; i < op.length; i++) {
        var s = op[i].trim();
        if (s) order.push(s);
      }
    }
    var buttons = [];
    for (var i = 1; i < parts.length; i++) {
      var f = parts[i].split(":");
      buttons.push({
        entity: f[0] || "",
        label: f[1] || "",
        icon: f[2] || "Auto",
        icon_on: f[3] || "Auto",
        sensor: f[4] || "",
        unit: f[5] || "",
      });
    }
    return { order: order, buttons: buttons };
  }

  function serializeSubpageConfig(sp) {
    if (!sp || !sp.buttons || sp.buttons.length === 0) return "";
    var out = sp.order.join(",");
    for (var i = 0; i < sp.buttons.length; i++) {
      var b = sp.buttons[i];
      var fields = [b.entity || "", b.label || "", b.icon || "Auto", b.icon_on || "Auto", b.sensor || "", b.unit || ""];
      while (fields.length > 1 && !fields[fields.length - 1]) fields.pop();
      if (fields.length > 1 && fields[fields.length - 1] === "Auto") {
        while (fields.length > 1 && (fields[fields.length - 1] === "Auto" || !fields[fields.length - 1])) fields.pop();
      }
      out += "|" + fields.join(":");
    }
    return out;
  }

  function getSubpage(homeSlot) {
    if (!state.subpages[homeSlot]) {
      state.subpages[homeSlot] = { order: [], buttons: [], grid: [], sizes: {} };
    }
    return state.subpages[homeSlot];
  }

  function buildSubpageGrid(sp) {
    var maxPos = NUM_SLOTS - 1;
    var grid = [];
    for (var i = 0; i < maxPos; i++) grid.push(0);
    sp.sizes = sp.sizes || {};
    if (sp.order.length > 0) {
      for (var i = 0; i < sp.order.length && i < maxPos; i++) {
        var s = sp.order[i];
        var dbl = s.charAt(s.length - 1) === "d";
        var n = parseInt(s, 10);
        if (n >= 1 && n <= sp.buttons.length && !isNaN(n)) {
          grid[i] = n;
          if (dbl) sp.sizes[n] = 2;
        }
      }
      for (var i = 0; i < maxPos; i++) {
        if (grid[i] > 0 && sp.sizes[grid[i]] === 2) {
          var below = i + GRID_COLS;
          if (below < maxPos) grid[below] = -1;
        }
      }
    }
    sp.grid = grid;
    return grid;
  }

  function serializeSubpageGrid(sp) {
    var grid = sp.grid;
    var last = -1;
    for (var i = grid.length - 1; i >= 0; i--) {
      if (grid[i] > 0) { last = i; break; }
    }
    if (last < 0) return [];
    var order = [];
    for (var i = 0; i <= last; i++) {
      if (grid[i] <= 0) { order.push(""); continue; }
      order.push(grid[i] + (sp.sizes[grid[i]] === 2 ? "d" : ""));
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
    var json = serializeSubpageConfig(sp);
    postText("Subpage " + homeSlot + " Config", json);
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
    hint.textContent = "tap to configure \u2022 shift/ctrl+tap to multi-select \u2022 right click to manage";
    els.previewHint = hint;
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
    config.className = "sp-config fade-in";

    // --- Appearance ---
    var appearBody = document.createElement("div");

    appearBody.appendChild(fieldLabel("On Color"));
    var onColor = colorField("sp-set-on-color", "FF8C00", function (hex) {
      postText("Button On Color", hex);
    });
    appearBody.appendChild(onColor);
    els.setOnColor = onColor;

    appearBody.appendChild(fieldLabel("Off Color"));
    var offColor = colorField("sp-set-off-color", "313131", function (hex) {
      postText("Button Off Color", hex);
    });
    appearBody.appendChild(offColor);
    els.setOffColor = offColor;

    config.appendChild(makeCollapsibleCard("Appearance", appearBody, true));

    // --- Brightness ---
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

    config.appendChild(makeCollapsibleCard("Brightness", blBody, true));

    // --- Temperature ---
    var tempBody = document.createElement("div");

    var indoor = createEntityToggleSection("Indoor Temperature", "sp-set-indoor-toggle", state._indoorOn,
      "Indoor Temp Enable", "Indoor Temp Entity", "Indoor Temp Entity", "sensor.indoor_temperature");
    tempBody.appendChild(indoor.toggle.row);
    tempBody.appendChild(indoor.field);
    els.setIndoorToggle = indoor.toggle.input;
    els.setIndoorField = indoor.field;
    els.setIndoorEntity = indoor.input;

    var outdoor = createEntityToggleSection("Outdoor Temperature", "sp-set-outdoor-toggle", state._outdoorOn,
      "Outdoor Temp Enable", "Outdoor Temp Entity", "Outdoor Temp Entity", "sensor.outdoor_temperature");
    tempBody.appendChild(outdoor.toggle.row);
    tempBody.appendChild(outdoor.field);
    els.setOutdoorToggle = outdoor.toggle.input;
    els.setOutdoorField = outdoor.field;
    els.setOutdoorEntity = outdoor.input;

    config.appendChild(makeCollapsibleCard("Temperature", tempBody, true));

    // --- Screensaver ---
    var ssBody = document.createElement("div");
    var ssMode = state.presenceEntity ? "sensor" : "timer";

    ssBody.appendChild(fieldLabel("Mode"));
    var segment = document.createElement("div");
    segment.className = "sp-segment";
    var timerBtn = document.createElement("button");
    timerBtn.textContent = "Timer";
    timerBtn.type = "button";
    var sensorBtn = document.createElement("button");
    sensorBtn.textContent = "Sensor";
    sensorBtn.type = "button";
    segment.appendChild(timerBtn);
    segment.appendChild(sensorBtn);
    ssBody.appendChild(segment);

    // Timer panel
    var timerPanel = document.createElement("div");
    timerPanel.appendChild(fieldLabel("Timeout"));
    var timeoutSelect = document.createElement("select");
    timeoutSelect.className = "sp-select";
    timeoutSelect.id = "sp-set-ss-timeout";
    var timeoutOptions = [
      { label: "5 minutes", value: 300 },
      { label: "10 minutes", value: 600 },
      { label: "15 minutes", value: 900 },
      { label: "20 minutes", value: 1200 },
      { label: "30 minutes", value: 1800 },
      { label: "45 minutes", value: 2700 },
      { label: "1 hour", value: 3600 },
    ];
    timeoutOptions.forEach(function (opt) {
      var o = document.createElement("option");
      o.value = opt.value;
      o.textContent = opt.label;
      if (opt.value === state.screensaverTimeout) o.selected = true;
      timeoutSelect.appendChild(o);
    });
    timeoutSelect.addEventListener("change", function () {
      postNumber("Screensaver Timeout", this.value);
    });
    timerPanel.appendChild(timeoutSelect);
    ssBody.appendChild(timerPanel);
    els.setSSTimeout = timeoutSelect;

    // Sensor panel
    var sensorPanel = document.createElement("div");
    sensorPanel.appendChild(fieldLabel("Presence Entity"));
    var presInp = textInput("sp-set-presence", "", "Presence sensor entity");
    sensorPanel.appendChild(presInp);
    bindTextPost(presInp, "Presence Sensor Entity", {});
    ssBody.appendChild(sensorPanel);
    els.setPresence = presInp;

    function setSsMode(mode) {
      ssMode = mode;
      timerBtn.className = mode === "timer" ? "active" : "";
      sensorBtn.className = mode === "sensor" ? "active" : "";
      timerPanel.style.display = mode === "timer" ? "" : "none";
      sensorPanel.style.display = mode === "sensor" ? "" : "none";
    }
    timerBtn.addEventListener("click", function () { setSsMode("timer"); });
    sensorBtn.addEventListener("click", function () { setSsMode("sensor"); });
    els.setSsMode = setSsMode;
    setSsMode(ssMode);

    config.appendChild(makeCollapsibleCard("Screensaver", ssBody, true));

    // --- Backup ---
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
    config.appendChild(makeCollapsibleCard("Backup", backupBody, true));

    // --- Firmware ---
    var fwBody = document.createElement("div");

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
    fwBody.appendChild(fwVersionRow);

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

    page.appendChild(config);
    page.appendChild(buildApplyBar());

    parent.appendChild(page);
    els.settingsPage = page;
  }

  // ── Settings page helpers ───────────────────────────────────────────

  function makeCollapsibleCard(title, bodyElement, defaultCollapsed) {
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
    range.addEventListener("change", function () { postNumber(postName, this.value); });
    row.appendChild(range);
    row.appendChild(val);
    wrap.appendChild(row);
    return { wrap: wrap, range: range, val: val };
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

    if (state.editingSubpage) {
      renderSubpagePreview(main);
      return;
    }

    if (els.previewHint) els.previewHint.style.display = "";

    for (var pos = 0; pos < NUM_SLOTS; pos++) {
      var slot = state.grid[pos];

      if (slot === -1) continue;

      if (slot > 0) {
        var b = state.buttons[slot - 1];
        var iconName = resolveIcon(slot);
        var label = b.label || b.entity || "Configure";
        var color = state.offColor;
        var isSubpage = b.type === "subpage";

        var btn = document.createElement("div");
        btn.className = "sp-btn" + (state.sizes[slot] === 2 ? " sp-btn-double" : "") + (state.selectedSlots.indexOf(slot) !== -1 ? " sp-selected" : "");
        btn.style.backgroundColor = "#" + (color.length === 6 ? color : "313131");
        btn.draggable = true;
        btn.setAttribute("data-pos", pos);
        var hasWhenOn = !isSubpage && (b.sensor || (b.icon_on && b.icon_on !== "Auto"));
        var badgeIcon = b.sensor ? "gauge" : "swap-horizontal";
        var sensorBadge = hasWhenOn
          ? '<span class="sp-sensor-badge mdi mdi-' + badgeIcon + '"></span>'
          : '';
        var subpageBadge = isSubpage
          ? '<span class="sp-subpage-badge mdi mdi-chevron-right"></span>'
          : '';
        btn.innerHTML =
          sensorBadge + subpageBadge +
          '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>' +
          '<span class="sp-btn-label">' + escHtml(label) + "</span>";
        (function (s, p) {
          btn.addEventListener("click", function (e) {
            if (didDrag) { didDrag = false; return; }
            if (e.shiftKey && state.lastClickedSlot > 0) {
              var anchorPos = state.grid.indexOf(state.lastClickedSlot);
              var curPos = p;
              if (anchorPos !== -1) {
                var from = Math.min(anchorPos, curPos);
                var to = Math.max(anchorPos, curPos);
                state.selectedSlots = [];
                for (var i = from; i <= to; i++) {
                  if (state.grid[i] > 0) state.selectedSlots.push(state.grid[i]);
                }
                renderPreview();
                renderButtonSettings();
                return;
              }
            }
            if (e.ctrlKey || e.metaKey) {
              var idx = state.selectedSlots.indexOf(s);
              if (idx !== -1) {
                state.selectedSlots.splice(idx, 1);
              } else {
                state.selectedSlots.push(s);
                state.lastClickedSlot = s;
              }
              renderPreview();
              renderButtonSettings();
              return;
            }
            if (state.selectedSlots.length === 1 && state.selectedSlots[0] === s) {
              selectButton(-1);
            } else {
              selectButton(s);
            }
          });
          btn.addEventListener("contextmenu", function (e) {
            showContextMenu(e, s);
          });
          setupPreviewDrag(btn, p);
        })(slot, pos);
        main.appendChild(btn);
      } else {
        var empty = document.createElement("div");
        empty.className = "sp-empty-cell";
        empty.setAttribute("data-pos", pos);
        empty.innerHTML = '<span class="sp-add-icon mdi mdi-plus"></span>';
        (function (p) {
          empty.addEventListener("click", function () { addButton(p); });
          empty.addEventListener("contextmenu", function (ev) { showPasteContextMenu(ev, p); });
        })(pos);
        main.appendChild(empty);
      }
    }
  }

  function renderSubpagePreview(main) {
    var homeSlot = state.editingSubpage;
    var sp = getSubpage(homeSlot);
    var homeBtn = state.buttons[homeSlot - 1];
    var maxPos = NUM_SLOTS - 1;

    // Hide hint
    if (els.previewHint) els.previewHint.style.display = "none";

    // Back button (grid position 0)
    var backBtn = document.createElement("div");
    backBtn.className = "sp-back-btn";
    backBtn.innerHTML =
      '<span class="sp-btn-icon mdi mdi-arrow-left"></span>' +
      '<span class="sp-btn-label">Back</span>';
    backBtn.style.cursor = "pointer";
    backBtn.addEventListener("click", exitSubpage);
    main.appendChild(backBtn);

    // Subpage buttons
    for (var pos = 0; pos < maxPos; pos++) {
      var slot = sp.grid[pos];

      if (slot === -1) continue;

      if (slot > 0 && slot <= sp.buttons.length) {
        var b = sp.buttons[slot - 1];
        var iconName = resolveSubpageIcon(b);
        var label = b.label || b.entity || "Configure";
        var color = state.offColor;

        var btn = document.createElement("div");
        btn.className = "sp-btn" +
          (sp.sizes[slot] === 2 ? " sp-btn-double" : "") +
          (state.subpageSelectedSlots.indexOf(slot) !== -1 ? " sp-selected" : "");
        btn.style.backgroundColor = "#" + (color.length === 6 ? color : "313131");
        btn.draggable = true;
        btn.setAttribute("data-pos", pos);
        var hasWhenOn = b.sensor || (b.icon_on && b.icon_on !== "Auto");
        var badgeIcon = b.sensor ? "gauge" : "swap-horizontal";
        var sensorBadge = hasWhenOn
          ? '<span class="sp-sensor-badge mdi mdi-' + badgeIcon + '"></span>'
          : '';
        btn.innerHTML =
          sensorBadge +
          '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>' +
          '<span class="sp-btn-label">' + escHtml(label) + "</span>";
        (function (s, p) {
          btn.addEventListener("click", function (e) {
            if (didDrag) { didDrag = false; return; }
            if (e.shiftKey && state.subpageLastClicked > 0) {
              var anchorPos = sp.grid.indexOf(state.subpageLastClicked);
              var curPos = p;
              if (anchorPos !== -1) {
                var from = Math.min(anchorPos, curPos);
                var to = Math.max(anchorPos, curPos);
                state.subpageSelectedSlots = [];
                for (var i = from; i <= to; i++) {
                  if (sp.grid[i] > 0) state.subpageSelectedSlots.push(sp.grid[i]);
                }
                renderPreview();
                renderButtonSettings();
                return;
              }
            }
            if (e.ctrlKey || e.metaKey) {
              var idx = state.subpageSelectedSlots.indexOf(s);
              if (idx !== -1) {
                state.subpageSelectedSlots.splice(idx, 1);
              } else {
                state.subpageSelectedSlots.push(s);
                state.subpageLastClicked = s;
              }
              renderPreview();
              renderButtonSettings();
              return;
            }
            if (state.subpageSelectedSlots.length === 1 && state.subpageSelectedSlots[0] === s) {
              state.subpageSelectedSlots = [];
            } else {
              state.subpageSelectedSlots = [s];
              state.subpageLastClicked = s;
            }
            renderPreview();
            renderButtonSettings();
          });
          btn.addEventListener("contextmenu", function (e) {
            showSubpageContextMenu(e, s);
          });
          setupPreviewDrag(btn, p);
        })(slot, pos);
        main.appendChild(btn);
      } else {
        var empty = document.createElement("div");
        empty.className = "sp-empty-cell";
        empty.setAttribute("data-pos", pos);
        empty.innerHTML = '<span class="sp-add-icon mdi mdi-plus"></span>';
        (function (p) {
          empty.addEventListener("click", function () { addSubpageButton(p); });
          empty.addEventListener("contextmenu", function (ev) { showSubpagePasteContextMenu(ev, p); });
        })(pos);
        main.appendChild(empty);
      }
    }
  }

  function resolveSubpageIcon(b) {
    var sel = b.icon || "Auto";
    if (sel === "Auto" && b.entity) {
      var domain = b.entity.split(".")[0];
      return DOMAIN_ICONS[domain] || "cog";
    }
    return ICON_MAP[sel] || "cog";
  }

  // ── Button settings panel (shown below preview) ─────────────────────

  function renderButtonSettings() {
    var container = els.buttonSettings;
    container.innerHTML = "";

    if (state.editingSubpage) {
      renderSubpageButtonSettings(container);
      return;
    }

    if (state.selectedSlots.length === 0) return;

    if (state.selectedSlots.length > 1) {
      var hint = document.createElement("div");
      hint.className = "sp-hint";
      hint.textContent = state.selectedSlots.length + " buttons selected \u2022 right click to delete";
      container.appendChild(hint);
      return;
    }

    var slot = state.selectedSlots[0];
    var b = state.buttons[slot - 1];

    var title = document.createElement("div");
    title.className = "sp-section-title";
    title.textContent = "Button Settings";
    container.appendChild(title);

    var panel = document.createElement("div");
    panel.className = "sp-panel";

    // Type selector
    var tf = document.createElement("div");
    tf.className = "sp-field";
    tf.appendChild(fieldLabel("Type"));
    var typeSelect = document.createElement("select");
    typeSelect.className = "sp-select";
    typeSelect.id = "sp-inp-type";
    var typeOpts = [["", "Toggle"], ["subpage", "Subpage"]];
    typeOpts.forEach(function (o) {
      var opt = document.createElement("option");
      opt.value = o[0];
      opt.textContent = o[1];
      if ((b.type || "") === o[0]) opt.selected = true;
      typeSelect.appendChild(opt);
    });
    typeSelect.addEventListener("change", function () {
      var newType = this.value;
      state.buttons[slot - 1].type = newType;
      postText("Button " + slot + " Type", newType);
      if (newType === "subpage") {
        state.buttons[slot - 1].entity = "";
        state.buttons[slot - 1].sensor = "";
        state.buttons[slot - 1].unit = "";
        state.buttons[slot - 1].icon_on = "Auto";
        postText("Button " + slot + " Entity", "");
        postText("Button " + slot + " Sensor", "");
        postText("Button " + slot + " Sensor Unit", "");
        postText("Button " + slot + " Icon On", "Auto");
      }
      renderPreview();
      renderButtonSettings();
    });
    tf.appendChild(typeSelect);
    panel.appendChild(tf);

    // Label field (shown for both types)
    var lf = document.createElement("div");
    lf.className = "sp-field";
    lf.appendChild(fieldLabel("Label"));
    var labelInp = textInput("sp-inp-label", b.label, b.type === "subpage" ? "e.g. Lighting" : "e.g. Kitchen");
    lf.appendChild(labelInp);
    panel.appendChild(lf);

    bindTextPost(labelInp, "Button " + slot + " Label", {
      onBlur: function (v) { state.buttons[slot - 1].label = v; },
      rerender: true,
    });

    if (b.type === "subpage") {
      // Subpage mode: icon + configure button
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

      var configBtn = document.createElement("button");
      configBtn.className = "sp-action-btn";
      configBtn.style.background = "var(--accent)";
      configBtn.style.color = "#fff";
      configBtn.style.width = "100%";
      configBtn.textContent = "Configure Subpage";
      configBtn.addEventListener("click", function () {
        enterSubpage(slot);
      });
      panel.appendChild(configBtn);
    } else {
      // Toggle mode: entity, icon, when-on settings
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

      var hasIconOn = b.icon_on && b.icon_on !== "Auto";
      var hasSensor = !!b.sensor;
      var whenOnEnabled = hasIconOn || hasSensor || !!b._whenOnActive;
      var whenOnMode = b._whenOnMode || (hasSensor ? "sensor" : "icon");

      var whenOnToggle = toggleRow("When Entity On", "sp-inp-whenon-toggle", whenOnEnabled);
      panel.appendChild(whenOnToggle.row);

      var whenOnCond = condField();
      if (whenOnEnabled) whenOnCond.classList.add("sp-visible");

      var seg = document.createElement("div");
      seg.className = "sp-segment";
      var btnIcon = document.createElement("button");
      btnIcon.type = "button";
      btnIcon.textContent = "Replace Icon";
      if (whenOnMode === "icon") btnIcon.classList.add("active");
      var btnSensor = document.createElement("button");
      btnSensor.type = "button";
      btnSensor.textContent = "Sensor Data";
      if (whenOnMode === "sensor") btnSensor.classList.add("active");
      seg.appendChild(btnIcon);
      seg.appendChild(btnSensor);
      whenOnCond.appendChild(seg);

      var iconOnSection = condField();
      if (whenOnMode === "icon") iconOnSection.classList.add("sp-visible");
      var ionLabel = fieldLabel("Icon When On");
      iconOnSection.appendChild(ionLabel);
      var iconOnPicker = document.createElement("div");
      iconOnPicker.className = "sp-icon-picker";
      iconOnPicker.id = "sp-inp-icon-on-picker";
      var iconOnVal = hasIconOn ? b.icon_on : "Auto";
      iconOnPicker.innerHTML =
        '<span class="sp-icon-picker-preview mdi mdi-' + (ICON_MAP[iconOnVal] || "cog") + '"></span>' +
        '<input class="sp-icon-picker-input" id="sp-inp-icon-on" type="text" ' +
        'placeholder="Search icons\u2026" value="' + escAttr(iconOnVal) + '" autocomplete="off">' +
        '<div class="sp-icon-dropdown"></div>';
      iconOnSection.appendChild(iconOnPicker);
      whenOnCond.appendChild(iconOnSection);

      initIconPicker(iconOnPicker, iconOnVal, slot, function (opt) {
        state.buttons[slot - 1].icon_on = opt;
        postText("Button " + slot + " Icon On", opt);
        renderPreview();
      });

      var sensorSection = condField();
      if (whenOnMode === "sensor") sensorSection.classList.add("sp-visible");
      sensorSection.appendChild(fieldLabel("Sensor Entity"));
      var sensorInp = textInput("sp-inp-sensor", b.sensor, "e.g. sensor.printer_percent_complete");
      sensorSection.appendChild(sensorInp);
      sensorSection.appendChild(fieldLabel("Unit"));
      var unitInp = textInput("sp-inp-unit", b.unit, "e.g. %");
      unitInp.className = "sp-input sp-input--narrow";
      sensorSection.appendChild(unitInp);
      var sensorHint = document.createElement("div");
      sensorHint.className = "sp-field-hint";
      sensorHint.textContent = "Show sensor value instead of icon when on";
      sensorSection.appendChild(sensorHint);
      whenOnCond.appendChild(sensorSection);

      panel.appendChild(whenOnCond);

      function setWhenOnMode(mode) {
        whenOnMode = mode;
        state.buttons[slot - 1]._whenOnActive = true;
        state.buttons[slot - 1]._whenOnMode = mode;
        btnIcon.classList.toggle("active", mode === "icon");
        btnSensor.classList.toggle("active", mode === "sensor");
        iconOnSection.classList.toggle("sp-visible", mode === "icon");
        sensorSection.classList.toggle("sp-visible", mode === "sensor");
        if (mode === "icon") {
          sensorInp.value = "";
          unitInp.value = "";
          state.buttons[slot - 1].sensor = "";
          state.buttons[slot - 1].unit = "";
          postText("Button " + slot + " Sensor", "");
          postText("Button " + slot + " Sensor Unit", "");
        } else {
          state.buttons[slot - 1].icon_on = "Auto";
          postText("Button " + slot + " Icon On", "Auto");
          var ionPreview = iconOnPicker.querySelector(".sp-icon-picker-preview");
          if (ionPreview) ionPreview.className = "sp-icon-picker-preview mdi mdi-cog";
          var ionInput = iconOnPicker.querySelector(".sp-icon-picker-input");
          if (ionInput) ionInput.value = "Auto";
        }
        renderPreview();
      }

      btnIcon.addEventListener("click", function () { setWhenOnMode("icon"); });
      btnSensor.addEventListener("click", function () { setWhenOnMode("sensor"); });

      whenOnToggle.input.addEventListener("change", function () {
        if (this.checked) {
          state.buttons[slot - 1]._whenOnActive = true;
          whenOnCond.classList.add("sp-visible");
        } else {
          state.buttons[slot - 1]._whenOnActive = false;
          state.buttons[slot - 1]._whenOnMode = null;
          whenOnCond.classList.remove("sp-visible");
          sensorInp.value = "";
          unitInp.value = "";
          state.buttons[slot - 1].sensor = "";
          state.buttons[slot - 1].unit = "";
          state.buttons[slot - 1].icon_on = "Auto";
          postText("Button " + slot + " Sensor", "");
          postText("Button " + slot + " Sensor Unit", "");
          postText("Button " + slot + " Icon On", "Auto");
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
    }

    container.appendChild(panel);
  }

  // ── Subpage button settings ─────────────────────────────────────────

  function renderSubpageButtonSettings(container) {
    var homeSlot = state.editingSubpage;
    var sp = getSubpage(homeSlot);

    if (state.subpageSelectedSlots.length === 0) return;

    if (state.subpageSelectedSlots.length > 1) {
      var hint = document.createElement("div");
      hint.className = "sp-hint";
      hint.textContent = state.subpageSelectedSlots.length + " buttons selected \u2022 right click to delete";
      container.appendChild(hint);
      return;
    }

    var slot = state.subpageSelectedSlots[0];
    if (slot < 1 || slot > sp.buttons.length) return;
    var b = sp.buttons[slot - 1];

    var title = document.createElement("div");
    title.className = "sp-section-title";
    title.textContent = "Subpage Button Settings";
    container.appendChild(title);

    var panel = document.createElement("div");
    panel.className = "sp-panel";

    // Label
    var lf = document.createElement("div");
    lf.className = "sp-field";
    lf.appendChild(fieldLabel("Label"));
    var labelInp = textInput("sp-sp-inp-label", b.label, "e.g. Kitchen");
    lf.appendChild(labelInp);
    panel.appendChild(lf);

    labelInp.addEventListener("blur", function () {
      b.label = this.value;
      saveSubpageConfig(homeSlot);
      renderPreview();
    });
    labelInp.addEventListener("keydown", function (e) { if (e.key === "Enter") this.blur(); });

    // Entity
    var ef = document.createElement("div");
    ef.className = "sp-field";
    ef.appendChild(fieldLabel("Entity ID"));
    var entityInp = textInput("sp-sp-inp-entity", b.entity, "e.g. light.kitchen");
    ef.appendChild(entityInp);
    panel.appendChild(ef);

    entityInp.addEventListener("blur", function () {
      b.entity = this.value;
      saveSubpageConfig(homeSlot);
      renderPreview();
    });
    entityInp.addEventListener("keydown", function (e) { if (e.key === "Enter") this.blur(); });

    // Icon
    var icf = document.createElement("div");
    icf.className = "sp-field";
    icf.appendChild(fieldLabel("Icon"));
    var iconPicker = document.createElement("div");
    iconPicker.className = "sp-icon-picker";
    iconPicker.id = "sp-sp-icon-picker";
    iconPicker.innerHTML =
      '<span class="sp-icon-picker-preview mdi mdi-' + (ICON_MAP[b.icon] || "cog") + '"></span>' +
      '<input class="sp-icon-picker-input" id="sp-sp-inp-icon" type="text" ' +
      'placeholder="Search icons\u2026" value="' + escAttr(b.icon) + '" autocomplete="off">' +
      '<div class="sp-icon-dropdown"></div>';
    icf.appendChild(iconPicker);
    panel.appendChild(icf);

    initIconPicker(iconPicker, b.icon, slot, function (opt) {
      b.icon = opt;
      saveSubpageConfig(homeSlot);
      renderPreview();
    });

    // Icon On
    var hasIconOn = b.icon_on && b.icon_on !== "Auto";
    var hasSensor = !!b.sensor;
    var whenOnEnabled = hasIconOn || hasSensor || !!b._whenOnActive;
    var whenOnMode = b._whenOnMode || (hasSensor ? "sensor" : "icon");

    var whenOnToggle = toggleRow("When Entity On", "sp-sp-whenon-toggle", whenOnEnabled);
    panel.appendChild(whenOnToggle.row);

    var whenOnCond = condField();
    if (whenOnEnabled) whenOnCond.classList.add("sp-visible");

    var seg = document.createElement("div");
    seg.className = "sp-segment";
    var btnIconSeg = document.createElement("button");
    btnIconSeg.type = "button";
    btnIconSeg.textContent = "Replace Icon";
    if (whenOnMode === "icon") btnIconSeg.classList.add("active");
    var btnSensorSeg = document.createElement("button");
    btnSensorSeg.type = "button";
    btnSensorSeg.textContent = "Sensor Data";
    if (whenOnMode === "sensor") btnSensorSeg.classList.add("active");
    seg.appendChild(btnIconSeg);
    seg.appendChild(btnSensorSeg);
    whenOnCond.appendChild(seg);

    var ionSection = condField();
    if (whenOnMode === "icon") ionSection.classList.add("sp-visible");
    ionSection.appendChild(fieldLabel("Icon When On"));
    var ionPicker = document.createElement("div");
    ionPicker.className = "sp-icon-picker";
    ionPicker.id = "sp-sp-icon-on-picker";
    var ionVal = hasIconOn ? b.icon_on : "Auto";
    ionPicker.innerHTML =
      '<span class="sp-icon-picker-preview mdi mdi-' + (ICON_MAP[ionVal] || "cog") + '"></span>' +
      '<input class="sp-icon-picker-input" id="sp-sp-inp-icon-on" type="text" ' +
      'placeholder="Search icons\u2026" value="' + escAttr(ionVal) + '" autocomplete="off">' +
      '<div class="sp-icon-dropdown"></div>';
    ionSection.appendChild(ionPicker);
    whenOnCond.appendChild(ionSection);

    initIconPicker(ionPicker, ionVal, slot, function (opt) {
      b.icon_on = opt;
      saveSubpageConfig(homeSlot);
      renderPreview();
    });

    var sensorSection = condField();
    if (whenOnMode === "sensor") sensorSection.classList.add("sp-visible");
    sensorSection.appendChild(fieldLabel("Sensor Entity"));
    var sensorInp = textInput("sp-sp-inp-sensor", b.sensor, "e.g. sensor.temperature");
    sensorSection.appendChild(sensorInp);
    sensorSection.appendChild(fieldLabel("Unit"));
    var unitInp = textInput("sp-sp-inp-unit", b.unit, "e.g. %");
    unitInp.className = "sp-input sp-input--narrow";
    sensorSection.appendChild(unitInp);
    whenOnCond.appendChild(sensorSection);

    panel.appendChild(whenOnCond);

    function setSpWhenOnMode(mode) {
      whenOnMode = mode;
      b._whenOnActive = true;
      b._whenOnMode = mode;
      btnIconSeg.classList.toggle("active", mode === "icon");
      btnSensorSeg.classList.toggle("active", mode === "sensor");
      ionSection.classList.toggle("sp-visible", mode === "icon");
      sensorSection.classList.toggle("sp-visible", mode === "sensor");
      if (mode === "icon") {
        sensorInp.value = "";
        unitInp.value = "";
        b.sensor = "";
        b.unit = "";
      } else {
        b.icon_on = "Auto";
      }
      saveSubpageConfig(homeSlot);
      renderPreview();
    }

    btnIconSeg.addEventListener("click", function () { setSpWhenOnMode("icon"); });
    btnSensorSeg.addEventListener("click", function () { setSpWhenOnMode("sensor"); });

    whenOnToggle.input.addEventListener("change", function () {
      if (this.checked) {
        b._whenOnActive = true;
        whenOnCond.classList.add("sp-visible");
      } else {
        b._whenOnActive = false;
        b._whenOnMode = null;
        whenOnCond.classList.remove("sp-visible");
        b.sensor = "";
        b.unit = "";
        b.icon_on = "Auto";
        saveSubpageConfig(homeSlot);
        renderPreview();
      }
    });

    sensorInp.addEventListener("blur", function () {
      b.sensor = this.value;
      saveSubpageConfig(homeSlot);
      renderPreview();
    });
    sensorInp.addEventListener("keydown", function (e) { if (e.key === "Enter") this.blur(); });

    unitInp.addEventListener("blur", function () {
      b.unit = this.value;
      saveSubpageConfig(homeSlot);
    });
    unitInp.addEventListener("keydown", function (e) { if (e.key === "Enter") this.blur(); });

    // Delete button
    var btnRow = document.createElement("div");
    btnRow.className = "sp-btn-row";
    var delBtn = document.createElement("button");
    delBtn.className = "sp-action-btn sp-delete-btn";
    delBtn.textContent = "Delete";
    delBtn.addEventListener("click", function () {
      deleteSubpageButton(slot);
    });
    btnRow.appendChild(delBtn);
    panel.appendChild(btnRow);

    container.appendChild(panel);
  }

  // ── Searchable icon picker ──────────────────────────────────────────

  function initIconPicker(picker, currentIcon, slot, onSelectOpt) {
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
      if (onSelectOpt) {
        onSelectOpt(opt);
      } else {
        state.buttons[slot - 1].icon = opt;
        postText("Button " + slot + " Icon", opt);
        renderPreview();
      }
    }

    function openPicker() {
      if (currentIcon === "Auto") {
        input.value = "";
        buildOptions("");
      } else {
        buildOptions(currentIcon);
        setTimeout(function () { input.select(); }, 0);
      }
      picker.classList.add("sp-open");
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

  function resolveSpanPos(pos) {
    var grid = state.editingSubpage ? getSubpage(state.editingSubpage).grid : state.grid;
    var sizes = state.editingSubpage ? getSubpage(state.editingSubpage).sizes : state.sizes;
    if (grid[pos] === -1) {
      var above = pos - GRID_COLS;
      if (above >= 0 && grid[above] > 0 && sizes[grid[above]] === 2) return above;
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
    var skip = dragIsSubpage ? 1 : 0;
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

  function clearSpans(grid) {
    for (var i = 0; i < NUM_SLOTS; i++) {
      if (grid[i] === -1) grid[i] = 0;
    }
  }

  function moveToCell(fromPos, toPos) {
    toPos = resolveSpanPos(toPos);
    if (state.grid[toPos] === -1) return;
    var grid = state.grid.slice();
    var movingSlot = grid[fromPos];
    clearSpans(grid);
    if (CFG.dragMode === "swap") {
      var targetSlot = grid[toPos];
      grid[toPos] = movingSlot;
      grid[fromPos] = targetSlot;
      for (var i = 0; i < NUM_SLOTS; i++) {
        if (grid[i] > 0 && state.sizes[grid[i]] === 2) {
          var below = i + GRID_COLS;
          if (below < NUM_SLOTS) {
            if (grid[below] > 0) {
              var displaced = grid[below];
              grid[below] = -1;
              for (var j = 0; j < NUM_SLOTS; j++) {
                if (grid[j] === 0) { grid[j] = displaced; break; }
              }
            } else {
              grid[below] = -1;
            }
          }
        }
      }
    } else {
      grid[fromPos] = 0;
      if (grid[toPos] > 0) {
        var displaced = grid[toPos];
        grid[toPos] = 0;
        for (var i = 1; i < NUM_SLOTS; i++) {
          var candidate = (toPos + i) % NUM_SLOTS;
          if (grid[candidate] === 0) { grid[candidate] = displaced; break; }
        }
      }
      grid[toPos] = movingSlot;
      applySpans(grid);
    }
    if (state.sizes[movingSlot] === 2 && toPos + GRID_COLS >= NUM_SLOTS) {
      delete state.sizes[movingSlot];
    }
    state.grid = grid;
  }

  function moveToCellSubpage(fromPos, toPos) {
    var sp = getSubpage(state.editingSubpage);
    var maxPos = NUM_SLOTS - 1;
    toPos = resolveSpanPos(toPos);
    if (toPos >= maxPos || sp.grid[toPos] === -1) return;
    var grid = sp.grid.slice();
    var movingSlot = grid[fromPos];
    clearSpans(grid);
    if (CFG.dragMode === "swap") {
      var targetSlot = grid[toPos];
      grid[toPos] = movingSlot;
      grid[fromPos] = targetSlot;
      for (var i = 0; i < maxPos; i++) {
        if (grid[i] > 0 && sp.sizes[grid[i]] === 2) {
          var below = i + GRID_COLS;
          if (below < maxPos) {
            if (grid[below] > 0) {
              var displaced = grid[below];
              grid[below] = -1;
              for (var j = 0; j < maxPos; j++) {
                if (grid[j] === 0) { grid[j] = displaced; break; }
              }
            } else {
              grid[below] = -1;
            }
          }
        }
      }
    } else {
      grid[fromPos] = 0;
      if (grid[toPos] > 0) {
        var displaced = grid[toPos];
        grid[toPos] = 0;
        for (var i = 1; i < maxPos; i++) {
          var candidate = (toPos + i) % maxPos;
          if (grid[candidate] === 0) { grid[candidate] = displaced; break; }
        }
      }
      grid[toPos] = movingSlot;
      for (var i = 0; i < maxPos; i++) {
        if (grid[i] > 0 && sp.sizes[grid[i]] === 2) {
          var below = i + GRID_COLS;
          if (below < maxPos) grid[below] = -1;
        }
      }
    }
    if (sp.sizes[movingSlot] === 2 && toPos + GRID_COLS >= maxPos) {
      delete sp.sizes[movingSlot];
    }
    sp.grid = grid;
  }

  function clearPlaceholder() {
    if (previewPlaceholder) {
      previewPlaceholder.classList.remove("sp-drop-placeholder");
      previewPlaceholder = null;
    }
  }

  function setupPreviewDropZone() {
    var container = els.previewMain;
    var pendingCellIdx = -1;

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
      if (dragIsSubpage) {
        var maxPos = NUM_SLOTS - 1;
        if (dragSrcPos < 0 || toPos < 0 || toPos >= maxPos) { dragSrcPos = -1; dragIsSubpage = false; return; }
        if (dragSrcPos === toPos) { dragSrcPos = -1; dragIsSubpage = false; return; }
        moveToCellSubpage(dragSrcPos, toPos);
        renderPreview();
        renderButtonSettings();
        saveSubpageConfig(state.editingSubpage);
      } else {
        if (dragSrcPos < 0 || toPos < 0 || toPos >= NUM_SLOTS) { dragSrcPos = -1; return; }
        if (dragSrcPos === toPos) { dragSrcPos = -1; return; }
        moveToCell(dragSrcPos, toPos);
        renderPreview();
        renderButtonSettings();
        postText("Button Order", serializeGrid(state.grid));
      }
      dragSrcPos = -1;
      dragIsSubpage = false;
    });
  }

  function setupPreviewDrag(btn, pos) {
    btn.addEventListener("dragstart", function (e) {
      dragSrcPos = pos;
      if (CFG.dragAnimation) dragSrcEl = btn;
      dragIsSubpage = !!state.editingSubpage;
      didDrag = true;
      dragEnterCount = 0;
      e.dataTransfer.effectAllowed = "move";
      e.dataTransfer.setData("text/plain", String(pos));
      if (CFG.dragAnimation) {
        requestAnimationFrame(function () { btn.classList.add("sp-dragging"); });
      }
    });
    btn.addEventListener("dragend", function () {
      dragSrcPos = -1;
      previewDropIdx = -1;
      dragIsSubpage = false;
      dragEnterCount = 0;
      clearPlaceholder();
      if (dragSrcEl) { dragSrcEl.classList.remove("sp-dragging"); dragSrcEl = null; }
    });
  }

  // ── Button settings panel ───────────────────────────────────────────

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

  // ── Context menu ────────────────────────────────────────────────────

  var ctxMenu = null;

  function showContextMenu(e, slot) {
    e.preventDefault();
    hideContextMenu();

    var isMulti = state.selectedSlots.length > 1 && state.selectedSlots.indexOf(slot) !== -1;

    if (state.selectedSlots.indexOf(slot) === -1 && state.selectedSlots.length > 1) {
      state.selectedSlots.push(slot);
      isMulti = true;
      renderPreview();
      renderButtonSettings();
    }

    ctxMenu = document.createElement("div");
    ctxMenu.className = "sp-ctx-menu";

    if (isMulti) {
      var bulkSlots = state.selectedSlots.slice();
      var cutItem = document.createElement("div");
      cutItem.className = "sp-ctx-item";
      cutItem.innerHTML = '<span class="mdi mdi-content-cut"></span>Cut ' + bulkSlots.length + " Buttons";
      cutItem.addEventListener("mousedown", function (ev) {
        ev.preventDefault();
        hideContextMenu();
        cutButtons(bulkSlots);
      });
      ctxMenu.appendChild(cutItem);
      var delItem = document.createElement("div");
      delItem.className = "sp-ctx-item sp-ctx-danger";
      delItem.innerHTML = '<span class="mdi mdi-delete"></span>Delete ' + bulkSlots.length + " Buttons";
      delItem.addEventListener("mousedown", function (ev) {
        ev.preventDefault();
        hideContextMenu();
        deleteButtons(bulkSlots);
      });
      ctxMenu.appendChild(delItem);
    } else {
      var b = state.buttons[slot - 1];
      if (b && b.type === "subpage") {
        var setupItem = document.createElement("div");
        setupItem.className = "sp-ctx-item";
        setupItem.innerHTML = '<span class="mdi mdi-cog"></span>Setup Subpage';
        setupItem.addEventListener("mousedown", function (ev) {
          ev.preventDefault();
          hideContextMenu();
          enterSubpage(slot);
        });
        ctxMenu.appendChild(setupItem);
        var spDivider = document.createElement("div");
        spDivider.className = "sp-ctx-divider";
        ctxMenu.appendChild(spDivider);
      }

      var isDbl = state.sizes[slot] === 2;
      var dblItem = document.createElement("div");
      dblItem.className = "sp-ctx-item";
      dblItem.innerHTML = '<span class="mdi mdi-arrow-expand-vertical"></span>' + (isDbl ? "Single Height" : "Double Height");
      dblItem.addEventListener("mousedown", function (ev) {
        ev.preventDefault();
        hideContextMenu();
        var slotPos = state.grid.indexOf(slot);
        var belowPos = slotPos + GRID_COLS;
        if (state.sizes[slot] === 2) {
          delete state.sizes[slot];
          if (belowPos < NUM_SLOTS && state.grid[belowPos] === -1) state.grid[belowPos] = 0;
        } else {
          if (belowPos >= NUM_SLOTS) return;
          if (state.grid[belowPos] > 0) {
            var displaced = state.grid[belowPos];
            state.grid[belowPos] = 0;
            var freeCell = firstFreeCell(belowPos + 1);
            if (freeCell >= 0) state.grid[freeCell] = displaced;
          }
          state.sizes[slot] = 2;
          state.grid[belowPos] = -1;
        }
        renderPreview();
        renderButtonSettings();
        postText("Button Order", serializeGrid(state.grid));
      });
      ctxMenu.appendChild(dblItem);

      var dupItem = document.createElement("div");
      dupItem.className = "sp-ctx-item";
      dupItem.innerHTML = '<span class="mdi mdi-content-copy"></span>Duplicate';
      dupItem.addEventListener("mousedown", function (ev) {
        ev.preventDefault();
        hideContextMenu();
        duplicateButton(slot);
      });
      ctxMenu.appendChild(dupItem);

      var cutItem = document.createElement("div");
      cutItem.className = "sp-ctx-item";
      cutItem.innerHTML = '<span class="mdi mdi-content-cut"></span>Cut';
      cutItem.addEventListener("mousedown", function (ev) {
        ev.preventDefault();
        hideContextMenu();
        cutButton(slot);
      });
      ctxMenu.appendChild(cutItem);

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
    }

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

  function addButton(pos) {
    var slot = firstFreeSlot();
    if (slot < 0) return;
    state.grid[pos] = slot;
    postText("Button Order", serializeGrid(state.grid));
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
      icon_on: src.icon_on,
      sensor: src.sensor,
      unit: src.unit,
      type: src.type || "",
    };

    if (state.sizes[srcSlot] === 2) state.sizes[newSlot] = 2;

    var srcPos = state.grid.indexOf(srcSlot);
    var newPos = firstFreeCell(srcPos + 1);
    if (newPos < 0) return;
    state.grid[newPos] = newSlot;
    if (state.sizes[newSlot] === 2) {
      var belowNew = newPos + GRID_COLS;
      if (belowNew < NUM_SLOTS && state.grid[belowNew] === 0) state.grid[belowNew] = -1;
    }

    postText("Button Order", serializeGrid(state.grid));
    postText("Button " + newSlot + " Entity", src.entity);
    postText("Button " + newSlot + " Label", src.label);
    postText("Button " + newSlot + " Sensor", src.sensor);
    postText("Button " + newSlot + " Sensor Unit", src.unit);
    postText("Button " + newSlot + " Icon", src.icon || "Auto");
    postText("Button " + newSlot + " Icon On", src.icon_on || "Auto");
    postText("Button " + newSlot + " Type", src.type || "");
    if (src.type === "subpage" && state.subpages[srcSlot]) {
      var spJson = serializeSubpageConfig(state.subpages[srcSlot]);
      var spCopy = parseSubpageConfig(spJson);
      spCopy.sizes = {};
      buildSubpageGrid(spCopy);
      state.subpages[newSlot] = spCopy;
      postText("Subpage " + newSlot + " Config", spJson);
    }
    selectButton(newSlot);
  }

  function deleteButton(slot) {
    for (var i = 0; i < NUM_SLOTS; i++) {
      if (state.grid[i] === slot) {
        state.grid[i] = 0;
        if (state.sizes[slot] === 2 && i + GRID_COLS < NUM_SLOTS && state.grid[i + GRID_COLS] === -1) {
          state.grid[i + GRID_COLS] = 0;
        }
        break;
      }
    }
    delete state.sizes[slot];
    var pos = state.selectedSlots.indexOf(slot);
    if (pos !== -1) state.selectedSlots.splice(pos, 1);
    renderPreview();
    renderButtonSettings();
    postText("Button Order", serializeGrid(state.grid));
    postText("Button " + slot + " Entity", "");
    postText("Button " + slot + " Label", "");
    postText("Button " + slot + " Sensor", "");
    postText("Button " + slot + " Sensor Unit", "");
    postText("Button " + slot + " Icon", "Auto");
    postText("Button " + slot + " Icon On", "Auto");
    postText("Button " + slot + " Type", "");
    postText("Subpage " + slot + " Config", "");
    state.buttons[slot - 1].type = "";
    delete state.subpages[slot];
  }

  function deleteButtons(slots) {
    for (var i = 0; i < NUM_SLOTS; i++) {
      if (slots.indexOf(state.grid[i]) !== -1) {
        if (state.sizes[state.grid[i]] === 2 && i + GRID_COLS < NUM_SLOTS && state.grid[i + GRID_COLS] === -1) {
          state.grid[i + GRID_COLS] = 0;
        }
        state.grid[i] = 0;
      }
    }
    slots.forEach(function (slot) { delete state.sizes[slot]; });
    state.selectedSlots = [];
    state.lastClickedSlot = -1;
    slots.forEach(function (slot) {
      postText("Button " + slot + " Entity", "");
      postText("Button " + slot + " Label", "");
      postText("Button " + slot + " Sensor", "");
      postText("Button " + slot + " Sensor Unit", "");
      postText("Button " + slot + " Icon", "Auto");
      postText("Button " + slot + " Icon On", "Auto");
      postText("Button " + slot + " Type", "");
      postText("Subpage " + slot + " Config", "");
      state.buttons[slot - 1].type = "";
      delete state.subpages[slot];
    });
    postText("Button Order", serializeGrid(state.grid));
    renderPreview();
    renderButtonSettings();
  }

  // ── Subpage add / delete / context menu ─────────────────────────────

  function addSubpageButton(pos) {
    var homeSlot = state.editingSubpage;
    var sp = getSubpage(homeSlot);
    var maxPos = NUM_SLOTS - 1;
    var newSlot = subpageFirstFreeSlot(sp);
    while (sp.buttons.length < newSlot) {
      sp.buttons.push({ entity: "", label: "", icon: "Auto", icon_on: "Auto", sensor: "", unit: "" });
    }
    sp.grid[pos] = newSlot;
    sp.order = serializeSubpageGrid(sp);
    saveSubpageConfig(homeSlot);
    state.subpageSelectedSlots = [newSlot];
    state.subpageLastClicked = newSlot;
    renderPreview();
    renderButtonSettings();
  }

  function deleteSubpageButton(slot) {
    var homeSlot = state.editingSubpage;
    var sp = getSubpage(homeSlot);
    var maxPos = NUM_SLOTS - 1;
    for (var i = 0; i < maxPos; i++) {
      if (sp.grid[i] === slot) {
        sp.grid[i] = 0;
        if (sp.sizes[slot] === 2 && i + GRID_COLS < maxPos && sp.grid[i + GRID_COLS] === -1) {
          sp.grid[i + GRID_COLS] = 0;
        }
        break;
      }
    }
    delete sp.sizes[slot];
    if (slot >= 1 && slot <= sp.buttons.length) {
      sp.buttons[slot - 1] = { entity: "", label: "", icon: "Auto", icon_on: "Auto", sensor: "", unit: "" };
    }
    sp.order = serializeSubpageGrid(sp);
    state.subpageSelectedSlots = [];
    state.subpageLastClicked = -1;
    saveSubpageConfig(homeSlot);
    renderPreview();
    renderButtonSettings();
  }

  // ── Cut / Paste ────────────────────────────────────────────────────

  function cutButton(slot) {
    var src = state.buttons[slot - 1];
    var entry = {
      entity: src.entity, label: src.label, icon: src.icon,
      icon_on: src.icon_on, sensor: src.sensor, unit: src.unit,
      type: src.type || "", subpageConfig: null, size: state.sizes[slot] || 1,
    };
    if (src.type === "subpage" && state.subpages[slot]) {
      entry.subpageConfig = serializeSubpageConfig(state.subpages[slot]);
    }
    state.clipboard = { buttons: [entry] };
    deleteButton(slot);
  }

  function cutButtons(slots) {
    var entries = [];
    slots.forEach(function (slot) {
      var src = state.buttons[slot - 1];
      var entry = {
        entity: src.entity, label: src.label, icon: src.icon,
        icon_on: src.icon_on, sensor: src.sensor, unit: src.unit,
        type: src.type || "", subpageConfig: null, size: state.sizes[slot] || 1,
      };
      if (src.type === "subpage" && state.subpages[slot]) {
        entry.subpageConfig = serializeSubpageConfig(state.subpages[slot]);
      }
      entries.push(entry);
    });
    state.clipboard = { buttons: entries };
    deleteButtons(slots);
  }

  function cutSubpageButton(slot) {
    var homeSlot = state.editingSubpage;
    var sp = getSubpage(homeSlot);
    var src = sp.buttons[slot - 1];
    state.clipboard = { buttons: [{
      entity: src.entity, label: src.label, icon: src.icon,
      icon_on: src.icon_on, sensor: src.sensor, unit: src.unit,
      type: "", subpageConfig: null, size: sp.sizes[slot] || 1,
    }] };
    deleteSubpageButton(slot);
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
        icon_on: e.icon_on, sensor: e.sensor, unit: e.unit, type: e.type || "",
      };
      if (e.size === 2) state.sizes[newSlot] = 2;
      state.grid[cell] = newSlot;
      if (e.size === 2) {
        var below = cell + GRID_COLS;
        if (below < NUM_SLOTS && state.grid[below] === 0) state.grid[below] = -1;
      }
      postText("Button " + newSlot + " Entity", e.entity);
      postText("Button " + newSlot + " Label", e.label);
      postText("Button " + newSlot + " Sensor", e.sensor);
      postText("Button " + newSlot + " Sensor Unit", e.unit);
      postText("Button " + newSlot + " Icon", e.icon || "Auto");
      postText("Button " + newSlot + " Icon On", e.icon_on || "Auto");
      postText("Button " + newSlot + " Type", e.type || "");
      if (e.subpageConfig) {
        var spCopy = parseSubpageConfig(e.subpageConfig);
        spCopy.sizes = {};
        buildSubpageGrid(spCopy);
        state.subpages[newSlot] = spCopy;
        postText("Subpage " + newSlot + " Config", e.subpageConfig);
      }
      lastSlot = newSlot;
    }
    postText("Button Order", serializeGrid(state.grid));
    state.clipboard = null;
    if (lastSlot > 0) selectButton(lastSlot);
    renderPreview();
    renderButtonSettings();
  }

  function pasteSubpageButton(pos) {
    if (!state.clipboard) return;
    var homeSlot = state.editingSubpage;
    var sp = getSubpage(homeSlot);
    var maxPos = NUM_SLOTS - 1;
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
        sp.buttons.push({ entity: "", label: "", icon: "Auto", icon_on: "Auto", sensor: "", unit: "" });
      }
      var e = entries[i];
      sp.buttons[newSlot - 1] = {
        entity: e.entity, label: e.label, icon: e.icon,
        icon_on: e.icon_on, sensor: e.sensor, unit: e.unit,
      };
      if (e.size === 2) sp.sizes[newSlot] = 2;
      sp.grid[cell] = newSlot;
      if (e.size === 2) {
        var below = cell + GRID_COLS;
        if (below < maxPos && sp.grid[below] === 0) sp.grid[below] = -1;
      }
      lastSlot = newSlot;
    }
    sp.order = serializeSubpageGrid(sp);
    state.clipboard = null;
    saveSubpageConfig(homeSlot);
    if (lastSlot > 0) {
      state.subpageSelectedSlots = [lastSlot];
      state.subpageLastClicked = lastSlot;
    }
    renderPreview();
    renderButtonSettings();
  }

  function showSubpageContextMenu(e, slot) {
    e.preventDefault();
    hideContextMenu();

    ctxMenu = document.createElement("div");
    ctxMenu.className = "sp-ctx-menu";

    var homeSlot = state.editingSubpage;
    var sp = getSubpage(homeSlot);
    var isDbl = sp.sizes[slot] === 2;
    var maxPos = NUM_SLOTS - 1;

    var dblItem = document.createElement("div");
    dblItem.className = "sp-ctx-item";
    dblItem.innerHTML = '<span class="mdi mdi-arrow-expand-vertical"></span>' + (isDbl ? "Single Height" : "Double Height");
    dblItem.addEventListener("mousedown", function (ev) {
      ev.preventDefault();
      hideContextMenu();
      var slotPos = sp.grid.indexOf(slot);
      var belowPos = slotPos + GRID_COLS;
      if (sp.sizes[slot] === 2) {
        delete sp.sizes[slot];
        if (belowPos < maxPos && sp.grid[belowPos] === -1) sp.grid[belowPos] = 0;
      } else {
        if (belowPos >= maxPos) return;
        if (sp.grid[belowPos] > 0) return;
        sp.sizes[slot] = 2;
        sp.grid[belowPos] = -1;
      }
      sp.order = serializeSubpageGrid(sp);
      saveSubpageConfig(homeSlot);
      renderPreview();
      renderButtonSettings();
    });
    ctxMenu.appendChild(dblItem);

    var cutItem = document.createElement("div");
    cutItem.className = "sp-ctx-item";
    cutItem.innerHTML = '<span class="mdi mdi-content-cut"></span>Cut';
    cutItem.addEventListener("mousedown", function (ev) {
      ev.preventDefault();
      hideContextMenu();
      cutSubpageButton(slot);
    });
    ctxMenu.appendChild(cutItem);

    var divider = document.createElement("div");
    divider.className = "sp-ctx-divider";
    ctxMenu.appendChild(divider);

    var delItem = document.createElement("div");
    delItem.className = "sp-ctx-item sp-ctx-danger";
    delItem.innerHTML = '<span class="mdi mdi-delete"></span>Delete';
    delItem.addEventListener("mousedown", function (ev) {
      ev.preventDefault();
      hideContextMenu();
      deleteSubpageButton(slot);
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

  // ── Paste context menus (empty cells) ──────────────────────────────

  function showPasteContextMenu(e, pos) {
    e.preventDefault();
    if (!state.clipboard) return;
    hideContextMenu();
    ctxMenu = document.createElement("div");
    ctxMenu.className = "sp-ctx-menu";
    var count = state.clipboard.buttons.length;
    var pasteItem = document.createElement("div");
    pasteItem.className = "sp-ctx-item";
    pasteItem.innerHTML = '<span class="mdi mdi-content-paste"></span>' + (count > 1 ? "Paste " + count + " Buttons" : "Paste");
    pasteItem.addEventListener("mousedown", function (ev) {
      ev.preventDefault();
      hideContextMenu();
      pasteButton(pos);
    });
    ctxMenu.appendChild(pasteItem);
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

  function showSubpagePasteContextMenu(e, pos) {
    e.preventDefault();
    if (!state.clipboard) return;
    hideContextMenu();
    ctxMenu = document.createElement("div");
    ctxMenu.className = "sp-ctx-menu";
    var count = state.clipboard.buttons.length;
    var pasteItem = document.createElement("div");
    pasteItem.className = "sp-ctx-item";
    pasteItem.innerHTML = '<span class="mdi mdi-content-paste"></span>' + (count > 1 ? "Paste " + count + " Buttons" : "Paste");
    pasteItem.addEventListener("mousedown", function (ev) {
      ev.preventDefault();
      hideContextMenu();
      pasteSubpageButton(pos);
    });
    ctxMenu.appendChild(pasteItem);
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

  // ── Export / Import ─────────────────────────────────────────────────

  function exportConfig() {
    var data = {
      version: 1,
      device: DEVICE_ID,
      exported_at: new Date().toISOString(),
      button_order: serializeGrid(state.grid),
      button_on_color: state.onColor,
      button_off_color: state.offColor,
      buttons: state.buttons.map(function (b) {
        return {
          entity: b.entity,
          label: b.label,
          icon: b.icon,
          icon_on: b.icon_on,
          sensor: b.sensor,
          unit: b.unit,
          type: b.type || "",
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
        if (data.device && data.device !== DEVICE_ID) {
          showBanner("Config was exported from a different panel (" + data.device + ") \u2014 layout may look different", "warning");
        }
        var importedCount = data.buttons.length;
        if (importedCount !== NUM_SLOTS) {
          showBanner("Backup has " + importedCount + " slots, current config has " + NUM_SLOTS + " \u2014 adapting", "warning");
        }

        postText("Button On Color", data.button_on_color || "FF8C00");
        postText("Button Off Color", data.button_off_color || "313131");

        var empty = { entity: "", label: "", icon: "Auto", icon_on: "Auto", sensor: "", unit: "", type: "" };
        var buttons, orderStr, spKeyMap;

        if (importedCount > NUM_SLOTS) {
          // Remap: collect used buttons in grid-visual order, compact into available slots
          var origParts = (data.button_order || "").split(",");
          var usedSlots = [];
          var seen = {};
          for (var j = 0; j < origParts.length; j++) {
            var tok = origParts[j].trim();
            if (!tok) continue;
            var dbl = tok.charAt(tok.length - 1) === "d";
            var num = parseInt(tok, 10);
            if (isNaN(num) || num < 1 || num > importedCount || seen[num]) continue;
            seen[num] = true;
            usedSlots.push({ oldSlot: num, isDouble: dbl });
          }
          for (var j = 0; j < importedCount; j++) {
            var sn = j + 1;
            if (seen[sn]) continue;
            var bb = data.buttons[j];
            if (bb.entity || bb.label || bb.type) {
              usedSlots.push({ oldSlot: sn, isDouble: false });
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
            if (usedSlots[j].isDouble) newSizes[ns] = 2;
          }
          for (var j = limit; j < NUM_SLOTS; j++) buttons.push(empty);

          var newGrid = [];
          for (var j = 0; j < NUM_SLOTS; j++) newGrid.push(0);
          var pos = 0;
          for (var j = 0; j < limit && pos < NUM_SLOTS; j++) {
            var ns = j + 1;
            var isD = newSizes[ns] === 2;
            var row = Math.floor(pos / GRID_COLS);
            if (isD && row >= GRID_ROWS - 1) { isD = false; delete newSizes[ns]; }
            newGrid[pos] = ns;
            if (isD) {
              var bp = pos + GRID_COLS;
              if (bp < NUM_SLOTS) newGrid[bp] = -1;
            }
            pos++;
            while (pos < NUM_SLOTS && newGrid[pos] === -1) pos++;
          }

          state.sizes = newSizes;
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
          postText("Button " + n + " Entity", b.entity || "");
          postText("Button " + n + " Label", b.label || "");
          postText("Button " + n + " Sensor", b.sensor || "");
          postText("Button " + n + " Sensor Unit", b.unit || "");
          postText("Button " + n + " Icon", b.icon || "Auto");
          postText("Button " + n + " Icon On", b.icon_on || "Auto");
          postText("Button " + n + " Type", b.type || "");

          state.buttons[i] = {
            entity: b.entity || "",
            label: b.label || "",
            icon: b.icon || "Auto",
            icon_on: b.icon_on || "Auto",
            sensor: b.sensor || "",
            unit: b.unit || "",
            type: b.type || "",
          };
        }

        state.subpages = {};
        if (data.subpages) {
          for (var k in data.subpages) {
            var newKey = spKeyMap[k];
            if (!newKey) continue;
            var sp = parseSubpageConfig(data.subpages[k]);
            sp.sizes = {};
            buildSubpageGrid(sp);
            state.subpages[String(newKey)] = sp;
            postText("Subpage " + newKey + " Config", data.subpages[k]);
          }
        }

        postText("Button Order", orderStr);
        state.grid = parseOrder(orderStr);
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
          if (els.setSSTimeout) els.setSSTimeout.value = String(state.screensaverTimeout);
          if (els.setSsMode) els.setSsMode(state.presenceEntity ? "sensor" : "timer");
          updateTempPreview();
        }

        state.selectedSlots = [];
        state.lastClickedSlot = -1;
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
        orderReceived = !!(val && val.trim());
        state.grid = parseOrder(val);
        state.selectedSlots = state.selectedSlots.filter(function (s) {
          return state.grid.indexOf(s) !== -1;
        });
        renderPreview();
        renderButtonSettings();
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
        if (els.setSSTimeout) els.setSSTimeout.value = String(state.screensaverTimeout);
      },
      "text-presence_sensor_entity": function (val) {
        state.presenceEntity = val;
        syncInput(els.setPresence, val);
        if (els.setSsMode) els.setSsMode(val ? "sensor" : "timer");
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
        if (els.updateFreqWrap) els.updateFreqWrap.style.display = state.autoUpdate ? "" : "none";
      },
      "select-firmware__update_frequency": function (val, d) {
        state.updateFrequency = d.value || d.option || val || state.updateFrequency;
        if (els.setUpdateFreq) els.setUpdateFreq.value = state.updateFrequency;
        if (d.options && Array.isArray(d.options)) {
          state.updateFreqOptions = d.options;
        }
      },
    };

    var ssePatterns = [
      {
        re: /^text-button_(\d+)_type$/,
        fn: function (m, val) {
          var slot = parseInt(m[1], 10);
          if (slot >= 1 && slot <= NUM_SLOTS) {
            state.buttons[slot - 1].type = val;
            renderPreview();
            renderButtonSettings();
          }
        },
      },
      {
        re: /^text-subpage_(\d+)_config$/,
        fn: function (m, val) {
          var slot = parseInt(m[1], 10);
          if (slot >= 1 && slot <= NUM_SLOTS) {
            var sp = parseSubpageConfig(val);
            sp.sizes = sp.sizes || {};
            buildSubpageGrid(sp);
            state.subpages[slot] = sp;
            if (state.editingSubpage === slot) {
              renderPreview();
              renderButtonSettings();
            }
          }
        },
      },
      {
        re: /^text-button_(\d+)_(entity|label|sensor|sensor_unit)$/,
        fn: function (m, val) {
          var slot = parseInt(m[1], 10);
          var field = m[2] === "sensor_unit" ? "unit" : m[2];
          if (slot >= 1 && slot <= NUM_SLOTS) {
            state.buttons[slot - 1][field] = val;
            renderPreview();
            if (state.selectedSlots.length === 1 && state.selectedSlots[0] === slot && isSettingsFocused()) {
              var idMap = { entity: "sp-inp-entity", label: "sp-inp-label", sensor: "sp-inp-sensor", unit: "sp-inp-unit" };
              syncInput(document.getElementById(idMap[field]), val);
            } else {
              renderButtonSettings();
            }
          }
        },
      },
      {
        re: /^text-button_(\d+)_icon$/,
        fn: function (m, val) {
          var slot = parseInt(m[1], 10);
          if (slot >= 1 && slot <= NUM_SLOTS) {
            state.buttons[slot - 1].icon = val;
            renderPreview();
            if (state.selectedSlots.length === 1 && state.selectedSlots[0] === slot && isSettingsFocused()) {
              syncInput(document.getElementById("sp-inp-icon"), val);
              var prev = document.querySelector(".sp-icon-picker-preview");
              if (prev) prev.className = "sp-icon-picker-preview mdi mdi-" + (ICON_MAP[val] || "cog");
            } else {
              renderButtonSettings();
            }
          }
        },
      },
      {
        re: /^text-button_(\d+)_icon_on$/,
        fn: function (m, val) {
          var slot = parseInt(m[1], 10);
          if (slot >= 1 && slot <= NUM_SLOTS) {
            state.buttons[slot - 1].icon_on = val;
            renderPreview();
            if (state.selectedSlots.length === 1 && state.selectedSlots[0] === slot && isSettingsFocused()) {
              syncInput(document.getElementById("sp-inp-icon-on"), val);
              var prev = document.getElementById("sp-inp-icon-on-picker");
              if (prev) {
                var p = prev.querySelector(".sp-icon-picker-preview");
                if (p) p.className = "sp-icon-picker-preview mdi mdi-" + (ICON_MAP[val] || "cog");
              }
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
    var show = state._indoorOn || state._outdoorOn;
    els.temp.className = "sp-temp" + (show ? " sp-visible" : "");
    var indoor = state._indoorVal != null ? state._indoorVal + "\u00B0" : "24\u00B0";
    var outdoor = state._outdoorVal != null ? state._outdoorVal + "\u00B0" : "17\u00B0";
    if (state._indoorOn && state._outdoorOn) {
      els.temp.textContent = indoor + " / " + outdoor;
    } else if (state._indoorOn) {
      els.temp.textContent = indoor;
    } else if (state._outdoorOn) {
      els.temp.textContent = outdoor;
    }
  }

  // ── Log viewer ──────────────────────────────────────────────────────

  function appendLog(msg, lvl) {
    if (!els.logOutput) return;
    var line = document.createElement("div");
    line.className = "sp-log-line";
    if (lvl === 1) line.classList.add("sp-log-error");
    else if (lvl === 2) line.classList.add("sp-log-warn");
    else if (lvl === 3) line.classList.add("sp-log-info");
    else if (lvl === 4) line.classList.add("sp-log-config");
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
