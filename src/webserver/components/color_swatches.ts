import { cardPreviewTextColor } from "../features/preview";
import { WEB_UI_COLORS } from "../state/ui_tokens";

const COLOR_PRESETS: ReadonlyArray<readonly [string, string]> = [
    ["FF0000", "Red"], ["EB1460", "Pink"], ["9C1AB1", "Purple"], ["6633B9", "Deep purple"], ["3D4DB7", "Indigo"],
    ["46AF4A", "Green"], ["009687", "Teal"], ["00BBD5", "Cyan"], ["00A6F6", "Light blue"], ["1093F5", "Blue"],
    ["88C440", "Lime"], ["CCDD1E", "Yellow green"], ["FFEC16", "Yellow"], ["FFC100", "Amber"], [WEB_UI_COLORS.primary, "Orange"],
    ["000000", "Black"], ["5E7C8B", "Blue grey"], ["9D9D9D", "Grey"], ["7A5547", "Brown"], ["FFFFFF", "White"],
];

export const COLOR_SWATCH_STYLES =
    ".sp-card-color-preset:focus-visible{outline:2px solid var(--accent);outline-offset:3px}" +
    ".sp-card-color-presets{display:grid;grid-template-columns:repeat(5,minmax(0,1fr));width:224px;max-width:100%;gap:6px}" +
    ".sp-card-color-preset{width:100%;max-width:40px;aspect-ratio:1;padding:0;justify-self:center;box-sizing:border-box;" +
    "display:flex;align-items:center;justify-content:center;border:1px solid rgba(255,255,255,.35);border-radius:50%;cursor:pointer}" +
    ".sp-card-color-check{display:none;font-size:20px;line-height:1;font-weight:700}" +
    ".sp-card-color-preset[aria-pressed=true] .sp-card-color-check{display:block}";

export function createColorSwatches(value: string, onChange: (hex: string) => void, label: string) {
    const grid = document.createElement("div");
    grid.className = "sp-card-color-presets";
    grid.setAttribute("role", "group");
    grid.setAttribute("aria-label", label);
    let customSwatch: HTMLButtonElement | undefined;

    function createSwatch(hex: string, name: string) {
        const swatch = document.createElement("button");
        swatch.type = "button";
        swatch.className = "sp-card-color-preset";
        swatch.title = name + " (#" + hex + ")";
        swatch.setAttribute("data-color", hex);
        swatch.setAttribute("aria-label", "Set colour to " + name.toLowerCase() +
            (name === "Custom colour" ? " (#" + hex + ")" : ""));
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
        return swatch;
    }

    function syncColor(value: string) {
        const selected = value.trim().replace(/^#/, "").toUpperCase();
        // Keep a saved custom colour available, including values received after
        // construction. Replace that extra swatch instead of growing the grid.
        if (/^[0-9A-F]{6}$/.test(selected) && !COLOR_PRESETS.some(([hex]) => hex === selected) &&
            customSwatch?.getAttribute("data-color") !== selected) {
            customSwatch?.remove();
            customSwatch = createSwatch(selected, "Custom colour");
            grid.appendChild(customSwatch);
        }
        for (const swatch of Array.from(grid.children)) {
            swatch.setAttribute("aria-pressed", String(swatch.getAttribute("data-color") === selected));
        }
    }

    for (const [hex, name] of COLOR_PRESETS) grid.appendChild(createSwatch(hex, name));
    syncColor(value);
    return Object.assign(grid, { _syncColor: syncColor });
}
