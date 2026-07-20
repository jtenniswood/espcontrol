import { state } from "../state/app_instance";
import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installEntityStateModule(): GlobalDescriptors {
    // ── Entity State Helpers ───────────────────────────────────────────────
    function uniquePush(this: any, list?: any, value?: any) {
        if (value && list.indexOf(value) === -1)
            list.push(value);
    }
    function entityDef(this: any, key?: any) {
        return ENTITY_CATALOG.entities[key] || {};
    }
    function entityName(this: any, key?: any) {
        return entityDef(key).name || "";
    }
    function entityNameForSlot(this: any, key?: any, slot?: any) {
        return String(entityDef(key).template || "").replace("{slot}", String(slot));
    }
    function entityObjectIds(this: any, key?: any) {
        return (entityDef(key).objectIds || []).slice();
    }
    function entityLookupNames(this: any, key?: any) {
        var names: any = [];
        uniquePush(names, entityName(key));
        entityObjectIds(key).forEach(function (this: any, objectId?: any) { uniquePush(names, objectId); });
        return names;
    }
    function entityStateItem(this: any, key?: any) {
        var def: any = entityDef(key);
        return [def.domain, def.name];
    }
    function entityStateItems(this: any, keys?: any) {
        return keys.map(entityStateItem);
    }
    function entityStateItemsForSlots(this: any, keys?: any) {
        var items: any = [];
        for (var i: any = 1; i <= TOTAL_SLOTS; i++) {
            keys.forEach(function (this: any, key?: any) {
                items.push([entityDef(key).domain, entityNameForSlot(key, i)]);
            });
        }
        return items;
    }
    function esphomeObjectId(this: any, value?: any) {
        return String(value || "").replace(/./g, function (this: any, ch?: any) {
            if (ch === " ")
                return "_";
            var lower: any = ch.toLowerCase();
            if ((lower >= "a" && lower <= "z") || (ch >= "0" && ch <= "9") || ch === "-" || ch === "_")
                return lower;
            return "_";
        });
    }
    function parseEntityId(this: any, value?: any) {
        var id: any = String(value || "");
        if (!id)
            return null;
        if (id.indexOf("/") !== -1) {
            var parts: any = id.split("/");
            if (parts.length < 2 || !parts[0] || !parts[parts.length - 1])
                return null;
            return {
                raw: id,
                domain: parts[0],
                name: parts[parts.length - 1],
                objectId: esphomeObjectId(parts[parts.length - 1]),
                path: "/" + parts.map(encodeURIComponent).join("/"),
            };
        }
        var dash: any = id.indexOf("-");
        if (dash <= 0)
            return null;
        return {
            raw: id,
            domain: id.substring(0, dash),
            objectId: id.substring(dash + 1),
            path: "/" + encodeURIComponent(id.substring(0, dash)) + "/" + encodeURIComponent(id.substring(dash + 1)),
        };
    }
    function parseHomeAssistantEntity(this: any, value?: any) {
        var text: any = String(value || "").trim();
        var dot: any = text.indexOf(".");
        if (dot <= 0 || dot >= text.length - 1)
            return null;
        return {
            id: text,
            domain: text.substring(0, dot),
            objectId: text.substring(dot + 1),
        };
    }
    function titleFromEntityId(this: any, entityId?: any) {
        var parsed: any = parseHomeAssistantEntity(entityId);
        if (!parsed)
            return entityId;
        return parsed.objectId.replace(/_/g, " ").replace(/\b\w/g, function (this: any, ch?: any) {
            return ch.toUpperCase();
        });
    }
    function rememberEntityName(this: any, entityId?: any, name?: any) {
        var parsed: any = parseHomeAssistantEntity(entityId);
        if (!parsed || !name)
            return;
        if (!state.entityNames[parsed.id])
            state.entityNames[parsed.id] = [];
        uniquePush(state.entityNames[parsed.id], String(name));
    }
    function rememberConfiguredButtonEntities(this: any, button?: any) {
        if (!button)
            return;
        var label: any = button.label || "";
        if (button.entity)
            rememberEntityName(button.entity, label || titleFromEntityId(button.entity));
        if (button.sensor && parseHomeAssistantEntity(button.sensor)) {
            rememberEntityName(button.sensor, label || titleFromEntityId(button.sensor));
        }
        if (button.type === "action") {
            var stateEntity: any = actionCardStateEntity(button);
            if (stateEntity)
                rememberEntityName(stateEntity, titleFromEntityId(stateEntity));
        }
    }
    function rememberConfiguredEntities(this: any) {
        for (var i: any = 0; i < state.buttons.length; i++)
            rememberConfiguredButtonEntities(state.buttons[i]);
        for (var slot in state.subpages) {
            var sp: any = state.subpages[slot];
            if (!sp || !sp.buttons)
                continue;
            for (var bi: any = 0; bi < sp.buttons.length; bi++)
                rememberConfiguredButtonEntities(sp.buttons[bi]);
        }
        rememberEntityName(state.indoorEntity, "Indoor Temperature");
        rememberEntityName(state.outdoorEntity, "Outdoor Temperature");
        clockBarTemperatureEntities().forEach(function (this: any, entityId?: any, index?: any) {
            rememberEntityName(entityId, "Clock Bar Temperature " + (index + 1));
        });
        rememberEntityName(state.presenceEntity, "Presence Sensor");
        rememberEntityName(state.coverArtMediaPlayerEntity, "Media Player");
        rememberEntityName(state.coverArtSecondaryMediaPlayerEntity, "External Source Media Player");
    }
    function optionLabelForEntity(this: any, entityId?: any) {
        var names: any = state.entityNames[entityId] || [];
        if (!names.length)
            return titleFromEntityId(entityId);
        return names.join(" / ");
    }
    function entitySuggestions(this: any, domains?: any) {
        rememberConfiguredEntities();
        var allowed: any = {};
        (domains || []).forEach(function (this: any, domain?: any) { allowed[domain] = true; });
        var ids: any = [];
        for (var id in state.entityNames) {
            var parsed: any = parseHomeAssistantEntity(id);
            if (!parsed)
                continue;
            if (domains && domains.length && !allowed[parsed.domain])
                continue;
            ids.push(id);
        }
        ids.sort(function (this: any, a?: any, b?: any) {
            var al: any = optionLabelForEntity(a).toLowerCase();
            var bl: any = optionLabelForEntity(b).toLowerCase();
            if (al === bl)
                return a.localeCompare(b);
            return al.localeCompare(bl);
        });
        return ids.map(function (this: any, id?: any) {
            return { value: id, label: optionLabelForEntity(id) };
        });
    }
    function ensureEntityDropdown(this: any, input?: any) {
        if (!input || input._entityDropdown || !input.parentNode)
            return;
        // The dropdown lives on document.body with fixed positioning so the
        // settings modal's overflow scrolling can never clip it. Stale
        // dropdowns from re-rendered inputs are pruned here.
        document.querySelectorAll(".sp-entity-dropdown").forEach(function (this: any, other?: any) {
            if (other._entityInput && !other._entityInput.isConnected)
                other.remove();
        });
        var dropdown: any = document.createElement("div");
        dropdown.className = "sp-entity-dropdown";
        dropdown._entityInput = input;
        document.body.appendChild(dropdown);
        input._entityDropdown = dropdown;
    }
    function positionEntityDropdown(this: any, input?: any, dropdown?: any) {
        var rect: any = input.getBoundingClientRect();
        var viewportH: any = window.innerHeight || 0;
        var below: any = viewportH - rect.bottom - 16;
        var above: any = rect.top - 16;
        dropdown.style.left = rect.left + "px";
        dropdown.style.width = rect.width + "px";
        if (below < 140 && above > below) {
            var maxH: any = Math.min(220, above);
            dropdown.style.maxHeight = maxH + "px";
            dropdown.style.top = "";
            dropdown.style.bottom = (viewportH - rect.top + 6) + "px";
        }
        else {
            dropdown.style.maxHeight = Math.min(220, Math.max(below, 140)) + "px";
            dropdown.style.bottom = "";
            dropdown.style.top = (rect.bottom + 6) + "px";
        }
    }
    function closeEntityDropdown(this: any, input?: any) {
        if (input && input._entityDropdown)
            input._entityDropdown.classList.remove("sp-open");
    }
    function refreshEntityDatalist(this: any, input?: any) {
        if (!input)
            return;
        ensureEntityDropdown(input);
        var dropdown: any = input._entityDropdown;
        if (!dropdown)
            return;
        if (input._entitySuppressDropdown) {
            closeEntityDropdown(input);
            return;
        }
        dropdown.innerHTML = "";
        var raw: any = String(input.value || "");
        var prefix: any = "";
        var chosen: any = [];
        if (input._entityMulti) {
            // Comma-separated list: suggest against the segment after the last
            // comma and keep the earlier entries as a prefix on selection.
            var comma: any = raw.lastIndexOf(",");
            if (comma !== -1) {
                prefix = raw.slice(0, comma + 1);
                raw = raw.slice(comma + 1);
                chosen = prefix.split(",").map(function (this: any, part?: any) {
                    // Entries may carry a ":#RRGGBB" color suffix; dedup on
                    // the bare entity id.
                    return part.trim().toLowerCase().split(":#")[0];
                }).filter(Boolean);
            }
        }
        input._entityMultiPrefix = prefix;
        var query: any = raw.trim().toLowerCase();
        var items: any = entitySuggestions(input._entityDomains || []).filter(function (this: any, item?: any) {
            if (chosen.indexOf(item.value.toLowerCase()) !== -1)
                return false;
            if (!query)
                return true;
            return item.value.toLowerCase().indexOf(query) !== -1 ||
                item.label.toLowerCase().indexOf(query) !== -1;
        }).slice(0, 12);
        items.forEach(function (this: any, item?: any) {
            var option: any = document.createElement("button");
            option.type = "button";
            option.className = "sp-entity-option";
            option.textContent = item.value;
            option.addEventListener("mousedown", function (this: any, e?: any) {
                e.preventDefault();
                input._entitySuppressDropdown = true;
                var listPrefix: any = input._entityMulti ? String(input._entityMultiPrefix || "") : "";
                input.value = listPrefix ? listPrefix.replace(/\s+$/, "") + " " + item.value : item.value;
                rememberEntityName(item.value, item.label || titleFromEntityId(item.value));
                input.dispatchEvent(new Event("input", { bubbles: true }));
                input.dispatchEvent(new Event("change", { bubbles: true }));
                closeEntityDropdown(input);
                input._entitySuppressDropdown = false;
            });
            dropdown.appendChild(option);
        });
        var open: any = document.activeElement === input && items.length > 0;
        if (open)
            positionEntityDropdown(input, dropdown);
        dropdown.classList.toggle("sp-open", open);
    }
    function attachEntitySuggestions(this: any, input?: any, domains?: any, multi?: any) {
        if (!input || input._entitySuggestionsAttached)
            return input;
        input._entityDomains = domains || [];
        input._entityMulti = !!multi;
        input._entitySuggestionsAttached = true;
        input.addEventListener("focus", function (this: any) { refreshEntityDatalist(input); });
        input.addEventListener("input", function (this: any) {
            rememberEntityName(input.value, optionLabelForEntity(input.value));
            refreshEntityDatalist(input);
        });
        input.addEventListener("blur", function (this: any) {
            setTimeout(function (this: any) { closeEntityDropdown(input); }, 120);
        });
        input.addEventListener("keydown", function (this: any, e?: any) {
            if (e.key === "Escape")
                closeEntityDropdown(input);
        });
        refreshEntityDatalist(input);
        return input;
    }
    function entityInput(this: any, id?: any, value?: any, placeholder?: any, domains?: any, multi?: any) {
        var el: any = textInput(id, value, placeholder);
        return attachEntitySuggestions(el, domains, multi);
    }
    function rememberEntityPostPath(this: any, data?: any) {
        var preferred: any = parseEntityId(data && data.name_id) || parseEntityId(data && data.id);
        if (data && data.domain && data.name)
            rememberEntityName(data.domain + "." + esphomeObjectId(data.name), data.name);
        if (!preferred || !preferred.path)
            return;
        entityStateKeys(data).forEach(function (this: any, key?: any) {
            state.entityPostPaths[key] = preferred.path;
        });
        if (preferred.domain && preferred.name)
            state.entityPostPaths[preferred.domain + ":" + preferred.name] = preferred.path;
        if (preferred.domain && preferred.objectId)
            state.entityPostPaths[preferred.domain + ":" + preferred.objectId] = preferred.path;
    }
    function rememberedPostUrls(this: any, domain?: any, name?: any, objectIds?: any, action?: any) {
        var urls: any = [];
        var keys: any = [domain + ":" + name, domain + "-" + esphomeObjectId(name)];
        objectIds.forEach(function (this: any, objectId?: any) {
            keys.push(domain + ":" + objectId);
            keys.push(domain + "-" + objectId);
        });
        keys.forEach(function (this: any, key?: any) {
            if (state.entityPostPaths[key])
                uniquePush(urls, state.entityPostPaths[key] + "/" + action);
        });
        return urls;
    }
    function hasRememberedPostPath(this: any, domain?: any, name?: any, objectIds?: any) {
        var keys: any = [domain + ":" + name, domain + "-" + esphomeObjectId(name)];
        (objectIds || []).forEach(function (this: any, objectId?: any) {
            keys.push(domain + ":" + objectId);
            keys.push(domain + "-" + objectId);
        });
        return keys.some(function (this: any, key?: any) {
            return !!state.entityPostPaths[key];
        });
    }
    function entityPostUrls(this: any, domain?: any, name?: any, objectIds?: any, action?: any) {
        var urls: any = [];
        rememberedPostUrls(domain, name, objectIds || [], action).forEach(function (this: any, url?: any) {
            uniquePush(urls, url);
        });
        (objectIds || []).forEach(function (this: any, objectId?: any) {
            uniquePush(urls, "/" + domain + "/" + encodeURIComponent(objectId) + "/" + action);
        });
        uniquePush(urls, "/" + domain + "/" + encodeURIComponent(name) + "/" + action);
        uniquePush(urls, "/" + domain + "/" + encodeURIComponent(esphomeObjectId(name)) + "/" + action);
        return urls;
    }
    return {
        "uniquePush": staticGlobal(uniquePush),
        "entityDef": staticGlobal(entityDef),
        "entityName": staticGlobal(entityName),
        "entityNameForSlot": staticGlobal(entityNameForSlot),
        "entityObjectIds": staticGlobal(entityObjectIds),
        "entityLookupNames": staticGlobal(entityLookupNames),
        "entityStateItem": staticGlobal(entityStateItem),
        "entityStateItems": staticGlobal(entityStateItems),
        "entityStateItemsForSlots": staticGlobal(entityStateItemsForSlots),
        "esphomeObjectId": staticGlobal(esphomeObjectId),
        "parseEntityId": staticGlobal(parseEntityId),
        "parseHomeAssistantEntity": staticGlobal(parseHomeAssistantEntity),
        "titleFromEntityId": staticGlobal(titleFromEntityId),
        "rememberEntityName": staticGlobal(rememberEntityName),
        "rememberConfiguredButtonEntities": staticGlobal(rememberConfiguredButtonEntities),
        "rememberConfiguredEntities": staticGlobal(rememberConfiguredEntities),
        "optionLabelForEntity": staticGlobal(optionLabelForEntity),
        "entitySuggestions": staticGlobal(entitySuggestions),
        "ensureEntityDropdown": staticGlobal(ensureEntityDropdown),
        "closeEntityDropdown": staticGlobal(closeEntityDropdown),
        "refreshEntityDatalist": staticGlobal(refreshEntityDatalist),
        "attachEntitySuggestions": staticGlobal(attachEntitySuggestions),
        "entityInput": staticGlobal(entityInput),
        "rememberEntityPostPath": staticGlobal(rememberEntityPostPath),
        "rememberedPostUrls": staticGlobal(rememberedPostUrls),
        "hasRememberedPostPath": staticGlobal(hasRememberedPostPath),
        "entityPostUrls": staticGlobal(entityPostUrls),
    };
}
