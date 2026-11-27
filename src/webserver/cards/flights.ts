import type { CardRegistry, CardUiServices } from "../application/card_registry";
import { configOptionValue, setConfigOptionValue } from "../model/config_primitives";
import {
  cardContractAllowInSubpage,
  cardContractCardLabel,
  cardContractDefaultConfig,
  cardContractDomains,
} from "../generated/card_contract";

// Flights card: aircraft near home from a Home Assistant template sensor built
// on the Flightradar24 integration (see docs/card-types/flights.md).
// The selected template sensor supplies both the count and flight list.

const AREA_VIEWS = {
  current: { label: "Current in area", sensor: "sensor.espcontrol_flights", icon: "material-symbol-travel" },
  entered: { label: "Entered area", sensor: "sensor.espcontrol_flights_entered", icon: "material-symbol-connecting-airports" },
  exited: { label: "Exited area", sensor: "sensor.espcontrol_flights_exited", icon: "material-symbol-airplanemode-inactive" },
};

function flightsArea(options: string): keyof typeof AREA_VIEWS {
  const area = configOptionValue(options, "area");
  return area === "entered" || area === "exited" ? area : "current";
}

function flightsUnits(options: string): "metric" | "imperial" {
  return configOptionValue(options, "units") === "imperial" ? "imperial" : "metric";
}

export function registerFlightsCardTypes(registry: CardRegistry, cardUi: CardUiServices): void {
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
      const area = flightsArea(b.options);
      helpers.renderCardModeSelector(panel, b, helpers, {
        mode: {
          label: "Area view",
          idSuffix: "flights-area",
          options: Object.entries(AREA_VIEWS).map(([value, view]) => [value, view.label]),
          value: area,
          onChange(this: HTMLSelectElement, button, cardHelpers) {
            button.options = setConfigOptionValue(button.options, "area",
              this.value === "current" ? "" : this.value);
            if (!button.entity || button.entity === AREA_VIEWS[area].sensor) {
              button.entity = AREA_VIEWS[flightsArea(button.options)].sensor;
              cardHelpers.saveField("entity", button.entity);
            }
            cardHelpers.saveField("options", button.options);
            cardUi.renderPreview();
            cardUi.renderButtonSettings();
          },
        },
      });

      const entity = helpers.renderCardEntityField(panel, b, helpers, {
        entity: {
          label: "Flights Sensor",
          idSuffix: "entity",
          placeholder: "e.g. " + AREA_VIEWS[area].sensor,
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
        buttonClass: "sp-flights-card",
        iconHtml: '<span class="sp-flights-reading">' +
          '<span class="mdi mdi-' + AREA_VIEWS[flightsArea(b.options)].icon + '" aria-hidden="true"></span>' +
          '<span class="sp-flights-count">3</span>' +
          '<span class="sp-flights-sublabel">' + helpers.escHtml(name) + '</span></span>',
        labelHtml: "",
        labelEditable: false,
      };
    },
  });
}
