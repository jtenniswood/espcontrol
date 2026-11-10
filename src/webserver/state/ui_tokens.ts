export const WEB_UI_COLORS = {
  primary: "FF8C00",
  secondary: "313131",
  tertiary: "212121",
} as const;

// Notification card severity palette. Backgrounds are paired with a fixed
// foreground so the contrast stays readable on a screen: dark text on the
// muted yellow (about 8.4:1) and white text on the red (about 6.5:1).
// Warning and alert stay the same in both themes; firmware repeats them in
// button_grid_style.h, so keep both in step. Information is the normal
// information-card colours, shown here in their Dark theme values; on the
// device that level follows the active theme.
export const NOTIFICATION_LEVEL_COLORS = {
  information: { background: "212121", text: "FFFFFF" },
  warning: { background: "E3B341", text: "212121" },
  alert: { background: "B3261E", text: "FFFFFF" },
} as const;
