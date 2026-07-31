import {
  CARD_CONTRACT_MIGRATION_ALIASES,
  cardContractCard,
  cardContractCardKeys,
  cardContractDefaultConfig,
  cardContractMigrationAlias,
  cardContractSubpageTypeCode,
  cardContractSubpageTypeFromCode,
  cardRuntimeSpec,
  resolveCardRuntimeSpec,
} from "../../src/webserver/generated/card_contract";

function equal<T>(actual: T, expected: T, message: string): void {
  if (actual !== expected) throw new Error(`${message}: expected ${String(expected)}, received ${String(actual)}`);
}

export function runCardContractTests(): void {
  const keys = cardContractCardKeys();
  if (keys.length < 35) throw new Error(`expected the complete card contract, received ${keys.length} card types`);
  for (const type of keys) {
    const card = cardContractCard(type);
    const runtime = cardRuntimeSpec(type);
    if (!card) throw new Error(`${type || "switch"}: card contract is missing`);
    if (!runtime) throw new Error(`${type || "switch"}: runtime contract is missing`);
    const defaults = cardContractDefaultConfig(type);
    equal(defaults.type, card.default.type, `${type || "switch"} default type`);
    if (!runtime.driver) throw new Error(`${type || "switch"}: runtime driver is empty`);

    const code = cardContractSubpageTypeCode(type);
    equal(cardContractSubpageTypeFromCode(code), type, `${type || "switch"} compact type code round-trip`);
    const resolved = resolveCardRuntimeSpec(defaults);
    if (!resolved?.driver) throw new Error(`${type || "switch"}: default runtime driver does not resolve`);
  }

  for (const [legacyType, expected] of Object.entries(CARD_CONTRACT_MIGRATION_ALIASES)) {
    const alias = cardContractMigrationAlias(legacyType);
    if (!alias) throw new Error(`${legacyType}: migration alias is missing`);
    equal(JSON.stringify(alias), JSON.stringify(expected), `${legacyType} migration alias copy`);
  }

  const media = resolveCardRuntimeSpec({
    ...cardContractDefaultConfig("media"),
    type: "media",
    sensor: "cover_art",
  });
  equal(media?.driver, "media_cover_art", "mode-aware Media driver resolution");
  equal(cardContractCard("todo"), null, "retired Todo card stays outside the public contract");
}
