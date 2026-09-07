import type { CardConfig } from "../contracts/types";
import { COMPANION_MEDIA_ACTIONS, COMPANION_SYSTEM_METRICS } from "../generated/companion_capabilities";
import type { CompanionCardModel, CompanionCardModeId } from "./companion_card";

export function companionMetricForEntity(entity: unknown) {
  return COMPANION_SYSTEM_METRICS.find((metric) => metric.id === entity || metric.freeId === entity);
}

export function companionSavedCardMode(config: Partial<CardConfig>): CompanionCardModeId {
  const entity = config.entity || "";
  if (entity.startsWith("shortcut.")) return "shortcut";
  if (entity.startsWith("window.")) return "window";
  if (entity === "com.apple.finder" || entity.startsWith("folder.")) return "folder";
  if (COMPANION_MEDIA_ACTIONS.some((action) => action.id === entity)) return "media";
  if (entity === "stats" || companionMetricForEntity(entity)) return "stats";
  if (config.sensor?.startsWith("url.")) return "url";
  return "app";
}

export function decodeCompanionCard(config: Partial<CardConfig>, mode = companionSavedCardMode(config)): CompanionCardModel {
  const entity = config.entity || "";
  switch (mode) {
    case "app": return { mode, applicationId: entity };
    case "shortcut": return { mode, shortcutId: entity };
    case "folder": return { mode, folderId: entity };
    case "url": return { mode, applicationId: entity, encodedUrl: config.sensor || "" };
    case "media": case "window": return { mode, actionId: entity };
    case "stats": return { mode, metricId: entity, precision: config.precision || "", unit: config.unit || "" };
  }
}

// Preserve presentation, shortcut drafts, and unknown options verbatim. Only
// fields owned by the selected variant are written through this adapter.
export function encodeCompanionCard(model: CompanionCardModel, original: CardConfig): CardConfig {
  const config = { ...original };
  switch (model.mode) {
    case "app": config.entity = model.applicationId; break;
    case "shortcut": config.entity = model.shortcutId; break;
    case "folder": config.entity = model.folderId; break;
    case "url": config.entity = model.applicationId; config.sensor = model.encodedUrl; break;
    case "media": case "window": config.entity = model.actionId; break;
    case "stats":
      config.entity = model.metricId; config.precision = model.precision; config.unit = model.unit;
      break;
  }
  return config;
}
