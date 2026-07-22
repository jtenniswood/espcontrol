import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
import { initializeConfigurationTransaction } from "./configuration_transaction";
export function installAppModule(): GlobalDescriptors {
    // ── Init ───────────────────────────────────────────────────────────────
    var FAVICON_SVG: any = '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><path fill="#5c73e7" d="M12,3L2,12H5V20H19V12H22L12,3M12,8.5C14.34,8.5 16.46,9.43 18,10.94L16.8,12.12C15.58,10.91 13.88,10.17 12,10.17C10.12,10.17 8.42,10.91 7.2,12.12L6,10.94C7.54,9.43 9.66,8.5 12,8.5M12,11.83C13.4,11.83 14.67,12.39 15.6,13.3L14.4,14.47C13.79,13.87 12.94,13.5 12,13.5C11.06,13.5 10.21,13.87 9.6,14.47L8.4,13.3C9.33,12.39 10.6,11.83 12,11.83M12,15.17C12.94,15.17 13.7,15.91 13.7,16.83C13.7,17.75 12.94,18.5 12,18.5C11.06,18.5 10.3,17.75 10.3,16.83C10.3,15.91 11.06,15.17 12,15.17Z"/></svg>';
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
        var link: any = document.createElement("a");
        link.className = "sp-support-btn";
        link.href = "https://www.buymeacoffee.com/jtenniswood";
        link.target = "_blank";
        link.rel = "noopener";
        link.innerHTML = '<img src="https://cdn.buymeacoffee.com/buttons/v2/default-yellow.png" alt="Buy Me A Coffee" height="60" style="border-radius:999px;">';
        document.body.appendChild(link);
        syncTabChrome();
    }
    function init(this: any) {
        void initializeConfigurationTransaction();
        setViewportMeta();
        setFavicon();
        applyPageTitle("");
        loadPageTitleFromEventStream();
        // Set CSS custom properties from the active device orientation.
        syncPreviewOrientation();
        startInitialScreenRotationCheck();
        var style: any = document.createElement("style");
        style.textContent = WEB_STYLES;
        document.head.appendChild(style);
        var mdi: any = document.createElement("link");
        mdi.rel = "stylesheet";
        mdi.href = "https://cdn.jsdelivr.net/npm/@mdi/font@7.4.47/css/materialdesignicons.min.css";
        document.head.appendChild(mdi);
        var fonts: any = document.createElement("link");
        fonts.rel = "stylesheet";
        fonts.href = "https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700&family=Roboto:wght@100;300;400;500;700&display=swap";
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
        document.addEventListener("keydown", function (this: any, e?: any) {
            if (e.key === "Escape")
                hideContextMenu();
        });
    }
    return {
        "FAVICON_SVG": liveGlobal(() => FAVICON_SVG, (value?: any) => { FAVICON_SVG = value; }),
        "setFavicon": staticGlobal(setFavicon),
        "setViewportMeta": staticGlobal(setViewportMeta),
        "addSupportButton": staticGlobal(addSupportButton),
        "init": staticGlobal(init),
    };
}
