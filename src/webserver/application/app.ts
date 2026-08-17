import type { AppTitleFeature } from "./app_title";
import type { CoreFeature } from "./core";
import type { ScreenRotationFeature } from "./screen_rotation_state";
import type { ClockBarFeature } from "./clock_bar_state";
import type { ControlsShellFeature } from "./controls_shell";
import type { AppEventsFeature } from "./app_events";
import type { AppStatusPreviewFeature } from "./app_status_preview";
import type { ButtonSettingsSelectionFeature } from "./button_settings_selection";
import type { PreviewContextMenuFeature } from "./preview_context_menu";
import type { PreviewInteractionsFeature } from "./preview_interactions";
import type { PreviewRenderFeature } from "./preview_render";
import type { ButtonSettingsFeature } from "./button_settings";

declare const __ESPCONTROL_EMBEDDED_MDI_STYLES__: string;

export interface AppFeature {
    init(): void;
}

export function createAppFeature(pageTitle: AppTitleFeature, webStyles: string, core: Pick<CoreFeature, "syncPreviewOrientation">, screenRotation: ScreenRotationFeature, clockBar: ClockBarFeature, shell: Pick<ControlsShellFeature, "buildUI" | "syncTabChrome">, appEvents: Pick<AppEventsFeature, "connect">, statusPreview: Pick<AppStatusPreviewFeature, "updateClock">, selection: Pick<ButtonSettingsSelectionFeature, "handleDocumentSelectionMouseDown">, contextMenu: Pick<PreviewContextMenuFeature, "hide">, interactions: Pick<PreviewInteractionsFeature, "setup">, preview: Pick<PreviewRenderFeature, "render">, buttonSettings: Pick<ButtonSettingsFeature, "render">): AppFeature {
    const { buildUI, syncTabChrome } = shell;
    const { syncPreviewOrientation } = core;
    const { startInitialCheck: startInitialScreenRotationCheck } = screenRotation;
    const { syncUi: syncClockBarUi } = clockBar;
    const { render: renderPreview } = preview;
    const { render: renderButtonSettings } = buttonSettings;
    // ── Init ───────────────────────────────────────────────────────────────
    const FAVICON_SVG: any = '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><path fill="#5c73e7" d="M12,3L2,12H5V20H19V12H22L12,3M12,8.5C14.34,8.5 16.46,9.43 18,10.94L16.8,12.12C15.58,10.91 13.88,10.17 12,10.17C10.12,10.17 8.42,10.91 7.2,12.12L6,10.94C7.54,9.43 9.66,8.5 12,8.5M12,11.83C13.4,11.83 14.67,12.39 15.6,13.3L14.4,14.47C13.79,13.87 12.94,13.5 12,13.5C11.06,13.5 10.21,13.87 9.6,14.47L8.4,13.3C9.33,12.39 10.6,11.83 12,11.83M12,15.17C12.94,15.17 13.7,15.91 13.7,16.83C13.7,17.75 12.94,18.5 12,18.5C11.06,18.5 10.3,17.75 10.3,16.83C10.3,15.91 11.06,15.17 12,15.17Z"/></svg>';
    function setFavicon(this: any) {
        var link: any = document.querySelector('link[rel="icon"]') || document.createElement("link");
        link.rel = "icon";
        link.type = "image/svg+xml";
        link.href = "data:image/svg+xml," + encodeURIComponent(FAVICON_SVG);
        if (!link.parentNode)
            document.head.appendChild(link);
    }
    function setViewportMeta(this: any) {
        var meta: any = document.querySelector('meta[name="viewport"]') || document.createElement("meta");
        meta.name = "viewport";
        meta.content = "width=device-width,initial-scale=1";
        if (!meta.parentNode)
            document.head.appendChild(meta);
    }
    function addSupportButton(this: any) {
        if (document.querySelector(".sp-support-btn"))
            return;
        var panel: any = document.createElement("div");
        panel.className = "sp-support-btn";
        var link: any = document.createElement("a");
        link.className = "sp-support-link";
        link.href = "https://www.buymeacoffee.com/jtenniswood";
        link.target = "_blank";
        link.rel = "noopener";
        link.textContent = "Buy me a coffee";
        panel.appendChild(link);
        document.body.appendChild(panel);
        syncTabChrome();
    }
    function installLocalWebAssets(this: any) {
        if (document.getElementById("espcontrol-local-web-assets"))
            return;
        var style: any = document.createElement("style");
        style.id = "espcontrol-local-web-assets";
        style.textContent = __ESPCONTROL_EMBEDDED_MDI_STYLES__;
        document.head.appendChild(style);
    }
    function init(this: any) {
        setViewportMeta();
        setFavicon();
        pageTitle.applyPageTitle();
        pageTitle.loadPageTitleFromEventStream();
        // Set CSS custom properties from the active device orientation.
        syncPreviewOrientation();
        startInitialScreenRotationCheck();
        var style: any = document.createElement("style");
        style.textContent = webStyles;
        document.head.appendChild(style);
        installLocalWebAssets();
        buildUI();
        addSupportButton();
        syncClockBarUi();
        interactions.setup();
        renderPreview();
        renderButtonSettings();
        appEvents.connect();
        statusPreview.updateClock();
        document.addEventListener("click", contextMenu.hide);
        document.addEventListener("mousedown", selection.handleDocumentSelectionMouseDown);
        document.addEventListener("scroll", contextMenu.hide, true);
        document.addEventListener("keydown", function (this: any, e?: any) {
            if (e.key === "Escape")
                contextMenu.hide();
        });
    }
    return {
        init,
    };
}
