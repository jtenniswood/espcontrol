import type { AppState } from "./types";

// check_firmware_display_tokens.py guards parity with ThemePalette. The browser shell has
// its own CSS theme; only the simulated device screen consumes these values.
export const PREVIEW_THEME_COLORS = {
  Dark: {
    background: "303030", surfacePrimary: "303030", surfaceSecondary: "404040",
    textPrimary: "FFFFFF", textMuted: "B0B0B0", textDisabled: "707070",
    border: "303030", trackBackground: "313131", controlNeutral: "303030",
  },
  Light: {
    background: "F4F4F4", surfacePrimary: "FFFFFF", surfaceSecondary: "E8E8E8",
    textPrimary: "333333", textMuted: "606060", textDisabled: "9A9A9A",
    border: "D0D0D0", trackBackground: "D0D0D0", controlNeutral: "E0E0E0",
  },
} as const;

export function previewEffectiveTheme(state: Pick<AppState, "themeMode" | "themeActive"> | undefined): "Dark" | "Light" {
  if (!state) return "Dark";
  if (state.themeMode === "Dark" || state.themeMode === "Light") return state.themeMode;
  return state.themeActive === "Light" ? "Light" : "Dark";
}

export function previewThemeCss(mode: "Dark" | "Light"): string {
  const theme = PREVIEW_THEME_COLORS[mode];
  return `--preview-background:#${theme.background};--preview-surface-primary:#${theme.surfacePrimary};` +
    `--preview-surface-secondary:#${theme.surfaceSecondary};--preview-text-primary:#${theme.textPrimary};` +
    `--preview-text-muted:#${theme.textMuted};--preview-text-disabled:#${theme.textDisabled};` +
    `--preview-border:#${theme.border};--preview-track-background:#${theme.trackBackground};` +
    `--preview-control-neutral:#${theme.controlNeutral};--screen-secondary:#${theme.surfacePrimary};` +
    `--screen-tertiary:#${theme.surfaceSecondary};`;
}
