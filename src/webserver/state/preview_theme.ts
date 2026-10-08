import type { AppState } from "./types";

// check_firmware_display_tokens.py guards parity with ThemePalette. The browser shell has
// its own CSS theme; only the simulated device screen consumes these values.
export const PREVIEW_THEME_COLORS = {
  Dark: {
    background: "000000", surfacePrimary: "313131", surfaceSecondary: "212121", surfaceSensor: "212121", surfaceCard: "313131",
    textPrimary: "FFFFFF", clockbarText: "FFFFFF", textMuted: "B0B0B0", textDisabled: "707070",
    border: "313131", trackBackground: "313131", controlNeutral: "313131",
  },
  Light: {
    background: "E3E3E3", surfacePrimary: "B0B0B0", surfaceSecondary: "FFFFFF", surfaceSensor: "F5F5F5", surfaceCard: "FFFFFF",
    textPrimary: "333333", clockbarText: "333333", textMuted: "606060", textDisabled: "9A9A9A",
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
    `--preview-surface-secondary:#${theme.surfaceSecondary};--preview-surface-sensor:#${theme.surfaceSensor};` +
    `--preview-surface-card:#${theme.surfaceCard};` +
    `--preview-text-primary:#${theme.textPrimary};` +
    `--preview-clockbar-text:#${theme.clockbarText};` +
    `--preview-text-muted:#${theme.textMuted};--preview-text-disabled:#${theme.textDisabled};` +
    `--preview-border:#${theme.border};--preview-track-background:#${theme.trackBackground};` +
    `--preview-control-neutral:#${theme.controlNeutral};--screen-secondary:#${theme.surfacePrimary};` +
    `--screen-tertiary:#${theme.surfaceSecondary};`;
}
