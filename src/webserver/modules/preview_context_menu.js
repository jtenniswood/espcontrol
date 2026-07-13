// ── Preview Context Menu ──────────────────────────────────────────
// @web-module-requires: state, clock_bar_state, grid, config_codec, config_post_api, api, button_settings_selection, button_settings, preview_render

// ── Context menu (unified) ─────────────────────────────────────────────

var ctxMenu = null;

function positionMenu(menu, e) {
  var position = PreviewFeature.clampMenuPosition(
    { x: e.clientX, y: e.clientY },
    menu.offsetWidth,
    menu.offsetHeight,
    window.innerWidth,
    window.innerHeight
  );
  menu.style.left = position.x + "px";
  menu.style.top = position.y + "px";
}

function addCtxItem(icon, text, handler, danger) {
  var item = document.createElement("div");
  item.className = "sp-ctx-item" + (danger ? " sp-ctx-danger" : "");
  item.appendChild(mdiIcon(icon));
  item.appendChild(document.createTextNode(text));
  item.addEventListener("mousedown", function (ev) {
    ev.preventDefault();
    ev.stopPropagation();
    hideContextMenu();
    handler();
  });
  ctxMenu.appendChild(item);
}

function addCtxDivider() {
  var div = document.createElement("div");
  div.className = "sp-ctx-divider";
  ctxMenu.appendChild(div);
}

function addCtxSubmenu(icon, text, buildFn) {
  var wrapper = document.createElement("div");
  wrapper.className = "sp-ctx-item sp-ctx-sub";
  wrapper.appendChild(mdiIcon(icon));
  wrapper.appendChild(document.createTextNode(text));
  var sub = document.createElement("div");
  sub.className = "sp-ctx-submenu";
  buildFn(sub);
  wrapper.appendChild(sub);
  wrapper.addEventListener("mouseenter", function () {
    sub.style.left = "100%"; sub.style.right = "auto";
    var r = sub.getBoundingClientRect();
    if (r.right > window.innerWidth - 4) { sub.style.left = "auto"; sub.style.right = "100%"; }
  });
  wrapper.addEventListener("mousedown", function (ev) { ev.preventDefault(); ev.stopPropagation(); });
  ctxMenu.appendChild(wrapper);
}

function addSubItem(container, icon, text, handler, active) {
  var item = document.createElement("div");
  item.className = "sp-ctx-item";
  if (active) {
    item.appendChild(mdiIcon("check", "sp-ctx-check mdi"));
  } else {
    var spacer = document.createElement("span");
    spacer.style.width = "18px";
    item.appendChild(spacer);
  }
  item.appendChild(document.createTextNode(text));
  item.addEventListener("mousedown", function (ev) {
    ev.preventDefault();
    ev.stopPropagation();
    hideContextMenu();
    handler();
  });
  container.appendChild(item);
}

function resizeSlot(slot, targetSz) {
  if (isConfigLocked()) return;
  var c = ctx();
  var slotPos = slot === -2 ? c.grid.indexOf(-2) : c.grid.indexOf(slot);
  if (slotPos < 0) return;
  var curSz = c.sizes[slot] || 1;
  if (curSz === targetSz) return;

  var oldCells = coveredCells(slotPos, curSz, c.maxSlots, false);
  for (var oi = 0; oi < oldCells.length; oi++) {
    if (c.grid[oldCells[oi]] === -1) c.grid[oldCells[oi]] = 0;
  }

  if (targetSz > 1 && !sizeFitsAt(slotPos, targetSz, c.maxSlots)) {
    delete c.sizes[slot];
    return;
  }
  var need = coveredCells(slotPos, targetSz, c.maxSlots, false);

  for (var i = 0; i < need.length; i++) {
    var p = need[i];
    if (c.grid[p] > 0 || c.grid[p] === -2) {
      if (c.isSub && c.grid[p] > 0) return;
      var displaced = c.grid[p];
      c.grid[p] = 0;
      if (c.isSub) {
        for (var j = 0; j < c.maxSlots; j++) { if (c.grid[j] === 0 && need.indexOf(j) === -1) { c.grid[j] = displaced; break; } }
      } else {
        var fc = firstFreeCell(p + 1);
        if (fc >= 0) c.grid[fc] = displaced;
      }
    }
  }
  for (var i = 0; i < need.length; i++) c.grid[need[i]] = -1;

  if (targetSz === 1) delete c.sizes[slot]; else c.sizes[slot] = targetSz;

  if (c.isSub) {
    var sp = getSubpage(state.editingSubpage);
    sp.order = serializeSubpageGrid(sp);
    saveSubpageConfig(state.editingSubpage);
  } else {
    postText(entityName("button_order"), serializeGrid(state.grid));
  }
  renderPreview();
  renderButtonSettings();
}


function addBulkCardMenuItems(slots) {
  addCtxItem("clipboard-outline", "Copy " + slots.length + " Cards", function () { copyButtons(slots); });
  addCtxItem("content-cut", "Cut " + slots.length + " Cards", function () { cutButtons(slots); });
  addCtxItem("delete", "Delete " + slots.length + " Cards", function () { deleteButtons(slots); }, true);
}

function addSingleCardMenuItems(slot) {
  if (slot === -2) {
    addBackButtonMenuItems();
    return;
  }

  var c = ctx();
  var b = c.buttons[slot - 1];
  addCtxItem("pencil", "Edit Card", function () { openCardSettings(slot); });

  var ctxTypeDef = BUTTON_TYPES[(b && b.type) || ""];
  if (ctxTypeDef && ctxTypeDef.contextMenuItems &&
      (!c.isSub || buttonTypeRegistryValue(ctxTypeDef, "allowInSubpage", false))) {
    ctxTypeDef.contextMenuItems(slot, b, { addCtxItem: addCtxItem });
  }

  var sz = c.sizes[slot] || 1;
  addCtxSubmenu("arrow-expand-all", "Size", function (sub) {
    addSubItem(sub, "", "Single (1x1)", function () { resizeSlot(slot, 1); }, sz === 1);
    addSubItem(sub, "", "Tall (2x1)", function () { resizeSlot(slot, 2); }, sz === 2);
    addSubItem(sub, "", "Extra Tall (3x1)", function () { resizeSlot(slot, 5); }, sz === 5);
    addSubItem(sub, "", "Wide (1x2)", function () { resizeSlot(slot, 3); }, sz === 3);
    addSubItem(sub, "", "Extra Wide (1x3)", function () { resizeSlot(slot, 6); }, sz === 6);
    addSubItem(sub, "", "Large (2x2)", function () { resizeSlot(slot, 4); }, sz === 4);
  });

  addCtxDivider();
  addCtxItem("content-copy", "Duplicate", function () {
    if (c.isSub) { duplicateSubpageButton(slot); } else { duplicateButton(slot); }
  });

  addCtxItem("clipboard-outline", "Copy", function () { copySlot(slot); });
  addCtxItem("content-cut", "Cut", function () { cutSlot(slot); });
  addCtxItem("delete", "Delete", function () { deleteSlot(slot); }, true);
}

function addClockBarMenuItems(item) {
  if (isClockBarTemperatureItem(item)) {
    addCtxItem("pencil", "Edit Temperature", function () { openClockBarTemperatureSettings(); });
    addCtxDivider();
  } else if (item === "voice") {
    addCtxItem("pencil", "Edit Voice Services", function () { openVoiceServicesSettings(); });
    addCtxDivider();
  }

  var visible = clockBarItemActive(item);
  var label = clockBarItemLabel(item);
  addCtxItem(visible ? "eye-off-outline" : "eye-outline", (visible ? "Hide " : "Show ") + label, function () {
    setClockBarItemVisible(item, !visible);
  });
}

function showSelectionMenu(e) {
  if (isConfigLocked()) return;
  hideContextMenu();
  var c = ctx();
  if (!c.selected.length) return;

  ctxMenu = document.createElement("div");
  ctxMenu.className = "sp-ctx-menu";
  if (c.selected.length > 1) {
    addBulkCardMenuItems(c.selected.slice());
  } else {
    addSingleCardMenuItems(c.selected[0]);
  }
  document.body.appendChild(ctxMenu);
  positionMenu(ctxMenu, e);
}

function showClockBarContextMenu(e, item) {
  if (isConfigLocked() || clockBarItems().indexOf(item) === -1) return;
  hideContextMenu();
  var c = ctx();

  if (state.clockBarSelectedItem !== item) {
    c.setSelected([]);
    c.setLastClicked(-1);
    state.clockBarSelectedItem = item;
    hideSettingsOverlay();
    updateClockBarItemUi();
    renderPreview();
    renderButtonSettings();
  }

  ctxMenu = document.createElement("div");
  ctxMenu.className = "sp-ctx-menu";
  addClockBarMenuItems(item);
  document.body.appendChild(ctxMenu);
  positionMenu(ctxMenu, e);
}

function showContextMenu(e, slot) {
  if (isConfigLocked()) return;
  hideContextMenu();
  var c = ctx();

  if (c.selected.indexOf(slot) === -1) {
    if (c.selected.length > 1) {
      c.selected.push(slot);
    } else {
      c.setSelected([slot]);
      c.setLastClicked(slot);
    }
    renderPreview();
    renderButtonSettings();
    c = ctx();
  }

  ctxMenu = document.createElement("div");
  ctxMenu.className = "sp-ctx-menu";

  if (c.selected.length > 1 && c.selected.indexOf(slot) !== -1) {
    addBulkCardMenuItems(c.selected.slice());
  } else {
    addSingleCardMenuItems(slot);
  }

  document.body.appendChild(ctxMenu);
  positionMenu(ctxMenu, e);
}

function showBackContextMenu(e) {
  if (isConfigLocked()) return;
  hideContextMenu();
  ctxMenu = document.createElement("div");
  ctxMenu.className = "sp-ctx-menu";
  addBackButtonMenuItems();
  document.body.appendChild(ctxMenu);
  positionMenu(ctxMenu, e);
}

function addBackButtonMenuItems() {
  var sp = getSubpage(state.editingSubpage);
  var bkSz = sp.sizes[-2] || 1;
  addCtxItem("pencil", "Edit Label", function () { openCardSettings(-2); });
  addCtxItem("keyboard-return", "Exit Subpage", function () { exitSubpage(); });
  addCtxDivider();
  addCtxSubmenu("arrow-expand-all", "Size", function (sub) {
    addSubItem(sub, "", "Single (1x1)", function () { resizeSlot(-2, 1); }, bkSz === 1);
    addSubItem(sub, "", "Tall (2x1)", function () { resizeSlot(-2, 2); }, bkSz === 2);
    addSubItem(sub, "", "Extra Tall (3x1)", function () { resizeSlot(-2, 5); }, bkSz === 5);
    addSubItem(sub, "", "Wide (1x2)", function () { resizeSlot(-2, 3); }, bkSz === 3);
    addSubItem(sub, "", "Extra Wide (1x3)", function () { resizeSlot(-2, 6); }, bkSz === 6);
    addSubItem(sub, "", "Large (2x2)", function () { resizeSlot(-2, 4); }, bkSz === 4);
  });
}

function showEmptySlotMenu(e, pos) {
  if (isConfigLocked()) return;
  hideContextMenu();
  ctxMenu = document.createElement("div");
  ctxMenu.className = "sp-ctx-menu";
  var c = ctx();
  if (state.clipboard) {
    var count = state.clipboard.buttons.length;
    addCtxItem("content-paste", count > 1 ? "Paste " + count + " Cards" : "Paste", function () {
      if (c.isSub) {
        pasteSubpageButton(pos);
      } else {
        pasteButton(pos);
      }
    });
    addCtxDivider();
  }
  addCtxItem("plus", "Create Card", function () { addSlot(pos); });
  if (!c.isSub) {
    addCtxItem("folder-plus", "Create Subpage", function () { addSubpageSlot(pos); });
  }
  document.body.appendChild(ctxMenu);
  positionMenu(ctxMenu, e);
}

function hideContextMenu() {
  if (ctxMenu && ctxMenu.parentNode) {
    ctxMenu.parentNode.removeChild(ctxMenu);
  }
  ctxMenu = null;
}
