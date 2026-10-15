import type { CardConfig } from "../contracts/types";
import { configOptionValue } from "./config_primitives";
import { cardBadgeUsesSlider } from "./badge_card";

export type BadgeCardDisplay = "card" | "label" | "value";
export type BadgeCardContent = "auto" | "text" | "label" | "value" | "slider" | "track" | "date" | "datetime";

export function badgeText(title: string, reading: string, labelOnly: boolean): string {
  return (labelOnly ? title || reading : reading || title).replace(/[\r\n]/g, " ");
}
export function badgeCardDisplay(badge: Partial<CardConfig>): BadgeCardDisplay {
  const value = configOptionValue(badge.options || "", "badge_display");
  return value === "label" || value === "value" ? value : "card";
}

/** Select the card's intended compact content, rather than the first visible value. */
export function badgeCardContent(card: Pick<CardConfig, "type" | "sensor" | "precision">, display: string): BadgeCardContent {
  if (display === "label") return "label";
  if (card.type === "calendar") return card.precision === "datetime" ? "datetime" : "date";
  if (display === "value") {
    if (card.type === "media" && ["now_playing", "cover_art"].includes(card.sensor)) return "track";
    return cardBadgeUsesSlider(card) ? "slider" : "value";
  }
  if (card.type === "media") {
    if (["now_playing", "cover_art"].includes(card.sensor)) return "track";
    return card.sensor === "volume" ? "value" : "text";
  }
  if (cardBadgeUsesSlider(card)) return "slider";
  if (["sensor", "local_sensor", "timer", "calendar", "clock", "timezone", "weather_forecast"].includes(card.type)) return "value";
  if (card.type === "weather") return card.precision === "today" || card.precision === "tomorrow" ? "value" : "text";
  if (["climate", "climate_control", "light_control", "light_switch", "fan_control", "fan_switch", "fan_direction", "fan_oscillate", "fan_preset",
    "alarm", "alarm_action", "door_window", "presence", "garage", "gate", "lock", "vacuum", "lawn_mower", "image", "wifi_qr", "wifi_qr_card", "screen_lock", "push", "webhook"].includes(card.type)) return "text";
  return "auto";
}

export function badgeCardSupportsValue(card: Pick<CardConfig, "type" | "sensor" | "precision">): boolean {
  return ["value", "slider", "track", "date", "datetime"].includes(badgeCardContent(card, "card")) ||
    (card.type === "media" && card.sensor === "position");
}
