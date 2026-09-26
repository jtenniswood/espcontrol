import {
  cardContractAllowInSubpage,
  cardContractCardLabel,
  cardContractDefaultConfig,
  cardContractDomains,
  cardContractHidden,
  cardContractPickerKey,
} from "../generated/card_contract";
import type { CardRegistry } from "../application/card_registry";
import type { ControlsFieldsFeature } from "../application/controls_fields";

export function registerBatteryCardTypes(registry: CardRegistry, fields: ControlsFieldsFeature): void {
  const metadata = {
    entity: {
      label: "Battery Entity",
      idSuffix: "entity",
      placeholder: "e.g. sensor.phone_battery",
      domains: () => cardContractDomains("battery"),
      bindName: "entity",
      rerender: true,
      requiredMessage: "Add an entity before saving.",
    },
  };
  registry.register("battery", {
    label: () => cardContractCardLabel("battery"),
    allowInSubpage: () => cardContractAllowInSubpage("battery"),
    pickerKey: () => cardContractPickerKey("battery"),
    hidden: () => cardContractHidden("battery"),
    hideLabel: true,
    defaultConfig: () => cardContractDefaultConfig("battery"),
    cardMetadata: metadata,
    onSelect(b: any) {
      Object.assign(b, cardContractDefaultConfig("battery"), { entity: b.entity });
    },
    renderSettings(panel: any, b: any, _slot: any, helpers: any) {
      helpers.renderCardEntityField(panel, b, helpers, metadata);
    },
    renderPreview(_b: any, helpers: any) {
      return {
        iconHtml: '<span class="sp-btn-icon mdi mdi-battery-80"></span>',
        labelHtml: fields.cardBadgeLabelHtml(helpers, "80%", "battery"),
      };
    },
  });
}
