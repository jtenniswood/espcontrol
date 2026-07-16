import { staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installGridDimensionsStateModule(): GlobalDescriptors {
    // ── Main grid dimensions (user-configurable rows/columns) ──────────────
    function clampGridDimension(this: any, value?: any, fallback?: any, max?: any) {
        var n: any = parseInt(value, 10);
        if (!isFinite(n) || n < 1)
            n = fallback;
        if (isFinite(max) && max >= 1 && n > max)
            n = max;
        return n;
    }
    function clampGridCols(this: any, value?: any) {
        return clampGridDimension(value, CFG.cols, CFG.maxCols);
    }
    function clampGridRows(this: any, value?: any) {
        return clampGridDimension(value, CFG.rows, CFG.maxRows);
    }
    function gridColumnsMax(this: any) {
        return clampGridDimension(CFG.maxCols, CFG.cols, CFG.maxCols);
    }
    function gridRowsMax(this: any) {
        return clampGridDimension(CFG.maxRows, CFG.rows, CFG.maxRows);
    }
    // Re-flow the saved button order into the new grid size and refresh the
    // preview. Parsing from the untruncated raw order avoids losing any card
    // placement when the grid grows or shrinks.
    function applyGridDimensions(this: any, cols?: any, rows?: any) {
        state.gridCols = clampGridCols(cols);
        state.gridRows = clampGridRows(rows);
        syncPreviewOrientation();
        applyButtonOrderValue(state.buttonOrderRaw, true);
        syncGridDimensionInputs();
        if (els.previewMain)
            renderPreview();
    }
    function setGridDimensions(this: any, cols?: any, rows?: any) {
        var nextCols: any = clampGridCols(cols);
        var nextRows: any = clampGridRows(rows);
        applyGridDimensions(nextCols, nextRows);
        postNumber(entityName("grid_columns"), nextCols);
        postNumber(entityName("grid_rows"), nextRows);
    }
    function syncGridDimensionInputs(this: any) {
        if (els.setGridColumns) {
            els.setGridColumns.max = String(gridColumnsMax());
            els.setGridColumns.value = String(state.gridCols);
        }
        if (els.setGridRows) {
            els.setGridRows.max = String(gridRowsMax());
            els.setGridRows.value = String(state.gridRows);
        }
    }
    return {
        "clampGridCols": staticGlobal(clampGridCols),
        "clampGridRows": staticGlobal(clampGridRows),
        "gridColumnsMax": staticGlobal(gridColumnsMax),
        "gridRowsMax": staticGlobal(gridRowsMax),
        "applyGridDimensions": staticGlobal(applyGridDimensions),
        "setGridDimensions": staticGlobal(setGridDimensions),
        "syncGridDimensionInputs": staticGlobal(syncGridDimensionInputs),
    };
}
