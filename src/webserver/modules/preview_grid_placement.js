// ── Preview Grid Placement ────────────────────────────────────────
// @web-module-requires: state, grid, config_codec

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
