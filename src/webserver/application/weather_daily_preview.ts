import type { DeviceConfig } from "../state/types";

// Use the firmware's width + gap rule in display pixels. One observer serves
// all weather cards and is disconnected whenever the preview is rebuilt.
export function createWeatherDailyPreviewLayout(
    document: Document,
    config: Pick<DeviceConfig, "displayWidth" | "displayHeight">,
    isPortraitRotation: () => boolean,
) {
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
        const textStyle = view.getComputedStyle(firstDay);
        const nativeFontSize = Number((parseFloat(textStyle.fontSize) / scale).toFixed(3));
        // Measure at the panel's font size so browser font hinting at enlarged
        // preview sizes cannot change the number of days that fit.
        fontContext.font = textStyle.font.replace(textStyle.fontSize, nativeFontSize + "px");
        // ESPHome rounds each glyph advance to display pixels before measuring.
        const measured = Array.from("-18.8°").reduce((width, glyph) =>
            width + Math.round(fontContext.measureText(glyph).width), 0);
        const fontHeight = Math.round(nativeFontSize * 1.2);
        const gap = Math.max(8, fontHeight);
        const width = (card.clientWidth - parseFloat(style.paddingLeft) - parseFloat(style.paddingRight)) / scale;
        const minimumDayWidth = measured + 24;
        const count = Math.max(1, Math.min(days.length, Math.floor((width + gap) / (minimumDayWidth + gap))));
        const columnWidth = Math.max(1, Math.min(minimumDayWidth, Math.floor((width - gap * (count - 1)) / count)));
        row.style.gap = (gap * scale) + "px";
        row.style.setProperty("--weather-day-width", (columnWidth * scale) + "px");
        days.forEach((day, index) => { day.hidden = index >= count; });
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
