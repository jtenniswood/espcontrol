import { state } from "../state/app_instance";
import { CLOCK_BAR_CATALOG, CLOCK_BAR_MAX_ITEMS, createClockBarElement, isClockBarSensor, moveClockBarElement,
  type ClockBarElement, type ClockBarGroup, type ClockBarType } from "../model/clock_bar";
import type { ClockBarFeature } from "./clock_bar_state";
import type { UiRuntimeState } from "./state";
import { createButtonSettingsIconPickerFeature } from "./button_settings_icon_picker";
import { iconOptions, mdiIcon } from "./ui_primitives";

export function createClockBarEditor(runtime: UiRuntimeState, clockBar: ClockBarFeature, dependencies: {
  entityInput(id: string, value: string, placeholder: string, domains: string[]): HTMLInputElement;
  locked(): boolean;
  close(): void;
}) {
  const els = runtime.els;
  function open(id?: string) {
    if (dependencies.locked() || !state.clockBarLayoutSupported) return;
    const layout = clockBar.layout();
    const selected = layout.items.find(item => item.id === id);
    const draft = selected ? { ...selected } : createClockBarElement("temperature", "left", layout.items);
    const container = els.buttonSettings as HTMLElement;
    container.replaceChildren();
    els.settingsOverlay?.classList.add("sp-visible");
    const title = document.createElement("div");
    title.className = "sp-section-title";
    title.textContent = selected ? "Edit clock bar element" : "Clock Bar";
    container.appendChild(title);
    const panel = document.createElement("div"); panel.className = "sp-panel"; container.appendChild(panel);
    const error = document.createElement("p"); error.className = "sp-banner sp-error"; error.hidden = true;
    const showError = (message: string) => { error.textContent = message; error.hidden = false; };
    function field(label: string, input: HTMLElement) {
      const wrapper = document.createElement("div"); wrapper.className = "sp-field";
      const labelEl = document.createElement("label"); labelEl.textContent = label; labelEl.htmlFor = input.id;
      wrapper.append(labelEl, input); panel.appendChild(wrapper); return wrapper;
    }
    function select(id: string, entries: [string, string][], current: string) {
      const input = document.createElement("select"); input.id = id; input.className = "sp-select";
      for (const [value, text] of entries) { const option = document.createElement("option"); option.value = value; option.textContent = text; input.appendChild(option); }
      input.value = current; return input;
    }
    function text(id: string, current: string, max: number) {
      const input = document.createElement("input"); input.id = id; input.className = "sp-input"; input.value = current; input.maxLength = max; return input;
    }
    const type = select("sp-clockbar-add-type", clockBar.availableTypes().map(key => [key, CLOCK_BAR_CATALOG[key].label]), draft.type);
    field("Element", type);
    const group = select("sp-clockbar-group", [["left", "Left"], ["middle", "Middle"], ["right", "Right"]], draft.group);
    field("Position", group);
    const source = select("sp-clockbar-source", [["ha", "Home Assistant"], ["local", "Local sensor"]], draft.source);
    const sourceField = field("Source", source);
    let entity: HTMLInputElement = dependencies.entityInput("sp-clockbar-entity", draft.entity, "sensor.room_temperature", ["sensor"]);
    entity.maxLength = 120;
    const entityField = field("Sensor", entity);
    const localList = document.createElement("datalist"); localList.id = "sp-clockbar-local-sensors"; panel.appendChild(localList);
    let localSensors: { key: string; name: string; unit?: string; type: string }[] = [];
    const label = text("sp-clockbar-label", draft.label, 32); const labelField = field("Short label (optional)", label);
    let selectedIcon = draft.icon || "";
    const iconPicker = document.createElement("div"); iconPicker.className = "sp-icon-picker";
    iconPicker.appendChild(mdiIcon(selectedIcon || "None", "sp-icon-picker-preview mdi"));
    const iconInput = text("sp-clockbar-icon", selectedIcon || "None", 48);
    iconInput.className = "sp-icon-picker-input"; iconInput.placeholder = "Search icons…"; iconInput.autocomplete = "off";
    const iconDropdown = document.createElement("div"); iconDropdown.className = "sp-icon-dropdown";
    iconPicker.append(iconInput, iconDropdown);
    const iconField = field("Icon (optional)", iconPicker);
    iconField.querySelector("label")!.htmlFor = iconInput.id;
    createButtonSettingsIconPickerFeature(document, () => {}).init(iconPicker, selectedIcon || "None", icon => {
      selectedIcon = icon === "None" ? "" : icon;
    }, ["None", ...iconOptions.filter(icon => icon !== "Auto")]);
    const unit = text("sp-clockbar-unit", draft.unit, 12); const unitField = field("Unit", unit);
    const precision = select("sp-clockbar-precision", [["0", "0"], ["1", "1"], ["2", "2"], ["3", "3"]], String(draft.precision));
    const precisionField = field("Decimal places", precision);
    function updateFields(changeSource = false) {
      const sensor = isClockBarSensor(type.value as ClockBarType);
      for (const wrapper of [sourceField, entityField, labelField, iconField, unitField, precisionField]) wrapper.hidden = !sensor;
      if (changeSource) {
        const next = source.value === "local" ? text("sp-clockbar-entity", "", 120) : dependencies.entityInput("sp-clockbar-entity", "", "sensor.room_temperature", ["sensor"]);
        entity.replaceWith(next); entity = next;
      }
      if (source.value === "local") { entity.setAttribute("list", localList.id); entity.placeholder = "Choose a local sensor or enter its object_id"; }
    }
    source.addEventListener("change", () => updateFields(true));
    type.addEventListener("change", () => {
      const definition = CLOCK_BAR_CATALOG[type.value as ClockBarType]; unit.value = definition.unit; precision.value = String(definition.precision); updateFields();
    });
    entityField.addEventListener("change", () => {
      if (source.value !== "local") return;
      const sensor = localSensors.find(sensor => sensor.key === entity.value);
      if (sensor?.unit) unit.value = sensor.unit;
    });
    void fetch("/local_sensors", { credentials: "include" }).then(response => response.ok ? response.json() : null).then(data => {
      const sensors = Array.isArray(data) ? data : data?.sensors;
      if (!Array.isArray(sensors)) return;
      localSensors = sensors.filter(sensor => sensor.type === "numeric");
      for (const sensor of localSensors) { const option = document.createElement("option"); option.value = sensor.key; option.label = sensor.name; localList.appendChild(option); }
    }).catch(() => {});
    updateFields();
    const note = document.createElement("p"); note.className = "sp-field-hint";
    note.textContent = "Drag elements in the preview to move or reorder them. The preview uses example sensor values. Elements that do not fit remain saved and can be edited below.";
    panel.append(note, error);
    const actions = document.createElement("div"); actions.className = "sp-btn-row sp-btn-row--save sp-has-secondary"; panel.appendChild(actions);
    function button(text: string, handler: () => void | Promise<void>, parent = actions) {
      const button = document.createElement("button"); button.type = "button"; button.className = "sp-action-btn"; button.textContent = text;
      button.addEventListener("click", () => { if (!dependencies.locked() && !state.clockBarLayoutSaving) void handler(); }); parent.appendChild(button); return button;
    }
    const save = button(selected ? "Save" : "Add element", async () => {
      if (!selected && layout.items.length >= CLOCK_BAR_MAX_ITEMS) { showError("A clock bar supports up to 12 elements."); return; }
      if (label.value.length > 32 || unit.value.length > 12) { showError("Shorten the label or unit before saving."); return; }
      const next: ClockBarElement = { ...draft, type: type.value as ClockBarType, group: group.value as ClockBarGroup,
        source: source.value as "ha" | "local", entity: entity.value.trim(), label: label.value.trim(), unit: unit.value, precision: Number(precision.value), icon: selectedIcon };
      if (isClockBarSensor(next.type) && (!next.entity || (next.source === "ha" && !/^sensor\.[a-z0-9_]+$/.test(next.entity)))) {
        showError(next.source === "ha" ? "Choose a sensor entity before saving." : "Choose a local sensor or enter its object_id."); return;
      }
      if (!isClockBarSensor(next.type)) { next.entity = ""; next.label = ""; next.unit = ""; next.source = "ha"; next.precision = 0; delete next.icon; }
      save.disabled = true;
      try {
        const current = clockBar.layout();
        const items = selected ? current.items.map(item => item.id === selected.id ? next : item) : [...current.items, next];
        if (await clockBar.saveLayout({ version: 1, items })) dependencies.close();
        else showError("The clock bar could not be saved. Please try again.");
      } catch (error) { showError(error instanceof Error ? error.message : "Could not save clock bar."); }
      finally { save.disabled = false; }
    }); save.classList.add("sp-save-btn");
    if (selected) {
      button("Duplicate", async () => { await duplicate(selected.id); }).disabled = layout.items.length >= CLOCK_BAR_MAX_ITEMS;
      button("Delete", async () => { if (await remove(selected.id)) dependencies.close(); });
      const own = layout.items.filter(item => item.group === selected.group);
      const index = own.findIndex(item => item.id === selected.id);
      button("Move earlier", async () => {
        if (index > 0 && await clockBar.saveLayout(moveClockBarElement(clockBar.layout(), selected.id, selected.group, own[index - 1]!.id))) open(selected.id);
      }).disabled = index === 0;
      button("Move later", async () => {
        if (index < own.length - 1 && await clockBar.saveLayout(moveClockBarElement(clockBar.layout(), selected.id, selected.group, own[index + 2]?.id))) open(selected.id);
      }).disabled = index === own.length - 1;
    }
    const heading = document.createElement("div"); heading.className = "sp-section-title"; heading.textContent = "All clock bar elements"; panel.appendChild(heading);
    for (const groupName of ["left", "middle", "right"] as const) {
      for (const item of layout.items.filter(item => item.group === groupName)) {
        const overflow = els.clockBarItems?.[item.id]?.dataset.overflow === "true";
        button(`${item.label || CLOCK_BAR_CATALOG[item.type].label} · ${groupName}${overflow ? " · hidden to fit" : ""}`, () => open(item.id), panel);
      }
    }
  }
  async function remove(id: string): Promise<boolean> {
    return clockBar.saveLayout({ version: 1, items: clockBar.layout().items.filter(item => item.id !== id) });
  }
  async function duplicate(id: string) {
    const layout = clockBar.layout(); const item = layout.items.find(item => item.id === id);
    if (!item || layout.items.length >= CLOCK_BAR_MAX_ITEMS) return;
    const copy = { ...item, id: createClockBarElement(item.type, item.group, layout.items).id };
    const items = [...layout.items]; items.splice(items.indexOf(item) + 1, 0, copy);
    if (await clockBar.saveLayout({ version: 1, items })) open(copy.id);
  }
  return { open, remove, duplicate };
}
