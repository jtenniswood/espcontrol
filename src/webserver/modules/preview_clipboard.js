// ── Preview Clipboard ─────────────────────────────────────────────
// @web-module-requires: state, grid, config_codec, config_post_api, api, preview_grid_placement, preview_render

// ── Cut / Paste ────────────────────────────────────────────────────────

function buildClipboardEntry(slot) {
  if (slot < 1) return null;
  var c = ctx();
  var src = c.buttons[slot - 1];
  var subpageConfig = null;
  if (!c.isSub && src.type === "subpage" && state.subpages[slot]) {
    subpageConfig = serializeSubpageConfig(state.subpages[slot]);
  }
  return ClipboardFeature.createClipboardEntry(src, c.sizes[slot] || 1, subpageConfig);
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
  var used = {};
  state.grid.forEach(function (slot) { if (slot > 0) used[slot] = true; });
  var availableSlots = [];
  for (var slot = 1; slot <= NUM_SLOTS; slot++) { if (!used[slot]) availableSlots.push(slot); }
  var plan = ClipboardFeature.planClipboardPaste(
    entries, state.grid, state.sizes, pos, availableSlots, NUM_SLOTS, GRID_COLS
  );
  state.grid = plan.grid;
  state.sizes = plan.sizes;
  plan.placements.forEach(function (placement) {
    var newSlot = placement.slot;
    state.buttons[newSlot - 1] = placement.button;
    if (placement.subpageConfig) {
      var spCopy = parseSubpageConfig(placement.subpageConfig);
      spCopy.sizes = {};
      buildSubpageGrid(spCopy);
      state.subpages[newSlot] = spCopy;
    }
    saveButtonConfig(newSlot);
    saveSubpageEntity(newSlot);
  });
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
  var used = {};
  sp.grid.forEach(function (slot) { if (slot > 0) used[slot] = true; });
  var availableSlots = [];
  for (var slot = 1; slot <= maxPos; slot++) { if (!used[slot]) availableSlots.push(slot); }
  var plan = ClipboardFeature.planClipboardPaste(
    entries, sp.grid, sp.sizes, pos, availableSlots, maxPos, GRID_COLS
  );
  sp.grid = plan.grid;
  sp.sizes = plan.sizes;
  plan.placements.forEach(function (placement) {
    var newSlot = placement.slot;
    while (sp.buttons.length < newSlot) {
      sp.buttons.push(emptyButtonConfig());
    }
    sp.buttons[newSlot - 1] = placement.button;
  });
  sp.order = serializeSubpageGrid(sp);
  state.clipboard = null;
  saveSubpageConfig(homeSlot);
  state.subpageSelectedSlots = [];
  renderPreview();
}
