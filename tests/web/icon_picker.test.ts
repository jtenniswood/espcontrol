import { createButtonSettingsIconPickerFeature } from "../../src/webserver/application/button_settings_icon_picker";
import { iconOptions } from "../../src/webserver/application/ui_primitives";

// Just enough of the DOM for the icon picker: class lists, display style,
// child order, events and a class-name query.
class FakeElement {
  children: FakeElement[] = [];
  parent: FakeElement | null = null;
  style: { display: string } = { display: "" };
  textContent = "";
  classes = new Set<string>();
  listeners: Record<string, ((event: any) => void)[]> = {};
  value = "";
  [key: string]: any;

  get className(): string { return [...this.classes].join(" "); }
  set className(value: string) { this.classes = new Set(value.split(/\s+/).filter(Boolean)); }
  get classList() {
    const classes = this.classes;
    return {
      add: (name: string) => { classes.add(name); },
      remove: (name: string) => { classes.delete(name); },
      contains: (name: string) => classes.has(name),
      toggle: (name: string, force?: boolean) => {
        const on = force === undefined ? !classes.has(name) : force;
        if (on) classes.add(name); else classes.delete(name);
        return on;
      },
    };
  }
  appendChild(child: FakeElement): FakeElement {
    if (child.isFragment) {
      for (const inner of [...child.children]) this.appendChild(inner);
      child.children = [];
      return child;
    }
    child.remove();
    child.parent = this;
    this.children.push(child);
    return child;
  }
  insertBefore(child: FakeElement, reference: FakeElement | null): FakeElement {
    child.remove();
    const index = reference ? this.children.indexOf(reference) : -1;
    child.parent = this;
    if (index < 0) this.children.push(child); else this.children.splice(index, 0, child);
    return child;
  }
  remove(): void {
    if (!this.parent) return;
    this.parent.children.splice(this.parent.children.indexOf(this), 1);
    this.parent = null;
  }
  replaceChildren(): void {
    for (const child of [...this.children]) child.remove();
  }
  addEventListener(type: string, listener: (event: any) => void): void {
    (this.listeners[type] = this.listeners[type] || []).push(listener);
  }
  dispatch(type: string, extra: Record<string, unknown> = {}): void {
    const event = { preventDefault() {}, ...extra };
    for (const listener of this.listeners[type] || []) listener.call(this, event);
  }
  querySelector(selector: string): FakeElement | null {
    const name = selector.replace(/^\./, "");
    for (const child of this.children) {
      if (child.classes.has(name)) return child;
      const found = child.querySelector(selector);
      if (found) return found;
    }
    return null;
  }
  scrollIntoView(): void {}
  get labelText(): string {
    return this.children.map((child) => child.textContent).join("");
  }
}

function fakeDocument() {
  return {
    head: new FakeElement(),
    createElement: () => new FakeElement(),
    createTextNode: (text: string) => Object.assign(new FakeElement(), { textContent: text }),
    createDocumentFragment: () => Object.assign(new FakeElement(), { isFragment: true }),
  };
}

function check(value: unknown, message: string): void {
  if (!value) throw new Error(message);
}

function buildPicker(currentIcon: string) {
  const doc = fakeDocument();
  (globalThis as any).document = doc;
  const picker = new FakeElement();
  const input = picker.appendChild(Object.assign(new FakeElement(), { className: "sp-icon-picker-input" }));
  const dropdown = picker.appendChild(Object.assign(new FakeElement(), { className: "sp-icon-dropdown" }));
  picker.appendChild(Object.assign(new FakeElement(), { className: "sp-icon-picker-preview" }));
  const selected: string[] = [];
  createButtonSettingsIconPickerFeature(doc as any, () => {}).init(picker, currentIcon,
    (icon: string) => { selected.push(icon); });
  const rows = (): any[] => dropdown.children.filter((row) => row.style.display !== "none" &&
    !row.classes.has("sp-icon-option--empty"));
  const type = (text: string) => { input.value = text; input.dispatch("input"); };
  const key = (name: string) => input.dispatch("keydown", { key: name });
  return { input, rows, type, key, selected };
}

export function runIconPickerTests(): void {
  const previousDocument = (globalThis as any).document;
  try {
    // A card using a custom icon shows it at the top of the list, marked as
    // the current icon, when the picker opens.
    {
      const picker = buildPicker("mdi:F0EFA");
      picker.input.dispatch("focus");
      const first = picker.rows()[0];
      check(first && first.labelText === "Custom icon mdi:F0EFA",
        "The current custom icon must be the first row when the picker opens");
      check(first.classes.has("sp-active"), "The current custom icon must be marked as selected");
      check(picker.rows().filter((row) => row.classes.has("sp-active")).length === 1,
        "Only the current icon may be marked as selected");
      picker.key("ArrowDown");
      check(picker.rows()[0].classes.has("sp-highlighted"),
        "Keyboard navigation must start from the current custom icon");
      picker.key("Enter");
      check(picker.selected[0] === "mdi:F0EFA", "Enter must keep the current custom icon");
    }

    // A built-in icon card never shows the custom row on open.
    {
      const picker = buildPicker("Lightbulb");
      picker.input.dispatch("focus");
      check(!picker.rows().some((row) => row.classes.has("sp-icon-option--custom")),
        "Built-in icon cards must not show a custom row when the picker opens");
      check(picker.rows().find((row) => row._optName === "Lightbulb")?.classes.has("sp-active"),
        "The current built-in icon must stay marked as selected");
    }

    // Typing a codepoint on its own offers it as a custom icon.
    {
      const picker = buildPicker("Lightbulb");
      picker.input.dispatch("focus");
      picker.type("F0F92");
      const visible = picker.rows();
      check(visible.length === 1 && visible[0].labelText === "Custom icon mdi:F0F92",
        "A typed codepoint must be offered as a custom icon");
      picker.key("Enter");
      check(picker.selected[0] === "mdi:F0F92", "Enter must pick the typed custom icon");
    }

    // When a typed value matches a built-in name and is also a valid
    // codepoint, the built-in icon comes first and Enter picks it.
    {
      const names = iconOptions as string[];
      names.push("Test Icon F0E0A");
      try {
        const picker = buildPicker("Lightbulb");
        picker.input.dispatch("focus");
        picker.type("f0e0a");
        const visible = picker.rows();
        check(visible.length === 2, "Both the built-in and custom rows must be offered");
        check(visible[0]._optName === "Test Icon F0E0A", "The built-in name match must come first");
        check(visible[1].labelText === "Custom icon mdi:F0E0A", "The custom row must come after name matches");
        picker.key("Enter");
        check(picker.selected[0] === "Test Icon F0E0A", "Enter must prefer the matching built-in icon");
      } finally {
        names.pop();
      }
    }
  } finally {
    (globalThis as any).document = previousDocument;
  }
}
