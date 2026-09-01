import {
  COMPANION_WINDOW_ACTIONS,
  companionAppLabel,
  companionCardMode,
  companionShortcutActionId,
  companionUrlConfig,
  companionUrlValue,
  companionWindowActionLabel,
  formatCompanionShortcutActionId,
  normalizeCompanionCard,
} from "../../src/webserver/cards/companion";

function shortcutEvent(overrides: Partial<KeyboardEvent>): Pick<KeyboardEvent,
  "code" | "metaKey" | "ctrlKey" | "altKey" | "shiftKey"> {
  return {
    code: "KeyA",
    metaKey: false,
    ctrlKey: false,
    altKey: false,
    shiftKey: false,
    ...overrides,
  };
}

export function runCompanionShortcutFeatureTests(): void {
  if (companionAppLabel("", "", "Safari") !== "Safari") {
    throw new Error("Selecting a Companion app must prefill an empty card label");
  }
  if (companionAppLabel("Safari", "Safari", "Google Chrome") !== "Google Chrome") {
    throw new Error("Changing a Companion app must refresh its generated label");
  }
  if (companionAppLabel("Work browser", "Safari", "Google Chrome") !== "Work browser") {
    throw new Error("Changing a Companion app must preserve a custom card label");
  }

  if (COMPANION_WINDOW_ACTIONS.length !== 19) {
    throw new Error("Companion window controls must expose the complete approved preset list");
  }
  const windowIds = new Set(COMPANION_WINDOW_ACTIONS.map((action) => action.id));
  if (windowIds.size !== COMPANION_WINDOW_ACTIONS.length || !windowIds.has("window.close")
      || !windowIds.has("window.arrange.bottom-quarters")) {
    throw new Error("Companion window action identifiers must be unique and complete");
  }
  if (companionCardMode({ entity: "window.left", sensor: "" }) !== "window") {
    throw new Error("Window actions must restore as the Window controls subtype");
  }
  if (companionCardMode({ entity: "window.unknown", sensor: "" }) !== "window") {
    throw new Error("Unknown restored window actions must remain visible for correction in Window controls");
  }
  if (companionWindowActionLabel("window.center") !== "Centre"
      || companionWindowActionLabel("window.unknown") !== "") {
    throw new Error("Window actions must use allow-listed display labels");
  }
  if (companionAppLabel("Centre", "Centre", "Left") !== "Left"
      || companionAppLabel("Work layout", "Centre", "Left") !== "Work layout") {
    throw new Error("Window preset changes must update generated labels and preserve custom labels");
  }

  const selectAll = companionShortcutActionId(shortcutEvent({ metaKey: true }));
  if (selectAll !== "shortcut.command+a") throw new Error("Command-A shortcut encoding changed");
  if (formatCompanionShortcutActionId(selectAll) !== "⌘A") throw new Error("Command-A shortcut label changed");

  const previousTab = companionShortcutActionId(shortcutEvent({
    code: "Tab",
    ctrlKey: true,
    shiftKey: true,
  }));
  if (previousTab !== "shortcut.control+shift+tab") throw new Error("Control-Shift-Tab shortcut encoding changed");
  if (formatCompanionShortcutActionId(previousTab) !== "⌃⇧Tab") throw new Error("Control-Shift-Tab shortcut label changed");

  if (companionShortcutActionId(shortcutEvent({ shiftKey: true })) !== "") {
    throw new Error("Shift-only key presses must not become remote shortcuts");
  }
  if (companionShortcutActionId(shortcutEvent({ code: "AudioVolumeUp", metaKey: true })) !== "") {
    throw new Error("Unsupported keys must not become remote shortcuts");
  }

  const urlConfig = companionUrlConfig("https://example.com/dashboard?room=office");
  if (!urlConfig.startsWith("url.https%3A%2F%2Fexample.com%2Fdashboard")) {
    throw new Error("HTTPS Companion URLs must use the encoded URL card format");
  }
  if (companionUrlValue(urlConfig) !== "https://example.com/dashboard?room=office") {
    throw new Error("Companion URL card values must round-trip");
  }
  if (companionUrlConfig("file:///Applications/Calculator.app") !== "") {
    throw new Error("Companion URL cards must reject non-web schemes");
  }
  if (companionUrlConfig("https://user:password@example.com") !== "") {
    throw new Error("Companion URL cards must reject embedded credentials");
  }
  if (companionUrlConfig("https://example.com/" + "a".repeat(200)) !== "") {
    throw new Error("Companion URL cards must stay within the main-grid storage limit");
  }
  const urlCard = { entity: "com.apple.Safari", sensor: urlConfig, icon: "Monitor" };
  normalizeCompanionCard(urlCard);
  if (urlCard.sensor !== urlConfig) throw new Error("Companion URL configuration must survive card normalization");
  const windowCard = { entity: "window.arrange.left-right", sensor: "url.stale", icon: "Monitor" };
  normalizeCompanionCard(windowCard);
  if (windowCard.entity !== "window.arrange.left-right" || windowCard.sensor !== "") {
    throw new Error("Companion window actions must survive normalization without URL state");
  }
}
