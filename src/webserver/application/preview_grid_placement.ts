import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installPreviewGridPlacementModule(): GlobalDescriptors {
    // ── Preview Grid Placement ────────────────────────────────────────
    function resolveSpanPos(this: any, pos?: any) {
        var c: any = ctx();
        return PreviewGridFeature.resolveSpanPosition(c.grid, c.sizes, pos, c.maxSlots, GRID_COLS);
    }
    function getCellFromEvent(this: any, e?: any, container?: any) {
        if (CFG.dragMode === "swap") {
            var rect: any = container.getBoundingClientRect();
            return resolveSpanPos(PreviewFeature.swapGridCell({ x: e.clientX, y: e.clientY }, { left: rect.left, top: rect.top, right: rect.right, bottom: rect.bottom }, GRID_COLS, GRID_ROWS));
        }
        var children: any = container.children;
        var cells: any = [];
        for (var i: any = 0; i < children.length; i++) {
            var r: any = children[i].getBoundingClientRect();
            var pos: any = parseInt(children[i].getAttribute("data-pos"), 10);
            if (isNaN(pos))
                continue;
            cells.push({ pos: pos, left: r.left, top: r.top, right: r.right, bottom: r.bottom });
        }
        return PreviewFeature.closestGridCell({ x: e.clientX, y: e.clientY }, cells);
    }
    function moveToCell(this: any, fromPos?: any, toPos?: any) {
        var c: any = ctx();
        toPos = resolveSpanPos(toPos);
        if (toPos >= c.maxSlots || c.grid[toPos] === -1)
            return;
        var grid: any = c.grid.slice();
        var movingSlot: any = grid[fromPos];
        clearSpans(grid, c.maxSlots);
        var targetSlot: any = grid[toPos];
        grid[toPos] = movingSlot;
        grid[fromPos] = targetSlot;
        applySpans(grid, c.sizes, c.maxSlots);
        if ((c.sizes[movingSlot] || 1) > 1 && !sizeFitsAt(toPos, c.sizes[movingSlot], c.maxSlots)) {
            delete c.sizes[movingSlot];
        }
        if (c.isSub) {
            getSubpage(state.editingSubpage).grid = grid;
        }
        else {
            state.grid = grid;
        }
    }
    function canPlaceSlotAt(this: any, grid?: any, pos?: any, size?: any, maxSlots?: any) {
        return PreviewGridFeature.canPlaceSlotAt(grid, pos, size, maxSlots, GRID_COLS);
    }
    function findPlacementCell(this: any, grid?: any, start?: any, size?: any, maxSlots?: any) {
        return PreviewGridFeature.findPlacementCell(grid, start, size, maxSlots, GRID_COLS);
    }
    function findDuplicatePlacement(this: any, grid?: any, start?: any, size?: any, maxSlots?: any) {
        return PreviewGridFeature.findDuplicatePlacement(grid, start, size, maxSlots, GRID_COLS);
    }
    function placeSlotAt(this: any, grid?: any, slot?: any, pos?: any, size?: any) {
        PreviewGridFeature.placeSlotAt(grid, slot, pos, size, GRID_COLS);
    }
    function placeOrderedGridEntries(this: any, entries?: any, sizes?: any, maxSlots?: any) {
        return PreviewGridFeature.placeOrderedGridEntries(entries, sizes, maxSlots, GRID_COLS);
    }
    function moveSelectedToCell(this: any, fromPos?: any, toPos?: any) {
        var c: any = ctx();
        var result: any = PreviewGridFeature.moveSelectedGridEntries(c.grid, c.sizes, c.selected, fromPos, toPos, c.maxSlots, GRID_COLS);
        if (!result.accepted)
            return false;
        if (c.isSub) {
            getSubpage(state.editingSubpage).grid = result.grid;
        }
        else {
            state.grid = result.grid;
        }
        return true;
    }
    return {
        "resolveSpanPos": staticGlobal(resolveSpanPos),
        "getCellFromEvent": staticGlobal(getCellFromEvent),
        "moveToCell": staticGlobal(moveToCell),
        "canPlaceSlotAt": staticGlobal(canPlaceSlotAt),
        "findPlacementCell": staticGlobal(findPlacementCell),
        "findDuplicatePlacement": staticGlobal(findDuplicatePlacement),
        "placeSlotAt": staticGlobal(placeSlotAt),
        "placeOrderedGridEntries": staticGlobal(placeOrderedGridEntries),
        "moveSelectedToCell": staticGlobal(moveSelectedToCell),
    };
}
