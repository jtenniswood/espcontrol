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
  ".sp-config-locked .sp-screen{filter:grayscale(1) brightness(.58);opacity:.62;pointer-events:none}" +
  ".sp-config-locked .sp-screen::after{content:'';position:absolute;inset:0;background:rgba(80,80,84,.28);z-index:5}" +
  ".sp-topbar{position:absolute;top:0;left:0;right:0;height:var(--topbar-h);" +
  "display:flex;align-items:center;padding:var(--topbar-pad);z-index:1}" +
  ".sp-topbar.sp-hidden{display:none}" +
  ".sp-temp{color:#fff;font-size:var(--topbar-fs);white-space:nowrap;opacity:0;transition:opacity .3s;margin-left:1cqw}" +
  ".sp-temp.sp-visible{opacity:1}" +
  ".sp-clock{position:absolute;left:50%;transform:translateX(-50%);" +
  "color:#fff;font-size:var(--clock-fs,var(--topbar-fs));white-space:nowrap}" +
  ".sp-network-preview{margin-left:auto;margin-right:1cqw;color:#fff;font-size:calc(var(--topbar-fs)*.86);" +
  "line-height:1;width:calc(var(--topbar-fs)*1.2);height:100%;display:flex;align-items:center;" +
  "justify-content:center;opacity:0;transition:opacity .3s}" +
  ".sp-network-preview.sp-visible{opacity:1}" +
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
  ".sp-sensor-preview{display:flex;align-items:baseline;gap:.18em;color:#fff}" +
  ".sp-btn-big .sp-sensor-preview-large .sp-sensor-value{font-size:calc(var(--btn-icon)*3);font-weight:100}" +
  ".sp-btn-big .sp-sensor-preview-large .sp-sensor-unit{transform:translateY(var(--large-sensor-unit-offset-y,-20px))}" +
  ".sp-forecast-preview{white-space:nowrap;gap:0}" +
  ".sp-sensor-value{font-size:var(--btn-icon);line-height:1;font-weight:300}" +
  ".sp-sensor-unit{font-size:var(--btn-label);line-height:1;color:#fff}" +
  ".sp-slider-preview{position:absolute;inset:0;border-radius:var(--r);overflow:hidden;pointer-events:none}" +
  ".sp-slider-track{width:100%;height:100%;position:relative}" +
  ".sp-slider-fill{position:absolute;left:0;bottom:0;width:100%;height:80%;background:var(--accent);" +
  "border-radius:var(--r)}" +
  ".sp-media-h-slider{position:absolute;left:8%;right:8%;bottom:10%;height:7.5%;border-radius:999px;" +
  "background:#212121;overflow:hidden;pointer-events:none}" +
  ".sp-media-h-slider span{display:block;width:62%;height:100%;background:#fff;border-radius:999px}" +
  ".sp-media-position-time{z-index:1}" +
  ".sp-media-now-title{font-size:var(--btn-icon);line-height:1.08;color:#fff;font-weight:300;z-index:1;" +
  "display:-webkit-box;-webkit-box-orient:vertical;-webkit-line-clamp:2;overflow:hidden;word-break:break-word}" +
  ".sp-media-now-artist{font-size:var(--btn-label);line-height:1.2;color:#fff}" +
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
  ".sp-segment-scroll{overflow-x:auto;overflow-y:hidden;-webkit-overflow-scrolling:touch;scrollbar-width:none}" +
  ".sp-segment-scroll::-webkit-scrollbar{display:none}" +
  ".sp-segment.sp-segment-scroll button{flex:1 0 auto;min-width:112px;padding-left:12px;padding-right:12px}" +
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

  ".sp-btn-label-row{display:flex;align-items:flex-end;width:100%;overflow:hidden;position:relative;z-index:1}" +
  ".sp-btn-label-row .sp-btn-label{flex:1;min-width:0}" +
  ".sp-subpage-badge{font-size:var(--btn-label);line-height:1.2;opacity:.5;flex-shrink:0;" +
  "cursor:pointer;padding:2px 0 2px 4px;border-radius:4px;transition:opacity .15s}" +
  ".sp-subpage-badge:hover{opacity:1}" +
  ".sp-type-badge{display:none}" +

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
