// ── Preview event delegation & drag ────────────────────────────────────

function resolveSpanPos(pos) {
  var c = ctx();
  if (c.grid[pos] === -1) {
    for (var anchor = 0; anchor < c.maxSlots; anchor++) {
      var slot = c.grid[anchor];
      if (!(slot > 0 || slot === -2)) continue;
      var cells = coveredCells(anchor, c.sizes[slot] || 1, c.maxSlots, false);
      if (cells.indexOf(pos) !== -1) return anchor;
    }
  }
  return pos;
}

function getCellFromEvent(e, container) {
  if (CFG.dragMode === "swap") {
    var rect = container.getBoundingClientRect();
    var col = Math.floor((e.clientX - rect.left) / (rect.width / GRID_COLS));
    var row = Math.floor((e.clientY - rect.top) / (rect.height / GRID_ROWS));
    col = Math.max(0, Math.min(col, GRID_COLS - 1));
    row = Math.max(0, Math.min(row, GRID_ROWS - 1));
    return resolveSpanPos(row * GRID_COLS + col);
  }
  var x = e.clientX, y = e.clientY;
  var children = container.children;
  var skip = 0;
  var bestDist = Infinity, bestPos = -1;
  for (var i = skip; i < children.length; i++) {
    var r = children[i].getBoundingClientRect();
    var pos = parseInt(children[i].getAttribute("data-pos"), 10);
    if (isNaN(pos)) continue;
    if (x >= r.left && x <= r.right && y >= r.top && y <= r.bottom) return pos;
    var cx = (r.left + r.right) / 2, cy = (r.top + r.bottom) / 2;
    var d = (x - cx) * (x - cx) + (y - cy) * (y - cy);
    if (d < bestDist) { bestDist = d; bestPos = pos; }
  }
  return bestPos;
}

function moveToCell(fromPos, toPos) {
  var c = ctx();
  toPos = resolveSpanPos(toPos);
  if (toPos >= c.maxSlots || c.grid[toPos] === -1) return;
  var grid = c.grid.slice();
  var movingSlot = grid[fromPos];
  clearSpans(grid, c.maxSlots);
  var targetSlot = grid[toPos];
  grid[toPos] = movingSlot;
  grid[fromPos] = targetSlot;
  applySpans(grid, c.sizes, c.maxSlots);
  if ((c.sizes[movingSlot] || 1) > 1 && !sizeFitsAt(toPos, c.sizes[movingSlot], c.maxSlots)) {
    delete c.sizes[movingSlot];
  }
  if (c.isSub) {
    getSubpage(state.editingSubpage).grid = grid;
  } else {
    state.grid = grid;
  }
}


function canPlaceSlotAt(grid, pos, size, maxSlots) {
  if (pos < 0 || pos >= maxSlots || grid[pos] !== 0) return false;
  if (!sizeFitsAt(pos, size, maxSlots)) return false;
  var cells = coveredCells(pos, size, maxSlots, false);
  for (var i = 0; i < cells.length; i++) {
    if (grid[cells[i]] !== 0) return false;
  }
  return true;
}

function findPlacementCell(grid, start, size, maxSlots) {
  for (var i = 0; i < maxSlots; i++) {
    var candidate = (start + i) % maxSlots;
    if (canPlaceSlotAt(grid, candidate, size, maxSlots)) return candidate;
  }
  return -1;
}

function findDuplicatePlacement(grid, start, size, maxSlots) {
  var targetSize = size || 1;
  var pos = findPlacementCell(grid, start, targetSize, maxSlots);
  if (pos >= 0) return { pos: pos, size: targetSize };
  if (targetSize !== 1) {
    pos = findPlacementCell(grid, start, 1, maxSlots);
    if (pos >= 0) return { pos: pos, size: 1 };
  }
  return { pos: -1, size: targetSize };
}

function placeSlotAt(grid, slot, pos, size) {
  grid[pos] = slot;
  markSpannedCells(grid, pos, size, grid.length);
}

function placeOrderedGridEntries(entries, sizes, maxSlots) {
  var grid = [];
  for (var i = 0; i < maxSlots; i++) grid.push(0);

  for (var j = 0; j < entries.length && j < maxSlots; j++) {
    var slot = entries[j];
    if (!(slot > 0 || slot === -2)) continue;

    var targetSize = sizes[slot] || 1;
    var place = j;
    if (!canPlaceSlotAt(grid, place, targetSize, maxSlots)) {
      place = findPlacementCell(grid, place, targetSize, maxSlots);
    }
    if (place < 0 && targetSize !== 1) {
      targetSize = 1;
      place = canPlaceSlotAt(grid, j, targetSize, maxSlots)
        ? j
        : findPlacementCell(grid, j, targetSize, maxSlots);
    }
    if (place < 0) continue;

    if (targetSize === 1) delete sizes[slot]; else sizes[slot] = targetSize;
    placeSlotAt(grid, slot, place, targetSize);
  }

  return grid;
}

function moveSelectedToCell(fromPos, toPos) {
  var c = ctx();
  toPos = resolveSpanPos(toPos);
  if (toPos < 0 || toPos >= c.maxSlots) return false;

  var sourceEntries = c.grid.slice();
  clearSpans(sourceEntries, c.maxSlots);

  var movingSlot = sourceEntries[fromPos];
  if (movingSlot === -2 || c.selected.indexOf(-2) !== -1) return false;
  if (c.selected.length <= 1 || c.selected.indexOf(movingSlot) === -1) return false;

  var movingSlots = c.selected.slice();
  var targetSlot = sourceEntries[toPos];
  if (targetSlot > 0 && c.selected.indexOf(targetSlot) !== -1) return true;

  var entries = [];
  for (var i = 0; i < c.maxSlots; i++) {
    var entry = sourceEntries[i];
    if (entry > 0 && c.selected.indexOf(entry) !== -1) continue;
    entries.push(entry);
  }
  while (entries.length < c.maxSlots) entries.push(0);

  var insertPos;
  if (targetSlot > 0 || targetSlot === -2) {
    insertPos = entries.indexOf(targetSlot);
    insertPos = insertPos < 0 ? toPos : insertPos + 1;
  } else {
    insertPos = toPos;
    for (var r = 0; r < toPos; r++) {
      if (sourceEntries[r] > 0 && c.selected.indexOf(sourceEntries[r]) !== -1) insertPos--;
    }
  }
  insertPos = Math.max(0, Math.min(insertPos, entries.length));
  entries.splice.apply(entries, [insertPos, 0].concat(movingSlots));
  entries = entries.slice(0, c.maxSlots);

  var grid = placeOrderedGridEntries(entries, c.sizes, c.maxSlots);

  if (c.isSub) {
    getSubpage(state.editingSubpage).grid = grid;
  } else {
    state.grid = grid;
  }
  return true;
}

function clearPlaceholder() {
  if (previewPlaceholder) {
    previewPlaceholder.classList.remove("sp-drop-placeholder");
    previewPlaceholder = null;
  }
}

function clearTextSelection() {
  var selection = window.getSelection && window.getSelection();
  if (selection && selection.removeAllRanges) selection.removeAllRanges();
}

function setupPreviewEvents() {
  var container = els.previewMain;
  var pendingCellIdx = -1;

  state.clockBarDragItem = "";

  if (els.topbar) {
    els.topbar.addEventListener("click", function (e) {
      if (isConfigLocked()) {
        e.preventDefault();
        e.stopPropagation();
        return;
      }
      var target = e.target.closest("[data-clockbar-item]");
      if (!target) return;
      e.preventDefault();
      e.stopPropagation();
      selectClockBarItem(target.getAttribute("data-clockbar-item"));
    });
    els.topbar.addEventListener("keydown", function (e) {
      if (e.key !== "Enter" && e.key !== " ") return;
      var target = e.target.closest("[data-clockbar-item]");
      if (!target) return;
      e.preventDefault();
      selectClockBarItem(target.getAttribute("data-clockbar-item"));
    });
    els.topbar.addEventListener("contextmenu", function (e) {
      if (isConfigLocked()) {
        e.preventDefault();
        return;
      }
      var target = e.target.closest("[data-clockbar-item]");
      if (!target) return;
      e.preventDefault();
      e.stopPropagation();
      showClockBarContextMenu(e, target.getAttribute("data-clockbar-item"));
    });
  }

  function isBackExitTarget(e, target) {
    var icon = target.querySelector(".sp-back-hit");
    if (!icon) return false;
    var rect = icon.getBoundingClientRect();
    var pad = 12;
    return e.clientX >= rect.left - pad &&
      e.clientX <= rect.right + pad &&
      e.clientY >= rect.top - pad &&
      e.clientY <= rect.bottom + pad;
  }

  container.addEventListener("mousedown", function (e) {
    if (isConfigLocked()) return;
    if (!e.target.closest("[data-pos]")) return;
    if (e.shiftKey || e.ctrlKey || e.metaKey) e.preventDefault();
  });

  // Click delegation
  container.addEventListener("click", function (e) {
    if (isConfigLocked()) {
      e.preventDefault();
      e.stopPropagation();
      return;
    }
    if (e.target.closest(".sp-subpage-badge")) {
      var btnEl = e.target.closest("[data-slot]");
      if (btnEl) {
        var badgeSlot = parseInt(btnEl.getAttribute("data-slot"), 10);
        enterSubpage(badgeSlot);
        return;
      }
    }
    var target = e.target.closest("[data-pos]");
    if (!target) return;
    var pos = parseInt(target.getAttribute("data-pos"), 10);
    var c = ctx();
    var slot = c.grid[pos];
    if (slot > 0) {
      handleBtnClick(e, slot, pos);
    } else if (slot === -2) {
      if (didDrag) { didDrag = false; return; }
      if (isBackExitTarget(e, target)) {
        exitSubpage();
      } else {
        handleBtnClick(e, slot, pos);
      }
    } else if (slot === 0) {
      if (state.clipboard) {
        e.preventDefault();
        e.stopPropagation();
        showEmptySlotMenu(e, pos);
      } else {
        addSlot(pos);
      }
    }
  });

  // Context menu delegation
  container.addEventListener("contextmenu", function (e) {
    if (isConfigLocked()) {
      e.preventDefault();
      return;
    }
    var target = e.target.closest("[data-pos]");
    if (!target) return;
    e.preventDefault();
    var pos = parseInt(target.getAttribute("data-pos"), 10);
    var c = ctx();
    var slot = c.grid[pos];
    if (slot > 0) {
      showContextMenu(e, slot);
    } else if (slot === -2) {
      showBackContextMenu(e);
    } else if (slot === 0) {
      showEmptySlotMenu(e, pos);
    }
  });

  // Drag delegation
  container.addEventListener("dragstart", function (e) {
    if (isConfigLocked()) {
      e.preventDefault();
      return;
    }
    var target = e.target.closest(".sp-btn") || e.target.closest(".sp-back-btn");
    if (!target) return;
    var pos = parseInt(target.getAttribute("data-pos"), 10);
    dragSrcPos = pos;
    if (CFG.dragAnimation) dragSrcEl = target;
    dragIsSubpage = !!state.editingSubpage;
    didDrag = true;
    dragEnterCount = 0;
    container.classList.add("sp-drag-active");
    e.dataTransfer.effectAllowed = "move";
    e.dataTransfer.setData("text/plain", String(pos));
    if (CFG.dragAnimation) {
      requestAnimationFrame(function () { target.classList.add("sp-dragging"); });
    }
  });

  container.addEventListener("dragend", function () {
    dragSrcPos = -1;
    previewDropIdx = -1;
    dragIsSubpage = false;
    dragEnterCount = 0;
    clearPlaceholder();
    if (dragSrcEl) { dragSrcEl.classList.remove("sp-dragging"); dragSrcEl = null; }
    setTimeout(function () { container.classList.remove("sp-drag-active"); }, 50);
  });

  // Drop zone
  function updatePlaceholder(cellIdx) {
    if (cellIdx === previewDropIdx) return;
    previewDropIdx = cellIdx;
    clearPlaceholder();
    var target = container.querySelector('[data-pos="' + cellIdx + '"]');
    if (target) {
      previewPlaceholder = target;
      previewPlaceholder.classList.add("sp-drop-placeholder");
    }
  }

  container.addEventListener("dragenter", function (e) {
    if (isConfigLocked()) return;
    if (dragSrcPos < 0) return;
    e.preventDefault();
    dragEnterCount++;
  });

  container.addEventListener("dragover", function (e) {
    if (isConfigLocked()) return;
    if (dragSrcPos < 0) return;
    e.preventDefault();
    e.dataTransfer.dropEffect = "move";
    if (CFG.dragAnimation) {
      pendingCellIdx = getCellFromEvent(e, container);
      if (dragRafPending) return;
      dragRafPending = true;
      requestAnimationFrame(function () {
        dragRafPending = false;
        if (dragSrcPos < 0) return;
        updatePlaceholder(pendingCellIdx);
      });
    } else {
      updatePlaceholder(getCellFromEvent(e, container));
    }
  });

  container.addEventListener("dragleave", function () {
    dragEnterCount--;
    if (dragEnterCount <= 0) {
      dragEnterCount = 0;
      previewDropIdx = -1;
      clearPlaceholder();
    }
  });

  container.addEventListener("drop", function (e) {
    if (isConfigLocked()) {
      e.preventDefault();
      return;
    }
    e.preventDefault();
    dragEnterCount = 0;
    var toPos = previewDropIdx;
    previewDropIdx = -1;
    clearPlaceholder();
    if (dragSrcEl) { dragSrcEl.classList.remove("sp-dragging"); dragSrcEl = null; }
    var c = ctx();
    if (dragSrcPos < 0 || toPos < 0 || toPos >= c.maxSlots) { dragSrcPos = -1; dragIsSubpage = false; return; }
    if (dragSrcPos === toPos) { dragSrcPos = -1; dragIsSubpage = false; return; }
    if (!moveSelectedToCell(dragSrcPos, toPos)) moveToCell(dragSrcPos, toPos);
    renderPreview();
    renderButtonSettings();
    c.save();
    dragSrcPos = -1;
    dragIsSubpage = false;
  });
}

function handleBtnClick(e, slot, pos) {
  if (isConfigLocked()) return;
  if (didDrag) { didDrag = false; return; }
  state.clockBarSelectedItem = "";
  var c = ctx();
  if (e.shiftKey || e.ctrlKey || e.metaKey) e.preventDefault();

  if (slot === -2) {
    if (c.selected.length === 1 && c.selected[0] === -2) {
      c.setSelected([]);
    } else {
      c.setSelected([-2]);
    }
    c.setLastClicked(-1);
    renderPreview();
    renderButtonSettings();
    clearTextSelection();
    return;
  }

  if (e.shiftKey && c.getLastClicked() > 0) {
    var anchorPos = c.grid.indexOf(c.getLastClicked());
    if (anchorPos !== -1) {
      var from = Math.min(anchorPos, pos);
      var to = Math.max(anchorPos, pos);
      var newSel = [];
      for (var i = from; i <= to; i++) {
        if (c.grid[i] > 0) newSel.push(c.grid[i]);
      }
      c.setSelected(newSel);
      renderPreview();
      hideSettingsOverlay();
      clearTextSelection();
      return;
    }
  }

  if (e.ctrlKey || e.metaKey) {
    var idx = c.selected.indexOf(slot);
    if (idx !== -1) {
      c.selected.splice(idx, 1);
    } else {
      c.selected.push(slot);
      c.setLastClicked(slot);
    }
    renderPreview();
    hideSettingsOverlay();
    clearTextSelection();
    return;
  }

  if (c.selected.length === 1 && c.selected[0] === slot) {
    c.setSelected([]);
    c.setLastClicked(-1);
  } else {
    c.setSelected([slot]);
    c.setLastClicked(slot);
  }
  renderPreview();
  renderButtonSettings();
}

function selectButton(slot) {
  if (isConfigLocked()) return;
  if (slot < 1) {
    state.selectedSlots = [];
  } else {
    state.selectedSlots = [slot];
    state.lastClickedSlot = slot;
  }
  renderPreview();
  renderButtonSettings();
}

// ── Button management (unified) ────────────────────────────────────────

function firstFreeSlot() {
  var used = {};
  state.grid.forEach(function (s) { if (s > 0) used[s] = true; });
  for (var i = 1; i <= NUM_SLOTS; i++) {
    if (!used[i]) return i;
  }
  return -1;
}

function firstFreeCell(afterPos) {
  var start = afterPos != null ? afterPos : 0;
  for (var i = 0; i < NUM_SLOTS; i++) {
    var candidate = (start + i) % NUM_SLOTS;
    if (state.grid[candidate] === 0) return candidate;
  }
  return -1;
}

function emptyButtonConfig(type) {
  return EspControlModel.emptyCardConfig(type);
}

function newCardDraftKey(isSub, homeSlot, pos, slot) {
  return (isSub ? "sub:" + homeSlot : "main") + ":new:" + pos + ":" + slot;
}

function beginNewCardDraft(pos, slot, isSub) {
  state.settingsDraft = {
    key: newCardDraftKey(isSub, state.editingSubpage, pos, slot),
    slot: slot,
    homeSlot: state.editingSubpage,
    isSub: isSub,
    isNew: true,
    pos: pos,
    dirty: false,
    typeSelected: false,
    button: emptyButtonConfig(),
  };
  if (isSub) {
    state.subpageSelectedSlots = [slot];
    state.subpageLastClicked = slot;
  } else {
    state.selectedSlots = [slot];
    state.lastClickedSlot = slot;
  }
  renderPreview();
  renderButtonSettings(true);
}

function addSlot(pos) {
  if (isConfigLocked()) return;
  var c = ctx();
  if (pos < 0 || pos >= c.maxSlots || c.grid[pos] !== 0) return;
  if (c.isSub) {
    var sp = getSubpage(state.editingSubpage);
    var newSlot = subpageFirstFreeSlot(sp);
    beginNewCardDraft(pos, newSlot, true);
  } else {
    var slot = firstFreeSlot();
    if (slot < 0) return;
    beginNewCardDraft(pos, slot, false);
  }
}

function addSubpageSlot(pos) {
  if (isConfigLocked()) return;
  var c = ctx();
  if (c.isSub) return;
  var slot = firstFreeSlot();
  if (slot < 0) return;
  state.buttons[slot - 1] = emptyButtonConfig("subpage");
  state.grid[pos] = slot;
  state.subpages[slot] = { order: [], buttons: [], grid: [], sizes: {} };
  buildSubpageGrid(state.subpages[slot]);
  postText(entityName("button_order"), serializeGrid(state.grid));
  saveButtonConfig(slot);
  saveSubpageEntity(slot);
  selectButton(slot);
}

function duplicateButton(srcSlot) {
  if (isConfigLocked()) return;
  var newSlot = firstFreeSlot();
  if (newSlot < 0) return;
  var srcSz = state.sizes[srcSlot] || 1;
  var srcPos = state.grid.indexOf(srcSlot);
  var placement = findDuplicatePlacement(state.grid, srcPos + 1, srcSz, NUM_SLOTS);
  if (placement.pos < 0) return;

  var src = state.buttons[srcSlot - 1];
  var extraImageCards = isImageCard(src) ? 1 : 0;
  if (state.subpages[srcSlot]) extraImageCards += imageCardCountInSubpage(state.subpages[srcSlot]);
  if (!canAddImageCards(extraImageCards)) {
    showImageCardLimitBanner();
    return;
  }
  state.buttons[newSlot - 1] = {
    entity: src.entity, label: src.label, icon: src.icon,
    icon_on: src.icon_on, sensor: src.sensor, unit: src.unit,
    type: src.type || "", precision: src.precision || "",
    options: src.options || "",
  };

  if (placement.size === 1) delete state.sizes[newSlot]; else state.sizes[newSlot] = placement.size;
  placeSlotAt(state.grid, newSlot, placement.pos, placement.size);

  if (state.subpages[srcSlot]) {
    var spJson = serializeSubpageConfig(state.subpages[srcSlot]);
    var spCopy = parseSubpageConfig(spJson);
    spCopy.sizes = {};
    buildSubpageGrid(spCopy);
    state.subpages[newSlot] = spCopy;
  }
  postText(entityName("button_order"), serializeGrid(state.grid));
  saveButtonConfig(newSlot);
  saveSubpageEntity(newSlot);
  state.selectedSlots = [newSlot];
  state.lastClickedSlot = newSlot;
  renderPreview();
}

function duplicateSubpageButton(srcSlot) {
  if (isConfigLocked()) return;
  var homeSlot = state.editingSubpage;
  var sp = getSubpage(homeSlot);
  var newSlot = subpageFirstFreeSlot(sp);
  while (sp.buttons.length < newSlot) {
    sp.buttons.push(emptyButtonConfig());
  }
  var srcSz = sp.sizes[srcSlot] || 1;
  var srcPos = sp.grid.indexOf(srcSlot);
  var placement = findDuplicatePlacement(sp.grid, srcPos + 1, srcSz, NUM_SLOTS);
  if (placement.pos < 0) return;

  var src = sp.buttons[srcSlot - 1];
  if (!canAddImageCards(isImageCard(src) ? 1 : 0)) {
    showImageCardLimitBanner();
    return;
  }
  sp.buttons[newSlot - 1] = {
    entity: src.entity, label: src.label, icon: src.icon,
    icon_on: src.icon_on, sensor: src.sensor, unit: src.unit,
    type: src.type || "", precision: src.precision || "",
    options: src.options || "",
  };

  if (placement.size === 1) delete sp.sizes[newSlot]; else sp.sizes[newSlot] = placement.size;
  placeSlotAt(sp.grid, newSlot, placement.pos, placement.size);

  sp.order = serializeSubpageGrid(sp);
  saveSubpageConfig(homeSlot);
  state.subpageSelectedSlots = [newSlot];
  state.subpageLastClicked = newSlot;
  renderPreview();
}

function deleteSlot(slot) {
  if (isConfigLocked()) return;
  var c = ctx();
  for (var i = 0; i < c.maxSlots; i++) {
    if (c.grid[i] === slot) {
      c.grid[i] = 0;
      var cells = coveredCells(i, c.sizes[slot] || 1, c.maxSlots, false);
      for (var ci = 0; ci < cells.length; ci++) {
        if (c.grid[cells[ci]] === -1) c.grid[cells[ci]] = 0;
      }
      break;
    }
  }
  delete c.sizes[slot];

  var selIdx = c.selected.indexOf(slot);
  if (selIdx !== -1) c.selected.splice(selIdx, 1);

  if (c.isSub) {
    var sp = getSubpage(state.editingSubpage);
    if (slot >= 1 && slot <= sp.buttons.length) {
      sp.buttons[slot - 1] = emptyButtonConfig();
    }
    sp.order = serializeSubpageGrid(sp);
    state.subpageLastClicked = -1;
    saveSubpageConfig(state.editingSubpage);
  } else {
    postText(entityName("button_order"), serializeGrid(state.grid));
    state.buttons[slot - 1] = emptyButtonConfig();
    delete state.subpages[slot];
    saveButtonConfig(slot);
    saveSubpageEntity(slot);
  }

  renderPreview();
  renderButtonSettings();
}

function deleteButtons(slots) {
  if (isConfigLocked()) return;
  var c = ctx();
  for (var i = 0; i < c.maxSlots; i++) {
    if (slots.indexOf(c.grid[i]) !== -1) {
      var cells = coveredCells(i, c.sizes[c.grid[i]] || 1, c.maxSlots, false);
      for (var ci = 0; ci < cells.length; ci++) {
        if (c.grid[cells[ci]] === -1) c.grid[cells[ci]] = 0;
      }
      c.grid[i] = 0;
    }
  }
  slots.forEach(function (slot) { delete c.sizes[slot]; });
  c.setSelected([]);
  c.setLastClicked(-1);
  if (c.isSub) {
    var sp = getSubpage(state.editingSubpage);
    slots.forEach(function (slot) {
      if (slot >= 1 && slot <= sp.buttons.length) {
        sp.buttons[slot - 1] = emptyButtonConfig();
      }
    });
    sp.order = serializeSubpageGrid(sp);
    saveSubpageConfig(state.editingSubpage);
  } else {
    slots.forEach(function (slot) {
      state.buttons[slot - 1] = emptyButtonConfig();
      delete state.subpages[slot];
      saveButtonConfig(slot);
      saveSubpageEntity(slot);
    });
    postText(entityName("button_order"), serializeGrid(state.grid));
  }
  renderPreview();
  renderButtonSettings();
}

// ── Context menu (unified) ─────────────────────────────────────────────

var ctxMenu = null;

function positionMenu(menu, e) {
  var w = menu.offsetWidth, h = menu.offsetHeight;
  var x = Math.max(4, Math.min(e.clientX, window.innerWidth - w - 4));
  var y = Math.max(4, Math.min(e.clientY, window.innerHeight - h - 4));
  menu.style.left = x + "px";
  menu.style.top = y + "px";
}

function addCtxItem(icon, text, handler, danger) {
  var item = document.createElement("div");
  item.className = "sp-ctx-item" + (danger ? " sp-ctx-danger" : "");
  item.innerHTML = '<span class="mdi mdi-' + icon + '"></span>' + escHtml(text);
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
  wrapper.innerHTML = '<span class="mdi mdi-' + icon + '"></span>' + escHtml(text);
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
  item.innerHTML = (active ? '<span class="sp-ctx-check mdi mdi-check"></span>' : '<span style="width:18px"></span>') + escHtml(text);
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

// ── Cut / Paste ────────────────────────────────────────────────────────

function buildClipboardEntry(slot) {
  if (slot < 1) return null;
  var c = ctx();
  var src = c.buttons[slot - 1];
  var entry = {
    entity: src.entity, label: src.label, icon: src.icon,
    icon_on: src.icon_on, sensor: src.sensor, unit: src.unit,
    type: src.type || "", precision: src.precision || "",
    options: src.options || "",
    subpageConfig: null, size: c.sizes[slot] || 1,
  };
  if (!c.isSub && src.type === "subpage" && state.subpages[slot]) {
    entry.subpageConfig = serializeSubpageConfig(state.subpages[slot]);
  }
  return entry;
}

function copySlot(slot) {
  var entry = buildClipboardEntry(slot);
  if (!entry) return;
  state.clipboard = { buttons: [entry] };
}

function copyButtons(slots) {
  var entries = [];
  slots.forEach(function (slot) {
    var entry = buildClipboardEntry(slot);
    if (entry) entries.push(entry);
  });
  if (!entries.length) return;
  state.clipboard = { buttons: entries };
}

function cutSlot(slot) {
  if (isConfigLocked()) return;
  if (slot < 1) return;
  copySlot(slot);
  deleteSlot(slot);
}

function cutButtons(slots) {
  if (isConfigLocked()) return;
  var cardSlots = slots.filter(function (slot) { return slot > 0; });
  if (!cardSlots.length) return;
  copyButtons(cardSlots);
  deleteButtons(cardSlots);
}

function pasteButton(pos) {
  if (isConfigLocked()) return;
  if (!state.clipboard) return;
  var entries = state.clipboard.buttons;
  if (!canAddImageCards(imageCardCountInClipboardEntries(entries))) {
    showImageCardLimitBanner();
    return;
  }
  var lastSlot = -1;
  for (var i = 0; i < entries.length; i++) {
    var newSlot = firstFreeSlot();
    if (newSlot < 0) break;
    var e = entries[i];
    var placement = findDuplicatePlacement(state.grid, pos, e.size || 1, NUM_SLOTS);
    if (placement.pos < 0) break;
    var cell = placement.pos;
    var placeSize = placement.size;
    state.buttons[newSlot - 1] = {
      entity: e.entity, label: e.label, icon: e.icon,
      icon_on: e.icon_on, sensor: e.sensor, unit: e.unit,
      type: e.type || "", precision: e.precision || "",
      options: e.options || "",
    };
    if (placeSize === 1) delete state.sizes[newSlot]; else state.sizes[newSlot] = placeSize;
    placeSlotAt(state.grid, newSlot, cell, placeSize);
    if (e.subpageConfig) {
      var spCopy = parseSubpageConfig(e.subpageConfig);
      spCopy.sizes = {};
      buildSubpageGrid(spCopy);
      state.subpages[newSlot] = spCopy;
    }
    saveButtonConfig(newSlot);
    saveSubpageEntity(newSlot);
    lastSlot = newSlot;
  }
  postText(entityName("button_order"), serializeGrid(state.grid));
  state.clipboard = null;
  state.selectedSlots = [];
  renderPreview();
}

function pasteSubpageButton(pos) {
  if (isConfigLocked()) return;
  if (!state.clipboard) return;
  var homeSlot = state.editingSubpage;
  var sp = getSubpage(homeSlot);
  var maxPos = NUM_SLOTS;
  var entries = state.clipboard.buttons;
  if (!canAddImageCards(imageCardCountInClipboardEntries(entries))) {
    showImageCardLimitBanner();
    return;
  }
  var lastSlot = -1;
  for (var i = 0; i < entries.length; i++) {
    var e = entries[i];
    var placement = findDuplicatePlacement(sp.grid, pos, e.size || 1, maxPos);
    if (placement.pos < 0) break;
    var cell = placement.pos;
    var placeSize = placement.size;
    var newSlot = subpageFirstFreeSlot(sp);
    while (sp.buttons.length < newSlot) {
      sp.buttons.push(emptyButtonConfig());
    }
    sp.buttons[newSlot - 1] = {
      entity: e.entity, label: e.label, icon: e.icon,
      icon_on: e.icon_on, sensor: e.sensor, unit: e.unit,
      type: e.type || "", precision: e.precision || "",
      options: e.options || "",
    };
    if (placeSize === 1) delete sp.sizes[newSlot]; else sp.sizes[newSlot] = placeSize;
    placeSlotAt(sp.grid, newSlot, cell, placeSize);
    lastSlot = newSlot;
  }
  sp.order = serializeSubpageGrid(sp);
  state.clipboard = null;
  saveSubpageConfig(homeSlot);
  state.subpageSelectedSlots = [];
  renderPreview();
}
