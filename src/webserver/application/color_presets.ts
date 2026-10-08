import { cardPreviewTextColor } from "../features/preview";
import { WEB_UI_COLORS } from "../state/ui_tokens";

const COLOR_PRESETS = [
    "FF0000", "EB1460", "9C1AB1", "6633B9", "3D4DB7",
    "46AF4A", "009687", "00BBD5", "00A6F6", "1093F5",
    "88C440", "CCDD1E", "FFEC16", "FFC100", WEB_UI_COLORS.primary,
    "000000", "5E7C8B", "9D9D9D", "7A5547", "FF5505",
];

export function createColorPresetGrid(value: string, onChange: (hex: string) => void, label: string) {
    const grid = document.createElement("div");
    grid.className = "sp-card-color-presets";
    grid.setAttribute("role", "group");
    grid.setAttribute("aria-label", label);
    function syncColor(hex: string) {
        const selected = hex.replace(/^#/, "").toUpperCase();
        for (const swatch of Array.from(grid.children)) {
            swatch.setAttribute("aria-pressed", String(swatch.getAttribute("data-color") === selected));
        }
    }
    for (const hex of COLOR_PRESETS) {
        const swatch = document.createElement("button");
        swatch.type = "button";
        swatch.className = "sp-card-color-preset";
        swatch.title = "#" + hex;
        swatch.setAttribute("data-color", hex);
        swatch.setAttribute("aria-label", "Set colour to #" + hex);
        swatch.style.backgroundColor = "#" + hex;
        swatch.style.color = cardPreviewTextColor(hex);
        const check = document.createElement("span");
        check.className = "sp-card-color-check";
        check.textContent = "✓";
        check.setAttribute("aria-hidden", "true");
        swatch.appendChild(check);
        swatch.addEventListener("click", () => {
            syncColor(hex);
            onChange(hex);
        });
        grid.appendChild(swatch);
    }
    syncColor(value);
    return Object.assign(grid, { _syncColor: syncColor });
}
