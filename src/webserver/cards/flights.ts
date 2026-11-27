import type { CardRegistry, CardUiServices } from "../application/card_registry";
import { iconSlug } from "../application/ui_primitives";
import { FLIGHTS_TITLES } from "../generated/flights_titles";
import { state } from "../state/app_instance";
import { configOptionValue, setConfigOptionValue } from "../model/config_primitives";
import {
  cardContractAllowInSubpage,
  cardContractCardLabel,
  cardContractDefaultConfig,
  cardContractDomains,
} from "../generated/card_contract";

// Shows a flight count from a Home Assistant sensor.

const AREA_VIEWS = {
  current: { sensor: "sensor.flightradar24_current_in_area", template: "sensor.espcontrol_flights", icon: "material-symbol-travel" },
  entered: { sensor: "sensor.flightradar24_entered_area", template: "sensor.espcontrol_flights_entered", icon: "material-symbol-connecting-airports" },
  exited: { sensor: "sensor.flightradar24_exited_area", template: "sensor.espcontrol_flights_exited", icon: "material-symbol-airplanemode-inactive" },
};

function flightsViewTitles() {
  return FLIGHTS_TITLES[state.language as keyof typeof FLIGHTS_TITLES] ?? FLIGHTS_TITLES.en;
}

function usesViewTitle(label: string, options: string, area: keyof typeof AREA_VIEWS): boolean {
  return label === FLIGHTS_TITLES.en[area] || (!label && configOptionValue(options, "title_source") !== "sensor");
}

function flightsArea(options: string): keyof typeof AREA_VIEWS {
  const area = configOptionValue(options, "area");
  return area === "entered" || area === "exited" ? area : "current";
}

function flightsCustomIcon(icon: string, options: string): string {
  return icon && icon !== "Auto" && configOptionValue(options, "icon_source") === "custom" ? icon : "";
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
      const viewTitles = flightsViewTitles();
      helpers.renderCardModeSelector(panel, b, helpers, {
        mode: {
          label: "Area view",
          idSuffix: "flights-area",
          options: Object.entries(FLIGHTS_TITLES.en),
          value: area,
          onChange(this: HTMLSelectElement, button: typeof b, cardHelpers: typeof helpers) {
            const useDefault = usesViewTitle(button.label, button.options, area);
            button.options = setConfigOptionValue(button.options, "area",
              this.value === "current" ? "" : this.value);
            const nextArea = flightsArea(button.options);
            if (useDefault) {
              button.label = FLIGHTS_TITLES.en[nextArea];
              cardHelpers.saveField("label", button.label);
            }
            const previousEntity = button.entity;
            if (previousEntity === AREA_VIEWS[area].template) {
              button.entity = AREA_VIEWS[nextArea].template;
            } else if (!previousEntity || previousEntity === AREA_VIEWS[area].sensor) {
              button.entity = AREA_VIEWS[nextArea].sensor;
            }
            if (button.entity !== previousEntity) {
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
          requiredMessage: "Add the Flights sensor before saving.",
        },
      });
      helpers.requireEntityDomain(entity.input, ["sensor"], "Choose a sensor entity (sensor.*).");

      helpers.renderCardIconPicker(panel, b, helpers, {
        value: flightsCustomIcon(b.icon, b.options) || "Auto",
        onChange(button: typeof b, cardHelpers: typeof helpers, icon: string) {
          button.options = setConfigOptionValue(button.options, "icon_source", icon === "Auto" ? "" : "custom");
          cardHelpers.saveField("options", button.options);
          cardUi.renderPreview();
        },
      });

      const title = helpers.renderCardTextField(panel, b, helpers, {
        label: "Title",
        idSuffix: "label",
        bindName: "",
        value: usesViewTitle(b.label, b.options, area) ? viewTitles[area] : b.label,
        placeholder: "Home Assistant sensor name",
      });
      const updateTitle = () => {
        const text = title.input.value;
        b.label = text === viewTitles[area] ? FLIGHTS_TITLES.en[area] : text;
        b.options = setConfigOptionValue(b.options, "title_source", text ? "" : "sensor");
        helpers.saveField("label", b.label);
        cardUi.renderPreview();
      };
      title.input.addEventListener("input", updateTitle);
      title.input.addEventListener("change", updateTitle);

    },
    renderPreview: function (b, helpers) {
      const entityName = b.entity?.replace(/^sensor\./, "").replace(/_/g, " ");
      const area = flightsArea(b.options);
      const customIcon = flightsCustomIcon(b.icon, b.options);
      const name = usesViewTitle(b.label, b.options, area) ? flightsViewTitles()[area] : b.label || (entityName
        ? entityName[0].toUpperCase() + entityName.slice(1)
        : "Flights");
      return {
        buttonClass: "sp-flights-card",
        iconHtml: '<span class="sp-flights-reading">' +
          '<span class="mdi mdi-' + (customIcon ? iconSlug(customIcon) : AREA_VIEWS[area].icon) + '" aria-hidden="true"></span>' +
          '<span class="sp-flights-count">3</span>' +
          '<span class="sp-flights-sublabel">' + helpers.escHtml(name) + '</span></span>',
        labelHtml: "",
        labelEditable: false,
      };
    },
  });
}
