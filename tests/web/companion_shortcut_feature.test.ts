import {
  applyCompanionMediaPresentation,
  companionAppLabel,
  companionApplicationActions,
  companionApplicationActionIdCanSave,
  companionApplicationActionIdValid,
  companionCardMode,
  companionFolderActionIdCanSave,
  companionFolderActions,
  companionEntityForMode,
  companionCardIsMetric,
  COMPANION_STATS_OPTIONS,
  companionLabelPlaceholder,
  companionMetricDisplayMode,
  companionMetricPreviewValue,
  companionMediaIcon,
  COMPANION_MEDIA_PLAY_PAUSE_ACTION,
  COMPANION_MEDIA_ACTIONS,
  companionShortcutActionId,
  companionSubtypeDefaultIcon,
  companionSubtypeIcon,
  companionUrlConfig,
  companionUrlValue,
  COMPANION_WINDOW_ACTIONS,
  companionWindowActionLabel,
  formatCompanionShortcutActionId,
  normalizeCompanionCard,
  resetCompanionMediaPresentation,
  resetCompanionMetricPresentation,
} from "../../src/webserver/cards/companion";
import {
  normalizeSubpageKind,
  subpageKindOptions,
} from "../../src/webserver/application/config_subpage_options";
import {
  COMPANION_INPUT_VOLUME_ID,
  COMPANION_OUTPUT_VOLUME_ID,
  companionSliderIcon,
  companionSliderMode,
} from "../../src/webserver/cards/slider";
import {
  companionAppShortcutFolderEnabled,
  companionAppShortcutAutoSwitchEnabled,
  companionShortcutActionIdValid,
  companionShortcutFolderEditorAvailable,
  CODEX_BUNDLE_ID,
  SLACK_BUNDLE_ID,
  createSafariShortcutSubpage,
  createCodexShortcutSubpage,
  codexShortcutPresetCards,
  createSlackShortcutSubpage,
  slackShortcutPresetCards,
  normalizeCompanionAppShortcutOptions,
  safariShortcutPresetCards,
  setCompanionAppShortcutFolderEnabled,
  setCompanionAppShortcutAutoSwitchEnabled,
} from "../../src/webserver/application/companion_shortcut_folder";
import { cardTransferOwnsSubpage } from "../../src/webserver/model/card_transfer";

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
  if (normalizeSubpageKind("companion_stat") !== "companion_stat" ||
      !subpageKindOptions().some((option: any) => option[0] === "companion_stat" && option[1] === "Companion Stat")) {
    throw new Error("Subpage settings must expose the Companion Stat subtype");
  }
  if (companionCardMode({ entity: "stat.cpu", sensor: "" }) !== "stats") {
    throw new Error("Processor statistics must be grouped under the Stats Companion subtype");
  }
  const safariFolderCard = {
    type: "companion", entity: "com.apple.Safari", options: "", sensor: "", icon: "Monitor",
  };
  setCompanionAppShortcutFolderEnabled(safariFolderCard, true);
  if (!companionAppShortcutFolderEnabled(safariFolderCard) || safariFolderCard.options !== "app_shortcuts") {
    throw new Error("Safari launch cards must retain the shortcut-folder option");
  }
  if (companionAppShortcutAutoSwitchEnabled(safariFolderCard)) {
    throw new Error("App subpage auto-switch must be off by default");
  }
  setCompanionAppShortcutAutoSwitchEnabled(safariFolderCard, true);
  if (!companionAppShortcutAutoSwitchEnabled(safariFolderCard) ||
      String(safariFolderCard.options) !== "app_shortcuts,app_shortcuts_auto_switch" ||
      normalizeCompanionAppShortcutOptions(safariFolderCard) !== "app_shortcuts,app_shortcuts_auto_switch") {
    throw new Error("Safari app subpages must retain the auto-switch option");
  }
  setCompanionAppShortcutFolderEnabled(safariFolderCard, false);
  if (companionAppShortcutFolderEnabled(safariFolderCard) ||
      companionAppShortcutAutoSwitchEnabled(safariFolderCard) || String(safariFolderCard.options) !== "") {
    throw new Error("Disabling app subpages must also clear auto-switch");
  }
  setCompanionAppShortcutFolderEnabled(safariFolderCard, true);
  if (companionShortcutFolderEditorAvailable(safariFolderCard, { ...safariFolderCard, options: "" })) {
    throw new Error("The Safari shortcut editor must wait until the folder option is saved");
  }
  if (!companionShortcutFolderEditorAvailable(safariFolderCard, { ...safariFolderCard })) {
    throw new Error("Saved Safari app subpages must expose their editor");
  }
  const safariUrlFolderCard = {
    ...safariFolderCard,
    sensor: "url.https%3A%2F%2Fexample.com",
    options: "app_shortcuts",
  };
  if (normalizeCompanionAppShortcutOptions(safariUrlFolderCard) !== "" ||
      companionAppShortcutFolderEnabled(safariUrlFolderCard)) {
    throw new Error("Safari Open URL cards must not retain the shortcut-folder option");
  }
  const chromeFolderCard = {
    type: "companion", entity: "com.google.Chrome", options: "app_shortcuts",
  };
  if (normalizeCompanionAppShortcutOptions(chromeFolderCard) !== "" ||
      companionAppShortcutFolderEnabled(chromeFolderCard)) {
    throw new Error("Unsupported apps must not retain the shortcut-folder option");
  }
  const invalidAutoSwitchCard = {
    ...safariFolderCard,
    options: "app_shortcuts_auto_switch",
  };
  if (normalizeCompanionAppShortcutOptions(invalidAutoSwitchCard) !== "") {
    throw new Error("Auto-switch must require the app subpage option");
  }
  const codexFolderCard = {
    type: "companion", entity: CODEX_BUNDLE_ID, options: "",
  };
  setCompanionAppShortcutFolderEnabled(codexFolderCard, true);
  if (!companionAppShortcutFolderEnabled(codexFolderCard) ||
      normalizeCompanionAppShortcutOptions(codexFolderCard) !== "app_shortcuts") {
    throw new Error("Codex launch cards must support app subpages");
  }
  const codexUrlFolderCard = { ...codexFolderCard, sensor: "url.https%3A%2F%2Fexample.com" };
  if (normalizeCompanionAppShortcutOptions(codexUrlFolderCard) !== "" ||
      companionAppShortcutFolderEnabled(codexUrlFolderCard)) {
    throw new Error("Codex Open URL cards must not retain the shortcut-folder option");
  }
  const safariPreset = safariShortcutPresetCards();
  const expectedSafariShortcuts = [
    "shortcut.command+keybracketleft",
    "shortcut.command+keybracketright",
    "shortcut.command+r",
    "shortcut.command+t",
    "shortcut.command+w",
  ];
  if (safariPreset.map((card) => card.entity).join("|") !== expectedSafariShortcuts.join("|")) {
    throw new Error("Safari shortcut defaults changed");
  }
  if (!safariPreset.every((card) => card.type === "companion" && companionShortcutActionIdValid(card.entity))) {
    throw new Error("Safari presets must contain only Companion keyboard shortcuts");
  }
  const codexPreset = codexShortcutPresetCards();
  const expectedCodexShortcuts = [
    "shortcut.command+k",
    "shortcut.command+enter",
    "shortcut.command+t",
    "shortcut.command+b",
    "shortcut.option+command+b",
    "shortcut.command+j",
    "shortcut.control+keybackquote",
  ];
  if (codexPreset.map((card) => card.entity).join("|") !== expectedCodexShortcuts.join("|")) {
    throw new Error("Codex shortcut defaults changed");
  }
  if (!codexPreset.every((card) => card.type === "companion" && companionShortcutActionIdValid(card.entity))) {
    throw new Error("Codex presets must contain only Companion keyboard shortcuts");
  }
  const expectedCodexLabels = ["Command", "Approve", "Browser", "Sidebar", "Side panel", "Terminal", "Terminal"];
  if (codexPreset.map((card) => card.label).join("|") !== expectedCodexLabels.join("|")) {
    throw new Error("Codex shortcut labels should stay short");
  }
  const slackFolderCard = {
    type: "companion", entity: SLACK_BUNDLE_ID, options: "",
  };
  setCompanionAppShortcutFolderEnabled(slackFolderCard, true);
  if (!companionAppShortcutFolderEnabled(slackFolderCard) ||
      normalizeCompanionAppShortcutOptions(slackFolderCard) !== "app_shortcuts") {
    throw new Error("Slack launch cards must support app subpages");
  }
  const slackPreset = slackShortcutPresetCards();
  const expectedSlackShortcuts = [
    "shortcut.command+n",
    "shortcut.command+g",
    "shortcut.command+shift+k",
    "shortcut.command+j",
    "shortcut.command+shift+a",
  ];
  if (slackPreset.map((card) => card.entity).join("|") !== expectedSlackShortcuts.join("|")) {
    throw new Error("Slack shortcut defaults changed");
  }
  if (!slackPreset.every((card) => card.type === "companion" && companionShortcutActionIdValid(card.entity))) {
    throw new Error("Slack presets must contain only Companion keyboard shortcuts");
  }
  const expectedSlackLabels = ["Compose", "Search", "DMs", "Unread", "All Unread"];
  if (slackPreset.map((card) => card.label).join("|") !== expectedSlackLabels.join("|")) {
    throw new Error("Slack shortcut labels should stay short");
  }
  if (companionShortcutActionIdValid("shortcut.") ||
      companionShortcutActionIdValid("shortcut.shift+a") ||
      companionShortcutActionIdValid("shortcut.command+command+a")) {
    throw new Error("Incomplete or invalid shortcuts must not be accepted in Safari app subpages");
  }
  for (const app of ["com.apple.Safari", CODEX_BUNDLE_ID, SLACK_BUNDLE_ID]) {
    if (!cardTransferOwnsSubpage({
      type: "companion", entity: app, sensor: "", options: "app_shortcuts",
    })) {
      throw new Error("Companion app subpages must remain transferable for every supported app");
    }
  }
  const safariSubpage = createSafariShortcutSubpage();
  if (safariSubpage.backLabel !== "Back" || safariSubpage.order.join("|") !== "B|1|2|3|4|5") {
    throw new Error("Safari app subpage layout changed");
  }
  const codexSubpage = createCodexShortcutSubpage();
  if (codexSubpage.backLabel !== "Back" || codexSubpage.order.join("|") !== "B|1|2|3|4|5|6|7" ||
      codexSubpage.buttons.map((card: any) => card.entity).join("|") !== expectedCodexShortcuts.join("|")) {
    throw new Error("Codex app subpage layout changed");
  }
  const slackSubpage = createSlackShortcutSubpage();
  if (slackSubpage.backLabel !== "Back" || slackSubpage.order.join("|") !== "B|1|2|3|4|5" ||
      slackSubpage.buttons.map((card: any) => card.entity).join("|") !== expectedSlackShortcuts.join("|")) {
    throw new Error("Slack app subpage layout changed");
  }
  if (!companionCardIsMetric({ entity: "stat.memory" }) ||
      !companionCardIsMetric({ entity: "stat.memory_free" }) ||
      companionCardIsMetric({ entity: "sensor.memory_use" })) {
    throw new Error("Companion statistics must remain separate from Home Assistant sensors");
  }
  if (companionMetricDisplayMode({ entity: "stat.memory" }) !== "used" ||
      companionMetricDisplayMode({ entity: "stat.memory_free" }) !== "free" ||
      companionMetricDisplayMode({ entity: "stat.storage_free" }) !== "free") {
    throw new Error("Memory and storage statistics must retain their Used or Free display choice");
  }
  if (companionLabelPlaceholder({ entity: "stat.network_throughput" }) !== "e.g. Network") {
    throw new Error("Network throughput must use Network as its default label");
  }
  if (companionLabelPlaceholder({ entity: "folder." }) !== "e.g. Folder Name") {
    throw new Error("Open folder cards must use Folder Name as their empty label placeholder");
  }
  if (JSON.stringify(COMPANION_STATS_OPTIONS) !== JSON.stringify([
    ["battery", "Battery"],
    ["memory_usage", "Memory"],
    ["network_throughput", "Network"],
    ["processor", "Processor"],
    ["storage", "Storage"],
  ])) {
    throw new Error("Companion statistic choices must use concise alphabetical labels");
  }
  if (companionLabelPlaceholder({ entity: "stat.cpu" }) !== "e.g. Processor" ||
      companionLabelPlaceholder({ entity: "com.apple.Safari" }) !== "e.g. Safari or Select all") {
    throw new Error("Companion cards must use one mode-appropriate label field");
  }
  if (companionMetricPreviewValue("0", 0.4) !== "42" ||
      companionMetricPreviewValue("1", 0.4) !== "42.0" ||
      companionMetricPreviewValue("2", 0.4) !== "42.00" ||
      companionMetricPreviewValue("0", 0.1) === companionMetricPreviewValue("0", 0.9)) {
    throw new Error("Companion statistic previews must randomize while matching the selected precision");
  }
  const generatedMetricCard: any = { entity: "stat.cpu", label: "", icon: "Auto" };
  normalizeCompanionCard(generatedMetricCard);
  if (generatedMetricCard.label !== "" || generatedMetricCard.precision !== "0") {
    throw new Error("Companion statistics must leave generated labels empty and use whole-number precision");
  }
  const customMetricCard = {
    entity: "stat.memory", label: "Mac RAM", icon: "Auto", precision: "1",
  };
  normalizeCompanionCard(customMetricCard);
  if (customMetricCard.label !== "Mac RAM" || customMetricCard.precision !== "1") {
    throw new Error("Companion statistics must preserve custom labels and precision");
  }
  const networkCard: any = { entity: "stat.network_throughput", label: "", icon: "Auto" };
  normalizeCompanionCard(networkCard);
  if (networkCard.label !== "" || networkCard.unit !== "MB/s") {
    throw new Error("Network throughput must use its rate label and unit");
  }
  const legacyNetworkCard: any = { entity: "stat.network_throughput", unit: "KB/s" };
  normalizeCompanionCard(legacyNetworkCard);
  if (legacyNetworkCard.unit !== "MB/s") {
    throw new Error("Network throughput cards must migrate to MB/s");
  }
  if (companionCardMode({ entity: "media.play_pause", sensor: "" }) !== "media") {
    throw new Error("Companion media actions must retain their card subtype");
  }
  if (companionCardMode({ entity: "media.thirdparty.app", sensor: "" }) !== "app") {
    throw new Error("Installed apps beginning with media. must remain app actions");
  }
  const emptyFolderEntity = companionEntityForMode("folder");
  if (emptyFolderEntity !== "folder." ||
      companionCardMode({ entity: emptyFolderEntity, sensor: "" }) !== "folder") {
    throw new Error("Open folder must retain its subtype while waiting for a folder selection");
  }
  if (companionEntityForMode("stats") !== "stat.cpu" ||
      companionEntityForMode("processor") !== "stat.cpu" ||
      companionEntityForMode("memory_usage") !== "stat.memory" ||
      companionEntityForMode("network_throughput") !== "stat.network_throughput") {
    throw new Error("System statistic subtypes must select their Companion metric entities");
  }
  if (companionSubtypeDefaultIcon("url") !== "Web" ||
      companionSubtypeDefaultIcon("folder") !== "Folder Outline" ||
      companionSubtypeDefaultIcon("stats") !== "Gauge" ||
      companionSubtypeDefaultIcon("shortcut") !== "Shortcut Command") {
    throw new Error("Companion subtypes must use their requested default icons");
  }
  if (companionSubtypeIcon("Shortcut Command", "shortcut", "url") !== "Web") {
    throw new Error("Changing Companion subtypes must refresh a generated default icon");
  }
  if (companionSubtypeIcon("Star", "shortcut", "url") !== "Star") {
    throw new Error("Changing Companion subtypes must preserve a custom icon");
  }
  const folderAction = "folder.00000000-0000-0000-0000-000000000001";
  if (companionCardMode({ entity: folderAction, sensor: "" }) !== "folder" ||
      companionCardMode({ entity: "com.apple.finder", sensor: "" }) !== "folder") {
    throw new Error("Folder actions and legacy Finder cards must use the folder subtype");
  }
  const catalogue = [
    { id: "com.apple.Safari", label: "Safari" },
    { id: "com.google.Chrome", label: "Google Chrome" },
    { id: "com.apple.finder", label: "Finder" },
    { id: folderAction, label: "Projects" },
    { id: "folder.00000000-0000-0000-0000-000000000002", label: "Archive" },
    { id: COMPANION_MEDIA_PLAY_PAUSE_ACTION, label: "Media Play/Pause" },
  ];
  if (companionApplicationActions(catalogue).map((action) => action.id).join() !== "com.google.Chrome,com.apple.Safari") {
    throw new Error("Finder and approved folders must not appear in the alphabetized application list");
  }
  if (!companionApplicationActionIdValid(catalogue, "com.apple.Safari") ||
      companionApplicationActionIdValid(catalogue, "com.apple.finder") ||
      companionApplicationActionIdValid([], "com.apple.Safari")) {
    throw new Error("Companion app selections require an available application action");
  }
  if (companionFolderActions(catalogue).map((action) => action.label).join() !== "Archive,Projects") {
    throw new Error("Approved folders must appear alphabetically in the folder list");
  }
  if (companionSliderMode({ entity: COMPANION_OUTPUT_VOLUME_ID }) !== "mac_output") {
    throw new Error("Output volume must be available as a Slider control");
  }
  if (companionSliderMode({ entity: COMPANION_INPUT_VOLUME_ID }) !== "mac_input") {
    throw new Error("Input volume must be available as a Slider control");
  }
  if (companionSliderMode({ entity: "light.office" }) !== "home_assistant") {
    throw new Error("Existing Home Assistant sliders must remain unchanged");
  }
  if (companionSliderIcon("Volume High", "mac_output", "mac_input") !== "Microphone") {
    throw new Error("Changing volume controls must refresh generated slider icons");
  }
  if (companionSliderIcon("Palette", "mac_output", "mac_input") !== "Palette") {
    throw new Error("Changing volume controls must preserve custom slider icons");
  }
  if (companionSliderIcon("Microphone", "mac_input", "home_assistant") !== "Auto") {
    throw new Error("Leaving a volume control must clear its generated slider icon");
  }
  if (companionAppLabel("", "", "Safari") !== "Safari") {
    throw new Error("Selecting a Companion app must prefill an empty card label");
  }
  if (companionAppLabel("Safari", "Safari", "Google Chrome") !== "Google Chrome") {
    throw new Error("Changing a Companion app must refresh its generated label");
  }
  if (companionAppLabel("Work browser", "Safari", "Google Chrome") !== "Work browser") {
    throw new Error("Changing a Companion app must preserve a custom card label");
  }
  if (companionMediaIcon("Play Pause", "Play Pause", "Skip Next") !== "Skip Next") {
    throw new Error("Changing media actions must refresh a generated icon");
  }
  if (companionMediaIcon("Music", "Play Pause", "Skip Next") !== "Music") {
    throw new Error("Changing media actions must preserve a custom icon");
  }
  if (COMPANION_MEDIA_ACTIONS.find((action) => action.id === "media.previous")?.label !== "Previous" ||
      COMPANION_MEDIA_ACTIONS.find((action) => action.id === "media.next")?.label !== "Next") {
    throw new Error("Companion media actions must use the short Previous and Next labels");
  }
  const generatedAppCard = { entity: "com.apple.Safari", label: "Safari", icon: "Monitor" };
  applyCompanionMediaPresentation(generatedAppCard, "Safari");
  if (generatedAppCard.label !== "Play / Pause" || generatedAppCard.icon !== "Play Pause") {
    throw new Error("Entering Media Control must refresh generated app presentation fields");
  }
  const customAppCard = { entity: "com.apple.Safari", label: "Work", icon: "Briefcase" };
  applyCompanionMediaPresentation(customAppCard, "Safari");
  if (customAppCard.label !== "Work" || customAppCard.icon !== "Briefcase") {
    throw new Error("Entering Media Control must preserve custom presentation fields");
  }
  const generatedMediaCard = { entity: "media.play_pause", label: "Play / Pause", icon: "Play Pause" };
  resetCompanionMediaPresentation(generatedMediaCard, "app");
  if (generatedMediaCard.label !== "" || generatedMediaCard.icon !== "Monitor") {
    throw new Error("Leaving Media Control must clear generated media presentation fields");
  }
  const customMediaCard = { entity: "media.next", label: "Skip", icon: "Music" };
  resetCompanionMediaPresentation(customMediaCard, "shortcut");
  if (customMediaCard.label !== "Skip" || customMediaCard.icon !== "Music") {
    throw new Error("Leaving Media Control must preserve custom presentation fields");
  }
  const metricCard = {
    entity: "stat.cpu", label: "Processor", icon: "Monitor", sensor: "ignored",
    unit: "", precision: "", options: "large_numbers,active_color", icon_on: "Auto",
  };
  normalizeCompanionCard(metricCard);
  if (metricCard.sensor !== "" || metricCard.unit !== "%" || metricCard.precision !== "0" ||
      metricCard.options !== "large_numbers") {
    throw new Error("Companion statistics must normalize their own sensor-style fields");
  }
  resetCompanionMetricPresentation(metricCard, "app");
  if (metricCard.label !== "" || String(metricCard.unit) !== "" || String(metricCard.precision) !== "" ||
      String(metricCard.options) !== "") {
    throw new Error("Leaving a generated statistic card must clear its generated presentation");
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
  if (companionWindowActionLabel("window.center") !== "Centre"
      || companionWindowActionLabel("window.unknown") !== "") {
    throw new Error("Window actions must use allow-listed display labels");
  }
  const offlineSavedApp = "com.apple.Safari";
  if (!companionApplicationActionIdCanSave([], offlineSavedApp, offlineSavedApp)
      || companionApplicationActionIdCanSave([], "com.apple.TextEdit", offlineSavedApp)) {
    throw new Error("Offline app editing must preserve only the card's existing app identifier");
  }
  const offlineSavedFolder = "folder.approved-documents";
  if (!companionFolderActionIdCanSave([], offlineSavedFolder, offlineSavedFolder)
      || companionFolderActionIdCanSave([], "folder.unapproved", offlineSavedFolder)) {
    throw new Error("Offline folder editing must preserve only the card's existing folder identifier");
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
  if (urlCard.icon !== "Web") throw new Error("Existing URL cards must adopt the Web default icon");
  const shortcutCard = { entity: "shortcut.command+a", sensor: "", icon: "Monitor" };
  normalizeCompanionCard(shortcutCard);
  if (shortcutCard.icon !== "Shortcut Command") {
    throw new Error("Existing shortcut cards must adopt the command default icon");
  }
  const folderCard = { entity: folderAction, sensor: "", icon: "Folder" };
  normalizeCompanionCard(folderCard);
  if (folderCard.icon !== "Folder Outline") {
    throw new Error("Existing folder cards must adopt the Folder Outline default icon");
  }
  const statsCard = { entity: "stat.cpu", sensor: "", icon: "Monitor" };
  normalizeCompanionCard(statsCard);
  if (statsCard.icon !== "Gauge") throw new Error("Existing stats cards must adopt the Gauge default icon");
  const mediaCard = { entity: COMPANION_MEDIA_PLAY_PAUSE_ACTION, sensor: "", icon: "Auto" };
  normalizeCompanionCard(mediaCard);
  if (mediaCard.entity !== "media.play_pause" || mediaCard.icon !== "Play Pause") {
    throw new Error("Play / Pause cards must round-trip with their fixed default icon");
  }
}
