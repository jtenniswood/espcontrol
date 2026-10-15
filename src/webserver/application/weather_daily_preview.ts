import type { DeviceConfig } from "../state/types";
import { planWeatherCardLayout } from "./weather_daily_layout";

// Use the firmware's grid/list rules in display pixels. One observer serves
// all weather cards and is disconnected whenever the preview is rebuilt.
export function createWeatherDailyPreviewLayout(
    document: Document,
    config: Pick<DeviceConfig, "displayWidth" | "displayHeight">,
    isPortraitRotation: () => boolean,
) {
    const fontHeightFor = (size: number) => Math.round(size * 1.2);
    const view = document.defaultView;
    const cards = new Set<HTMLElement>();
    let context: CanvasRenderingContext2D | null = null;
    function layout(card: HTMLElement) {
        if (!view || !context || !card.isConnected) return;
        const fontContext = context;
        const screen = card.closest<HTMLElement>(".sp-screen");
        const pixels = isPortraitRotation() ? config.displayHeight : config.displayWidth;
        const scale = screen && pixels ? screen.clientWidth / pixels : 1;
        if (scale <= 0) return;
        const style = view.getComputedStyle(card);
        const days = Array.from(card.querySelectorAll<HTMLElement>(".sp-weather-day"));
        const row = card.querySelector<HTMLElement>(".sp-weather-days");
        const firstDay = days[0];
        if (!firstDay || !row) return;
        // Reset inherited sizes before measuring so repeated observer callbacks
        // cannot compound the enlargement or depend on browser zoom.
        row.style.removeProperty("--weather-label");
        row.style.removeProperty("--weather-icon");
        row.style.removeProperty("--weather-temperature");
        const width = Math.floor((card.clientWidth - parseFloat(style.paddingLeft) - parseFloat(style.paddingRight)) / scale);
        const height = Math.floor((card.clientHeight - parseFloat(style.paddingTop) - parseFloat(style.paddingBottom)) / scale);
        const measureFonts = () => {
            const textStyle = view.getComputedStyle(firstDay);
            const nativeFontSize = Number((parseFloat(textStyle.fontSize) / scale).toFixed(3));
            fontContext.font = textStyle.font.replace(textStyle.fontSize, nativeFontSize + "px");
            const measure = (text: string) => Array.from(text).reduce((width, glyph) =>
                width + Math.round(fontContext.measureText(glyph).width), 0);
            const icon = firstDay.querySelector<HTMLElement>(".mdi");
            const iconSize = icon ? Math.round(parseFloat(view.getComputedStyle(icon).fontSize) / scale) : 0;
            let measured = Math.max(iconSize, ...["-188°", "Today", "Sun", "Mon", "Tues", "Weds", "Thurs", "Fri", "Sat"].map(measure));
            const temperature = firstDay.querySelector<HTMLElement>(".sp-weather-temperature-high");
            let temperatureHeight = fontHeightFor(nativeFontSize), temperatureWidth = 0;
            if (temperature) {
                const ts = view.getComputedStyle(temperature);
                const size = Number((parseFloat(ts.fontSize) / scale).toFixed(3));
                temperatureHeight = fontHeightFor(size);
                fontContext.font = ts.font.replace(ts.fontSize, size + "px");
                temperatureWidth = Math.max(...days.map(day => measure(day.querySelector(".sp-weather-temperature-high")?.textContent || "")));
                measured = Math.max(measured, temperatureWidth);
            }
            const fontHeight = fontHeightFor(nativeFontSize), gap = Math.max(8, fontHeight);
            const rowGap = Math.max(8, Math.floor(fontHeight / 2)), listGap = Math.max(8, Math.floor(fontHeight / 3));
            const minimumWidth = measured + 24, minimumHeight = iconSize + fontHeight + temperatureHeight + 4;
            const plan = planWeatherCardLayout(width, height, minimumWidth, minimumHeight,
                iconSize + listGap + temperatureWidth, Math.max(iconSize, fontHeight + temperatureHeight + 2), gap, rowGap, days.length);
            return { textStyle, nativeFontSize, measure, iconSize, fontHeight, temperatureHeight,
                gap, rowGap, listGap, minimumWidth, minimumHeight, plan };
        };
        let metrics = measureFonts();
        let enlarged = false;
        if (width >= metrics.minimumWidth * 2 + metrics.gap && height >= metrics.minimumHeight * 2 + metrics.rowGap) {
            row.style.setProperty("--weather-label", "var(--weather-large-label,var(--btn-label))");
            row.style.setProperty("--weather-icon", "var(--weather-large-icon,var(--btn-icon))");
            row.style.setProperty("--weather-temperature", "var(--btn-label)");
            const larger = measureFonts();
            if (larger.plan.count >= metrics.plan.count && !larger.plan.list &&
                larger.nativeFontSize > metrics.nativeFontSize && larger.iconSize > metrics.iconSize) {
                metrics = larger; enlarged = true;
            } else {
                row.style.removeProperty("--weather-label");
                row.style.removeProperty("--weather-icon");
                row.style.removeProperty("--weather-temperature");
            }
        }
        card.dataset.weatherEnlarged = String(enlarged);
        const { textStyle, nativeFontSize, measure, iconSize, fontHeight, temperatureHeight, gap, rowGap, listGap, plan } = metrics;
        const grid = !plan.list && plan.rows > 1;
        const iconGap = grid ? Math.min(Math.max(6, Math.floor(fontHeight / 3)),
            Math.floor((height - rowGap * (plan.rows - 1)) / plan.rows)
                - iconSize - fontHeight - temperatureHeight - 2) : 2;
        const groupHeight = iconSize + iconGap + fontHeight + temperatureHeight + 2;
        const gridRowGap = grid ? Math.min(Math.max(rowGap, fontHeight),
            Math.floor((height - groupHeight * plan.rows) / (plan.rows - 1))) : rowGap;
        row.classList.toggle("sp-weather-list", plan.list);
        row.classList.toggle("sp-weather-grid", grid);
        row.style.gridTemplateColumns = `repeat(${plan.columns}, minmax(0, 1fr))`;
        row.style.gridTemplateRows = `repeat(${plan.rows}, minmax(0, 1fr))`;
        row.style.columnGap = (gap * scale) + "px";
        row.style.rowGap = (gridRowGap * scale) + "px";
        row.style.setProperty("--weather-list-gap", (listGap * scale) + "px");
        row.style.setProperty("--weather-icon-gap", ((iconGap - 2) * scale) + "px");
        card.dataset.weatherColumns = String(plan.columns);
        card.dataset.weatherRows = String(plan.rows);
        days.forEach((day, index) => {
            day.hidden = index >= plan.count;
            day.style.gap = (2 * scale) + "px";
            const label = day.children[1] as HTMLElement | undefined;
            if (!label) return;
            const fullLabel = label.dataset.weatherFullLabel || label.textContent || "";
            label.dataset.weatherFullLabel = fullLabel;
            // The browser preview uses English labels; restore them on wider cards.
            fontContext.font = textStyle.font.replace(textStyle.fontSize, nativeFontSize + "px");
            label.textContent = plan.list && measure(fullLabel) > width / plan.columns - iconSize - listGap
                ? day.dataset.weatherWeekday || fullLabel.slice(0, 3) : fullLabel;
        });
    }
    const Observer = view && (view as Window & typeof globalThis).ResizeObserver;
    let observer: ResizeObserver | null = null;
    if (document.fonts) document.fonts.ready.then(() => cards.forEach(layout));
    return {
        observe(card: HTMLElement) {
            if (!context) context = document.createElement("canvas").getContext("2d");
            if (!observer && Observer) observer = new Observer(entries => {
                entries.forEach(entry => layout(entry.target as HTMLElement));
            });
            cards.add(card);
            layout(card);
            if (observer) observer.observe(card);
        },
        disconnect() {
            if (observer) observer.disconnect();
            cards.clear();
        },
    };
}
