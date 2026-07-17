#!/usr/bin/env node
"use strict";

const crypto = require("crypto");
const fs = require("fs");
const path = require("path");
const vm = require("vm");
const { loadBuiltWebSource } = require("./web_source");

const ROOT = path.resolve(__dirname, "..");
const INVENTORY_PATH = path.join(ROOT, "common", "config", "card_runtime_inventory.json");
const CONTRACT_PATH = path.join(ROOT, "common", "config", "card_contract.json");
const NORMALIZATION_PATH = path.join(ROOT, "common", "config", "card_runtime_baseline_card_normalization_fixtures.json");
const SURFACE_PATH = path.join(ROOT, "compatibility", "fixtures", "card_runtime_surface_baseline.json");
const REPORT_PATH = path.join(ROOT, "docs", "generated", "cards", "runtime-coverage.md");
const RUNTIME_PATH = path.join(ROOT, "components", "espcontrol", "button_grid_card_runtime.h");
const GENERATED_RUNTIME_PATH = path.join(ROOT, "components", "espcontrol", "button_grid_contract_generated.h");
const CONFIG_FIELDS = ["entity", "label", "icon", "icon_on", "sensor", "unit", "type", "precision", "options"];
const CLASSIFICATIONS = new Set(["canonical", "accepted_legacy_input", "obsolete_implementation_residue"]);

function readJson(filePath) {
  return JSON.parse(fs.readFileSync(filePath, "utf8"));
}

function plain(value) {
  return JSON.parse(JSON.stringify(value));
}

function createWebSandbox() {
  const sandbox = {
    __ESPCONTROL_TEST_HOOKS__: {},
    console: { log() {}, warn() {}, error() {} },
    setTimeout,
    clearTimeout,
    requestAnimationFrame(fn) { return setTimeout(fn, 0); },
    URL,
    location: { href: "http://espcontrol.test/" },
    document: {
      readyState: "loading",
      activeElement: null,
      addEventListener() {},
    },
  };
  sandbox.window = sandbox;
  sandbox.globalThis = sandbox;
  return sandbox;
}

function loadHooks() {
  const sandbox = createWebSandbox();
  vm.createContext(sandbox);
  vm.runInContext(loadBuiltWebSource(), sandbox, { filename: "src/webserver/entry.ts" });
  const hooks = sandbox.__ESPCONTROL_TEST_HOOKS__.config;
  if (!hooks) throw new Error("web test hooks were not exported");
  return hooks;
}

function webRegistrationTypes() {
  const cardsDir = path.join(ROOT, "src", "webserver", "cards");
  const types = new Set();
  for (const name of fs.readdirSync(cardsDir).filter((item) => item.endsWith(".ts"))) {
    const source = fs.readFileSync(path.join(cardsDir, name), "utf8");
    for (const match of source.matchAll(/registerButtonType\(\s*(["'])(.*?)\1\s*,/g)) {
      types.add(match[2]);
    }
    if (source.includes("registerCoverLikeCardType(")) {
      for (const match of source.matchAll(/registerCoverLikeCardType\(\s*\{[\s\S]*?\btype:\s*(["'])(.*?)\1/g)) {
        types.add(match[2]);
      }
    }
  }
  return types;
}

function webParserTypes() {
  const source = fs.readFileSync(path.join(ROOT, "src", "webserver", "application", "config_codec.ts"), "utf8");
  return new Set([...source.matchAll(/["']([a-z][a-z0-9_]*)["']/g)].map((match) => match[1]));
}

function firmwareRegistrations() {
  const source = fs.readFileSync(RUNTIME_PATH, "utf8");
  const generated = fs.readFileSync(GENERATED_RUNTIME_PATH, "utf8");
  const typeNames = new Map();
  typeNames.set("SWITCH", "");
  for (const match of generated.matchAll(/if \(type == "([^"]+)"\) return CardTypeId::([A-Z_]+);/g)) {
    typeNames.set(match[2], match[1]);
  }

  const start = source.indexOf("inline Family family_for_runtime_type");
  const end = source.indexOf("inline Context context_for", start);
  if (start < 0 || end < 0) throw new Error("could not locate firmware runtime family mapping");
  const body = source.slice(start, end);
  const registrations = new Map();
  for (const match of body.matchAll(/((?:\s*case Type::[A-Z_]+:)+)\s*return Family::([A-Z_]+);/g)) {
    const family = match[2];
    for (const typeMatch of match[1].matchAll(/case Type::([A-Z_]+):/g)) {
      const type = typeNames.get(typeMatch[1]);
      if (type === undefined) throw new Error(`missing generated card type name for ${typeMatch[1]}`);
      registrations.set(type, family);
    }
  }
  const legacyBody = source.slice(end, source.indexOf("return context;", end));
  for (const match of legacyBody.matchAll(/type == "([^"]+)"\)[\s\S]*?context\.family = Family::([A-Z_]+);/g)) {
    registrations.set(match[1], match[2]);
  }
  return registrations;
}

function compactInput(config) {
  const fields = CONFIG_FIELDS.map((field) => String(config[field] || ""));
  return `~${fields.map((value) => encodeURIComponent(value)).join(",")}`;
}

function previewFingerprint(preview) {
  if (!preview) return "";
  return crypto.createHash("sha256").update(JSON.stringify(plain(preview))).digest("hex").slice(0, 16);
}

function mergedLifecycle(parent, mode) {
  return mode.lifecycle || parent.lifecycle;
}

function validateLifecycle(lifecycle, vocabulary, label) {
  if (!lifecycle || !Array.isArray(lifecycle.subscriptions) || !Array.isArray(lifecycle.actions)) {
    throw new Error(`${label}: lifecycle must declare subscriptions and actions`);
  }
  for (const value of lifecycle.subscriptions) {
    if (!vocabulary.subscriptions.includes(value)) throw new Error(`${label}: unknown subscription ${value}`);
  }
  for (const value of lifecycle.actions) {
    if (!vocabulary.actions.includes(value)) throw new Error(`${label}: unknown action ${value}`);
  }
  if (lifecycle.subscriptions.includes("none") && lifecycle.subscriptions.length !== 1) {
    throw new Error(`${label}: none cannot be combined with another subscription`);
  }
  if (lifecycle.actions.includes("none") && lifecycle.actions.length !== 1) {
    throw new Error(`${label}: none cannot be combined with another action`);
  }
  if (!vocabulary.modalOwners.includes(lifecycle.modalOwner)) {
    throw new Error(`${label}: unknown modal owner ${lifecycle.modalOwner}`);
  }
  if (lifecycle.modalOwner === "none" && lifecycle.actions.includes("modal")) {
    throw new Error(`${label}: modal actions must name their owner`);
  }
  if (lifecycle.modalOwner !== "none" && !lifecycle.actions.includes("modal")) {
    throw new Error(`${label}: modal owner requires a modal action`);
  }
}

function validateInventory(inventory, contract, webTypes, parserTypes, firmwareTypes) {
  if (inventory.inventoryVersion !== 1) throw new Error("inventoryVersion must be 1");
  const contractTypes = new Set(Object.keys(contract.cards));
  const inventoryTypes = new Set(Object.keys(inventory.types || {}));
  const missing = [...contractTypes].filter((type) => !inventoryTypes.has(type));
  const extra = [...inventoryTypes].filter((type) => !contractTypes.has(type));
  if (missing.length || extra.length) {
    throw new Error(`contract/inventory mismatch (missing: ${missing.join(", ") || "none"}; extra: ${extra.join(", ") || "none"})`);
  }

  const aliases = new Set(Object.keys(contract.migrationAliases || {}));
  const savedConfigAliases = new Set(aliases);
  for (const action of Object.values(contract.migrationActions || {})) {
    for (const condition of action.when || []) {
      if (condition.source === "field" && condition.name === "type" &&
          condition.operator === "equals") {
        savedConfigAliases.add(condition.value);
      }
    }
  }
  for (const [type, spec] of Object.entries(inventory.types)) {
    if (!CLASSIFICATIONS.has(spec.classification)) throw new Error(`${type || "<switch>"}: invalid classification`);
    if (!Array.isArray(spec.modes) || !spec.modes.length) throw new Error(`${type || "<switch>"}: at least one mode is required`);
    validateLifecycle(spec.lifecycle, inventory.lifecycleVocabulary, type || "<switch>");
    if (spec.classification === "canonical" && spec.webRegistration !== type) {
      throw new Error(`${type || "<switch>"}: canonical types must keep their own web registration`);
    }
    if (!webTypes.has(spec.webRegistration)) throw new Error(`${type || "<switch>"}: missing web registration ${spec.webRegistration}`);
    if (!firmwareTypes.has(type)) throw new Error(`${type || "<switch>"}: missing firmware registration`);
    if (firmwareTypes.get(type) !== spec.firmwareFamily) {
      throw new Error(`${type || "<switch>"}: firmware family is ${firmwareTypes.get(type)}, expected ${spec.firmwareFamily}`);
    }
    const modeNames = new Set();
    for (const mode of spec.modes) {
      if (!mode.name || modeNames.has(mode.name)) throw new Error(`${type || "<switch>"}: mode names must be unique and non-empty`);
      modeNames.add(mode.name);
      if (!mode.config || Object.keys(mode.config).some((field) => !CONFIG_FIELDS.includes(field))) {
        throw new Error(`${type || "<switch>"}/${mode.name}: config may only use saved card fields`);
      }
      if (Object.values(mode.config).some((value) => typeof value !== "string")) {
        throw new Error(`${type || "<switch>"}/${mode.name}: config values must be strings`);
      }
      validateLifecycle(mergedLifecycle(spec, mode), inventory.lifecycleVocabulary, `${type || "<switch>"}/${mode.name}`);
    }
  }

  const runtimeOnly = inventory.runtimeOnlyTypes || {};
  for (const alias of aliases) {
    const candidate = inventory.types[alias] || runtimeOnly[alias];
    if (!candidate || candidate.classification !== "accepted_legacy_input") {
      throw new Error(`${alias}: migration alias must be classified as accepted_legacy_input`);
    }
  }

  for (const [type, spec] of Object.entries(runtimeOnly)) {
    if (!CLASSIFICATIONS.has(spec.classification) || spec.classification === "canonical") {
      throw new Error(`${type}: runtime-only type must be legacy input or obsolete residue`);
    }
    validateLifecycle(spec.lifecycle, inventory.lifecycleVocabulary, `runtime-only ${type}`);
    if (!Array.isArray(spec.surfaces) || !spec.surfaces.length) throw new Error(`${type}: runtime-only surfaces are required`);
    if (spec.surfaces.some((surface) => !["web_parser", "web_registry", "saved_config_parser", "firmware"].includes(surface))) {
      throw new Error(`${type}: unknown runtime surface`);
    }
    if (spec.surfaces.includes("web_registry") && !webTypes.has(type)) throw new Error(`${type}: declared web residue is not registered`);
    if (spec.surfaces.includes("web_parser") && !parserTypes.has(type)) throw new Error(`${type}: declared web legacy input is not parsed`);
    if (spec.surfaces.includes("saved_config_parser") && !savedConfigAliases.has(type)) throw new Error(`${type}: declared saved-config alias has no generated migration`);
    if (spec.surfaces.includes("firmware") && !firmwareTypes.has(type)) throw new Error(`${type}: declared firmware legacy type is not registered`);
    if (spec.surfaces.includes("firmware") && spec.firmwareFamily &&
        firmwareTypes.get(type) !== spec.firmwareFamily) {
      throw new Error(`${type}: firmware family is ${firmwareTypes.get(type)}, expected ${spec.firmwareFamily}`);
    }
  }

  const expectedWebOnly = new Set(Object.entries(runtimeOnly).filter(([, spec]) => spec.surfaces.includes("web_registry")).map(([type]) => type));
  const unexpectedWeb = [...webTypes].filter((type) => !contractTypes.has(type) && !expectedWebOnly.has(type));
  const missingWebClassification = [...expectedWebOnly].filter((type) => !webTypes.has(type));
  if (unexpectedWeb.length || missingWebClassification.length) {
    throw new Error(`web runtime-only classification mismatch (unexpected: ${unexpectedWeb.join(", ") || "none"}; missing: ${missingWebClassification.join(", ") || "none"})`);
  }

  const expectedFirmwareOnly = new Set(Object.entries(runtimeOnly).filter(([, spec]) => spec.surfaces.includes("firmware")).map(([type]) => type));
  const unexpectedFirmware = [...firmwareTypes.keys()].filter((type) => !contractTypes.has(type) && !expectedFirmwareOnly.has(type));
  const missingFirmwareClassification = [...expectedFirmwareOnly].filter((type) => !firmwareTypes.has(type));
  if (unexpectedFirmware.length || missingFirmwareClassification.length) {
    throw new Error(`firmware runtime-only classification mismatch (unexpected: ${unexpectedFirmware.join(", ") || "none"}; missing: ${missingFirmwareClassification.join(", ") || "none"})`);
  }
}

function generateCases(inventory, contract, hooks) {
  const normalization = [];
  const surface = [];
  const infoOnlyKeys = new Set(Array.from(hooks.buttonTypePickerKeysForInfoOnly(true)));

  function addCase(sourceType, spec, mode, sourceKind) {
    const contractDefault = contract.cards[sourceType] ? contract.cards[sourceType].default : {};
    const config = Object.assign(Object.fromEntries(CONFIG_FIELDS.map((field) => [field, ""])), contractDefault, mode.config || {});
    const input = compactInput(config);
    const expected = plain(hooks.parseButtonConfig(input));
    if (expected.type !== spec.canonicalType) {
      throw new Error(`${sourceType || "<switch>"}/${mode.name}: normalized type ${expected.type} does not match ${spec.canonicalType}`);
    }
    const registration = spec.webRegistration;
    const preview = registration == null
      ? null
      : plain(hooks.buttonTypePreviewForMockNow(registration, expected));
    const displayType = sourceType || "switch";
    const name = `${displayType}: ${mode.name}`;
    normalization.push({ name, input, expected });
    surface.push({
      name,
      sourceKind,
      sourceType,
      mode: mode.name,
      canonicalType: spec.canonicalType,
      web: {
        registration: registration == null ? null : registration,
        picker: {
          main: !!hooks.buttonTypeVisibleInPickerFor(sourceType, false),
          subpage: !!hooks.buttonTypeVisibleInPickerFor(sourceType, true),
          infoOnly: infoOnlyKeys.has(sourceType),
        },
        preview: { available: !!preview, fingerprint: previewFingerprint(preview) },
      },
      firmware: { family: spec.firmwareFamily || null },
      lifecycle: mergedLifecycle(spec, mode),
    });
  }

  for (const [type, spec] of Object.entries(inventory.types)) {
    for (const mode of spec.modes) addCase(type, spec, mode, "contract");
  }
  for (const [type, spec] of Object.entries(inventory.runtimeOnlyTypes)) {
    addCase(type, spec, { name: spec.classification, config: spec.config, lifecycle: spec.lifecycle }, "runtime_only");
  }
  return { normalization, surface };
}

function contractRegistrationChecks(inventory, contract, hooks) {
  for (const [type, spec] of Object.entries(inventory.types)) {
    if (spec.classification !== "canonical" || spec.webRegistration !== type) continue;
    const actual = plain(hooks.buttonTypeRuntimeSpec(type));
    if (!actual) throw new Error(`${type || "<switch>"}: web runtime spec is missing`);
    const expected = contract.cards[type];
    const comparisons = {
      label: expected.label,
      allowInSubpage: expected.allowInSubpage,
      pickerKey: expected.pickerKey || "",
      hidden: !!expected.hidden,
      domains: expected.domains,
    };
    if (JSON.stringify(actual) !== JSON.stringify(comparisons)) {
      throw new Error(`${type || "<switch>"}: web runtime spec differs from the contract`);
    }
  }
}

function runtimeContractChecks(inventory, contract, hooks) {
  for (const [type, inventorySpec] of Object.entries(inventory.types)) {
    const runtimeSpec = contract.runtime.specs[type];
    if (!runtimeSpec) throw new Error(`${type || "<switch>"}: generated runtime metadata is missing`);
    const registeredSpec = hooks.buttonTypeGeneratedRuntimeSpec(type);
    if (hooks.buttonTypeRuntimeSpec(type) && JSON.stringify(plain(registeredSpec)) !== JSON.stringify(runtimeSpec)) {
      throw new Error(`${type || "<switch>"}: web registration does not use the generated runtime spec`);
    }
    const lifecycles = [inventorySpec.lifecycle].concat(
      inventorySpec.modes.map((mode) => mergedLifecycle(inventorySpec, mode))
    );
    const expected = {
      informationOnly: !!hooks.buttonTypeInfoOnlySupported(type),
      subscriptions: lifecycles.some((lifecycle) => !lifecycle.subscriptions.includes("none")),
      actions: lifecycles.some((lifecycle) => !lifecycle.actions.includes("none")),
      modal: lifecycles.some((lifecycle) => lifecycle.modalOwner !== "none" || lifecycle.actions.includes("modal")),
      subpage: !!contract.cards[type].allowInSubpage,
    };
    for (const [capability, value] of Object.entries(expected)) {
      if (runtimeSpec.capabilities[capability] !== value) {
        throw new Error(`${type || "<switch>"}: runtime capability ${capability} differs from the baseline`);
      }
    }
  }
}

function reportMarkdown(inventory, contract, cases) {
  const counts = new Map();
  for (const item of cases.surface) counts.set(item.sourceType, (counts.get(item.sourceType) || 0) + 1);
  const lines = [
    "# Card Runtime Coverage",
    "",
    "Generated by `scripts/generate_card_runtime_coverage.js` from the card contract and runtime inventory.",
    "It records the pre-migration baseline; it does not move executable behaviour into generated data.",
    "",
    `- Contract types: ${Object.keys(contract.cards).length}`,
    `- Runtime-only types: ${Object.keys(inventory.runtimeOnlyTypes).length}`,
    `- Baseline cases: ${cases.surface.length}`,
    "",
    "| Contract type | Classification | Runtime driver | Capabilities | Canonical saved type | Web registration | Firmware family | Cases |",
    "|---|---|---|---|---|---|---|---:|",
  ];
  for (const [type, spec] of Object.entries(inventory.types)) {
    const runtime = contract.runtime.specs[type];
    const enabledCapabilities = contract.runtime.capabilities.filter((capability) => runtime.capabilities[capability]);
    lines.push(`| ${type || "(switch)"} | ${spec.classification} | ${runtime.driver} | ${enabledCapabilities.join(", ") || "none"} | ${spec.canonicalType || "(switch)"} | ${spec.webRegistration || (spec.webRegistration === "" ? "(switch)" : "—")} | ${spec.firmwareFamily} | ${counts.get(type) || 0} |`);
  }
  lines.push("", "## Runtime-only type decisions", "", "| Runtime type | Classification | Canonical type | Surfaces | Reason |", "|---|---|---|---|---|");
  const reasons = {
    local: "Older local-action input; normalizes to Action with local dispatch.",
    text_sensor: "Older sensor input; normalizes to Sensor text mode.",
    todo: "Removed configurator type retained only for saved-card compatibility.",
    media_cover_art: "Hidden web picker implementation that creates a Media cover-art configuration.",
  };
  for (const [type, spec] of Object.entries(inventory.runtimeOnlyTypes)) {
    lines.push(`| ${type} | ${spec.classification} | ${spec.canonicalType} | ${spec.surfaces.join(", ")} | ${reasons[type] || "Reviewed runtime-only type."} |`);
  }
  lines.push("", "## Baseline lifecycle vocabulary", "", "The inventory records broad observable responsibilities so later family migrations can compare behaviour without encoding executable logic in JSON.", "", `- Subscriptions: ${inventory.lifecycleVocabulary.subscriptions.join(", ")}`, `- Actions: ${inventory.lifecycleVocabulary.actions.join(", ")}`, `- Modal owners: ${inventory.lifecycleVocabulary.modalOwners.join(", ")}`);
  return `${lines.join("\n")}\n`;
}

function formatJson(value) {
  return `${JSON.stringify(value, null, 2)}\n`;
}

function writeOrCheck(filePath, content, check) {
  if (check) {
    const current = fs.existsSync(filePath) ? fs.readFileSync(filePath, "utf8") : "";
    if (current !== content) throw new Error(`${path.relative(ROOT, filePath)} is stale; run node scripts/generate_card_runtime_coverage.js`);
    return;
  }
  fs.mkdirSync(path.dirname(filePath), { recursive: true });
  fs.writeFileSync(filePath, content);
}

function main() {
  const check = process.argv.includes("--check");
  const inventory = readJson(INVENTORY_PATH);
  const contract = readJson(CONTRACT_PATH);
  const hooks = loadHooks();
  const webTypes = webRegistrationTypes();
  const parserTypes = webParserTypes();
  const firmwareTypes = firmwareRegistrations();
  validateInventory(inventory, contract, webTypes, parserTypes, firmwareTypes);
  contractRegistrationChecks(inventory, contract, hooks);
  runtimeContractChecks(inventory, contract, hooks);
  const cases = generateCases(inventory, contract, hooks);
  writeOrCheck(NORMALIZATION_PATH, formatJson(cases.normalization), check);
  writeOrCheck(SURFACE_PATH, formatJson({ baselineVersion: 1, cases: cases.surface }), check);
  writeOrCheck(REPORT_PATH, reportMarkdown(inventory, contract, cases), check);
  console.log(check ? "Card runtime baseline and coverage outputs are current." : "Generated card runtime baseline and coverage outputs.");
}

try {
  main();
} catch (error) {
  console.error(`ERROR: ${error && error.message ? error.message : error}`);
  process.exit(1);
}
