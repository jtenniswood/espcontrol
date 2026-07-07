// ── Preview Clipboard ─────────────────────────────────────────────
// @web-module-requires: state, grid, config_codec, config_post_api, api, preview_grid_placement, preview_render

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
