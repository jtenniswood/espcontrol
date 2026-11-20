import type { CardRegistry } from "../application/card_registry";
import { cardContractAllowInSubpage, cardContractCardLabel, cardContractDefaultConfig, cardContractHidden, cardContractOptions } from "../generated/card_contract";
import { sizeColSpan } from "../model/grid";
import { normalizeSavedConfigPower } from "../generated/saved_config_power";
import { decodePowerCardConfigV1, normalizePowerOptions, validPowerEntity } from "../model/power_card";
import { setConfigOption, setConfigOptionValue, preserveCardColorOptions } from "../model/config_primitives";
import type { ControlsFieldsFeature } from "../application/controls_fields";

export function registerPowerCardTypes(registry: CardRegistry, fields: Pick<ControlsFieldsFeature, "cardSensorPreviewHtml">): void {
  registry.register("power", {
    label: () => cardContractCardLabel("power"),
    allowInSubpage: () => cardContractAllowInSubpage("power"),
    hidden: () => cardContractHidden("power"),
    cardMetadata: {},
    labelPlaceholder: "Power Dashboard",
    defaultConfig: () => cardContractDefaultConfig("power"),
    normalizeConfig: config => normalizeSavedConfigPower(config, normalizePowerOptions),
    onSelect: config => normalizeSavedConfigPower(config, normalizePowerOptions),
    renderSettings: (panel, card, _slot, helpers) => {
      const config = decodePowerCardConfigV1(card);
      const optionLabel = (name: string) => cardContractOptions("power").find(option => option.name === name)!.label;
      const sensors = [
        { key: "entity", label: "Home power", value: config.homeEntity, note: "Home is always included. Positive consumption; optional sensor reporting W or kW. Leave empty for an unmeasured Home.", component: "", bit: 2 },
        { key: "solar_entity", label: optionLabel("solar_entity"), value: config.solarEntity, note: "Positive production; W or kW. Leave empty if installed but unmeasured.", component: "solar", bit: 1 },
        { key: "grid_entity", label: optionLabel("grid_entity"), value: config.gridEntity, note: "Positive importing, negative exporting; W or kW. Sensor optional.", component: "grid", bit: 4 },
        { key: "battery_entity", label: optionLabel("battery_entity"), value: config.batteryEntity, note: "Positive discharging, negative charging; W or kW. Sensor optional.", component: "battery", bit: 8 },
        { key: "battery_soc_entity", label: optionLabel("battery_soc_entity") + " (optional)", value: config.batterySocEntity, note: "A separate percentage sensor (0–100%). Does not affect power flows.", component: "", bit: 8 },
      ];
      const sensorFields: Array<{ bit: number; field: HTMLElement; input: HTMLInputElement }> = [];
      for (const sensor of sensors) {
        const included = (config.installedComponents & sensor.bit) !== 0;
        const present = sensor.component ? helpers.toggleRow(
          sensor.component[0]!.toUpperCase() + sensor.component.slice(1) + " included in system",
          helpers.idPrefix + "power-include-" + sensor.component, included) : null;
        if (present) {
          const componentField = document.createElement("div");
          componentField.className = "sp-field";
          componentField.appendChild(present.row);
          panel.appendChild(componentField);
        }
        const field = helpers.entityField(sensor.label, helpers.idPrefix + "power-" + sensor.key,
          sensor.value, "sensor." + (sensor.key === "entity" ? "home_power" : sensor.key.replace("_entity", "")),
          ["sensor"], null, false);
        panel.appendChild(field.field);
        field.field.hidden = !included;
        field.input.disabled = !included;
        if (present) present.input.addEventListener("change", () => {
          field.field.hidden = !present.input.checked;
          field.input.disabled = !present.input.checked;
          card.options = preserveCardColorOptions(card.options, normalizePowerOptions(setConfigOption(card.options,
            "exclude_" + sensor.component, !present.input.checked)));
          const mask = decodePowerCardConfigV1(card).installedComponents;
          for (const control of sensorFields) {
            control.field.hidden = !(mask & control.bit);
            control.input.disabled = !(mask & control.bit);
          }
          helpers.saveField("options", card.options);
        });
        sensorFields.push({ bit: sensor.bit, field: field.field, input: field.input });
        if (sensor.key === "entity") helpers.markCardPrimaryField(field.field, "entity");
        helpers.requireField(field.input, "Choose a sensor entity (sensor.*) for " + sensor.label + ".", undefined,
          (value: string) => field.input.disabled || !value.trim() || validPowerEntity(value));
        const sync = () => {
          const value = field.input.value.trim();
          if (sensor.key === "entity") card.entity = value;
          else card.options = preserveCardColorOptions(card.options,
            normalizePowerOptions(setConfigOptionValue(card.options, sensor.key, value)));
          helpers.saveField(sensor.key === "entity" ? "entity" : "options", sensor.key === "entity" ? card.entity : card.options);
        };
        field.input.addEventListener("input", sync);
        field.input.addEventListener("change", sync);
        const note = document.createElement("div");
        note.className = "sp-apply-note";
        note.textContent = sensor.note;
        field.field.appendChild(note);
        if (sensor.component === "grid" || sensor.component === "battery") {
          const name = sensor.component === "grid" ? "invert_grid" : "invert_battery";
          const toggle = helpers.toggleRow(optionLabel(name), helpers.idPrefix + "power-" + name,
            name === "invert_grid" ? config.invertGrid : config.invertBattery);
          panel.appendChild(toggle.row);
          toggle.row.hidden = !included;
          toggle.input.disabled = !included;
          sensorFields.push({ bit: sensor.bit, field: toggle.row, input: toggle.input });
          toggle.input.addEventListener("change", () => {
            card.options = preserveCardColorOptions(card.options,
              normalizePowerOptions(setConfigOption(card.options, name, toggle.input.checked)));
            helpers.saveField("options", card.options);
          });
        }
      }
    },
    renderPreview: (card, helpers) => {
      const narrow = sizeColSpan(helpers.cardSize) < 2;
      const label = '<span class="sp-btn-label">' + helpers.escHtml(card.label || "Power Dashboard") + '</span>';
      const arrow = '<span class="sp-navigation-chevron sp-power-chevron mdi mdi-chevron-right" aria-hidden="true"></span>';
      return {
        buttonClass: "sp-power-card" + (narrow ? " sp-power-narrow" : ""),
        iconHtml: narrow ? "" : fields.cardSensorPreviewHtml(card, helpers, "--", "W"),
        labelHtml: narrow ? '<span class="sp-btn-label-row">' + label + arrow + '</span>'
          : '<span class="sp-power-labels">' + label + '<span class="sp-btn-label-row"><span class="sp-btn-label sp-power-secondary">Unavailable</span>' + arrow + '</span></span>',
      };
    },
  });
}
