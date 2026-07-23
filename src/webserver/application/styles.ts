import { state } from "../state/app_instance";
import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installStylesModule(): GlobalDescriptors {
    var WEB_STYLES: any = ":root{" +
        "--bg:#1b1b1f;--surface:#202127;--surface2:#2e2e32;--border:#3c3f44;" +
        "--text:#dfdfd6;--text2:#98989f;--text3:#6a6a71;--accent:#5c73e7;--accent-hover:#a8b1ff;" +
        "--screen-primary:#" + WEB_UI_COLORS.primary + ";--screen-secondary:#" + WEB_UI_COLORS.secondary + ";" +
        "--screen-tertiary:#" + WEB_UI_COLORS.tertiary + ";" +
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
        ".sp-support-btn.sp-support-hidden{display:none}" +
        ".fade-in{animation:fadeIn .3s ease}" +
        "@keyframes fadeIn{from{opacity:0;transform:translateY(8px)}to{opacity:1;transform:translateY(0)}}" +
        ".sp-wrap{display:flex;justify-content:center;padding:16px var(--gap) 4px;user-select:none}" +
        ".sp-screen{width:var(--screen-w);aspect-ratio:var(--screen-aspect);background:#000;" +
        "border-radius:var(--radius);position:relative;overflow:hidden;" +
        "box-shadow:0 2px 20px rgba(0,0,0,.35);border:2px solid var(--surface);" +
        "container-type:inline-size;font-family:Roboto,sans-serif;user-select:none}" +
        ".sp-config-locked .sp-screen{filter:grayscale(1) brightness(.58);opacity:.62;pointer-events:none}" +
        ".sp-config-locked .sp-screen::after{content:'';position:absolute;inset:0;background:rgba(80,80,84,.28);z-index:5}" +
        ".sp-topbar{position:absolute;top:0;left:0;right:0;height:var(--topbar-h);" +
        "display:flex;align-items:center;gap:.5cqw;padding:var(--topbar-pad);z-index:1}" +
        ".sp-topbar.sp-hidden{display:none}" +
        ".sp-clockbar-section{height:100%;min-width:0;flex:1;display:flex;align-items:center;gap:.4cqw;position:relative}" +
        ".sp-clockbar-left{justify-content:flex-start}.sp-clockbar-middle{justify-content:center}.sp-clockbar-right{justify-content:flex-end}" +
        ".sp-clockbar-item{height:min(36px,calc(100% - .45cqw));min-height:0;min-width:28px;border:1px solid transparent;border-radius:calc(var(--topbar-fs)*.3);" +
        "background:transparent;color:#fff;display:flex;align-items:center;justify-content:center;padding:0 calc(var(--topbar-fs)*.28);" +
        "box-sizing:border-box;cursor:pointer;line-height:1;font:inherit;pointer-events:auto;transition:background .2s,border-color .2s,opacity .2s}" +
        ".sp-clockbar-item:hover{background:rgba(255,255,255,.1);border-color:rgba(255,255,255,.26)}" +
        ".sp-clockbar-item.sp-selected{border-color:var(--accent);background:rgba(92,115,231,.18)}" +
        ".sp-clockbar-item.sp-clockbar-hidden{opacity:.42;background:rgba(255,255,255,.08);border-color:rgba(255,255,255,.18)}" +
        ".sp-clockbar-item.sp-clockbar-hidden.sp-selected{opacity:.65;border-color:var(--accent)}" +
        ".sp-clockbar-network,.sp-clockbar-voice{padding:0;width:min(32px,calc(var(--topbar-fs)*1.9));min-width:24px}" +
        ".sp-temp{color:#fff;font-size:var(--topbar-fs);white-space:nowrap;opacity:0;transition:opacity .3s}" +
        ".sp-temp.sp-visible{opacity:1}" +
        ".sp-clock{color:#fff;font-size:var(--clock-fs,var(--topbar-fs));white-space:nowrap;opacity:1;transition:opacity .3s}" +
        ".sp-network-preview,.sp-voice-preview{color:#fff;font-size:calc(var(--topbar-fs)*.86);" +
        "line-height:1;width:calc(var(--topbar-fs)*1.05);height:100%;display:flex;align-items:center;" +
        "justify-content:center;opacity:0;transition:opacity .3s}" +
        ".sp-network-preview.sp-visible,.sp-voice-preview.sp-visible{opacity:1}" +
        ".sp-voice-preview:not(.sp-visible){display:none}" +
        ".sp-main{position:absolute;top:var(--grid-top);left:var(--grid-left);right:var(--grid-right);bottom:var(--grid-bottom);" +
        "display:grid;grid-template-columns:var(--grid-cols);grid-template-rows:var(--grid-rows);gap:var(--grid-gap);overflow:hidden}" +
        ".sp-main.sp-grid-loading{visibility:hidden;pointer-events:none}" +
        ".sp-btn{border-radius:var(--btn-r);padding:var(--btn-pad);" +
        "display:flex;flex-direction:column;justify-content:space-between;" +
        "cursor:pointer;transition:all .2s;box-sizing:border-box;border:var(--btn-border,2px) solid transparent;" +
        "position:relative;overflow:hidden;min-width:0}" +
        ".sp-btn:hover{filter:brightness(1.15)}" +
        ".sp-drag-active .sp-btn:hover{filter:none}" +
        ".sp-btn.sp-selected{border-color:var(--accent)}" +
        ".sp-btn-icon{font-size:var(--btn-icon);line-height:1;color:#fff}" +
        ".sp-media-group-active{box-shadow:inset 0 0 0 3px var(--accent)}" +
        ".sp-media-group-count{position:absolute;top:10px;right:12px;min-width:24px;height:24px;padding:0 6px;border-radius:12px;background:var(--accent);color:#fff;font-size:14px;font-weight:700;display:flex;align-items:center;justify-content:center}" +
        ".sp-btn-label{font-size:var(--btn-label);line-height:1.2;color:#fff;font-weight:var(--btn-label-weight,400);" +
        "display:-webkit-box;-webkit-box-orient:vertical;-webkit-line-clamp:var(--btn-lines);" +
        "overflow:hidden;word-break:break-word;min-height:0}" +
        ".sp-sensor-badge{position:absolute;top:var(--sensor-top);right:var(--sensor-right);font-size:var(--sensor-fs);opacity:.5}" +
        ".sp-sensor-preview{display:flex;align-items:baseline;gap:.18em;color:#fff}" +
        ".sp-climate-temp-card .sp-sensor-preview{position:absolute;left:var(--btn-pad);top:var(--btn-pad)}" +
        ".sp-climate-temp-card .sp-btn-label-row{margin-top:auto}" +
        ".sp-btn-big .sp-sensor-preview-large .sp-sensor-value,.sp-btn-wide .sp-sensor-preview-large .sp-sensor-value{font-size:calc(var(--btn-icon)*2.5);font-weight:100}" +
        ".sp-btn-big .sp-sensor-preview-large .sp-sensor-unit,.sp-btn-wide .sp-sensor-preview-large .sp-sensor-unit{transform:translateY(var(--large-sensor-unit-offset-y,-20px))}" +
        ".sp-date-time-wide-large{justify-content:center;align-items:center}" +
        ".sp-clock-wide-large{justify-content:center;align-items:flex-start}" +
        ".sp-clock-wide-large .sp-sensor-value{font-family:Roboto,sans-serif;font-weight:100}" +
        ".sp-forecast-preview{white-space:nowrap;gap:0}" +
        ".sp-sensor-value{font-size:var(--btn-icon);line-height:1;font-weight:300}" +
        ".sp-sensor-unit{font-size:var(--btn-label);line-height:1;color:#fff}" +
        ".sp-slider-preview{position:absolute;inset:0;border-radius:var(--r);overflow:hidden;pointer-events:none}" +
        ".sp-slider-track{width:100%;height:100%;position:relative}" +
        ".sp-slider-fill{position:absolute;left:0;bottom:0;width:100%;height:80%;background:var(--accent);" +
        "border-radius:var(--r)}" +
        ".sp-image-card{padding:0}" +
        ".sp-image-preview{position:absolute;inset:0;display:block;overflow:hidden;border-radius:var(--btn-r);" +
        "background:var(--screen-tertiary)}" +
        ".sp-image-preview-icon{position:absolute;left:var(--btn-pad);top:var(--btn-pad);" +
        "font-size:var(--btn-icon);line-height:1;color:#fff}" +
        ".sp-image-label{position:absolute;left:0;right:0;bottom:0;box-sizing:border-box;" +
        "padding:var(--btn-pad);z-index:1;pointer-events:none}" +
        ".sp-image-label-stack{position:relative;display:block}" +
        ".sp-image-label-text{font-size:var(--btn-label);line-height:1.2;font-weight:var(--btn-label-weight,400);" +
        "display:-webkit-box;-webkit-box-orient:vertical;-webkit-line-clamp:var(--btn-lines);" +
        "overflow:hidden;word-break:break-word;min-height:0}" +
        ".sp-image-label-shadow{position:absolute;inset:1px -1px -1px 1px;color:rgba(0,0,0,.5)}" +
        ".sp-image-label-main{position:relative;color:#fff}" +
        ".sp-btn-double .sp-image-label-text,.sp-btn-wide .sp-image-label-text,.sp-btn-extra-tall .sp-image-label-text,.sp-btn-extra-wide .sp-image-label-text,.sp-btn-big .sp-image-label-text,.sp-btn-extra-large .sp-image-label-text,.sp-btn-max-wide .sp-image-label-text,.sp-btn-max-tall .sp-image-label-text,.sp-btn-portrait-large .sp-image-label-text{-webkit-line-clamp:var(--btn-lines-dbl)}" +
        ".sp-media-h-slider{position:absolute;left:8%;right:8%;bottom:10%;height:7.5%;border-radius:999px;" +
        "background:var(--screen-tertiary);overflow:hidden;pointer-events:none}" +
        ".sp-media-h-slider span{display:block;width:62%;height:100%;background:#fff;border-radius:999px}" +
        ".sp-media-position-time{z-index:1}" +
        ".sp-media-now-title{font-size:calc(var(--btn-label)*1.75);line-height:1.08;color:#fff;font-weight:300;z-index:1;" +
        "display:-webkit-box;-webkit-box-orient:vertical;-webkit-line-clamp:2;overflow:hidden;word-break:break-word}" +
        ".sp-media-now-artist{font-size:var(--btn-label);line-height:1.2;color:#fff;font-weight:300}" +
        ".sp-btn-big .sp-media-cover-details-title{font-size:var(--media-cover-artist)}" +
        ".sp-btn-big.sp-media-cover-control-fonts .sp-media-cover-details-title{font-size:calc(var(--btn-label)*1.75)}" +
        ".sp-btn-extra-large .sp-media-cover-details-title,.sp-btn-portrait-large .sp-media-cover-details-title{font-size:var(--media-cover-title)}" +
        ".sp-btn-extra-large .sp-media-cover-details-row .sp-media-now-artist,.sp-btn-portrait-large .sp-media-cover-details-row .sp-media-now-artist{font-size:var(--media-cover-artist)}" +
        ".sp-btn-big.sp-media-cover-details-card,.sp-btn-extra-large.sp-media-cover-details-card,.sp-btn-portrait-large.sp-media-cover-details-card{justify-content:flex-start}" +
        ".sp-media-cover-artwork{background-image:radial-gradient(circle at 68% 28%,#ea80fc 0 13%,transparent 34%),linear-gradient(135deg,#3949ab 0%,#00897b 48%,#ff7043 100%)}" +
        ".sp-media-cover-tint{position:absolute;inset:-2px;background:rgba(49,49,49,.6);z-index:0}" +
        ".sp-media-cover-details-title{margin:var(--btn-pad) var(--btn-pad) 0}" +
        ".sp-btn-label-row.sp-media-cover-details-row{width:auto;margin:0 var(--btn-pad) var(--btn-pad)}" +
        ".sp-btn-double{grid-row:span 2}" +
        ".sp-btn-double .sp-btn-label{-webkit-line-clamp:var(--btn-lines-dbl)}" +
        ".sp-btn-double .sp-btn-label-row .sp-btn-label{-webkit-line-clamp:var(--btn-lines-dbl)}" +
        ".sp-btn-double .sp-media-now-title{-webkit-line-clamp:2}" +
        ".sp-btn-wide{grid-column:span 2}" +
        ".sp-btn-wide .sp-media-now-title{-webkit-line-clamp:var(--btn-lines-dbl)}" +
        ".sp-btn-extra-tall{grid-row:span 3}" +
        ".sp-btn-extra-tall .sp-btn-label{-webkit-line-clamp:var(--btn-lines-dbl)}" +
        ".sp-btn-extra-tall .sp-btn-label-row .sp-btn-label{-webkit-line-clamp:var(--btn-lines-dbl)}" +
        ".sp-btn-extra-tall .sp-media-now-title{-webkit-line-clamp:var(--btn-lines-dbl)}" +
        ".sp-btn-extra-wide{grid-column:span 3}" +
        ".sp-btn-extra-wide .sp-media-now-title{-webkit-line-clamp:var(--btn-lines-dbl)}" +
        ".sp-btn-big{grid-row:span 2;grid-column:span 2}" +
        ".sp-btn-big .sp-btn-label{-webkit-line-clamp:var(--btn-lines-dbl)}" +
        ".sp-btn-big .sp-btn-label-row .sp-btn-label{-webkit-line-clamp:var(--btn-lines-dbl)}" +
        ".sp-btn-big .sp-media-now-title{-webkit-line-clamp:var(--btn-lines-dbl)}" +
        ".sp-btn-extra-large{grid-row:span 3;grid-column:span 3}" +
        ".sp-btn-extra-large .sp-btn-label{-webkit-line-clamp:var(--btn-lines-dbl)}" +
        ".sp-btn-extra-large .sp-btn-label-row .sp-btn-label{-webkit-line-clamp:var(--btn-lines-dbl)}" +
        ".sp-btn-extra-large .sp-media-now-title{-webkit-line-clamp:var(--btn-lines-dbl)}" +
        ".sp-btn-big .sp-media-cover-details-title{-webkit-line-clamp:2}" +
        ".sp-btn-wide .sp-media-cover-details-title,.sp-btn-extra-wide .sp-media-cover-details-title{-webkit-line-clamp:2}" +
        ".sp-btn-max-wide{grid-row:span 2;grid-column:span 3}" +
        ".sp-btn-max-tall{grid-row:span 3;grid-column:span 2}" +
        ".sp-btn-portrait-large{grid-row:span 4;grid-column:span 3}" +
        ".sp-empty-cell{border:2px dashed rgba(255,255,255,.15);background:transparent;" +
        "border-radius:var(--empty-r);display:flex;align-items:center;justify-content:center;" +
        "cursor:pointer;transition:border-color .2s,background-color .2s}" +
        ".sp-info-only-hidden{border-color:transparent!important;cursor:default;pointer-events:none}" +
        ".sp-empty-cell:hover{border-color:var(--accent)}" +
        ".sp-drag-active .sp-empty-cell:hover{border-color:rgba(255,255,255,.15)}" +
        ".sp-add-pill{display:inline-flex;align-items:center;justify-content:center;min-width:8cqw;" +
        "height:4.6cqw;padding:0 1.8cqw;border-radius:9999px;border:1px solid rgba(255,255,255,.18);" +
        "background:rgba(255,255,255,.06);box-shadow:var(--shadow-1);opacity:0;pointer-events:none;" +
        "transform:scale(.92);transition:opacity .16s,transform .16s,border-color .2s,background-color .2s}" +
        ".sp-empty-cell:hover .sp-add-pill{border-color:rgba(168,177,255,.45);background:rgba(92,115,231,.18);opacity:1;transform:scale(1)}" +
        ".sp-drag-active .sp-empty-cell:hover .sp-add-pill{opacity:0;transform:scale(.92)}" +
        ".sp-empty-cell .sp-add-icon{font-size:2.8cqw;color:rgba(255,255,255,.82)}" +
        ".sp-empty-cell.sp-drop-placeholder{border:2px dashed rgba(92,156,245,.5) !important;" +
        "background:rgba(92,156,245,.08) !important;cursor:default;pointer-events:none}" +
        ".sp-btn.sp-drop-placeholder{box-shadow:0 0 0 2px rgba(92,156,245,.6),0 0 12px rgba(92,156,245,.25) !important;" +
        "background:rgba(92,156,245,.08) !important}" +
        (CFG.dragAnimation ? ".sp-btn.sp-dragging{opacity:.4;transform:scale(.95)}" +
            ".sp-empty-cell.sp-drop-placeholder{border-color:rgba(92,156,245,.5)}" : "") +
        ".sp-hint{text-align:center;font-size:.7rem;color:var(--text3);padding:8px 0 12px;user-select:none}" +
        ".sp-selection-bar{display:none;align-items:center;justify-content:space-between;gap:12px;" +
        "width:100%;max-width:960px;margin:0 auto;box-sizing:border-box;overflow:hidden;" +
        "padding:14px var(--gap);background:var(--surface);" +
        "color:var(--text);font-family:'Inter',-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;user-select:none}" +
        ".sp-selection-bar.sp-visible{display:flex}" +
        ".sp-selection-label{font-size:.85rem;color:var(--text2);margin-right:auto;min-width:0;" +
        "overflow:hidden;text-overflow:ellipsis;white-space:nowrap}" +
        ".sp-selection-actions{display:flex;align-items:center;gap:8px;flex-shrink:0}" +
        ".sp-selection-btn{display:inline-flex;align-items:center;justify-content:center;gap:8px;" +
        "border:1px solid var(--border);border-radius:var(--action-r);background:var(--surface2);" +
        "color:var(--text);padding:8px 12px;font-size:.8rem;font-weight:500;cursor:pointer;" +
        "font-family:inherit;transition:all .2s;min-height:34px}" +
        ".sp-selection-btn:hover{background:var(--border);border-color:#4a4d54}" +
        ".sp-selection-btn-primary{background:var(--accent);border-color:var(--accent);color:#fff}" +
        ".sp-selection-btn-primary:hover{background:var(--accent-hover);border-color:var(--accent-hover)}" +
        ".sp-selection-btn:disabled,.sp-selection-btn:disabled:hover{opacity:.45;cursor:not-allowed;background:var(--surface2);border-color:var(--border);color:var(--text2)}" +
        ".sp-selection-btn .mdi{font-size:16px;line-height:1}" +
        "@media(max-width:600px){.sp-selection-bar{margin-left:0;margin-right:0;padding:12px var(--gap);gap:8px}" +
        ".sp-selection-actions{gap:6px}.sp-selection-btn{padding:8px 10px}" +
        ".sp-selection-btn[aria-label='Card actions']{width:36px;padding:0}.sp-selection-btn .mdi{flex-shrink:0}}" +
        ".sp-config{padding:var(--gap) var(--gap) var(--gap)}" +
        ".sp-settings-status-header{display:flex;align-items:baseline;justify-content:space-between;" +
        "gap:12px;margin:36px 2px 16px;color:var(--text);font-family:'Inter',-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif}" +
        ".sp-config>.sp-settings-status-header:first-child{margin-top:10px}" +
        ".sp-settings-status-title{font-size:.78rem;font-weight:700;text-transform:uppercase;letter-spacing:.06em}" +
        ".sp-settings-overlay{display:none;position:fixed;inset:0;background:rgba(0,0,0,.6);" +
        "z-index:180;align-items:center;justify-content:center;" +
        "backdrop-filter:blur(4px);-webkit-backdrop-filter:blur(4px)}" +
        ".sp-settings-overlay.sp-visible{display:flex}" +
        ".sp-settings-modal{position:relative;background:var(--bg);border:1px solid var(--border);" +
        "border-radius:var(--radius);width:90%;max-width:420px;max-height:80vh;" +
        "overflow-y:auto;box-shadow:var(--shadow-3);margin:40px;scrollbar-color:#fff transparent}" +
        ".sp-settings-modal.sp-card-type-picker-open{max-width:720px}" +
        ".sp-settings-modal::-webkit-scrollbar{width:10px}" +
        ".sp-settings-modal::-webkit-scrollbar-track{background:transparent}" +
        ".sp-settings-modal::-webkit-scrollbar-thumb{background:#fff;border-radius:999px}" +
        "@media(max-width:600px){.sp-settings-modal{width:100%;max-width:none;max-height:none;" +
        "height:100%;margin:0;border-radius:0;border:none}}" +
        ".sp-settings-close{position:absolute;top:10px;right:12px;width:36px;height:36px;" +
        "border-radius:999px;border:1px solid var(--border);background:var(--surface2);" +
        "color:var(--text2);display:flex;align-items:center;justify-content:center;" +
        "font-size:20px;cursor:pointer;z-index:1;line-height:1;padding:0;" +
        "box-shadow:var(--shadow-1);transition:background .2s,border-color .2s,color .2s}" +
        ".sp-transfer-overlay{position:fixed;inset:0;background:rgba(0,0,0,.65);z-index:260;" +
        "display:flex;align-items:center;justify-content:center;padding:20px;box-sizing:border-box;" +
        "backdrop-filter:blur(4px);-webkit-backdrop-filter:blur(4px)}" +
        ".sp-transfer-dialog{position:relative;width:min(620px,100%);max-height:90vh;overflow:auto;" +
        "box-sizing:border-box;padding:24px;background:var(--bg);border:1px solid var(--border);" +
        "border-radius:var(--radius);box-shadow:var(--shadow-3);color:var(--text);" +
        "font-family:'Inter',-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif}" +
        ".sp-transfer-dialog h2{margin:0 44px 12px 0;font-size:1.2rem;font-weight:600}" +
        ".sp-transfer-dialog p{margin:0 0 14px;color:var(--text2);line-height:1.45}" +
        ".sp-transfer-close{position:absolute;top:14px;right:14px;width:36px;height:36px;border-radius:999px;" +
        "border:1px solid var(--border);background:var(--surface2);color:var(--text2);cursor:pointer;" +
        "display:flex;align-items:center;justify-content:center;padding:0;z-index:1;line-height:1;" +
        "box-shadow:var(--shadow-1);touch-action:manipulation;" +
        "transition:background .2s,border-color .2s,color .2s,box-shadow .2s}" +
        ".sp-transfer-close:hover{background:var(--border);border-color:#4a4d54;color:var(--text)}" +
        ".sp-transfer-close:focus-visible{outline:none;border-color:var(--accent);" +
        "box-shadow:0 0 0 3px var(--accent-soft)}" +
        ".sp-transfer-close-icon{width:20px;height:20px;display:block;fill:currentColor;pointer-events:none}" +
        ".sp-transfer-dialog .sp-transfer-code{width:100%;min-height:170px;box-sizing:border-box;" +
        "font-family:ui-monospace,SFMono-Regular,Menlo,Consolas,monospace;font-weight:400;" +
        "font-size:.78rem;word-break:break-all}" +
        ".sp-transfer-note{margin-top:10px!important;font-size:.82rem}" +
        ".sp-transfer-error{min-height:20px;margin-top:8px;font-size:.86rem;color:#ff8b8b}" +
        ".sp-transfer-actions{justify-content:flex-end}" +
        "@media(max-width:600px){.sp-transfer-overlay{padding:0}.sp-transfer-dialog{width:100%;height:100%;" +
        "max-height:none;border:0;border-radius:0;padding:20px}.sp-transfer-code{min-height:45vh}" +
        ".sp-transfer-actions{position:sticky;bottom:0;background:var(--bg);padding-top:12px}}" +
        ".sp-settings-close:hover{background:var(--border);border-color:#4a4d54;color:var(--text)}" +
        ".sp-settings-close:focus-visible{outline:none;border-color:var(--accent);" +
        "box-shadow:0 0 0 3px var(--accent-soft)}" +
        ".sp-settings-close-icon{width:20px;height:20px;display:block;fill:currentColor;pointer-events:none}" +
        ".sp-section-title{font-size:.8rem;font-weight:600;color:var(--text2);" +
        "margin:var(--gap) 0 8px;letter-spacing:-.01em}" +
        ".sp-settings-modal .sp-section-title{font-size:1.05rem;color:var(--text);" +
        "margin:0 0 20px;letter-spacing:-.01em}" +
        ".sp-settings-modal .sp-panel{background:none;border:none;padding:0;margin:0}" +
        ".sp-card-type-picker-field{margin-bottom:0}" +
        ".sp-card-type-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(190px,1fr));gap:12px}" +
        ".sp-card-type-option{min-width:0;min-height:112px;display:flex;align-items:flex-start;gap:13px;" +
        "padding:16px;border:1px solid var(--border);border-radius:8px;background:var(--surface);" +
        "color:var(--text);font-family:inherit;text-align:left;cursor:pointer;transition:background .2s,border-color .2s,box-shadow .2s,transform .2s}" +
        ".sp-card-type-option:hover{background:var(--surface2);border-color:#4a4d54;transform:translateY(-1px)}" +
        ".sp-card-type-option:focus-visible{outline:none;border-color:var(--accent);box-shadow:0 0 0 3px var(--accent-soft)}" +
        ".sp-card-type-option:disabled{opacity:.48;cursor:not-allowed;transform:none}" +
        ".sp-card-type-icon{width:34px;height:34px;border-radius:8px;background:var(--accent-soft);" +
        "color:var(--accent-hover);display:inline-flex;align-items:center;justify-content:center;font-size:22px;line-height:1;flex:0 0 auto}" +
        ".sp-card-type-copy{display:grid;gap:6px;min-width:0}" +
        ".sp-card-type-title{font-size:.93rem;font-weight:600;line-height:1.25;color:var(--text);overflow-wrap:anywhere}" +
        ".sp-card-type-description{font-size:.78rem;line-height:1.35;color:var(--text2);overflow-wrap:anywhere}" +
        "@media(max-width:600px){.sp-card-type-grid{gap:10px}.sp-card-type-option{min-height:0;" +
        "padding:12px 14px;gap:12px;align-items:center}.sp-card-type-icon{width:30px;height:30px;" +
        "border-radius:7px;font-size:20px}.sp-card-type-copy{gap:3px}.sp-card-type-title{font-size:.88rem}" +
        ".sp-card-type-description{font-size:.75rem;line-height:1.3}}" +
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
        ".card-header-right{display:flex;align-items:center;gap:12px}" +
        ".card.collapsed .sp-card-header-action{display:none}" +
        ".sp-card-badge{display:inline-flex;align-items:center;gap:7px;min-height:24px;" +
        "padding:0 12px 0 10px;border-radius:999px;background:rgba(48,164,108,.16);" +
        "color:#30a46c;font-size:.68rem;font-weight:400;text-transform:uppercase;letter-spacing:.04em;line-height:1}" +
        ".sp-card-badge-dot{width:7px;height:7px;border-radius:999px;background:#30a46c;flex-shrink:0}" +
        ".card:not(.collapsed) .sp-card-badge{display:none}" +
        ".sp-card-badge.sp-hidden{display:none}" +
        ".sp-panel{background:var(--surface);border-radius:var(--radius);padding:24px;" +
        "margin-bottom:var(--gap);border:1px solid var(--border)}" +
        ".sp-field{margin-bottom:28px}.sp-field:last-child{margin-bottom:0}" +
        ".sp-field.sp-icon-on-field:last-child{margin-bottom:44px}" +
        ".sp-state-translation-grid{display:grid;grid-template-columns:minmax(0,1fr) minmax(0,1fr);gap:18px 14px;margin-bottom:28px}" +
        ".sp-state-translation-grid .sp-field{margin-bottom:0}" +
        ".sp-field-stack{display:grid;gap:10px}" +
        ".sp-field-stack.sp-hidden{display:none}" +
        ".sp-field-label{display:block;font-size:.8rem;font-weight:500;color:var(--text2);margin-bottom:8px}" +
        ".sp-input,.sp-select{width:100%;padding:10px 12px;background:var(--surface2);" +
        "border:1px solid var(--border);border-radius:8px;color:var(--text);font-size:.875rem;" +
        "font-family:inherit;box-sizing:border-box;outline:none;" +
        "transition:border-color .25s,box-shadow .25s}" +
        ".sp-textarea{min-height:84px;resize:vertical;line-height:1.35}" +
        ".sp-input[type=number]{color-scheme:dark}" +
        ".sp-input--no-stepper{-moz-appearance:textfield}" +
        ".sp-input--no-stepper::-webkit-outer-spin-button,.sp-input--no-stepper::-webkit-inner-spin-button{-webkit-appearance:none;margin:0}" +
        ".sp-input:focus,.sp-select:focus{border-color:var(--accent);" +
        "box-shadow:0 0 0 3px var(--accent-soft)}" +
        ".sp-input.sp-input-error,.sp-select.sp-input-error{border-color:var(--danger);" +
        "box-shadow:0 0 0 3px rgba(241,65,88,.16)}" +
        ".sp-input--narrow{width:80px}" +
        ".sp-repeat-list{display:grid;gap:12px;margin-bottom:18px}" +
        ".sp-repeat-row{display:grid;grid-template-columns:minmax(0,1fr) 40px;gap:8px;align-items:end}" +
        ".sp-repeat-field{margin-bottom:0}" +
        ".sp-icon-button{width:40px;height:40px;border:1px solid var(--border);border-radius:8px;" +
        "background:var(--surface2);color:var(--text2);display:inline-flex;align-items:center;justify-content:center;" +
        "font-size:18px;cursor:pointer;font-family:inherit;transition:background .25s,color .25s,border-color .25s}" +
        ".sp-icon-button:hover{background:var(--border);color:var(--text)}" +
        ".sp-icon-button.sp-card-header-action{width:24px;height:24px;margin-right:12px;border:0;" +
        "background:transparent;border-radius:0;color:var(--text3);font-size:20px;box-shadow:none}" +
        ".sp-icon-button.sp-card-header-action:hover,.sp-icon-button.sp-card-header-action:focus{" +
        "background:transparent;color:var(--text);box-shadow:none}" +
        ".sp-secondary-btn{display:inline-flex;align-items:center;justify-content:center;gap:8px;" +
        "border:1px solid var(--border);border-radius:var(--action-r);background:var(--surface2);" +
        "color:var(--text);padding:10px 14px;font-size:.875rem;font-weight:500;cursor:pointer;" +
        "font-family:inherit;transition:background .25s,border-color .25s}" +
        ".sp-secondary-btn:hover{background:var(--border);border-color:#4a4d54}" +
        ".sp-secondary-btn:disabled{opacity:.45;cursor:not-allowed}" +
        ".sp-select{appearance:none;-webkit-appearance:none;" +
        "background-image:url(\"data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='12' height='12' viewBox='0 0 12 12'%3E%3Cpath fill='%2398989f' d='M6 8L1 3h10z'/%3E%3C/svg%3E\");" +
        "background-repeat:no-repeat;background-position:right 12px center;padding-right:32px}" +
        "select option{background:var(--surface);color:var(--text)}" +
        ".sp-entity-input-wrap{position:relative}" +
        ".sp-entity-dropdown{display:none;position:absolute;left:0;right:0;top:100%;margin-top:6px;" +
        "background:var(--surface2);border:1px solid var(--border);border-radius:8px;max-height:220px;" +
        "overflow-y:auto;z-index:60;box-shadow:var(--shadow-3);padding:0}" +
        ".sp-entity-dropdown.sp-open{display:block}" +
        ".sp-entity-option{display:block;width:100%;padding:10px 12px;background:transparent;border:0;" +
        "color:var(--text);font-size:.875rem;line-height:1.4;font-family:inherit;text-align:left;" +
        "cursor:pointer;box-sizing:border-box}" +
        ".sp-entity-option:hover,.sp-entity-option:focus{background:var(--accent-soft);outline:none}" +
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
        ".sp-delete-btn,.sp-hide-btn,.sp-cancel-btn{background:var(--surface2);color:var(--text);display:inline-flex;align-items:center;gap:6px}" +
        ".sp-delete-btn:hover,.sp-hide-btn:hover,.sp-cancel-btn:hover{background:var(--border);color:var(--text)}" +
        ".sp-save-btn{background:var(--accent);color:#fff}" +
        ".sp-save-btn:hover{background:var(--accent-hover);box-shadow:var(--shadow-1)}" +
        ".sp-edit-subpage-btn{background:var(--accent);color:#fff}" +
        ".sp-edit-subpage-btn:hover{background:var(--accent-hover);box-shadow:var(--shadow-1)}" +
        ".sp-btn-row--save{margin-top:24px;justify-content:flex-end}" +
        ".sp-btn-row--save.sp-has-delete,.sp-btn-row--save.sp-has-secondary{justify-content:space-between}" +
        ".sp-btn-group-right{display:flex;gap:8px}" +
        ".sp-toggle-row{display:flex;align-items:center;justify-content:space-between;" +
        "min-height:36px;margin-bottom:14px}" +
        ".sp-toggle-row:last-child{margin-bottom:0}" +
        ".sp-cond-field+.sp-toggle-row{margin-top:16px}" +
        ".sp-toggle-label{font-size:.875rem;cursor:pointer}" +
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
        ".sp-light-tab-list{display:grid;gap:0;margin:-6px 0 18px}" +
        ".sp-light-tab-row{display:grid;grid-template-columns:auto minmax(0,1fr) auto;align-items:center;" +
        "gap:10px;min-height:42px;padding:5px 0;border-bottom:1px solid rgba(255,255,255,.08)}" +
        ".sp-light-tab-row:last-child{border-bottom:0}" +
        ".sp-light-tab-row.sp-dragging{opacity:.55}" +
        ".sp-light-tab-controls{display:inline-flex;align-items:center;gap:2px;color:var(--text3)}" +
        ".sp-light-tab-drag,.sp-light-tab-move{width:28px;height:28px;border:0;background:transparent;" +
        "color:var(--text3);display:inline-flex;align-items:center;justify-content:center;border-radius:8px;" +
        "font-size:18px;line-height:1;font-family:inherit;cursor:pointer;transition:background .2s,color .2s}" +
        ".sp-light-tab-drag{cursor:grab;font-size:20px}" +
        ".sp-light-tab-row.sp-dragging .sp-light-tab-drag{cursor:grabbing}" +
        ".sp-light-tab-drag:hover,.sp-light-tab-move:hover{background:var(--surface2);color:var(--text)}" +
        ".sp-light-tab-label{min-width:0;color:var(--text);font-size:.9rem;line-height:1.25;cursor:pointer}" +
        ".sp-segment{display:flex;border-radius:var(--action-r);overflow:hidden;border:1px solid var(--border);margin-bottom:14px}" +
        ".sp-segment-scroll{overflow-x:auto;overflow-y:hidden;-webkit-overflow-scrolling:touch;scrollbar-width:none}" +
        ".sp-segment-scroll::-webkit-scrollbar{display:none}" +
        ".sp-segment.sp-segment-scroll button{flex:1 0 auto;min-width:112px;padding-left:12px;padding-right:12px}" +
        ".sp-segment button{flex:1;padding:8px 0;background:var(--surface2);color:var(--text2);" +
        "border:none;font-size:.8rem;font-weight:500;cursor:pointer;transition:all .25s;font-family:inherit}" +
        ".sp-segment button:hover{color:var(--text)}" +
        ".sp-segment button.active{background:var(--accent);color:#fff}" +
        ".sp-screensaver-mode{margin-bottom:24px}" +
        ".sp-clock-brightness-field{margin:18px 0 22px}" +
        ".sp-cond-field{padding:0 0 4px;display:none}" +
        ".sp-cond-field.sp-visible{display:block}" +
        ".sp-action-confirm-section.sp-visible{margin-bottom:28px}" +
        ".sp-cond-field.sp-climate-settings-gap.sp-visible{margin-bottom:24px}" +
        ".sp-disclosure{border:1px solid var(--border);border-radius:8px;background:rgba(255,255,255,.02);margin-top:4px}" +
        ".sp-disclosure+.sp-disclosure{margin-top:14px}" +
        ".sp-disclosure-button{width:100%;display:flex;align-items:center;justify-content:space-between;gap:12px;" +
        "padding:12px 14px;background:transparent;border:0;color:var(--text);font:inherit;font-size:.875rem;font-weight:500;cursor:pointer;text-align:left}" +
        ".sp-disclosure-button:hover{background:rgba(255,255,255,.03)}" +
        ".sp-disclosure-button:focus-visible{outline:none;box-shadow:0 0 0 2px var(--accent-soft) inset}" +
        ".sp-disclosure-header-right{display:inline-flex;align-items:center;gap:10px;margin-left:auto}" +
        ".sp-disclosure-badge{display:inline-flex;align-items:center;gap:7px;min-height:22px;padding:0 10px 0 9px;" +
        "border-radius:999px;background:rgba(48,164,108,.16);color:#30a46c;font-size:.66rem;font-weight:500;" +
        "text-transform:uppercase;letter-spacing:.04em;line-height:1;white-space:nowrap}" +
        ".sp-disclosure-badge-dot{width:7px;height:7px;border-radius:999px;background:#30a46c;flex-shrink:0}" +
        ".sp-disclosure.sp-open .sp-disclosure-badge,.sp-disclosure-badge.sp-hidden{display:none}" +
        ".sp-disclosure-chevron{display:inline-flex;width:20px;height:20px;color:var(--text3);transition:transform .25s ease;flex-shrink:0}" +
        ".sp-disclosure-chevron svg{width:100%;height:100%}" +
        ".sp-disclosure-body{display:none;padding:30px 14px 28px}" +
        ".sp-disclosure.sp-open .sp-disclosure-chevron{transform:rotate(180deg)}" +
        ".sp-disclosure.sp-open .sp-disclosure-body{display:block}" +
        ".sp-schedule-times.sp-hidden{display:none}" +
        "#sp-set-schedule-actions{margin-top:28px}" +
        ".sp-info-panel{display:flex;align-items:flex-start;gap:10px;padding:10px 12px;margin-bottom:16px;" +
        "background:rgba(92,115,231,.12);border:1px solid rgba(92,115,231,.22);border-radius:8px;" +
        "color:var(--text2);font-size:.82rem;line-height:1.35}" +
        ".sp-info-panel .mdi{font-size:18px;color:var(--accent);line-height:1.1;flex:0 0 auto;margin-top:1px}" +
        ".sp-info-panel a{color:var(--accent);font-weight:600;text-decoration:none}" +
        ".sp-info-panel a:hover{text-decoration:underline}" +
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
        ".sp-local-picker-fallback .sp-banner{margin-bottom:28px}" +
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
        ".sp-fw-info-row{min-height:28px;margin-bottom:8px}" +
        ".sp-fw-overview{margin-bottom:0}" +
        ".sp-fw-subpanels{display:grid;gap:12px}" +
        ".sp-fw-subpanels .sp-disclosure{margin-top:0;background:var(--surface)}" +
        ".sp-fw-subpanels .sp-disclosure+.sp-disclosure{margin-top:0}" +
        ".sp-fw-subpanels .sp-disclosure-body{padding:18px 14px 16px}" +
        ".sp-fw-subpanels .sp-toggle-row:last-child{margin-bottom:0}" +
        ".sp-fw-version{font-size:.875rem;color:var(--text)}" +
        ".sp-fw-label{font-size:.875rem;color:var(--text2)}" +
        ".sp-fw-actions{display:flex;align-items:center;justify-content:flex-end;gap:12px;margin-left:auto}" +
        ".sp-fw-actions-full{justify-content:flex-end;margin:14px 0 0}" +
        ".sp-fw-previous-actions{display:flex;justify-content:flex-end;margin-top:12px}" +
        ".sp-fw-status{font-size:.8rem;color:var(--text2);line-height:1.4;margin:8px 0 0;text-align:right}" +
        ".sp-fw-status:empty{display:none}" +
        ".sp-fw-status.sp-update-available{color:#3dd68c}" +
        ".sp-fw-status.sp-update-installing{color:#f9b44e}" +
        ".sp-fw-status.sp-update-error{color:var(--danger)}" +
        ".sp-fw-status a{color:inherit;text-decoration:underline;text-underline-offset:2px}" +
        ".sp-fw-btn{background:var(--surface2);color:var(--text);border:1px solid var(--border);" +
        "border-radius:var(--action-r);padding:8px 14px;font-size:.875rem;font-weight:500;cursor:pointer;" +
        "font-family:inherit;transition:all .25s;white-space:nowrap}" +
        ".sp-fw-btn:hover{background:var(--border);border-color:#4a4d54}" +
        ".sp-fw-btn:disabled{opacity:.4;cursor:not-allowed}" +
        ".sp-fw-btn.sp-fw-btn-busy{background:rgba(35,37,43,.5);color:#8a8d94;" +
        "border-color:rgba(74,77,84,.45);border-radius:999px;padding:8px 28px;opacity:1}" +
        ".sp-fw-btn.sp-fw-btn-busy:hover{background:rgba(35,37,43,.5);border-color:rgba(74,77,84,.45)}" +
        ".sp-btn-label-row{display:flex;align-items:baseline;width:100%;overflow:hidden;position:relative;z-index:1}" +
        ".sp-btn-label-row .sp-btn-label{flex:1;min-width:0}" +
        ".sp-subpage-badge{font-size:var(--btn-label);line-height:1;opacity:.5;flex-shrink:0;" +
        "cursor:pointer;padding:0 0 0 2px;border-radius:4px;" +
        "transition:opacity .15s}" +
        ".sp-subpage-badge:hover{opacity:1}" +
        ".sp-hide-subpage-chevrons .sp-subpage-badge{display:none}" +
        ".sp-alarm-badge{font-size:var(--btn-label);line-height:1.2;opacity:.58;flex-shrink:0;padding:2px 0 2px 4px}" +
        ".sp-type-badge{display:none}" +
        "@media(max-width:768px){" +
        ":root{--gap:12px}" +
        "#sp-app{max-width:100%}" +
        ".sp-header{padding:0 12px;height:48px}" +
        ".sp-brand{font-size:.875rem}" +
        ".sp-tab{padding:0 12px;font-size:.8rem}" +
        ".sp-tab-docs{margin-left:4px;padding-left:18px}" +
        ".sp-wrap{padding-left:0;padding-right:0}" +
        ".sp-screen{box-sizing:border-box;width:100%;max-width:100%}" +
        ".sp-hint{padding-left:var(--gap);padding-right:var(--gap);line-height:1.35}" +
        ".card{padding:16px}" +
        ".card-header{margin:-16px -16px 0 -16px;padding:16px 16px 0 16px}" +
        ".card-body{padding-top:14px}" +
        ".sp-config{padding:var(--gap)}" +
        "#sp-settings .sp-config{padding-bottom:6px}" +
        "#sp-settings .sp-apply-bar{padding-top:6px;padding-bottom:max(var(--gap),env(safe-area-inset-bottom))}" +
        "}" +
        "@media(max-width:480px){" +
        ":root{--gap:10px}" +
        ".sp-header{padding:0 10px}" +
        ".sp-tab{padding:0 10px;font-size:.75rem}" +
        ".sp-tab-docs{margin-left:2px;padding-left:16px;gap:4px}" +
        ".sp-settings-status-header{margin:28px 2px 12px}" +
        ".sp-config>.sp-settings-status-header:first-child{margin-top:12px}" +
        ".card{border-radius:10px;margin-bottom:10px}" +
        ".card-header{min-height:56px;box-sizing:border-box}" +
        ".sp-field{margin-bottom:22px}" +
        "#sp-set-schedule-actions{margin-top:22px}" +
        ".sp-toggle-row{gap:16px}" +
        ".sp-toggle-label{min-width:0;line-height:1.35}" +
        ".sp-range-row{gap:10px}" +
        ".sp-action-btn{padding:10px 14px}" +
        ".sp-btn-row{flex-wrap:wrap}" +
        ".sp-btn-row--save,.sp-btn-row--save.sp-has-delete{justify-content:flex-start}" +
        ".sp-btn-group-right{margin-left:auto}" +
        ".sp-color-row{flex-wrap:wrap}" +
        ".sp-backup-btns{flex-direction:column}" +
        ".sp-fw-row{flex-direction:column;align-items:flex-start;gap:12px}" +
        ".sp-fw-actions{width:100%;margin-left:0;justify-content:flex-start;flex-wrap:wrap}" +
        ".sp-fw-status{text-align:left}" +
        ".sp-fw-actions .sp-fw-btn,.sp-fw-previous-actions .sp-fw-btn{width:100%}" +
        "}";
    return {
        "WEB_STYLES": liveGlobal(() => WEB_STYLES, (value?: any) => { WEB_STYLES = value; }),
    };
}
