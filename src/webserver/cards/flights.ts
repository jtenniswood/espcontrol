import type { CardRegistry } from "../application/card_registry";
import { configOptionValue, setConfigOptionValue } from "../model/config_primitives";
import {
  cardContractAllowInSubpage,
  cardContractCardLabel,
  cardContractDefaultConfig,
  cardContractDomains,
} from "../generated/card_contract";

// Flights card: aircraft near home from a Home Assistant template sensor built
// on the Flightradar24 integration (see docs/card-types/flights.md).
// Storage: entity=sensor.*, options="units=imperial" when imperial units are chosen.

function flightsUnits(options: string): "metric" | "imperial" {
  return configOptionValue(options, "units") === "imperial" ? "imperial" : "metric";
}

export function registerFlightsCardTypes(registry: CardRegistry): void {
  registry.register("flights", {
    label: () => cardContractCardLabel("flights"),
    allowInSubpage: () => cardContractAllowInSubpage("flights"),
    hideLabel: true,
    cardMetadata: { preview: { badge: "material-symbol-travel" } },
    defaultConfig: () => cardContractDefaultConfig("flights"),
    onSelect: function (b) {
      const defaults: any = cardContractDefaultConfig("flights");
      Object.keys(defaults).forEach(function (key) { (b as any)[key] = defaults[key]; });
    },
    renderSettings: function (panel, b, _slot, helpers) {
      const entity = helpers.renderCardEntityField(panel, b, helpers, {
        entity: {
          label: "Flights Sensor",
          idSuffix: "entity",
          placeholder: "e.g. sensor.espcontrol_flights",
          domains: cardContractDomains("flights"),
          requiredMessage: "Add the flights template sensor before saving.",
        },
      });
      helpers.requireEntityDomain(entity.input, ["sensor"], "Choose a sensor entity (sensor.*).");

      helpers.renderCardTextField(panel, b, helpers, {
        label: "List title",
        idSuffix: "label",
        field: "label",
        placeholder: "e.g. Flights",
      });

      helpers.renderCardSegmentControl(panel, b, helpers, {
        label: "Units",
        options: [
          ["metric", "Metric"],
          ["imperial", "Imperial"],
        ],
        value: () => flightsUnits(b.options),
        onSelect: function (button: any, cardHelpers: any, value: string) {
          button.options = setConfigOptionValue(button.options, "units", value === "imperial" ? "imperial" : "");
          cardHelpers.saveField("options", button.options);
        },
      });
    },
    renderPreview: function (b, helpers) {
      const entityName = b.entity?.replace(/^sensor\./, "").replace(/_/g, " ");
      const name = entityName
        ? entityName[0].toUpperCase() + entityName.slice(1)
        : "Flights";
      return {
        buttonClass: "sp-rainfall-card sp-flights-card",
        iconHtml: '<span class="sp-btn-icon mdi mdi-material-symbol-travel"></span>',
        labelHtml: '<span class="sp-btn-label">3<span class="sp-rainfall-value">' +
          helpers.escHtml(name) + '</span></span>',
        labelEditable: false,
      };
    },
  });
}
