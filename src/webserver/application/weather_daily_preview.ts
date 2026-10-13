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
        const measure = (text: string) => Array.from(text).reduce((width, glyph) =>
            width + Math.round(fontContext.measureText(glyph).width), 0);
        const icon = firstDay.querySelector<HTMLElement>(".mdi");
        const iconSize = icon ? Math.round(parseFloat(view.getComputedStyle(icon).fontSize) / scale) : 0;
        let measured = Math.max(iconSize, ...["-188°", "Today", "Sun", "Mon", "Tues", "Weds", "Thurs", "Fri", "Sat"].map(measure));
        const temperature = firstDay.querySelector<HTMLElement>(".sp-weather-temperature-high");
        if (temperature) {
            const temperatureStyle = view.getComputedStyle(temperature);
            const temperatureSize = Number((parseFloat(temperatureStyle.fontSize) / scale).toFixed(3));
            fontContext.font = temperatureStyle.font.replace(temperatureStyle.fontSize, temperatureSize + "px");
            measured = Math.max(measured, ...days.map(day => measure(day.querySelector(".sp-weather-temperature-high")?.textContent || "")));
        }
        const fontHeight = Math.round(nativeFontSize * 1.2);
        const gap = Math.max(8, fontHeight);
        const width = (card.clientWidth - parseFloat(style.paddingLeft) - parseFloat(style.paddingRight)) / scale;
        const minimumDayWidth = measured + 24;
        const count = Math.max(1, Math.min(days.length, Math.floor((width + gap) / (minimumDayWidth + gap))));
        row.style.gap = (gap * scale) + "px";
        // Flex shares the remaining width equally between the visible days.
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
