import { customIconCodepoint, iconOptions as ICON_OPTIONS, iconSlug, mdiIcon, normalizeCustomIcon, textSpan } from "./ui_primitives";

function sameCustomIcon(a: unknown, b: unknown): boolean {
    var codepoint = customIconCodepoint(a);
    return codepoint !== null && codepoint === customIconCodepoint(b);
}

export interface ButtonSettingsIconPickerFeature {
    init(picker: any, currentIcon: any, onSelect: (icon: any) => void): void;
}

export function createButtonSettingsIconPickerFeature(
    document: Document,
    renderPreview: () => void,
): ButtonSettingsIconPickerFeature {
    // ── Button Settings Icon Picker ───────────────────────────────────
    // ── Icon picker (optimized) ────────────────────────────────────────────
    function initIconPicker(this: any, picker?: any, currentIcon?: any, onSelect?: any) {
        var input: any = picker.querySelector(".sp-icon-picker-input");
        var dropdown: any = picker.querySelector(".sp-icon-dropdown");
        var preview: any = picker.querySelector(".sp-icon-picker-preview");
        var highlighted: any = -1;
        var optionEls: any = null;
        var emptyEl: any = null;
        var customEl: any = null;
        // The custom row sits above the built-in list when it shows the card's
        // current custom icon, and below any name matches when it shows a
        // typed codepoint, so Enter prefers a matching built-in icon.
        var customFirst: any = false;
        function ensureBuilt(this: any) {
            if (optionEls)
                return;
            optionEls = [];
            var frag: any = document.createDocumentFragment();
            // Typing an MDI codepoint (e.g. F0F92) offers it as a custom icon.
            customEl = document.createElement("div");
            customEl.className = "sp-icon-option sp-icon-option--custom";
            customEl.style.display = "none";
            customEl.addEventListener("mousedown", function (this: any, e?: any) {
                e.preventDefault();
                if (customEl._optName)
                    selectOpt(customEl._optName);
            });
            frag.appendChild(customEl);
            ICON_OPTIONS.forEach(function (this: any, opt?: any) {
                var row: any = document.createElement("div");
                row.className = "sp-icon-option" + (opt === currentIcon ? " sp-active" : "");
                row.appendChild(mdiIcon(opt, "sp-icon-option-icon mdi"));
                row.appendChild(textSpan(opt, "sp-icon-option-label"));
                row._lcName = opt.toLowerCase();
                row._optName = opt;
                row.addEventListener("mousedown", function (this: any, e?: any) {
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
        function filterOpts(this: any, filter?: any) {
            ensureBuilt();
            highlighted = -1;
            var lc: any = (filter || "").toLowerCase();
            var hasMatch: any = false;
            var typed: any = normalizeCustomIcon(filter);
            var custom: any = typed || (!lc && customIconCodepoint(currentIcon) !== null ? currentIcon : null);
            customFirst = !typed;
            dropdown.insertBefore(customEl, customFirst ? optionEls[0] || emptyEl : emptyEl);
            customEl._optName = custom;
            customEl.classList.remove("sp-highlighted");
            customEl.classList.toggle("sp-active", sameCustomIcon(custom, currentIcon));
            customEl.replaceChildren();
            if (custom) {
                customEl.appendChild(mdiIcon(custom, "sp-icon-option-icon mdi"));
                customEl.appendChild(textSpan("Custom icon " + custom, "sp-icon-option-label"));
                customEl.style.display = "";
                hasMatch = true;
            }
            else {
                customEl.style.display = "none";
            }
            for (var i: any = 0; i < optionEls.length; i++) {
                var match: any = !lc || optionEls[i]._lcName.indexOf(lc) !== -1;
                optionEls[i].style.display = match ? "" : "none";
                optionEls[i].classList.remove("sp-highlighted");
                if (match)
                    hasMatch = true;
            }
            emptyEl.style.display = hasMatch ? "none" : "";
        }
        function setPickerIcon(this: any, opt?: any) {
            currentIcon = opt;
            input.value = opt;
            preview.className = "sp-icon-picker-preview mdi mdi-" + iconSlug(opt);
            if (optionEls) {
                for (var i: any = 0; i < optionEls.length; i++) {
                    optionEls[i].classList.toggle("sp-active", optionEls[i]._optName === opt);
                }
                customEl.classList.toggle("sp-active", sameCustomIcon(customEl._optName, opt));
            }
        }
        function selectOpt(this: any, opt?: any) {
            setPickerIcon(opt);
            closePicker();
            onSelect(opt);
            renderPreview();
        }
        picker._setIcon = setPickerIcon;
        function openPicker(this: any) {
            input.value = "";
            filterOpts("");
            picker.classList.add("sp-open");
        }
        function closePicker(this: any) {
            picker.classList.remove("sp-open");
            input.value = currentIcon;
            highlighted = -1;
        }
        function getVisible(this: any) {
            var vis: any = [];
            var showCustom: any = customEl && customEl.style.display !== "none";
            if (showCustom && customFirst)
                vis.push(customEl);
            if (optionEls) {
                for (var i: any = 0; i < optionEls.length; i++) {
                    if (optionEls[i].style.display !== "none")
                        vis.push(optionEls[i]);
                }
            }
            if (showCustom && !customFirst)
                vis.push(customEl);
            return vis;
        }
        function highlightAt(this: any, idx?: any) {
            var visible: any = getVisible();
            if (visible.length === 0)
                return;
            if (optionEls)
                optionEls.forEach(function (this: any, el?: any) { el.classList.remove("sp-highlighted"); });
            if (customEl)
                customEl.classList.remove("sp-highlighted");
            if (idx < 0)
                idx = visible.length - 1;
            if (idx >= visible.length)
                idx = 0;
            highlighted = idx;
            visible[highlighted].classList.add("sp-highlighted");
            visible[highlighted].scrollIntoView({ block: "nearest" });
        }
        input.addEventListener("focus", openPicker);
        input.addEventListener("blur", closePicker);
        input.addEventListener("input", function (this: any) {
            filterOpts(this.value);
            var vis: any = getVisible();
            if (vis.length > 0)
                highlightAt(0);
        });
        input.addEventListener("keydown", function (this: any, e?: any) {
            if (e.key === "ArrowDown") {
                e.preventDefault();
                if (!picker.classList.contains("sp-open")) {
                    openPicker();
                    return;
                }
                highlightAt(highlighted + 1);
            }
            else if (e.key === "ArrowUp") {
                e.preventDefault();
                highlightAt(highlighted - 1);
            }
            else if (e.key === "Enter") {
                e.preventDefault();
                var visible: any = getVisible();
                if (highlighted >= 0 && highlighted < visible.length) {
                    selectOpt(visible[highlighted]._optName);
                }
            }
            else if (e.key === "Tab") {
                var visible: any = getVisible();
                if (picker.classList.contains("sp-open") && highlighted >= 0 && highlighted < visible.length) {
                    selectOpt(visible[highlighted]._optName);
                }
            }
            else if (e.key === "Escape") {
                e.preventDefault();
                closePicker();
                input.blur();
            }
        });
    }
    return { init: initIconPicker };
}
