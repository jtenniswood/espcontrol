import type { CardConfig } from "../contracts/types";
import {
  COMPANION_CARD_MODES,
  type CompanionCardMode,
} from "../generated/companion_capabilities";

export type CompanionCardModeId = typeof COMPANION_CARD_MODES[number]["id"];

export type CompanionCardModel = {
  [Mode in CompanionCardModeId]: {
    readonly mode: Mode;
    readonly capability: string;
    readonly config: CardConfig;
  }
}[CompanionCardModeId];

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

export function companionCardModel(config: CardConfig, mode: CompanionCardModeId): CompanionCardModel {
  const contract = companionCardModeContract(mode);
  if (!contract) throw new Error(`Unknown Companion card mode: ${mode}`);
  return { mode, capability: contract.capability, config } as CompanionCardModel;
}
