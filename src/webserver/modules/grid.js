// ── Context abstraction ────────────────────────────────────────────────
// @web-module-requires: state, model_generated

function ctx() {
  if (state.editingSubpage) {
    var sp = getSubpage(state.editingSubpage);
    return {
      grid: sp.grid, sizes: sp.sizes, buttons: sp.buttons,
      maxSlots: NUM_SLOTS, selected: state.subpageSelectedSlots,
      isSub: true,
      setSelected: function (s) { state.subpageSelectedSlots = s; },
      setLastClicked: function (s) { state.subpageLastClicked = s; },
      getLastClicked: function () { return state.subpageLastClicked; },
      save: function () { saveSubpageConfig(state.editingSubpage); },
    };
  }
  return {
    grid: state.grid, sizes: state.sizes, buttons: state.buttons,
    maxSlots: NUM_SLOTS, selected: state.selectedSlots,
    isSub: false,
    setSelected: function (s) { state.selectedSlots = s; },
    setLastClicked: function (s) { state.lastClickedSlot = s; },
    getLastClicked: function () { return state.lastClickedSlot; },
    save: function () { postText(entityName("button_order"), serializeGrid(state.grid)); },
  };
}

// ── Grid helpers ───────────────────────────────────────────────────────

var CARD_SIZE_SINGLE = EspControlModel.CARD_SIZE_SINGLE;
var CARD_SIZE_TALL = EspControlModel.CARD_SIZE_TALL;
var CARD_SIZE_WIDE = EspControlModel.CARD_SIZE_WIDE;
var CARD_SIZE_LARGE = EspControlModel.CARD_SIZE_LARGE;
var CARD_SIZE_EXTRA_TALL = EspControlModel.CARD_SIZE_EXTRA_TALL;
var CARD_SIZE_EXTRA_WIDE = EspControlModel.CARD_SIZE_EXTRA_WIDE;
var CARD_SIZE_EXTRA_LARGE = EspControlModel.CARD_SIZE_EXTRA_LARGE;

function sizeFromToken(token) {
  return EspControlModel.sizeFromToken(token);
}

function sizeToken(size) {
  return EspControlModel.sizeToken(size);
}

function sizeRowSpan(size) {
  return EspControlModel.sizeRowSpan(size);
}

function sizeColSpan(size) {
  return EspControlModel.sizeColSpan(size);
}

function cardSizeClass(size) {
  return EspControlModel.cardSizeClass(size);
}

function sizeClass(size) {
  var className = cardSizeClass(size);
  return className ? " " + className : "";
}

function coveredCells(pos, size, maxSlots, includeOrigin) {
  return EspControlModel.coveredCells(pos, size, maxSlots, GRID_COLS, includeOrigin);
}

function sizeFitsAt(pos, size, maxSlots) {
  return EspControlModel.sizeFitsAt(pos, size, maxSlots, GRID_COLS);
}

function markSpannedCells(grid, pos, size, maxSlots) {
  EspControlModel.markSpannedCells(grid, pos, size, maxSlots, GRID_COLS);
}

function parseOrder(str) {
  var parsed = EspControlModel.parseGridOrder(str, NUM_SLOTS, GRID_COLS, state.sizes);
  state.sizes = parsed.sizes;
  return parsed.grid;
}

function applyButtonOrderValue(val, skipRender) {
  orderReceived = !!(val && val.trim());
  state.sizes = {};
  state.grid = parseOrder(val);
  state.selectedSlots = state.selectedSlots.filter(function (s) {
    return state.grid.indexOf(s) !== -1;
  });
  if (!skipRender) scheduleRender();
}

function applySpans(grid, sizes, maxSlots) {
  EspControlModel.applySpans(grid, sizes, maxSlots, GRID_COLS);
}

function serializeGrid(grid) {
  return EspControlModel.serializeGridOrder(grid, state.sizes);
}

function applyImportedButtonOrder(orderStr, importedSizes) {
  state.sizes = importedSizes || {};
  state.grid = parseOrder(orderStr);
}

function clearSpans(grid, maxSlots) {
  EspControlModel.clearSpans(grid, maxSlots);
}


function resolveIcon(b) {
  var sel = b.icon || "Auto";
  if (sel === "Auto" && b.entity) {
    var domain = b.entity.split(".")[0];
    return DOMAIN_ICONS[domain] || "cog";
  }
  return iconSlug(sel);
}

function btnDisplayName(b) {
  return b.label || b.entity || "Configure";
}
