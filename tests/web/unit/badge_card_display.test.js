"use strict";
const { test } = require("node:test");
const assert = require("node:assert/strict");
const cases = require("../../fixtures/badge_card_display.json");
const { loadTypescriptTest: load } = require("./helpers/load_typescript_test");
const { badgeCardContent, badgeCardDisplay, badgeText } = load("src/webserver/model/badge_card_display.ts");
const { cardContractCardKeys } = load("src/webserver/generated/card_contract.ts");
const { badgeToCard, updateBadgeFromCard } = load("src/webserver/model/badge_card.ts");
const { serializeBadgeRow, parseBadgeRow } = load("src/webserver/model/badge_row.ts");
test("compact display acceptance cases cover every card type and resolve its intended content", () => {
  const covered = new Set(cases.map(item => item.type));
  for (const type of cardContractCardKeys().filter(type => type !== "badge")) assert(covered.has(type), "Missing badge display acceptance case: " + type);
  for (const item of cases) assert.equal(badgeCardContent({ type: item.type, sensor: item.mode, precision: item.precision }, item.display), item.content, item.name);
});
test("badge display choices survive save and mode changes independently of native settings", () => {
  const card = { entity: "media_player.office", sensor: "position", label: "Sonos", icon: "Volume High", icon_on: "Auto", type: "media", precision: "state", unit: "", options: "media_tabs=controls|volume,label_display=status" };
  const badge = { ...card, type: "badge", options: "badge_card_type=media,badge_display=label" };
  updateBadgeFromCard(badge, card);
  const restored = parseBadgeRow(serializeBadgeRow([badge]))[0];
  assert.equal(badgeCardDisplay(restored), "label");
  assert.deepEqual(badgeToCard(restored), card);
  const changed = { ...card, sensor: "volume", precision: "" };
  updateBadgeFromCard(restored, changed);
  assert.equal(badgeCardDisplay(restored), "label");
  assert.deepEqual(badgeToCard(restored), changed);
  assert.equal(badgeCardDisplay({ options: "badge_display=invalid" }), "card");
});

test("removed Subpage badges remain readable without losing dormant configuration", () => {
  const { BADGE_SUBTYPES, isSubpageBadge, normalizeBadgeConfig } = load("src/webserver/model/badge.ts");
  assert(!BADGE_SUBTYPES.some(type => type.key === "subpage"));
  for (const options of ["badge_action=subpage,badge_page=12", "badge_action=card,badge_card_type=subpage,badge_page=12,badge_display=label"]) {
    const original = { entity: "", label: "Stored room", icon: "Home", icon_on: "Auto", type: "badge", sensor: "", precision: "label", unit: "", options };
    const badge = { ...original };
    assert(isSubpageBadge(badge));
    normalizeBadgeConfig(badge);
    assert.deepEqual(badge, original);
    assert.deepEqual(parseBadgeRow(serializeBadgeRow([badge]))[0], original);
  }
  assert(!isSubpageBadge({ options: "badge_action=card,badge_card_type=media" }));
});

test("single-line badges honor Label and reading choices with a fallback", () => {
  assert.equal(badgeText("Energy", "3.98 kW solar", false), "3.98 kW solar");
  assert.equal(badgeText("Sonos", "35%", true), "Sonos");
  assert.equal(badgeText("Sonos", "Sonos", false), "Sonos");
  assert.equal(badgeText("", "Unavailable", false), "Unavailable");
  assert.equal(badgeText("Energy", "", false), "Energy");
  assert.equal(badgeText("Sonos", "Track\nArtist", false), "Track Artist");
});
