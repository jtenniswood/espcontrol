import type { CardConfig } from "../contracts/types";
import { cloneCardConfig } from "../model/card";
import { badgeAction, badgeSubtype, isSubpageBadge, normalizeBadgeConfig } from "../model/badge";
import { badgeCardType, badgeToCard } from "../model/badge_card";
import { badgeCardDisplay, badgeCardSupportsValue } from "../model/badge_card_display";
import { cardContractDomains, cardContractCard } from "../generated/card_contract";
import { configOptionEnabled, configOptionValue, setConfigOption, setConfigOptionValue } from "../model/config_primitives";
import type { ControlsFieldsFeature } from "./controls_fields";
import type { EntityStateFeature } from "./entity_state";

export interface BadgeBulkSettings {
  apply(source: CardConfig): CardConfig;
  validate(): string | null;
}

/** Record individual edits so mixed values and private page links survive a bulk save. */
export function renderBadgeBulkSettings(panel: HTMLElement, sources: CardConfig[], helpers: {
  document: Document;
  fields: ControlsFieldsFeature;
  entityInput: EntityStateFeature["entityInput"];
  iconPickerField(pickerId: string, inputId: string, value: string, onSelect: (value: string) => void, label: string): HTMLElement;
  onChange(): void;
}): BadgeBulkSettings {
  const { document, fields } = helpers;
  const edits = new Map<keyof CardConfig, string>();
  const optionEdits = new Map<string, string>();
  const same = <T>(values: T[]) => values.every(value => value === values[0]) ? values[0] : undefined;
  const common = (key: keyof CardConfig) => same(sources.map(source => source[key]));
  const commonOption = (key: string) => same(sources.map(source => configOptionValue(source.options, key)));
  const subtype = same(sources.map(badgeSubtype));
  const cardType = same(sources.map(badgeCardType));
  const editableDisplay = sources.every(source => !isSubpageBadge(source) && badgeCardType(source) === null && !["sensor", "button"].includes(badgeSubtype(source)));
  const validators: Array<() => string | null> = [];
  function field(label: string, suffix: string) {
    const wrapper = document.createElement("div");
    wrapper.className = "sp-field";
    const id = "sp-badge-bulk-" + suffix;
    wrapper.appendChild(fields.fieldLabel(label, id));
    panel.appendChild(wrapper);
    return { wrapper, id };
  }
  function text(label: string, key: keyof CardConfig, domains?: string[]) {
    const { wrapper, id } = field(label, key);
    if (label === "Entity") fields.markCardPrimaryField(wrapper, "entity");
    if (label === "Label") fields.markCardPrimaryField(wrapper, "name");
    const value = common(key);
    const input = domains
      ? helpers.entityInput(id, value || "", value === undefined ? "Multiple values" : "Entity", domains)
      : fields.textInput(id, value || "", value === undefined ? "Multiple values" : "Optional");
    wrapper.appendChild(input);
    const changed = () => {
      edits.set(key, input.value);
      if (key === "sensor" && subtype === "sensor") edits.set("precision", "value");
      helpers.onChange();
    };
    input.addEventListener("input", changed);
    input.addEventListener("change", changed);
    if (domains) validators.push(() => {
      if (!edits.has(key)) return null;
      if (!input.value.trim()) return "Choose an entity before saving.";
      if (domains.length && !domains.includes(input.value.split(".")[0])) return "Choose a supported entity for these badges.";
      return null;
    });
  }
  function select(label: string, suffix: string, value: string | undefined, choices: Array<[string, string]>, changed: (value: string) => void) {
    const { wrapper, id } = field(label, suffix);
    const input = document.createElement("select");
    input.id = id;
    input.className = "sp-select";
    const entries: Array<[string, string]> = [["__keep", "Keep existing values"], ...choices];
    for (const [key, text] of entries) {
      const option = document.createElement("option");
      option.value = key;
      option.textContent = text;
      input.appendChild(option);
    }
    input.value = value ?? "__keep";
    input.addEventListener("change", () => { changed(input.value); helpers.onChange(); });
    wrapper.appendChild(input);
  }
  const nativeCards = sources.map(badgeToCard);
  const localSource = sources.some(source => badgeCardType(source) === "local_sensor" ||
    (["sensor", "action", "local"].includes(badgeCardType(source) || "") && source.sensor === "local"));
  if (subtype && subtype !== "subpage" && (subtype !== "card" || cardType !== undefined) && !localSource) {
    let domains = cardType !== null && cardType !== undefined ? cardContractDomains(cardType) : subtype === "slider" ? ["light", "fan", "number", "input_number", "media_player"]
      : subtype === "switch" ? ["light", "switch", "input_boolean", "fan"]
      : subtype === "button" ? ["button", "input_button"] : [];
    // Native modes can bind different fields or domains. Keep mixed-mode entities intact.
    const sharedMode = same(nativeCards.map(card => card.sensor));
    const modeMatches = !["action", "local", "media", "cover"].includes(cardType || "") || sharedMode !== undefined;
    if (cardType === "action" && sharedMode?.includes(".")) domains = domains.filter(domain => domain === sharedMode.split(".")[0]);
    if (subtype !== "card" || (domains.length && modeMatches)) {
      text("Entity", subtype === "sensor" || ["sensor", "door_window", "presence"].includes(cardType || "") ? "sensor" : "entity", domains);
    }
  }
  if (sources.every(source => badgeSubtype(source) !== "sensor" && !["clock", "calendar", "screen_lock"].includes(badgeCardType(source) || ""))) text("Label", "label");
  panel.appendChild(helpers.iconPickerField("sp-badge-bulk-icon-picker", "sp-badge-bulk-icon", common("icon") || "Auto", value => {
    edits.set("icon", value);
    helpers.onChange();
  }, common("icon") === undefined ? "Icon (multiple values)" : "Icon"));
  if (editableDisplay) {
    select("Display", "display", common("precision"), [["label", "Label"], ["value", "Live value"]], value => {
      if (value === "__keep") edits.delete("precision"); else edits.set("precision", value);
    });
    if (subtype !== "sensor" && subtype !== "slider") text("Display Entity", "sensor");
  }
  if (sources.every(source => !isSubpageBadge(source) && badgeCardType(source) !== null)) {
    const choices: Array<[string, string]> = [["card", "Card display"], ["label", "Label"]];
    if (sources.every(source => badgeCardSupportsValue(badgeToCard(source))) || same(sources.map(badgeCardDisplay)) === "value") choices.push(["value", "Live value"]);
    select("Badge Display", "badge-display", same(sources.map(badgeCardDisplay)), choices, value => {
      if (value === "__keep") optionEdits.delete("badge_display");
      else optionEdits.set("badge_display", value === "card" ? "" : value);
    });
  }
  if (sources.every(source => !isSubpageBadge(source) && badgeCardType(source) === null && !["button", "slider"].includes(badgeSubtype(source)))) {
    text("Unit (optional)", "unit");
    const { wrapper, id } = field("Attribute (optional)", "attribute");
    const attribute = commonOption("badge_attribute");
    const input = fields.textInput(id, attribute || "", attribute === undefined ? "Multiple values" : "Entity state");
    input.addEventListener("input", () => { optionEdits.set("badge_attribute", input.value); helpers.onChange(); });
    wrapper.appendChild(input);
    select("Decimal Places", "decimals", same(sources.map(source => configOptionValue(source.options, "badge_decimals") || "1")), [["0", "0"], ["1", "1"], ["2", "2"]], value => {
      if (value === "__keep") optionEdits.delete("badge_decimals"); else optionEdits.set("badge_decimals", value);
    });
  }
  if (sources.every(source => !isSubpageBadge(source) && (badgeCardType(source) === null ? badgeSubtype(source) !== "button"
    : cardContractCard(badgeCardType(source))?.options?.some(option => option.name === "active_color")))) {
    const active = same(sources.map(source => configOptionEnabled(source.options, "active_color")));
    const toggle = fields.toggleRow(active === undefined ? "Lit When Active (multiple values)" : "Lit When Active", "sp-badge-bulk-active", active === true);
    toggle.input.indeterminate = active === undefined;
    toggle.input.addEventListener("change", () => { optionEdits.set("active_color", toggle.input.checked ? "1" : ""); helpers.onChange(); });
    panel.appendChild(toggle.row);
  }
  function apply(source: CardConfig) {
    const next = cloneCardConfig(source);
    for (const [key, value] of edits) next[key] = value;
    for (const [key, value] of optionEdits) next.options = key === "active_color"
      ? setConfigOption(next.options, key, value === "1") : setConfigOptionValue(next.options, key, value);
    if (badgeAction(next) === "slider") normalizeBadgeConfig(next);
    return next;
  }
  return { apply, validate() {
    for (const validate of validators) { const error = validate(); if (error) return error; }
    if (edits.has("precision") || edits.has("sensor")) {
      for (const source of sources) {
        const next = apply(source);
        if (next.precision === "value" && badgeAction(next) !== "slider" && !next.sensor.trim()) return "Choose a display entity for every badge showing a live value.";
      }
    }
    return null;
  } };
}
