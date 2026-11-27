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
    cardMetadata: { preview: { badge: "airplane" } },
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
        label: "Label",
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
      const label = b.label || "Flights";
      return {
        iconHtml:
          '<span class="sp-sensor-preview">' +
            '<span class="sp-sensor-value">3</span>' +
          '</span>',
        labelHtml:
          '<span class="sp-btn-label-row"><span class="sp-btn-label">' + helpers.escHtml(label) + '</span>' +
          '<span class="sp-type-badge mdi mdi-airplane"></span></span>',
      };
    },
  });
}
