import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installButtonSettingsIconPickerModule(): GlobalDescriptors {
    // ── Button Settings Icon Picker ───────────────────────────────────
    // ── Icon picker (optimized) ────────────────────────────────────────────
    function initIconPicker(this: any, picker?: any, currentIcon?: any, onSelect?: any) {
        var input: any = picker.querySelector(".sp-icon-picker-input");
        var dropdown: any = picker.querySelector(".sp-icon-dropdown");
        var preview: any = picker.querySelector(".sp-icon-picker-preview");
        var highlighted: any = -1;
        var optionEls: any = null;
        var emptyEl: any = null;
        function ensureBuilt(this: any) {
            if (optionEls)
                return;
            optionEls = [];
            var frag: any = document.createDocumentFragment();
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
            if (optionEls) {
                for (var i: any = 0; i < optionEls.length; i++) {
                    if (optionEls[i].style.display !== "none")
                        vis.push(optionEls[i]);
                }
            }
            return vis;
        }
        function highlightAt(this: any, idx?: any) {
            var visible: any = getVisible();
            if (visible.length === 0)
                return;
            if (optionEls)
                optionEls.forEach(function (this: any, el?: any) { el.classList.remove("sp-highlighted"); });
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
    return {
        "initIconPicker": staticGlobal(initIconPicker),
    };
}
