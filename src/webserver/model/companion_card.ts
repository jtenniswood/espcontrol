import {
  COMPANION_CARD_MODES,
  type CompanionCardMode,
} from "../generated/companion_capabilities";

export type CompanionCardModeId = typeof COMPANION_CARD_MODES[number]["id"];

// Drafts can contain empty identifiers. Availability is a separate catalogue
// concern, so losing a connection never changes the saved card identity.
export type CompanionCardModel =
  | { readonly mode: "app"; readonly applicationId: string }
  | { readonly mode: "shortcut"; readonly shortcutId: string }
  | { readonly mode: "url"; readonly applicationId: string; readonly encodedUrl: string }
  | { readonly mode: "folder"; readonly folderId: string }
  | { readonly mode: "media"; readonly actionId: string }
  | { readonly mode: "stats"; readonly metricId: string; readonly precision: string; readonly unit: string }
  | { readonly mode: "window"; readonly actionId: string };

export function companionCardModeContract(mode: unknown): CompanionCardMode | undefined {
  return COMPANION_CARD_MODES.find((candidate) => candidate.id === mode);
}

export function companionCardModeValid(mode: unknown): mode is CompanionCardModeId {
  return companionCardModeContract(mode) !== undefined;
}

export function companionCardModeOptions(): ReadonlyArray<readonly [CompanionCardModeId, string]> {
  return COMPANION_CARD_MODES.map((mode) => [mode.id, mode.label] as const);
}

export function companionCardDefaultIcon(mode: CompanionCardModeId): string {
  return companionCardModeContract(mode)?.defaultIcon || "Monitor";
}
