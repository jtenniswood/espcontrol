import { registerImageCardTypes } from "../../src/webserver/cards/image";
import { createConfigImageOptionsFeature } from "../../src/webserver/application/config_image_options";

export function runCameraNameTests(): void {
  function check(value: unknown, message: string) {
    if (!value) throw new Error(message);
  }
  const options = createConfigImageOptionsFeature({
    layout: { config: { imageSlotCapacity: 6 } },
    mediaOptions: { mediaEditorMode: () => "" },
    showBanner: () => {},
  } as any);
  let definition: any;
  registerImageCardTypes({ register: (_type: string, value: any) => { definition = value; } } as any,
    options, {} as any, { renderPreview: () => {} } as any);
  const camera = { type: "image", entity: "camera.front", label: "Front Door", options: "image_label" };
  options.setImageLabelEnabled(camera, false);
  check(camera.label === "Front Door", "Hiding the label must retain the modal name");
  const fields: string[] = [];
  let primaryName = false;
  const node = () => ({ classList: { add() {}, toggle() {} }, appendChild() {} });
  const helpers: any = {
    idPrefix: "test-",
    renderCardEntityField: () => fields.push("entity"),
    renderCardTextField: (_panel: any, _card: any, _helpers: any, metadata: any) => {
      fields.push(metadata.text.label);
      check(metadata.text.bindName === "label", "Name must reuse saved labels");
      return { field: { setAttribute: (key: string, value: string) => {
        primaryName = key === "data-sp-card-primary" && value === "name";
      } } };
    },
    toggleRow: () => ({ row: node(), input: { addEventListener() {} } }),
    renderCardIconPicker: node,
    disclosureSection: () => ({ section: node(), panel: node() }),
    selectField: () => ({ field: node(), select: { addEventListener() {} } }),
    escHtml: (value: string) => value,
  };
  definition.renderSettings(node(), camera, 1, helpers);
  check(fields.join(",") === "entity,Name", "Name must follow Entity without a separate Label field");
  check(primaryName, "Name must stay outside Card Settings");
  check(camera.label === "Front Door", "Opening settings must retain a hidden label's name");
  check(definition.renderPreview(camera, helpers).labelHtml === "", "Hidden labels must stay hidden");
  options.setImageLabelEnabled(camera, true);
  check(definition.renderPreview(camera, helpers).labelHtml.includes("Front Door"), "Enabled labels must use Name");
}
