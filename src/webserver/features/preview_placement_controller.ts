import { applySpans, clearSpans, sizeFitsAt, type SlotSizeMap } from "../model/grid";
import { moveSelectedGridEntries, resolveSpanPosition } from "./preview_grid";

export interface PreviewPlacementContext {
  readonly grid: readonly number[];
  readonly sizes: SlotSizeMap;
  readonly maxSlots: number;
  readonly selected: readonly number[];
}

export interface PreviewPlacementResult {
  readonly accepted: boolean;
  readonly grid: number[];
  readonly sizes: SlotSizeMap;
}

/**
 * Owns the grid transformations used by preview drag-and-drop.  The browser
 * compatibility layer remains responsible for choosing the active main or
 * subpage grid and rendering the result.
 */
export class PreviewPlacementController {
  moveSingle(
    context: PreviewPlacementContext,
    fromPos: number,
    toPos: number,
    gridCols: number,
  ): PreviewPlacementResult {
    const resolvedTarget = resolveSpanPosition(
      context.grid,
      context.sizes,
      toPos,
      context.maxSlots,
      gridCols,
    );
    const unchanged = (): PreviewPlacementResult => ({
      accepted: false,
      grid: context.grid.slice(),
      sizes: { ...context.sizes },
    });
    if (resolvedTarget < 0 || resolvedTarget >= context.maxSlots || context.grid[resolvedTarget] === -1) {
      return unchanged();
    }

    const grid = context.grid.slice(0, context.maxSlots);
    while (grid.length < context.maxSlots) grid.push(0);
    const sizes = { ...context.sizes };
    const movingSlot = grid[fromPos] ?? 0;
    clearSpans(grid, context.maxSlots);
    const targetSlot = grid[resolvedTarget] ?? 0;
    grid[resolvedTarget] = movingSlot;
    grid[fromPos] = targetSlot;
    applySpans(grid, sizes, context.maxSlots, gridCols);
    if ((sizes[String(movingSlot)] || 1) > 1 && !sizeFitsAt(resolvedTarget, sizes[String(movingSlot)], context.maxSlots, gridCols)) {
      delete sizes[String(movingSlot)];
    }
    return { accepted: true, grid, sizes };
  }

  moveSelected(
    context: PreviewPlacementContext,
    fromPos: number,
    toPos: number,
    gridCols: number,
  ): PreviewPlacementResult {
    const sizes = { ...context.sizes };
    const result = moveSelectedGridEntries(
      context.grid,
      sizes,
      context.selected,
      fromPos,
      toPos,
      context.maxSlots,
      gridCols,
    );
    return {
      accepted: result.accepted,
      grid: result.grid,
      sizes,
    };
  }
}

export function createPreviewPlacementController(): PreviewPlacementController {
  return new PreviewPlacementController();
}
