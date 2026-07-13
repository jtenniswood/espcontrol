// ── Preview Grid Placement ────────────────────────────────────────
// @web-module-requires: state, grid, config_codec

function resolveSpanPos(pos) {
  var c = ctx();
  return PreviewGridFeature.resolveSpanPosition(c.grid, c.sizes, pos, c.maxSlots, GRID_COLS);
}

function getCellFromEvent(e, container) {
  if (CFG.dragMode === "swap") {
    var rect = container.getBoundingClientRect();
    return resolveSpanPos(PreviewFeature.swapGridCell(
      { x: e.clientX, y: e.clientY },
      { left: rect.left, top: rect.top, right: rect.right, bottom: rect.bottom },
      GRID_COLS,
      GRID_ROWS
    ));
  }
  var children = container.children;
  var cells = [];
  for (var i = 0; i < children.length; i++) {
    var r = children[i].getBoundingClientRect();
    var pos = parseInt(children[i].getAttribute("data-pos"), 10);
    if (isNaN(pos)) continue;
    cells.push({ pos: pos, left: r.left, top: r.top, right: r.right, bottom: r.bottom });
  }
  return PreviewFeature.closestGridCell({ x: e.clientX, y: e.clientY }, cells);
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
  return PreviewGridFeature.canPlaceSlotAt(grid, pos, size, maxSlots, GRID_COLS);
}

function findPlacementCell(grid, start, size, maxSlots) {
  return PreviewGridFeature.findPlacementCell(grid, start, size, maxSlots, GRID_COLS);
}

function findDuplicatePlacement(grid, start, size, maxSlots) {
  return PreviewGridFeature.findDuplicatePlacement(grid, start, size, maxSlots, GRID_COLS);
}

function placeSlotAt(grid, slot, pos, size) {
  PreviewGridFeature.placeSlotAt(grid, slot, pos, size, GRID_COLS);
}

function placeOrderedGridEntries(entries, sizes, maxSlots) {
  return PreviewGridFeature.placeOrderedGridEntries(entries, sizes, maxSlots, GRID_COLS);
}

function moveSelectedToCell(fromPos, toPos) {
  var c = ctx();
  var result = PreviewGridFeature.moveSelectedGridEntries(
    c.grid, c.sizes, c.selected, fromPos, toPos, c.maxSlots, GRID_COLS
  );
  if (!result.accepted) return false;

  if (c.isSub) {
    getSubpage(state.editingSubpage).grid = result.grid;
  } else {
    state.grid = result.grid;
  }
  return true;
}
