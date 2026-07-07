// ── Button Settings Icon Picker ───────────────────────────────────
// @web-module-requires: state, controls, preview_render

// ── Icon picker (optimized) ────────────────────────────────────────────

function initIconPicker(picker, currentIcon, onSelect) {
  var input = picker.querySelector(".sp-icon-picker-input");
  var dropdown = picker.querySelector(".sp-icon-dropdown");
  var preview = picker.querySelector(".sp-icon-picker-preview");
  var highlighted = -1;
  var optionEls = null;
  var emptyEl = null;

  function ensureBuilt() {
    if (optionEls) return;
    optionEls = [];
    var frag = document.createDocumentFragment();
    ICON_OPTIONS.forEach(function (opt) {
      var row = document.createElement("div");
      row.className = "sp-icon-option" + (opt === currentIcon ? " sp-active" : "");
      row.appendChild(mdiIcon(opt, "sp-icon-option-icon mdi"));
      row.appendChild(textSpan(opt, "sp-icon-option-label"));
      row._lcName = opt.toLowerCase();
      row._optName = opt;
      row.addEventListener("mousedown", function (e) {
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

  function filterOpts(filter) {
    ensureBuilt();
    highlighted = -1;
    var lc = (filter || "").toLowerCase();
    var hasMatch = false;
    for (var i = 0; i < optionEls.length; i++) {
      var match = !lc || optionEls[i]._lcName.indexOf(lc) !== -1;
      optionEls[i].style.display = match ? "" : "none";
      optionEls[i].classList.remove("sp-highlighted");
      if (match) hasMatch = true;
    }
    emptyEl.style.display = hasMatch ? "none" : "";
  }

  function setPickerIcon(opt) {
    currentIcon = opt;
    input.value = opt;
    preview.className = "sp-icon-picker-preview mdi mdi-" + iconSlug(opt);
    if (optionEls) {
      for (var i = 0; i < optionEls.length; i++) {
        optionEls[i].classList.toggle("sp-active", optionEls[i]._optName === opt);
      }
    }
  }

  function selectOpt(opt) {
    setPickerIcon(opt);
    closePicker();
    onSelect(opt);
    renderPreview();
  }
  picker._setIcon = setPickerIcon;

  function openPicker() {
    input.value = "";
    filterOpts("");
    picker.classList.add("sp-open");
  }

  function closePicker() {
    picker.classList.remove("sp-open");
    input.value = currentIcon;
    highlighted = -1;
  }

  function getVisible() {
    var vis = [];
    if (optionEls) {
      for (var i = 0; i < optionEls.length; i++) {
        if (optionEls[i].style.display !== "none") vis.push(optionEls[i]);
      }
    }
    return vis;
  }

  function highlightAt(idx) {
    var visible = getVisible();
    if (visible.length === 0) return;
    if (optionEls) optionEls.forEach(function (el) { el.classList.remove("sp-highlighted"); });
    if (idx < 0) idx = visible.length - 1;
    if (idx >= visible.length) idx = 0;
    highlighted = idx;
    visible[highlighted].classList.add("sp-highlighted");
    visible[highlighted].scrollIntoView({ block: "nearest" });
  }

  input.addEventListener("focus", openPicker);
  input.addEventListener("blur", closePicker);

  input.addEventListener("input", function () {
    filterOpts(this.value);
    var vis = getVisible();
    if (vis.length > 0) highlightAt(0);
  });

  input.addEventListener("keydown", function (e) {
    if (e.key === "ArrowDown") {
      e.preventDefault();
      if (!picker.classList.contains("sp-open")) { openPicker(); return; }
      highlightAt(highlighted + 1);
    } else if (e.key === "ArrowUp") {
      e.preventDefault();
      highlightAt(highlighted - 1);
    } else if (e.key === "Enter") {
      e.preventDefault();
      var visible = getVisible();
      if (highlighted >= 0 && highlighted < visible.length) {
        selectOpt(visible[highlighted]._optName);
      }
    } else if (e.key === "Tab") {
      var visible = getVisible();
      if (picker.classList.contains("sp-open") && highlighted >= 0 && highlighted < visible.length) {
        selectOpt(visible[highlighted]._optName);
      }
    } else if (e.key === "Escape") {
      e.preventDefault();
      closePicker();
      input.blur();
    }
  });
}
