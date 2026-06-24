// ── Build UI ───────────────────────────────────────────────────────────

function createMdiIcon(name, className) {
  var icon = document.createElement("span");
  icon.className = (className || "mdi") + " mdi-" + name;
  icon.setAttribute("aria-hidden", "true");
  return icon;
}

function createDisclosureChevron(className) {
  var wrap = document.createElement("span");
  wrap.className = className;
  var svg = document.createElementNS("http://www.w3.org/2000/svg", "svg");
  svg.setAttribute("viewBox", "0 0 24 24");
  svg.setAttribute("fill", "none");
  svg.setAttribute("stroke", "currentColor");
  svg.setAttribute("stroke-width", "2.5");
  svg.setAttribute("stroke-linecap", "round");
  svg.setAttribute("stroke-linejoin", "round");
  var path = document.createElementNS("http://www.w3.org/2000/svg", "path");
  path.setAttribute("d", "M6 9l6 6 6-6");
  svg.appendChild(path);
  wrap.appendChild(svg);
  return wrap;
}

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
  root.setAttribute("data-preview-theme", CFG.previewTheme || "default");

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
  docsLink.appendChild(document.createTextNode("Docs "));
  docsLink.appendChild(createMdiIcon("arrow-top-right"));
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
    '<div class="sp-clockbar-section sp-clockbar-left" data-clockbar-section="left"></div>' +
    '<div class="sp-clockbar-section sp-clockbar-middle" data-clockbar-section="middle"></div>' +
    '<div class="sp-clockbar-section sp-clockbar-right" data-clockbar-section="right"></div>' +
    "</div>" +
    '<div class="sp-main"></div>' +
    "</div>";
  page.appendChild(wrap);

  els.topbar = wrap.querySelector(".sp-topbar");
  els.clockBarSections = {
    left: wrap.querySelector('[data-clockbar-section="left"]'),
    middle: wrap.querySelector('[data-clockbar-section="middle"]'),
    right: wrap.querySelector('[data-clockbar-section="right"]'),
  };
  els.clockBarItems = {};
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
  closeBtn.textContent = "x";
  closeBtn.setAttribute("aria-label", "Close settings");
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

// ── Shell controls ───────────────────────────────────────────────────

function buildApplyBar() {
  var bar = document.createElement("div");
  bar.className = "sp-apply-bar";
  var btn = document.createElement("button");
  btn.className = "sp-apply-btn";
  btn.textContent = "Apply Configuration";
  btn.addEventListener("click", function () {
    if (isConfigLocked()) return;
    if (document.activeElement && document.activeElement.blur) {
      document.activeElement.blur();
    }
    setConfigLocked(true, "Restarting device\u2026");
    setTimeout(function () {
      postButtonPress("Apply Configuration").then(function (response) {
        if (response && response.ok) {
          waitForReboot();
        } else if (response) {
          setConfigLocked(false);
        }
      });
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
  if (els.root) els.root.setAttribute("data-active-tab", tab);
  ["screen", "settings"].forEach(function (t) {
    els["tab_" + t].className = "sp-tab" + (tab === t ? " active" : "");
    els["tab_" + t].setAttribute("aria-selected", tab === t ? "true" : "false");
  });
  els.screenPage.className = "sp-page" + (tab === "screen" ? " active" : "");
  els.settingsPage.className = "sp-page" + (tab === "settings" ? " active" : "");
  syncTabChrome();
}

function syncTabChrome() {
  var support = document.querySelector(".sp-support-btn");
  if (support) support.classList.toggle("sp-support-hidden", state.activeTab === "settings");
}

function isConfigLocked() {
  return !!state.configLocked;
}

function syncConfigLockUi() {
  if (els.root) {
    els.root.classList.toggle("sp-config-locked", isConfigLocked());
  }
  if (els.previewMain) {
    els.previewMain.setAttribute("aria-disabled", isConfigLocked() ? "true" : "false");
  }
  if (els.root) {
    var text = "Waiting for device\u2026";
    if ((state.configLockReason || "").indexOf("Restarting") !== -1) text = "Restarting\u2026";
    els.root.querySelectorAll(".sp-apply-btn").forEach(function (btn) {
      btn.disabled = isConfigLocked();
      btn.textContent = isConfigLocked() ? text : "Apply Configuration";
    });
  }
  updatePreviewHint();
}

function setConfigLocked(locked, reason) {
  var nextLocked = !!locked;
  state.configLocked = nextLocked;
  state.configLockReason = nextLocked ? (reason || "Reconnecting to device\u2026") : "";

  if (nextLocked) {
    hideContextMenu();
    hideSettingsOverlay();
    state.settingsDraft = null;
    state.selectedSlots = [];
    state.lastClickedSlot = -1;
    state.subpageSelectedSlots = [];
    state.subpageLastClicked = -1;
    if (dragSrcEl) { dragSrcEl.classList.remove("sp-dragging"); dragSrcEl = null; }
    dragSrcPos = -1;
    previewDropIdx = -1;
    clearPlaceholder();
  }

  syncConfigLockUi();
  if (els.previewMain) renderPreview();
}
