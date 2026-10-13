import { cardPreviewTextColor } from "../features/preview";
import { activeCardColor } from "../model/config_primitives";

const COLOR_PRESETS: ReadonlyArray<readonly [string, string]> = [
    ["EA3323", "Red"], ["D83462", "Pink"], ["8F28AB", "Purple"], ["6036B2", "Deep purple"], ["3C49A5", "Indigo"],
    ["64AD54", "Green"], ["429471", "Teal"], ["54B7C8", "Cyan"], ["3E8AC7", "Sky blue"], ["3C64DE", "Blue"],
    ["AAC93F", "Lime"], ["D8D245", "Yellow green"], ["EED146", "Yellow"], ["FF8C00", "Orange"], ["DA5125", "Deep orange"],
    ["212121", "Dark grey"], ["313131", "Medium dark grey"], ["7B797F", "Grey"], ["F5F5F5", "Light grey"],
];

export const COLOR_SWATCH_STYLES =
    ".sp-card-color-preset:focus-visible{outline:2px solid var(--accent);outline-offset:3px}" +
    ".sp-card-color-presets{display:grid;grid-template-columns:repeat(5,minmax(0,1fr));width:224px;max-width:100%;gap:6px}" +
    ".sp-card-color-preset{width:100%;max-width:40px;aspect-ratio:1;padding:0;justify-self:center;box-sizing:border-box;" +
    "display:flex;align-items:center;justify-content:center;border:1px solid rgba(255,255,255,.35);border-radius:50%;cursor:pointer}" +
    ".sp-card-color-check{display:none;font-size:20px;line-height:1;font-weight:700}" +
    ".sp-card-color-preset[aria-pressed=true] .sp-card-color-check{display:block}" +
    ".sp-card-color-custom{position:relative;background:transparent;border:2px dashed var(--accent);color:var(--text);font-size:20px}" +
    ".sp-card-color-custom[data-selected=true]{outline:2px solid var(--accent);outline-offset:3px}" +
    ".sp-card-color-custom:focus-within{outline:2px solid var(--accent);outline-offset:3px}" +
    ".sp-card-color-custom input{position:absolute;inset:0;width:100%;height:100%;padding:0;border:0;opacity:0;cursor:pointer}";

export function createColorSwatches(value: string | (() => string), onChange: (hex: string) => void, label: string, tone: "standard" | "active" = "standard") {
    const presets = COLOR_PRESETS.map(([hex, name]) => [tone === "active" ? activeCardColor(hex) : hex, name] as const);
    const grid = document.createElement("div");
    grid.className = "sp-card-color-presets";
    grid.setAttribute("role", "group");
    grid.setAttribute("aria-label", label);
    let selectedColor = "";
    const customSwatch = document.createElement("label");
    customSwatch.className = "sp-card-color-preset sp-card-color-custom";
    const eyedropper = document.createElement("span");
    eyedropper.className = "mdi mdi-eyedropper";
    eyedropper.setAttribute("aria-hidden", "true");
    customSwatch.appendChild(eyedropper);
    const picker = document.createElement("input");
    picker.type = "color";
    picker.setAttribute("aria-label", "Choose custom colour");
    customSwatch.appendChild(picker);
    function pickCustomColor() {
        const hex = picker.value.slice(1).toUpperCase();
        if (hex === selectedColor) return;
        syncColor(hex);
        onChange(hex);
    }
    // Native dialogs can emit input events for every drag movement. Save once
    // when the choice is committed so Appearance does not queue device writes.
    picker.addEventListener("change", pickCustomColor);

    function createSwatch(hex: string, name: string) {
        const swatch = document.createElement("button");
        swatch.type = "button";
        swatch.className = "sp-card-color-preset";
        swatch.title = name + " (#" + hex + ")";
        swatch.setAttribute("data-color", hex);
        swatch.setAttribute("aria-label", "Set colour to " + name.toLowerCase());
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

    function syncColor(nextValue?: string) {
        const currentValue = nextValue ?? (typeof value === "function" ? value() : value);
        const selected = currentValue.trim().replace(/^#/, "").toUpperCase();
        selectedColor = selected;
        const valid = /^[0-9A-F]{6}$/.test(selected);
        picker.value = "#" + (valid ? selected : "000000");
        customSwatch.title = "Choose custom colour" + (valid ? " (#" + selected + ")" : "");
        const customSelected = valid && !presets.some(([hex]) => hex === selected);
        customSwatch.setAttribute("data-selected", String(customSelected));
        customSwatch.style.backgroundColor = customSelected ? "#" + selected : "transparent";
        customSwatch.style.color = customSelected ? cardPreviewTextColor(selected) : "";
        for (const swatch of Array.from(grid.querySelectorAll("button"))) {
            swatch.setAttribute("aria-pressed", String(swatch.getAttribute("data-color") === selected));
        }
    }

    for (const [hex, name] of presets) grid.appendChild(createSwatch(hex, name));
    grid.appendChild(customSwatch);
    syncColor();
    return Object.assign(grid, { _syncColor: syncColor });
}
