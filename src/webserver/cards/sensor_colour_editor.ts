import type { ConfigSensorOptionsFeature } from "../application/config_sensor_options";
import { state } from "../state/app_instance";
import { PREVIEW_THEME_COLORS, previewEffectiveTheme } from "../state/preview_theme";
import { createColorSwatches } from "../components/color_swatches";
import { cardPreviewTextColor } from "../features/preview";
import { sensorColourCasefold } from "../generated/sensor_colour_casefold";
import { cardContractDomains } from "../generated/card_contract";
import { sensorColourForState, SENSOR_COLOUR_RULES_MAX, validateSensorColourRules, type SensorColourCondition, type SensorColourRules } from "../features/sensor_colour_rules";

type Comparison = "below" | "above" | "exact" | "between" | "text";
const comparisons: [Comparison, string][] = [
    ["below", "Below"], ["above", "Above"], ["exact", "Exactly"], ["between", "Between"], ["text", "Text is"],
];

function element<K extends keyof HTMLElementTagNameMap>(tag: K, className = "", text = ""): HTMLElementTagNameMap[K] {
    const node = document.createElement(tag);
    node.className = className;
    if (text) node.textContent = text;
    return node;
}

function comparisonFor(condition: SensorColourCondition): Comparison {
    if (condition.kind === "text") return "text";
    if (!condition.lower.trim()) return "below";
    if (!condition.upper.trim()) return "above";
    if (Number(condition.lower) === Number(condition.upper) && condition.lowerInclusive && condition.upperInclusive) return "exact";
    return "between";
}

function conditionFor(comparison: Comparison, value: string, colour: string): SensorColourCondition {
    if (comparison === "text") return { kind: "text", state: value, colour };
    return {
        kind: "number", colour,
        lower: ["above", "exact", "between"].includes(comparison) ? value : "",
        lowerInclusive: comparison !== "above",
        upper: ["below", "exact"].includes(comparison) ? value : "",
        upperInclusive: comparison !== "below",
    };
}

function describe(condition: SensorColourCondition, comparison = comparisonFor(condition)): string {
    if (condition.kind === "text") return condition.state.trim() ? `Text is ${condition.state.trim()}` : "Enter a text state";
    if (comparison === "between") {
        return `Value between ${condition.lower.trim() || "…"} and ${condition.upper.trim() || "…"}`;
    }
    const value = condition.lower.trim() || condition.upper.trim();
    return value ? `Value ${comparisons.find(([key]) => key === comparison)![1].toLowerCase()} ${value}` : "Enter a value";
}

/** Edits the existing card draft; the sample state stays local to this view. */
export function renderSensorColourEditor(panel: HTMLElement, button: any, helpers: any, options: ConfigSensorOptionsFeature, supported: boolean, renderCardPreview: (sample: string | null) => string, swatches: HTMLElement): { sync(available?: boolean): void; reset(): void } {
    const root = element("div", "sp-sensor-colours");
    panel.appendChild(root);
    let rules: SensorColourRules = options.sensorColourRules(button) || { source: "", conditions: [] };
    // Keep older saved rules consistent with the available comparisons:
    // Above/Below exclude the threshold; Between/Exactly include it.
    function normalizeBoundaries(): void {
        if (!supported) return;
        let normalized = false;
        for (const condition of rules.conditions) {
            if (condition.kind !== "number") continue;
            const hasLower = condition.lower.trim() !== "";
            const hasUpper = condition.upper.trim() !== "";
            const lowerInclusive = !hasLower || hasUpper;
            const upperInclusive = hasLower || !hasUpper;
            if (condition.lowerInclusive !== lowerInclusive || condition.upperInclusive !== upperInclusive) {
                condition.lowerInclusive = lowerInclusive;
                condition.upperInclusive = upperInclusive;
                normalized = true;
            }
        }
        if (normalized) {
            options.setSensorColourRules(button, rules);
            helpers.saveField("options", button.options);
        }
    }
    normalizeBoundaries();
    let otherSource = !!rules.source;
    const mode = select([["default", "Single colour"], ["active", "Lit When Active"], ["custom", "Custom conditions"]]);
    mode.id = helpers.idPrefix + "sensor-colour-mode";
    mode.value = options.sensorColourRules(button) ? "custom" : options.sensorActiveColorEnabled(button) ? "active" : "default";
    (mode.querySelector('[value="custom"]') as HTMLOptionElement).disabled = !supported;
    root.appendChild(field("Colour mode", mode));
    const capabilityNote = element("p", "sp-sensor-colour-note", "Update the panel firmware to use custom colour conditions.");
    capabilityNote.hidden = supported;
    root.appendChild(capabilityNote);

    const paletteField = element("div", "sp-field");
    paletteField.append(element("span", "sp-field-label", "Use this colour"), swatches);
    root.appendChild(paletteField);

    const editor = element("div", "sp-sensor-colour-editor");
    root.appendChild(editor);
    const error = element("p", "sp-sensor-colour-error");
    error.setAttribute("role", "status");
    const warning = element("p", "sp-sensor-colour-note");
    warning.setAttribute("role", "status");
    let testState = "";
    let updatePreview = () => {};
    helpers.requireField(mode, "Complete or remove each colour condition before saving.",
        () => mode.value === "custom", () => !validationError());

    function select(entries: [string, string][]): HTMLSelectElement {
        const node = element("select", "sp-select");
        entries.forEach(([value, label]) => { const option = element("option", "", label); option.value = value; node.appendChild(option); });
        return node;
    }
    function field(label: string, control: HTMLElement): HTMLDivElement {
        const wrapper = element("div", "sp-field");
        if (!control.id) control.id = helpers.idPrefix + "sensor-colour-field-" + root.querySelectorAll("input,select").length;
        wrapper.append(helpers.fieldLabel(label, control.id), control);
        return wrapper;
    }
    function action(label: string, callback: () => void): HTMLButtonElement {
        const node = element("button", "sp-fw-btn", label);
        node.type = "button";
        node.addEventListener("click", callback);
        return node;
    }
    function validationError(): string | null {
        if (!supported) return "Update the panel firmware to use custom colour conditions.";
        if (otherSource && !rules.source.trim()) return "Choose the sensor that should control the colour.";
        return validateSensorColourRules(rules);
    }
    function changed(): void {
        error.textContent = validationError() || "";
        if (!error.textContent) {
            options.setSensorColourRules(button, rules);
            helpers.saveField("options", button.options);
            helpers.clearFieldError(mode);
        }
        updateWarning();
        updatePreview();
    }
    function updateWarning(): void {
        warning.textContent = "";
        if (validationError()) return;
        for (let left = 0; left < rules.conditions.length; left++) {
            for (let right = left + 1; right < rules.conditions.length; right++) {
                const a = rules.conditions[left]!, b = rules.conditions[right]!;
                let overlap = false;
                if (a.kind === "text" && b.kind === "text") overlap = sensorColourCasefold(a.state.trim()) === sensorColourCasefold(b.state.trim());
                if (a.kind === "number" && b.kind === "number") {
                    const before = (x: typeof a, y: typeof a) => x.upper.trim() && y.lower.trim() &&
                        (Number(x.upper) < Number(y.lower) || (Number(x.upper) === Number(y.lower) && (!x.upperInclusive || !y.lowerInclusive)));
                    overlap = !before(a, b) && !before(b, a);
                }
                if (overlap) { warning.textContent = "Some conditions overlap. The first matching condition sets the colour."; return; }
            }
        }
    }
    function render(): void {
        editor.replaceChildren();
        editor.hidden = mode.value !== "custom";
        paletteField.hidden = !editor.hidden;
        if (editor.hidden) return;
        if (!rules.conditions.length) rules.conditions.push(conditionFor("below", "", "FF8C00"));

        const sourceFields = element("div", "sp-sensor-colour-source-fields");
        const source = select([["", "Main Entity"], ["other", "Another sensor"]]);
        source.id = helpers.idPrefix + "sensor-colour-source";
        source.value = otherSource ? "other" : "";
        const entityControl = helpers.entityField("Sensor controlling the colour", helpers.idPrefix + "sensor-colour-entity", rules.source, "e.g. sensor.battery_power", cardContractDomains("sensor"), null, false);
        entityControl.field.hidden = !otherSource;
        source.addEventListener("change", () => {
            otherSource = source.value === "other";
            entityControl.field.hidden = !otherSource;
            rules.source = otherSource ? entityControl.input.value.trim() : "";
            changed();
        });
        entityControl.input.addEventListener("change", () => { rules.source = entityControl.input.value.trim(); changed(); });
        sourceFields.append(field("Use value from", source), entityControl.field);
        editor.appendChild(sourceFields);

        const list = element("div", "sp-sensor-colour-list");
        editor.appendChild(list);
        function renderRows(): void {
            list.replaceChildren();
            rules.conditions.forEach((initial, index) => {
                let condition = initial;
                const row = element("section", "sp-sensor-colour-rule");
                const header = element("div", "sp-sensor-colour-rule-header");
                const summary = element("h4");
                const actions = element("div", "sp-sensor-colour-actions");
                const move = (direction: number) => { const item = rules.conditions.splice(index, 1)[0]!; rules.conditions.splice(index + direction, 0, item); renderRows(); changed(); };
                const up = action("↑", () => move(-1)); up.setAttribute("aria-label", `Move condition ${index + 1} up`); up.disabled = index === 0;
                const down = action("↓", () => move(1)); down.setAttribute("aria-label", `Move condition ${index + 1} down`); down.disabled = index === rules.conditions.length - 1;
                const remove = action("", () => { rules.conditions.splice(index, 1); renderRows(); changed(); }); remove.setAttribute("aria-label", `Delete condition ${index + 1}`);
                const trash = element("span", "mdi mdi-trash-can-outline"); trash.setAttribute("aria-hidden", "true"); remove.appendChild(trash);
                actions.append(up, down, remove);
                header.append(summary, actions);
                row.appendChild(header);
                function updateTitle(): void {
                    summary.textContent = describe(condition, comparison.value as Comparison);
                    row.setAttribute("aria-label", summary.textContent);
                    remove.title = "Delete " + summary.textContent;
                }

                const comparison = select(comparisons);
                comparison.id = helpers.idPrefix + `sensor-colour-comparison-${index}`;
                comparison.value = comparisonFor(condition);
                const when = element("div", "sp-sensor-colour-when");
                const values = element("div", "sp-sensor-colour-values");
                when.append(field("When the value is", comparison), values);
                row.appendChild(when);
                function valueField(label: string, value: string, onInput: (value: string) => void): void {
                    const input = element("input", "sp-input");
                    input.type = "text"; input.value = value; input.placeholder = condition.kind === "text" ? "e.g. running" : "e.g. 8";
                    input.id = helpers.idPrefix + `sensor-colour-${index}-${label.toLowerCase()}`;
                    if (condition.kind === "number") input.inputMode = "decimal";
                    input.addEventListener("input", () => { onInput(input.value); updateTitle(); changed(); });
                    values.appendChild(field(label, input));
                }
                function renderValues(): void {
                    values.replaceChildren();
                    values.classList.toggle("sp-sensor-colour-between", comparison.value === "between");
                    if (condition.kind === "text") valueField("State", condition.state, value => { if (condition.kind === "text") condition.state = value; });
                    else if (comparison.value === "between") {
                        const range = condition;
                        valueField("From", range.lower, value => { range.lower = value; });
                        valueField("To", range.upper, value => { range.upper = value; });
                    } else {
                        valueField("Value", condition.lower || condition.upper, value => {
                            condition = conditionFor(comparison.value as Comparison, value, condition.colour);
                            rules.conditions[index] = condition;
                        });
                    }
                    updateTitle();
                }
                comparison.addEventListener("change", () => {
                    const value = condition.kind === "text" ? condition.state : condition.lower || condition.upper;
                    condition = conditionFor(comparison.value as Comparison, value, condition.colour);
                    rules.conditions[index] = condition;
                    renderValues(); changed();
                });
                renderValues();

                const colourControls = createColorSwatches(condition.colour, colour => {
                    condition.colour = colour;
                    changed();
                }, `Colour for condition ${index + 1}`);
                const colourField = element("div", "sp-field");
                const colourLabel = element("span", "sp-field-label", "Use this colour");
                colourLabel.id = helpers.idPrefix + `sensor-colour-label-${index}`;
                colourControls.setAttribute("aria-describedby", colourLabel.id);
                colourField.append(colourLabel, colourControls);
                row.appendChild(colourField);
                list.appendChild(row);
            });
            const add = action("+ Add condition", () => { rules.conditions.push(conditionFor("below", "", "FF8C00")); renderRows(); changed(); });
            add.disabled = rules.conditions.length >= SENSOR_COLOUR_RULES_MAX;
            list.appendChild(add);
            if (add.disabled) list.appendChild(element("p", "sp-sensor-colour-note", "Maximum of six conditions per card."));
        }
        renderRows();
        const defaultRow = element("section", "sp-sensor-colour-default");
        const defaultMode = select([["", "Theme default"], ["custom", "Custom colour"]]);
        defaultMode.id = helpers.idPrefix + "sensor-colour-default-mode";
        defaultMode.value = rules.defaultColour ? "custom" : "";
        const defaultSwatches = createColorSwatches(rules.defaultColour || "", colour => {
            rules.defaultColour = colour;
            changed();
        }, "Default colour presets");
        defaultSwatches.hidden = !rules.defaultColour;
        defaultMode.addEventListener("change", () => {
            if (defaultMode.value === "custom") {
                rules.defaultColour = rules.defaultColour || "FFFFFF";
            } else delete rules.defaultColour;
            defaultSwatches.hidden = defaultMode.value !== "custom";
            defaultSwatches._syncColor(rules.defaultColour || "");
            changed();
        });
        defaultRow.append(field("Default colour", defaultMode), defaultSwatches);
        editor.appendChild(defaultRow);
        editor.append(warning, error);

        const testPanel = helpers.disclosureSection("Test your conditions", helpers.idPrefix + "sensor-colour-test-panel", false);
        testPanel.panel.classList.add("sp-sensor-colour-test-panel");
        const test = element("div", "sp-sensor-colour-test");
        testPanel.section.appendChild(test);
        test.appendChild(element("p", "sp-sensor-colour-note", "Enter a sample value to test your conditions."));
        const testInput = element("input", "sp-input"); testInput.id = helpers.idPrefix + "sensor-colour-test-value"; testInput.value = testState;
        test.appendChild(field("Test state", testInput));
        const preview = element("div", "sp-sensor-colour-preview");
        const previewCard = element("div", "sp-btn");
        preview.appendChild(previewCard);
        const result = element("p", "sp-sensor-colour-result"); result.setAttribute("role", "status"); result.setAttribute("aria-live", "polite");
        test.append(preview, result);
        editor.appendChild(testPanel.panel);
        updatePreview = () => {
            const theme = PREVIEW_THEME_COLORS[previewEffectiveTheme(state)];
            preview.style.backgroundColor = "#" + theme.surfaceSensor;
            preview.style.color = "#" + theme.textPrimary;
            const fallback = rules.defaultColour;
            if (fallback) {
                preview.style.backgroundColor = "#" + fallback;
                preview.style.color = cardPreviewTextColor(fallback);
            }
            previewCard.innerHTML = renderCardPreview(otherSource ? null : testState);
            if (!testState.trim()) { result.textContent = ""; return; }
            if (validationError()) { result.textContent = "Complete your conditions to test them."; return; }
            const match = rules.conditions.findIndex(condition => sensorColourForState({ source: rules.source, conditions: [condition] }, testState) !== null);
            if (match < 0) { result.textContent = fallback ? "No condition matches. The default colour will be used." : "No condition matches. The theme default will be used."; return; }
            const colour = rules.conditions[match]!.colour;
            preview.style.backgroundColor = "#" + colour;
            preview.style.color = cardPreviewTextColor(colour);
            result.textContent = `Condition ${match + 1} matches: ${describe(rules.conditions[match]!)}. Using #${colour}.`;
        };
        testInput.addEventListener("input", () => { testState = testInput.value; updatePreview(); });
        error.textContent = validationError() || "";
        updateWarning();
        updatePreview();
    }
    mode.addEventListener("change", () => {
        if (mode.value === "custom") options.setSensorActiveColorEnabled(button, false);
        else {
            options.setSensorColourRules(button, null);
            options.setSensorActiveColorEnabled(button, mode.value === "active");
        }
        helpers.saveField("options", button.options);
        render();
        if (mode.value === "custom") changed();
    });
    render();
    const sync = (available?: boolean) => {
        if (available !== undefined && available !== supported) {
            supported = available;
            (mode.querySelector('[value="custom"]') as HTMLOptionElement).disabled = !supported;
            capabilityNote.hidden = supported;
            if (mode.value === "custom") { normalizeBoundaries(); changed(); }
        }
        (mode.querySelector('[value="active"]') as HTMLOptionElement).disabled = button.precision === "time";
        if (mode.value !== "custom") mode.value = options.sensorActiveColorEnabled(button) ? "active" : "default";
        updatePreview();
    };
    return { sync, reset: () => {
        rules = { source: "", conditions: [] };
        otherSource = false;
        testState = "";
        options.setSensorColourRules(button, null);
        options.setSensorActiveColorEnabled(button, false);
        mode.value = "default";
        helpers.saveField("options", button.options);
        helpers.clearFieldError(mode);
        render();
    } };
}
