import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installControlsShellModule(): GlobalDescriptors {
    // ── Build UI ───────────────────────────────────────────────────────────
    function createMdiIcon(this: any, name?: any, className?: any) {
        var icon: any = document.createElement("span");
        icon.className = (className || "mdi") + " mdi-" + name;
        icon.setAttribute("aria-hidden", "true");
        return icon;
    }
    function createActionButton(this: any, className?: any, label?: any, iconName?: any, ariaLabel?: any) {
        var btn: any = document.createElement("button");
        btn.type = "button";
        btn.className = className;
        if (ariaLabel)
            btn.setAttribute("aria-label", ariaLabel);
        if (iconName)
            btn.appendChild(createMdiIcon(iconName));
        if (label)
            btn.appendChild(document.createTextNode(label));
        return btn;
    }
    function createDisclosureChevron(this: any, className?: any) {
        var wrap: any = document.createElement("span");
        wrap.className = className;
        var svg: any = document.createElementNS("http://www.w3.org/2000/svg", "svg");
        svg.setAttribute("viewBox", "0 0 24 24");
        svg.setAttribute("fill", "none");
        svg.setAttribute("stroke", "currentColor");
        svg.setAttribute("stroke-width", "2.5");
        svg.setAttribute("stroke-linecap", "round");
        svg.setAttribute("stroke-linejoin", "round");
        var path: any = document.createElementNS("http://www.w3.org/2000/svg", "path");
        path.setAttribute("d", "M6 9l6 6 6-6");
        svg.appendChild(path);
        wrap.appendChild(svg);
        return wrap;
    }
    function showBanner(this: any, msg?: any, type?: any) {
        if (!els.banner)
            return;
        els.banner.textContent = msg;
        els.banner.className = "sp-banner sp-" + type;
        if (type === "error" || type === "success" || type === "warning") {
            clearTimeout(els._bannerTimer);
            els._bannerTimer = setTimeout(function (this: any) {
                els.banner.className = "sp-banner";
            }, 6000);
        }
    }
    function buildUI(this: any) {
        var root: any = document.createElement("div");
        root.id = "sp-app";
        var banner: any = document.createElement("div");
        banner.className = "sp-banner";
        root.appendChild(banner);
        els.banner = banner;
        buildHeader(root);
        buildScreenPage(root);
        buildSettingsPage(root);
        var app: any = document.querySelector("esp-app");
        if (app) {
            app.parentNode.insertBefore(root, app);
        }
        else {
            document.body.insertBefore(root, document.body.firstChild);
        }
        els.root = root;
        switchTab("screen");
    }
    function buildHeader(this: any, parent?: any) {
        var header: any = document.createElement("div");
        header.className = "sp-header";
        var brand: any = document.createElement("div");
        brand.className = "sp-brand";
        brand.textContent = "EspControl";
        header.appendChild(brand);
        var nav: any = document.createElement("nav");
        nav.className = "sp-nav";
        nav.setAttribute("aria-label", "Primary");
        var tabs: any = [
            { id: "screen", label: "Screen" },
            { id: "settings", label: "Settings" },
        ];
        tabs.forEach(function (this: any, t?: any) {
            var tab: any = document.createElement("div");
            tab.className = "sp-tab";
            tab.setAttribute("role", "tab");
            tab.setAttribute("aria-selected", "false");
            tab.textContent = t.label;
            tab.addEventListener("click", function (this: any) { switchTab(t.id); });
            nav.appendChild(tab);
            els["tab_" + t.id] = tab;
        });
        var docsLink: any = document.createElement("a");
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
    function buildScreenPage(this: any, parent?: any) {
        var page: any = document.createElement("div");
        page.id = "sp-screen";
        page.className = "sp-page";
        var selectionBar: any = document.createElement("div");
        selectionBar.className = "sp-selection-bar";
        els.selectionBar = selectionBar;
        page.appendChild(selectionBar);
        var wrap: any = document.createElement("div");
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
        var hint: any = document.createElement("div");
        hint.className = "sp-hint";
        hint.textContent = "tap to select \u2022 shift/ctrl+tap to multi-select \u2022 right click to manage";
        els.previewHint = hint;
        page.appendChild(hint);
        var overlay: any = document.createElement("div");
        overlay.className = "sp-settings-overlay";
        var modal: any = document.createElement("div");
        modal.className = "sp-settings-modal";
        var closeBtn: any = document.createElement("button");
        closeBtn.className = "sp-settings-close";
        closeBtn.innerHTML = '<svg class="sp-settings-close-icon" viewBox="0 0 24 24" aria-hidden="true" focusable="false">' +
            '<path d="M18.3 5.71 12 12l6.3 6.29-1.41 1.41L10.59 13.41 4.29 19.7 2.88 18.29 9.17 12 2.88 5.71 4.29 4.3l6.3 6.29 6.3-6.29z"></path>' +
            '</svg>';
        closeBtn.setAttribute("aria-label", "Close settings");
        closeBtn.addEventListener("click", closeSettings);
        modal.appendChild(closeBtn);
        var config: any = document.createElement("div");
        config.className = "sp-config";
        els.buttonSettings = config;
        modal.appendChild(config);
        overlay.appendChild(modal);
        overlay.addEventListener("click", function (this: any, e?: any) {
            if (e.target === overlay)
                closeSettings();
        });
        page.appendChild(overlay);
        els.settingsOverlay = overlay;
        page.appendChild(buildApplyBar());
        parent.appendChild(page);
        els.screenPage = page;
    }
    // ── Shell controls ───────────────────────────────────────────────────
    function buildApplyBar(this: any) {
        var bar: any = document.createElement("div");
        bar.className = "sp-apply-bar";
        var btn: any = document.createElement("button");
        btn.className = "sp-apply-btn";
        btn.textContent = "Apply Configuration";
        btn.addEventListener("click", function (this: any) {
            if (isConfigLocked())
                return;
            if (document.activeElement && "blur" in document.activeElement) {
                (document.activeElement as HTMLElement).blur();
            }
            setConfigLocked(true, "Restarting device\u2026");
            setTimeout(function (this: any) {
                postButtonPress("Apply Configuration").then(function (this: any, response?: any) {
                    if (response && response.ok) {
                        waitForReboot();
                    }
                    else if (response) {
                        setConfigLocked(false);
                    }
                });
            }, 600);
        });
        bar.appendChild(btn);
        var note: any = document.createElement("div");
        note.className = "sp-apply-note";
        note.textContent = "Restarts the device to apply changes";
        bar.appendChild(note);
        return bar;
    }
    function switchTab(this: any, tab?: any) {
        state.activeTab = tab;
        if (els.root)
            els.root.setAttribute("data-active-tab", tab);
        ["screen", "settings"].forEach(function (this: any, t?: any) {
            els["tab_" + t].className = "sp-tab" + (tab === t ? " active" : "");
            els["tab_" + t].setAttribute("aria-selected", tab === t ? "true" : "false");
        });
        els.screenPage.className = "sp-page" + (tab === "screen" ? " active" : "");
        els.settingsPage.className = "sp-page" + (tab === "settings" ? " active" : "");
        syncTabChrome();
    }
    function syncTabChrome(this: any) {
        var support: any = document.querySelector(".sp-support-btn");
        if (support)
            support.classList.toggle("sp-support-hidden", state.activeTab === "settings");
    }
    function isConfigLocked(this: any) {
        return !!state.configLocked;
    }
    function syncConfigLockUi(this: any) {
        if (els.root) {
            els.root.classList.toggle("sp-config-locked", isConfigLocked());
        }
        if (els.previewMain) {
            els.previewMain.setAttribute("aria-disabled", isConfigLocked() ? "true" : "false");
        }
        if (els.root) {
            var text: any = "Waiting for device\u2026";
            if ((state.configLockReason || "").indexOf("Restarting") !== -1)
                text = "Restarting\u2026";
            els.root.querySelectorAll(".sp-apply-btn").forEach(function (this: any, btn?: any) {
                btn.disabled = isConfigLocked();
                btn.textContent = isConfigLocked() ? text : "Apply Configuration";
            });
        }
        updatePreviewHint();
    }
    function setConfigLocked(this: any, locked?: any, reason?: any) {
        var nextLocked: any = !!locked;
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
            if (dragSrcEl) {
                dragSrcEl.classList.remove("sp-dragging");
                dragSrcEl = null;
            }
            dragSrcPos = -1;
            previewDropIdx = -1;
            clearPlaceholder();
        }
        syncConfigLockUi();
        if (els.previewMain)
            renderPreview();
    }
    return {
        "createMdiIcon": staticGlobal(createMdiIcon),
        "createActionButton": staticGlobal(createActionButton),
        "createDisclosureChevron": staticGlobal(createDisclosureChevron),
        "showBanner": staticGlobal(showBanner),
        "buildUI": staticGlobal(buildUI),
        "buildHeader": staticGlobal(buildHeader),
        "buildScreenPage": staticGlobal(buildScreenPage),
        "buildApplyBar": staticGlobal(buildApplyBar),
        "switchTab": staticGlobal(switchTab),
        "syncTabChrome": staticGlobal(syncTabChrome),
        "isConfigLocked": staticGlobal(isConfigLocked),
        "syncConfigLockUi": staticGlobal(syncConfigLockUi),
        "setConfigLocked": staticGlobal(setConfigLocked),
    };
}
