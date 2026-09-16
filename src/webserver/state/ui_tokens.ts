export const WEB_UI_COLORS = {
  primary: "FF8C00",
  secondary: "313131",
  tertiary: "212121",
} as const;

// Notification card severity palette. Backgrounds are paired with a fixed
// foreground so the contrast stays readable on a screen: dark text on the
// muted yellow (about 8.4:1) and white text on the red (about 6.5:1).
// Firmware repeats these values in button_grid_style.h; keep both in step.
export const NOTIFICATION_LEVEL_COLORS = {
  information: { background: "212121", text: "FFFFFF" },
  warning: { background: "E3B341", text: "212121" },
  alert: { background: "B3261E", text: "FFFFFF" },
} as const;
