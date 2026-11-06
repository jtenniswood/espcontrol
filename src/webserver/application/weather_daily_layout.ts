export interface WeatherCardLayout {
    count: number;
    columns: number;
    rows: number;
}

/** Kept in agreement with weather::card_layout by the cross-language tests. */
export function planWeatherCardLayout(
    width: number, height: number, minimumWidth: number, minimumHeight: number,
    columnGap: number, rowGap: number, available: number,
): WeatherCardLayout {
    available = Math.min(available, 8);
    if (!available || width <= 0 || height <= 0) return { count: 0, columns: 0, rows: 0 };
    const fit = (space: number, minimum: number, gap: number) =>
        Math.max(1, Math.floor((space + gap) / (Math.max(1, minimum) + gap)));
    const maxColumns = Math.min(available, fit(width, minimumWidth, columnGap));
    const maxRows = Math.min(available, fit(height, minimumHeight, rowGap));
    const count = Math.min(available, maxColumns * maxRows);
    let best: WeatherCardLayout = { count, columns: count, rows: 1 };
    let bestScore = Infinity;
    for (let rows = 1; rows <= Math.min(maxRows, count); rows++) {
        const columns = Math.ceil(count / rows);
        if (columns > maxColumns) continue;
        const cellWidth = Math.floor((width - (columns - 1) * columnGap) / columns);
        const cellHeight = Math.floor((height - (rows - 1) * rowGap) / rows);
        const score = Math.abs(cellWidth - cellHeight) / Math.max(1, cellWidth, cellHeight)
            + (columns * rows - count) * 0.15;
        if (score < bestScore) {
            bestScore = score;
            best = { count, columns, rows };
        }
    }
    return best;
}
