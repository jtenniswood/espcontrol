// ── Clock (minute-aligned) ─────────────────────────────────────────────
// @web-module-requires: state, environment_state, clock_bar_state

function getTzId(tz) {
  if (typeof isHomeAssistantAutoTimezone === "function" && isHomeAssistantAutoTimezone(tz)) return "UTC";
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
  if (typeof isHomeAssistantAutoTimezone === "function" && isHomeAssistantAutoTimezone(opt)) return opt;
  var tzId = getTzId(opt);
  var offset = timezoneOffsetMinutes(tzId, webserverNow());
  if (offset == null || !isFinite(offset)) return opt;
  return tzId + " (" + formatGmtOffset(offset) + ")";
}

function appendTimezoneOption(select, opt) {
  var o = document.createElement("option");
  o.value = opt;
  o.textContent = formatTimezoneOption(opt);
  select.appendChild(o);
}

function updateClockText() {
  if (!els.clock) return;
  var now = webserverNow();
  var tzId = getTzId(effectiveTimezoneOptionForWeb(state.timezone));
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
}

function updateClock() {
  updateClockText();
  var now = webserverNow();
  var msToNext = (60 - now.getSeconds()) * 1000 - now.getMilliseconds();
  setTimeout(updateClock, msToNext + 50);
}

function clockBarTemperatureActive() {
  return clockBarTemperatureVisible();
}

function clockBarTemperatureItemId(index) {
  return index === 0 ? "temperature" : "temperature_" + (index + 1);
}

function clockBarTemperatureItemIndex(item) {
  if (item === "temperature") return 0;
  return -1;
}

function isClockBarTemperatureItem(item) {
  return clockBarTemperatureItemIndex(item) >= 0;
}

function clockBarTemperatureItemIds() {
  return ["temperature"];
}

function clockBarItems() {
  var items = ["temperature", "time"];
  if (voiceServicesSupported()) items.push("voice");
  items.push("network");
  return items;
}

function clockBarDefaultSection(item) {
  if (isClockBarTemperatureItem(item)) return "left";
  if (item === "time") return "middle";
  if (item === "voice") return "right";
  if (item === "network") return "right";
  return "left";
}

function clockBarItemActive(item) {
  var tempIndex = clockBarTemperatureItemIndex(item);
  if (tempIndex >= 0) return clockBarTemperatureVisible();
  if (item === "time") return !!state.clockBarTimeOn;
  if (item === "voice") return voiceServicesSupported() && !!state.voiceServicesOn;
  if (item === "network") return !!state.networkStatusOn;
  return false;
}

function clockBarItemElement(item) {
  return els.clockBarItems && els.clockBarItems[item] || null;
}

function clockBarItemLabel(item) {
  if (isClockBarTemperatureItem(item)) return "Temperature";
  if (item === "time") return "Clock";
  if (item === "voice") return "Voice Services";
  if (item === "network") return "Connectivity";
  return "Clock Bar";
}

function createClockBarItemElement(item, section) {
  var button = document.createElement("div");
  button.className = "sp-clockbar-item sp-clockbar-" + (isClockBarTemperatureItem(item) ? "temperature" : item);
  button.setAttribute("data-clockbar-item", item);
  button.setAttribute("data-clockbar-section", section);
  button.setAttribute("aria-label", clockBarItemLabel(item));
  button.setAttribute("role", "button");
  button.setAttribute("tabindex", "0");

  if (isClockBarTemperatureItem(item)) {
    var temp = document.createElement("span");
    temp.className = "sp-temp";
    temp.textContent = "--";
    button.appendChild(temp);
    if (!els.temps) els.temps = {};
    els.temps[item] = temp;
  } else if (item === "time") {
    var clock = document.createElement("span");
    clock.className = "sp-clock";
    clock.textContent = "--:--";
    button.appendChild(clock);
    els.clock = clock;
  } else if (item === "network") {
    var network = document.createElement("span");
    network.className = "sp-network-preview mdi mdi-wifi-strength-4";
    button.appendChild(network);
    els.networkPreview = network;
  } else if (item === "voice") {
    var voice = document.createElement("span");
    voice.className = "sp-voice-preview mdi mdi-microphone";
    button.appendChild(voice);
    els.voicePreview = voice;
  }
  return button;
}

function renderClockBarLayout() {
  if (!els.clockBarSections) return;
  var layout = {
    left: ["temperature"],
    middle: ["time"],
    right: voiceServicesSupported() ? ["voice", "network"] : ["network"],
  };
  els.clockBarItems = {};
  els.temps = {};
  els.clock = null;
  els.networkPreview = null;
  els.voicePreview = null;
  ["left", "middle", "right"].forEach(function (section) {
    var container = els.clockBarSections[section];
    if (!container) return;
    container.innerHTML = "";
    var rendered = 0;
    layout[section].forEach(function (item) {
      var itemEl = createClockBarItemElement(item, section);
      container.appendChild(itemEl);
      els.clockBarItems[item] = itemEl;
      rendered++;
    });
    container.className = "sp-clockbar-section sp-clockbar-" + section + (rendered ? "" : " sp-clockbar-section-empty");
  });
  updateTempPreview();
  updateClockText();
  updateNetworkPreview();
  updateVoicePreview();
}

function syncClockBarItemElement(item) {
  var el = clockBarItemElement(item);
  if (!el) return;
  var active = clockBarItemActive(item);
  el.className = el.className
    .replace(/\s?sp-clockbar-hidden/g, "")
    .replace(/\s?sp-selected/g, "");
  if (!active) el.className += " sp-clockbar-hidden";
  if (state.clockBarSelectedItem === item) el.className += " sp-selected";
  el.setAttribute("title", clockBarItemLabel(item));
  el.setAttribute("aria-pressed", state.clockBarSelectedItem === item ? "true" : "false");
}

function updateClockBarItemUi() {
  renderClockBarLayout();
  clockBarItems().forEach(syncClockBarItemElement);
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
      postText(entityName("button_order"), serializeGrid(state.grid));
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
  var parts = [];
  if (state.sunrise) parts.push("Sunrise: " + state.sunrise);
  if (state.sunset) parts.push("Sunset: " + state.sunset);
  el.textContent = parts.join(" \u00a0/\u00a0 ");
}

function updateTempPreview() {
  if (!els.temps) return;
  var show = clockBarVisibleInPreview();
  var unit = clockBarTemperatureUnitSymbol();
  var sampleValues = ["17", "24", "21", "19", "22", "18"];
  clockBarTemperatureItemIds().forEach(function (item, index) {
    var el = els.temps[item];
    if (!el) return;
    var configured = primaryClockBarTemperatureEntity();
    var value = configured ? (sampleValues[index] || "--") : "--";
    if (index === 0 && state._outdoorVal != null) value = state._outdoorVal;
    el.className = "sp-temp" + (show ? " sp-visible" : "");
    el.textContent = value + unit;
  });
}

function normalizeNetworkTransport(value) {
  value = String(value == null ? "" : value).trim().toLowerCase();
  return value === "ethernet" ? "ethernet" : "wifi";
}

function normalizeWifiStrengthPercent(value) {
  var n = parseFloat(value);
  if (!isFinite(n)) return 100;
  if (n < 0) return 0;
  if (n > 100) return 100;
  return n;
}

function networkPreviewIconSlug(transport, strengthPercent) {
  if (normalizeNetworkTransport(transport) === "ethernet") return "ethernet";
  var strength = normalizeWifiStrengthPercent(strengthPercent);
  if (strength < 25) return "wifi-strength-1";
  if (strength < 50) return "wifi-strength-2";
  if (strength < 75) return "wifi-strength-3";
  return "wifi-strength-4";
}

function updateNetworkPreview() {
  if (!els.networkPreview) return;
  var show = clockBarVisibleInPreview();
  els.networkPreview.className = "sp-network-preview mdi mdi-" +
    networkPreviewIconSlug(state.networkTransport, state.wifiStrengthPercent) +
    (show ? " sp-visible" : "");
}

function updateVoicePreview() {
  if (!els.voicePreview) return;
  var show = clockBarVisibleInPreview();
  els.voicePreview.className = "sp-voice-preview mdi mdi-microphone" +
    (show && voiceServicesSupported() && state.voiceServicesOn ? " sp-visible" : "");
}
