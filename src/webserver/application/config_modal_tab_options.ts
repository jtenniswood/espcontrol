import { configOptionValue, setConfigOptionValue } from "../model/config_primitives";
import {
    CLIMATE_CONTROL_TABS_OPTION,
    COVER_CONTROL_TABS_OPTION,
    FAN_CONTROL_TABS_OPTION,
    FAN_LIGHT_ENTITY_OPTION,
    LIGHT_CONTROL_TABS_OPTION,
    cardContractOptionDefaultValue,
    cardContractOptionSpec,
} from "./config_option_core";
import { normalizeCoverMode } from "./config_cover_contract";
export interface ConfigModalTabOptionsDependencies {
    readonly document: Document;
    readonly renderButtonSettings: () => void;
}

export function createConfigModalTabOptionsFeature(
    dependencies: ConfigModalTabOptionsDependencies,
) {
    let normalizeClimateOptionsForTabs: (options: any, includeControlTabs: boolean) => any =
        (options) => options || "";
    function connectClimateOptionsNormalizer(
        normalizer: (options: any, includeControlTabs: boolean) => any,
    ) {
        normalizeClimateOptionsForTabs = normalizer;
    }
    // ── Modal Tab Options ──────────────────────────────────────────────
    function lightControlTabDefinitions(this: any) {
        var labels: any = {
            power: "Power",
            brightness: "Brightness",
            temperature: "Colour Temperature",
            color: "Colour Presets",
        };
        var spec: any = cardContractOptionSpec("light_control", LIGHT_CONTROL_TABS_OPTION);
        var values: any = spec && spec.values ? spec.values : [];
        return values.map(function (this: any, value?: any) {
            return { value: value, label: labels[value] || value };
        });
    }
    function lightControlDefaultTabs(this: any) {
        return cardContractOptionDefaultValue("light_control", LIGHT_CONTROL_TABS_OPTION, "power|brightness|temperature|color").split("|");
    }
    function normalizeLightControlTabs(this: any, value?: any) {
        var raw: any = String(value || "").trim();
        var parts: any = raw ? raw.split("|") : lightControlDefaultTabs();
        var definitions: any = lightControlTabDefinitions();
        var valid: any = {};
        definitions.forEach(function (this: any, tab?: any) { valid[tab.value] = true; });
        var out: any = [];
        parts.forEach(function (this: any, part?: any) {
            part = String(part || "").trim();
            if (valid[part] && out.indexOf(part) < 0)
                out.push(part);
        });
        return out.length ? out : ["power"];
    }
    function lightControlTabs(this: any, b?: any) {
        return normalizeLightControlTabs(configOptionValue(b && b.options, LIGHT_CONTROL_TABS_OPTION));
    }
    function lightControlTabsAreDefault(this: any, tabs?: any) {
        tabs = normalizeLightControlTabs((tabs || []).join("|"));
        var defaults: any = lightControlDefaultTabs();
        if (tabs.length !== defaults.length)
            return false;
        for (var i: any = 0; i < defaults.length; i++) {
            if (tabs[i] !== defaults[i])
                return false;
        }
        return true;
    }
    function normalizeLightControlOptions(this: any, options?: any) {
        var tabs: any = normalizeLightControlTabs(configOptionValue(options, LIGHT_CONTROL_TABS_OPTION));
        return lightControlTabsAreDefault(tabs)
            ? ""
            : setConfigOptionValue("", LIGHT_CONTROL_TABS_OPTION, tabs.join("|"));
    }
    function setLightControlTabs(this: any, b?: any, tabs?: any) {
        if (!b)
            return "";
        tabs = normalizeLightControlTabs((tabs || []).join("|"));
        b.options = lightControlTabsAreDefault(tabs)
            ? setConfigOptionValue(b.options, LIGHT_CONTROL_TABS_OPTION, "")
            : setConfigOptionValue(b.options, LIGHT_CONTROL_TABS_OPTION, tabs.join("|"));
        b.options = normalizeLightControlOptions(b.options);
        return b.options;
    }
    function coverControlTabDefinitions(this: any) {
        var labels: any = {
            position: "Position",
            controls: "Controls",
            tilt: "Tilt",
            presets: "Presets",
        };
        var spec: any = cardContractOptionSpec("cover", COVER_CONTROL_TABS_OPTION);
        var values: any = spec && spec.values ? spec.values : [];
        return values.map(function (this: any, value?: any) {
            return { value: value, label: labels[value] || value };
        });
    }
    function coverControlDefaultTabs(this: any) {
        return cardContractOptionDefaultValue("cover", COVER_CONTROL_TABS_OPTION, "position|controls|tilt|presets").split("|");
    }
    function normalizeTabList(this: any, value?: any, definitions?: any, defaults?: any, fallback?: any) {
        var raw: any = String(value || "").trim();
        var parts: any = raw ? raw.split("|") : defaults;
        var valid: any = {};
        definitions.forEach(function (this: any, tab?: any) { valid[tab.value] = true; });
        var out: any = [];
        parts.forEach(function (this: any, part?: any) {
            part = String(part || "").trim();
            if (valid[part] && out.indexOf(part) < 0)
                out.push(part);
        });
        return out.length ? out : [fallback];
    }
    function tabListIsDefault(this: any, tabs?: any, defaults?: any) {
        tabs = tabs || [];
        if (tabs.length !== defaults.length)
            return false;
        for (var i: any = 0; i < defaults.length; i++) {
            if (tabs[i] !== defaults[i])
                return false;
        }
        return true;
    }
    function normalizeCoverControlTabs(this: any, value?: any) {
        return normalizeTabList(value, coverControlTabDefinitions(), coverControlDefaultTabs(), "position");
    }
    function coverControlTabs(this: any, b?: any) {
        return normalizeCoverControlTabs(configOptionValue(b && b.options, COVER_CONTROL_TABS_OPTION));
    }
    function coverControlTabsAreDefault(this: any, tabs?: any) {
        return tabListIsDefault(normalizeCoverControlTabs((tabs || []).join("|")), coverControlDefaultTabs());
    }
    function normalizeCoverOptions(this: any, options?: any) {
        var tabs: any = normalizeCoverControlTabs(configOptionValue(options, COVER_CONTROL_TABS_OPTION));
        return coverControlTabsAreDefault(tabs)
            ? ""
            : setConfigOptionValue("", COVER_CONTROL_TABS_OPTION, tabs.join("|"));
    }
    function normalizeCoverOptionsForMode(this: any, options?: any, mode?: any) {
        return normalizeCoverMode(mode, true) === "modal" ? normalizeCoverOptions(options) : "";
    }
    function setCoverControlTabs(this: any, b?: any, tabs?: any) {
        if (!b)
            return "";
        tabs = normalizeCoverControlTabs((tabs || []).join("|"));
        b.options = coverControlTabsAreDefault(tabs)
            ? setConfigOptionValue(b.options, COVER_CONTROL_TABS_OPTION, "")
            : setConfigOptionValue(b.options, COVER_CONTROL_TABS_OPTION, tabs.join("|"));
        b.options = normalizeCoverOptions(b.options);
        return b.options;
    }
    function climateControlTabDefinitions(this: any) {
        return [
            { value: "temperature", label: "Temperature" },
            { value: "mode", label: "Mode" },
            { value: "preset", label: "Preset" },
            { value: "fan", label: "Fan" },
            { value: "swing", label: "Swing" },
        ];
    }
    function climateControlDefaultTabs(this: any) {
        return climateControlTabDefinitions().map(function (this: any, tab?: any) { return tab.value; });
    }
    function normalizeClimateControlTabs(this: any, value?: any) {
        return normalizeTabList(value, climateControlTabDefinitions(), climateControlDefaultTabs(), "temperature");
    }
    function climateControlTabs(this: any, b?: any) {
        return normalizeClimateControlTabs(configOptionValue(b && b.options, CLIMATE_CONTROL_TABS_OPTION));
    }
    function climateControlTabsAreDefault(this: any, tabs?: any) {
        return tabListIsDefault(normalizeClimateControlTabs((tabs || []).join("|")), climateControlDefaultTabs());
    }
    function setClimateControlTabs(this: any, b?: any, tabs?: any) {
        if (!b)
            return "";
        tabs = normalizeClimateControlTabs((tabs || []).join("|"));
        b.options = climateControlTabsAreDefault(tabs)
            ? setConfigOptionValue(b.options, CLIMATE_CONTROL_TABS_OPTION, "")
            : setConfigOptionValue(b.options, CLIMATE_CONTROL_TABS_OPTION, tabs.join("|"));
        b.options = normalizeClimateOptionsForTabs(b.options, true);
        return b.options;
    }
    function fanControlTabDefinitions(this: any) {
        return [
            { value: "power", label: "Power" },
            { value: "speed", label: "Speed" },
            { value: "preset", label: "Preset" },
            { value: "oscillation", label: "Oscillation" },
            { value: "direction", label: "Direction" },
            { value: "light", label: "Light" },
        ];
    }
    function fanControlDefaultTabs(this: any) {
        return ["power", "speed", "preset", "oscillation", "direction"];
    }
    function normalizeFanControlTabs(this: any, value?: any) {
        return normalizeTabList(value, fanControlTabDefinitions(), fanControlDefaultTabs(), "power");
    }
    function fanControlTabs(this: any, b?: any) {
        var configuredTabs: any = configOptionValue(b && b.options, FAN_CONTROL_TABS_OPTION);
        var tabs: any = normalizeFanControlTabs(configuredTabs);
        if (!configuredTabs && fanLightEntity(b))
            tabs.push("light");
        return tabs;
    }
    function fanControlTabsAreDefault(this: any, tabs?: any) {
        return tabListIsDefault(normalizeFanControlTabs((tabs || []).join("|")), fanControlDefaultTabs());
    }
    function fanLightEntity(this: any, b?: any) {
        return String(configOptionValue(b && b.options, FAN_LIGHT_ENTITY_OPTION) || "").trim();
    }
    function normalizeFanControlOptions(this: any, options?: any) {
        var lightEntity: any = String(configOptionValue(options, FAN_LIGHT_ENTITY_OPTION) || "").trim();
        var configuredTabs: any = configOptionValue(options, FAN_CONTROL_TABS_OPTION);
        var tabs: any = normalizeFanControlTabs(configuredTabs);
        if (!lightEntity)
            tabs = tabs.filter(function (this: any, tab?: any) { return tab !== "light"; });
        if (!lightEntity && tabs.length === 0)
            tabs = ["power"];
        var defaultTabs: any = fanControlDefaultTabs();
        if (lightEntity)
            defaultTabs = defaultTabs.concat(["light"]);
        var out: any = "";
        if (lightEntity)
            out = setConfigOptionValue(out, FAN_LIGHT_ENTITY_OPTION, lightEntity);
        var effectiveTabs: any = configuredTabs ? tabs : defaultTabs;
        if (!tabListIsDefault(effectiveTabs, defaultTabs))
            out = setConfigOptionValue(out, FAN_CONTROL_TABS_OPTION, effectiveTabs.join("|"));
        return out;
    }
    function setFanControlTabs(this: any, b?: any, tabs?: any) {
        if (!b)
            return "";
        tabs = normalizeFanControlTabs((tabs || []).join("|"));
        b.options = setConfigOptionValue(b.options, FAN_CONTROL_TABS_OPTION, tabs.join("|"));
        b.options = normalizeFanControlOptions(b.options);
        return b.options;
    }
    function setFanLightEntity(this: any, b?: any, entity?: any) {
        if (!b)
            return "";
        var previous: any = fanLightEntity(b);
        var next: any = String(entity || "").trim();
        b.options = setConfigOptionValue(b.options, FAN_LIGHT_ENTITY_OPTION, next);
        if (next && !previous) {
            var tabs: any = fanControlTabs(b);
            if (tabs.indexOf("light") < 0)
                tabs.push("light");
            b.options = setConfigOptionValue(b.options, FAN_CONTROL_TABS_OPTION, tabs.join("|"));
        }
        b.options = normalizeFanControlOptions(b.options);
        return b.options;
    }
    function renderModalTabSettings(this: any, panel?: any, b?: any, helpers?: any, config?: any) {
        var section: any = dependencies.document.createElement("div");
        panel.appendChild(section);
        b.options = config.normalizeOptions(b.options);
        var tabs: any = config.tabs(b);
        var definitions: any = config.definitions();
        var definitionByValue: any = {};
        definitions.forEach(function (this: any, definition?: any) {
            definitionByValue[definition.value] = definition;
        });
        var orderedDefinitions: any = [];
        tabs.forEach(function (this: any, tab?: any) {
            if (definitionByValue[tab])
                orderedDefinitions.push(definitionByValue[tab]);
        });
        definitions.forEach(function (this: any, definition?: any) {
            if (tabs.indexOf(definition.value) < 0)
                orderedDefinitions.push(definition);
        });
        if (!config.hideHeading) {
            var heading: any = dependencies.document.createElement("div");
            heading.className = "sp-field";
            heading.appendChild(helpers.fieldLabel("Modal Tabs"));
            section.appendChild(heading);
        }
        var list: any = dependencies.document.createElement("div");
        list.className = "sp-light-tab-list";
        section.appendChild(list);
        function listRows(this: any) {
            return Array.prototype.slice.call(list.querySelectorAll(".sp-light-tab-row"));
        }
        function saveTabsFromRows(this: any) {
            var nextTabs: any = [];
            listRows().forEach(function (this: any, row?: any) {
                var input: any = row.querySelector("input[type=checkbox]");
                if (input && input.checked)
                    nextTabs.push(row.getAttribute("data-tab"));
            });
            if (!nextTabs.length)
                return false;
            saveTabs(nextTabs);
            return true;
        }
        function saveTabs(this: any, nextTabs?: any) {
            config.setTabs(b, nextTabs);
            b._modalSettingsOpen = true;
            helpers.saveField("options", b.options);
            dependencies.renderButtonSettings();
        }
        function moveRow(this: any, row?: any, direction?: any) {
            var sibling: any = direction < 0 ? row.previousElementSibling : row.nextElementSibling;
            if (!sibling)
                return;
            if (direction < 0) {
                list.insertBefore(row, sibling);
            }
            else {
                list.insertBefore(sibling, row);
            }
            saveTabsFromRows();
        }
        orderedDefinitions.forEach(function (this: any, definition?: any) {
            var tabIndex: any = tabs.indexOf(definition.value);
            var visible: any = tabIndex >= 0;
            var available: any = !config.tabAvailable || config.tabAvailable(b, definition.value);
            var row: any = dependencies.document.createElement("div");
            row.className = "sp-light-tab-row";
            row.classList.toggle("sp-disabled", !available);
            row.setAttribute("data-tab", definition.value);
            row.draggable = available;
            var controls: any = dependencies.document.createElement("div");
            controls.className = "sp-light-tab-controls";
            var drag: any = dependencies.document.createElement("button");
            drag.type = "button";
            drag.className = "sp-light-tab-drag mdi mdi-drag";
            drag.setAttribute("aria-label", "Drag " + definition.label);
            drag.tabIndex = -1;
            var moveUp: any = dependencies.document.createElement("button");
            moveUp.type = "button";
            moveUp.className = "sp-light-tab-move mdi mdi-chevron-up";
            moveUp.setAttribute("aria-label", "Move " + definition.label + " up");
            var moveDown: any = dependencies.document.createElement("button");
            moveDown.type = "button";
            moveDown.className = "sp-light-tab-move mdi mdi-chevron-down";
            moveDown.setAttribute("aria-label", "Move " + definition.label + " down");
            moveUp.disabled = !available;
            moveDown.disabled = !available;
            controls.appendChild(drag);
            controls.appendChild(moveUp);
            controls.appendChild(moveDown);
            row.appendChild(controls);
            var label: any = dependencies.document.createElement("label");
            label.className = "sp-light-tab-label";
            label.htmlFor = helpers.idPrefix + config.idPrefix + definition.value;
            label.textContent = definition.label;
            row.appendChild(label);
            var toggle: any = dependencies.document.createElement("label");
            toggle.className = "sp-toggle";
            var input: any = dependencies.document.createElement("input");
            input.type = "checkbox";
            input.id = helpers.idPrefix + config.idPrefix + definition.value;
            input.checked = visible;
            input.disabled = !available;
            var track: any = dependencies.document.createElement("span");
            track.className = "sp-toggle-track";
            toggle.appendChild(input);
            toggle.appendChild(track);
            row.appendChild(toggle);
            input.addEventListener("change", function (this: any) {
                if (!available)
                    return;
                if (!this.checked) {
                    var visibleCount: any = listRows().filter(function (this: any, item?: any) {
                        var itemInput: any = item.querySelector("input[type=checkbox]");
                        return itemInput && itemInput.checked;
                    }).length;
                    if (visibleCount < 1) {
                        this.checked = true;
                        return;
                    }
                }
                saveTabsFromRows();
            });
            moveUp.addEventListener("click", function (this: any) { moveRow(row, -1); });
            moveDown.addEventListener("click", function (this: any) { moveRow(row, 1); });
            row.addEventListener("dragstart", function (this: any, event?: any) {
                if (!available)
                    return;
                row.classList.add("sp-dragging");
                event.dataTransfer.effectAllowed = "move";
                event.dataTransfer.setData("text/plain", definition.value);
            });
            row.addEventListener("dragend", function (this: any) {
                row.classList.remove("sp-dragging");
            });
            row.addEventListener("dragover", function (this: any, event?: any) {
                var dragging: any = list.querySelector(".sp-dragging");
                if (!dragging || dragging === row)
                    return;
                event.preventDefault();
                var rect: any = row.getBoundingClientRect();
                var after: any = event.clientY > rect.top + rect.height / 2;
                list.insertBefore(dragging, after ? row.nextSibling : row);
            });
            row.addEventListener("drop", function (this: any, event?: any) {
                event.preventDefault();
                saveTabsFromRows();
            });
            list.appendChild(row);
        });
        return section;
    }
    return {
        connectClimateOptionsNormalizer,
        lightControlTabDefinitions,
        lightControlDefaultTabs,
        normalizeLightControlTabs,
        lightControlTabs,
        lightControlTabsAreDefault,
        normalizeLightControlOptions,
        setLightControlTabs,
        coverControlTabDefinitions,
        coverControlDefaultTabs,
        normalizeTabList,
        tabListIsDefault,
        normalizeCoverControlTabs,
        coverControlTabs,
        coverControlTabsAreDefault,
        normalizeCoverOptions,
        normalizeCoverOptionsForMode,
        setCoverControlTabs,
        climateControlTabDefinitions,
        climateControlDefaultTabs,
        normalizeClimateControlTabs,
        climateControlTabs,
        climateControlTabsAreDefault,
        setClimateControlTabs,
        fanControlTabDefinitions,
        fanControlDefaultTabs,
        normalizeFanControlTabs,
        fanControlTabs,
        fanControlTabsAreDefault,
        fanLightEntity,
        normalizeFanControlOptions,
        setFanControlTabs,
        setFanLightEntity,
        renderModalTabSettings,
    };
}

export type ConfigModalTabOptionsFeature = ReturnType<typeof createConfigModalTabOptionsFeature>;
