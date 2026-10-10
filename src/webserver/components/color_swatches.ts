import { cardPreviewTextColor } from "../features/preview";

const COLOR_PRESETS: ReadonlyArray<readonly [string, string]> = [
    ["EA3323", "Red"], ["D83462", "Pink"], ["8F28AB", "Purple"], ["6036B2", "Deep purple"], ["3C49A5", "Indigo"],
    ["64AD54", "Green"], ["429471", "Teal"], ["54B7C8", "Cyan"], ["3E8AC7", "Sky blue"], ["3C64DE", "Blue"],
    ["AAC93F", "Lime"], ["D8D245", "Yellow green"], ["EED146", "Yellow"], ["DE8831", "Amber"], ["DA5125", "Orange"],
    ["2B2A2F", "Dark grey"], ["53515A", "Medium dark grey"], ["898890", "Grey"], ["BEBEC2", "Light grey"], ["FFFFFF", "White"],
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
        swatch.title = name;
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
