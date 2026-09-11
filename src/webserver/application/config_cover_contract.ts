import { cardContractOptionSpec } from "./config_option_core";

export function coverCommandMode(mode?: unknown): boolean {
    return mode === "open" || mode === "close" || mode === "stop" || mode === "set_position";
}

export function coverModeOptionValues(allowCommands?: unknown): string[] {
    const spec: any = cardContractOptionSpec("cover", "cover_mode");
    const values: string[] = spec && spec.values
        ? spec.values.slice()
        : ["modal", "", "tilt", "toggle", "open", "close", "stop", "set_position"];
    return values.filter((value) => !!allowCommands || !coverCommandMode(value));
}

export function normalizeCoverMode(mode?: unknown, allowCommands?: unknown): string {
    const value = String(mode || "");
    return coverModeOptionValues(allowCommands).indexOf(value) >= 0 ? value : "";
}

export function coverModeOptionsForSettings(_currentMode?: unknown): string[][] {
    return [
        ["modal", "All Controls"],
        ["", "Slider: Position"],
        ["tilt", "Slider: Tilt"],
        ["toggle", "Toggle"],
        ["open", "Open"],
        ["close", "Close"],
        ["stop", "Stop"],
        ["set_position", "Set Position"],
    ];
}

export function normalizeCoverPosition(value?: unknown): string {
    let parsed = parseInt(String(value), 10);
    const spec: any = cardContractOptionSpec("cover", "cover_position") || {};
    let fallback = parseInt(spec.defaultValue, 10);
    const min = typeof spec.min === "number" ? spec.min : 0;
    const max = typeof spec.max === "number" ? spec.max : 100;
    if (!isFinite(fallback)) fallback = 50;
    if (!isFinite(parsed)) parsed = fallback;
    if (parsed < min) parsed = min;
    if (parsed > max) parsed = max;
    return String(parsed);
}
