import type { CardConfig } from "../contracts/types";
import { cardContractCard } from "../generated/card_contract";
import { configOptionValue, setConfigOptionValue } from "./config_primitives";

/** A card-backed badge keeps the card's own fields and options inside the row. */
export function badgeCardType(button: Partial<CardConfig>): string | null {
  const key = configOptionValue(button.options || "", "badge_card_type");
  if (!key || key === "badge") return null;
  const type = key === "switch" ? "" : key;
  return cardContractCard(type) ? type : null;
}

export function badgeToCard(button: CardConfig): CardConfig {
  const options = ["badge_card_type", "badge_action", "badge_page", "badge_display"].reduce(
    (options, key) => setConfigOptionValue(options, key, ""), button.options);
  return { ...button, type: badgeCardType(button) ?? "", options };
}

export function updateBadgeFromCard(badge: CardConfig, card: CardConfig): void {
  const page = configOptionValue(badge.options, "badge_page");
  const display = configOptionValue(badge.options, "badge_display");
  Object.assign(badge, card, { type: "badge" });
  badge.options = setConfigOptionValue(card.options, "badge_card_type", card.type || "switch");
  badge.options = setConfigOptionValue(badge.options, "badge_action", card.type === "subpage" ? "subpage" : "card");
  if (card.type === "subpage" && page) badge.options = setConfigOptionValue(badge.options, "badge_page", page);
  if (display === "label" || display === "value") badge.options = setConfigOptionValue(badge.options, "badge_display", display);
}

export function cardBadgeUsesSlider(card: Pick<CardConfig, "type" | "sensor" | "precision">): boolean {
  return ["slider", "light_brightness", "light_temperature", "fan_speed"].includes(card.type) ||
    (card.type === "cover" && ["", "tilt"].includes(card.sensor)) ||
    (card.type === "media" && (card.sensor === "position" || (card.sensor === "now_playing" && card.precision === "progress")));
}
