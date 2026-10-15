import type { CardRegistry, CardUiServices } from "../application/card_registry";
import { cardContractDefaultConfig, cardContractCardLabel } from "../generated/card_contract";
import { configOptionEnabled, configOptionValue, setConfigOptionValue } from "../model/config_primitives";
import { badgeAction, badgeSubtype, isSubpageBadge, normalizeBadgeConfig } from "../model/badge";
import { iconSlug } from "../application/ui_primitives";
import { badgeCardType, badgeToCard, cardBadgeUsesSlider } from "../model/badge_card";
import { badgeCardDisplay, badgeCardContent, badgeText } from "../model/badge_card_display";

export function registerBadgeCardTypes(registry: CardRegistry, cardUi: CardUiServices): void {
  registry.register("badge", {
    label: "Badge",
    isAvailable: () => false,
    allowInSubpage: true,
    hideLabel: true,
    cardMetadata: { preview: { badge: "label" } },
    defaultConfig: () => cardContractDefaultConfig("badge"),
    normalizeConfig: normalizeBadgeConfig,
    onSelect(button) {
      Object.assign(button, cardContractDefaultConfig("badge"));
    },
    renderSettings(panel, button, _slot, helpers) {
      const rerender = () => helpers.renderBadgeSettings ? helpers.renderBadgeSettings() : cardUi.renderButtonSettings();
      const action = badgeAction(button);
      const subtype = badgeSubtype(button);
      function select(label: string, suffix: string, value: string, choices: Array<[string, string]>, save: (value: string) => void) {
        const field = document.createElement("div");
        field.className = "sp-field";
        const id = helpers.idPrefix + suffix;
        field.appendChild(helpers.fieldLabel(label, id));
        const input = document.createElement("select");
        input.className = "sp-select";
        input.id = id;
        for (const [key, text] of choices) {
          const option = document.createElement("option");
          option.value = key;
          option.textContent = text;
          input.appendChild(option);
        }
        input.value = value;
        input.addEventListener("change", () => save(input.value));
        field.appendChild(input);
        panel.appendChild(field);
        return input;
      }
      function option(key: string, value: string, rerender = false) {
        button.options = setConfigOptionValue(button.options, key, value);
        helpers.saveField("options", button.options);
        if (rerender) helpers.renderBadgeSettings ? helpers.renderBadgeSettings() : cardUi.renderButtonSettings();
      }
      function text(label: string, field: string, placeholder: string, domains?: string[], required = false) {
        const wrapper = document.createElement("div");
        wrapper.className = "sp-field";
        const id = helpers.idPrefix + field;
        wrapper.appendChild(helpers.fieldLabel(label, id));
        const input = domains ? helpers.entityInput(id, button[field], placeholder, domains) : helpers.textInput(id, button[field], placeholder);
        wrapper.appendChild(input);
        panel.appendChild(wrapper);
        helpers.bindField(input, field, true);
        if (domains) helpers.markCardPrimaryField(wrapper, "entity");
        else if (field === "label") helpers.markCardPrimaryField(wrapper, "name");
        if (required) helpers.requireField(input, "Choose an entity before saving.");
        return input;
      }
      if (action !== "subpage" && subtype === "sensor") {
        const input = text("Entity", "sensor", "e.g. sensor.kitchen_temperature", [], true);
        input.addEventListener("input", () => { button.precision = "value"; helpers.saveField("precision", "value"); });
        input.addEventListener("change", () => { button.precision = "value"; helpers.saveField("precision", "value"); });
      } else if (action !== "subpage") {
        const domains = action === "slider" ? ["light", "fan", "number", "input_number", "media_player"]
          : action === "toggle" ? ["light", "switch", "input_boolean", "fan"]
          : action === "button" ? ["button", "input_button"] : [];
        const input = text("Entity", "entity", action === "button" ? "e.g. button.restart" : "e.g. light.kitchen", domains, true);
        if (domains.length) helpers.requireEntityDomain(input, domains, "Choose a supported entity for this interaction.");
      }
      if (subtype !== "sensor" && button.precision !== "value") text("Label", "label", "e.g. Kitchen");
      helpers.renderCardIconPicker(panel, button, helpers, { field: "icon", fallback: "Auto", label: "Icon" });
      if (subtype !== "sensor" && subtype !== "button") {
        select("Display", "badge-display", button.precision || "label", [["label", "Label"], ["value", "Live value"]], value => {
          button.precision = value;
          helpers.saveField("precision", value);
          rerender();
        });
      }
      if (subtype !== "sensor" && subtype !== "slider" && (button.precision === "value" || configOptionEnabled(button.options, "active_color"))) {
        text("Display Entity", "sensor", "e.g. sensor.kitchen_temperature", [], button.precision === "value" || action === "modal");
      }
      if (subtype !== "slider" && (subtype === "sensor" || button.precision === "value")) {
        text("Unit (optional)", "unit", "Auto, e.g. °C");
        const field = document.createElement("div");
        field.className = "sp-field";
        field.appendChild(helpers.fieldLabel("Attribute (optional)", helpers.idPrefix + "badge-attribute"));
        const input = helpers.textInput(helpers.idPrefix + "badge-attribute", configOptionValue(button.options, "badge_attribute"), "Entity state, e.g. current_temperature");
        input.addEventListener("change", () => option("badge_attribute", input.value));
        field.appendChild(input);
        panel.appendChild(field);
        select("Decimal Places", "badge-decimals", configOptionValue(button.options, "badge_decimals") || "1", [["0", "0"], ["1", "1"], ["2", "2"]], value => option("badge_decimals", value));
      }
      if (subtype !== "button") {
        const tint = helpers.toggleRow("Lit When Active", helpers.idPrefix + "badge-active", configOptionEnabled(button.options, "active_color"));
        tint.input.addEventListener("change", () => option("active_color", tint.input.checked ? "1" : "", true));
        panel.appendChild(tint.row);
      }
    },
    renderPreview(button, helpers) {
      function markup(icon: string, title: string, reading: string, labelOnly: boolean, slider = false) {
        const text = badgeText(title, reading, labelOnly);
        return (slider ? '<span class="sp-badge-fill"></span>' : '') + '<span class="sp-badge-content"><span class="sp-badge-icon"><span class="' + helpers.escHtml(icon) + '"></span></span>' +
          '<span class="sp-badge-copy"><span class="sp-badge-text">' + helpers.escHtml(text) + '</span></span></span>';
      }
      if (isSubpageBadge(button)) {
        return { buttonClass: "sp-badge-card sp-badge-unavailable", iconHtml:
          markup("mdi mdi-" + iconSlug(button.icon === "Auto" ? "Home" : button.icon), button.label || "Badge", "", true), labelHtml: "" };
      }
      if (badgeCardType(button) !== null) {
        const card = badgeToCard(button);
        const definition = registry.definitions[card.type];
        const preview: { iconHtml?: string; labelHtml?: string } = definition?.renderPreview?.(card, {
          ...helpers, resolveIcon: (icon: string) => iconSlug(icon || "Home"), cardSize: 1,
          cardBackgroundColor: helpers.cardBackgroundColor || "", cardProgressColor: helpers.cardProgressColor || "FF8C00", cardTextColor: helpers.cardTextColor || "#fff",
        }) || {};
        const sample = document.createElement("div");
        sample.innerHTML = (preview.iconHtml || "") + (preview.labelHtml || "");
        const numeric = sample.querySelector(".sp-sensor-value");
        const reading = numeric ? (numeric.textContent || "") + (sample.querySelector(".sp-sensor-unit")?.textContent || "") : "";
        const caption = sample.querySelector(".sp-btn-label")?.textContent || card.label || cardContractCardLabel(card.type);
        const track = sample.querySelector(".sp-media-now-title,.sp-media-cover-details-title")?.textContent || caption;
        const slider = cardBadgeUsesSlider(card);
        const content = badgeCardContent(card, badgeCardDisplay(button));
        const sliderReading = card.type === "media" ? reading || track : card.type === "light_temperature" ? "3000K"
          : (/^(number|input_number)\./.test(card.entity) ? "35" : "35%");
        const label = content === "label" ? card.label || cardContractCardLabel(card.type)
          : content === "text" ? caption : content === "track" ? track
          : content === "date" ? reading + " " + caption : content === "datetime" ? caption + " · " + reading
          : content === "slider" ? sliderReading : reading || caption;
        const icon = sample.querySelector(".mdi")?.className.split(/\s+/).filter(name => name === "mdi" || name.startsWith("mdi-")).join(" ")
          || "mdi mdi-" + iconSlug(card.icon === "Auto" ? "Home" : card.icon);
        return { buttonClass: "sp-badge-card" + (slider ? " sp-badge-slider" : ""),
          iconHtml: markup(icon, card.label || cardContractCardLabel(card.type), label, content === "label", slider), labelHtml: "" };
      }
      const action = badgeAction(button);
      const value = button.precision === "value";
      const decimals = configOptionValue(button.options, "badge_decimals") || "1";
      const reading = (21.4).toFixed(Number(decimals));
      const sample = action === "slider" ? (/^(number|input_number)\./.test(button.entity) ? "35" : "35%") : reading + (button.unit || "°C");
      const label = value ? sample : button.label || "Badge";
      const icon = iconSlug(button.icon && button.icon !== "Auto" ? button.icon : action === "slider" ? "Volume High" : "Home");
      return {
        buttonClass: "sp-badge-card" + (action === "slider" ? " sp-badge-slider" : ""),
        iconHtml: markup("mdi mdi-" + icon, button.label || "Badge", label, !value, action === "slider"),
        labelHtml: "",
      };
    },
  });
}
