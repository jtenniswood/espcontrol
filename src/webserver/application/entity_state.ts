import { state } from "../state/app_instance";
import { ENTITY_CATALOG } from "../generated/entity_catalog";
import { entityStateKeys } from "../state/event_state";
import type { ConfigConfirmationOptionsFeature } from "./config_confirmation_options";
import type { EntityCatalogClient } from "./entity_catalog";

type EntityDefinition = {
    readonly domain?: string;
    readonly name?: string;
    readonly template?: string;
    readonly objectIds?: readonly string[];
};

const entityDefinitions = ENTITY_CATALOG.entities as unknown as Readonly<Record<string, EntityDefinition>>;

export interface EntityStateDependencies {
    readonly actionCardStateEntity: ConfigConfirmationOptionsFeature["actionCardStateEntity"];
    readonly totalSlots: () => number;
    readonly clockBarTemperatureEntities: () => any[];
    readonly textInput: (id: any, value: any, placeholder: any) => any;
    readonly entityCatalog?: EntityCatalogClient;
}

export function createEntityStateFeature(dependencies: EntityStateDependencies) {
    const { actionCardStateEntity, clockBarTemperatureEntities } = dependencies;
    const entityCatalog = dependencies.entityCatalog;
    // ── Entity State Helpers ───────────────────────────────────────────────
    function uniquePush(this: any, list?: any, value?: any) {
        if (value && list.indexOf(value) === -1)
            list.push(value);
    }
    function entityDef(this: any, key?: any) {
        return entityDefinitions[String(key)] || {};
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
        for (var i: any = 1; i <= dependencies.totalSlots(); i++) {
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
    function rememberEntityRecord(this: any, record?: any) {
        if (!record || !record.entity_id)
            return;
        rememberEntityName(record.entity_id, record.name || titleFromEntityId(record.entity_id));
        if (!state.entityCatalogRecords)
            state.entityCatalogRecords = {};
        state.entityCatalogRecords[record.entity_id] = record;
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
        var records: any = state.entityCatalogRecords || {};
        Object.keys(records).forEach(function (this: any, id?: any) {
            var parsed: any = parseHomeAssistantEntity(id);
            if (!parsed || (domains && domains.length && !allowed[parsed.domain]) || ids.indexOf(id) !== -1)
                return;
            var record: any = records[id];
            // Hidden and disabled entities are available through an explicit
            // manual ID, but should not crowd the normal picker results.
            if (record && (record.hidden || record.disabled))
                return;
            ids.push(id);
        });
        ids.sort(function (this: any, a?: any, b?: any) {
            var al: any = optionLabelForEntity(a).toLowerCase();
            var bl: any = optionLabelForEntity(b).toLowerCase();
            return al === bl ? a.localeCompare(b) : al.localeCompare(bl);
        });
        var presentationCounts: any = {};
        ids.forEach(function (this: any, id?: any) {
            var record: any = records[id];
            var label: any = record && record.name ? String(record.name) : optionLabelForEntity(id);
            var location: any = record && record.area_name ? String(record.area_name) : "";
            var key: any = (label + "\u0000" + location).toLowerCase();
            presentationCounts[key] = (presentationCounts[key] || 0) + 1;
        });
        return ids.map(function (this: any, id?: any) {
            var record: any = records[id];
            var label: any = record && record.name ? String(record.name) : optionLabelForEntity(id);
            var location: any = record && record.area_name ? String(record.area_name) : "";
            var key: any = (label + "\u0000" + location).toLowerCase();
            return {
                value: id,
                label: label,
                location: location,
                showValue: presentationCounts[key] > 1,
            };
        });
    }
    function ensureEntityDropdown(this: any, input?: any) {
        if (!input || input._entityDropdown || !input.parentNode)
            return;
        var wrap: any = document.createElement("div");
        wrap.className = "sp-entity-input-wrap";
        input.parentNode.insertBefore(wrap, input);
        wrap.appendChild(input);
        var dropdown: any = document.createElement("div");
        dropdown.className = "sp-entity-dropdown";
        wrap.appendChild(dropdown);
        input._entityDropdown = dropdown;
    }
    function closeEntityDropdown(this: any, input?: any) {
        if (input && input._entityDropdown)
            input._entityDropdown.classList.remove("sp-open");
    }
    function showSelectedEntityLabel(this: any, input?: any) {
        if (!input || !input._entityValue || !input._entityDisplayValue || document.activeElement === input)
            return;
        if (String(input.value || "") === String(input._entityValue))
            input.value = input._entityDisplayValue;
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
        var query: any = String(input.value || "").trim().toLowerCase();
        var items: any = entitySuggestions(input._entityDomains || []).filter(function (this: any, item?: any) {
            if (!query)
                return true;
            return item.value.toLowerCase().indexOf(query) !== -1 ||
                item.label.toLowerCase().indexOf(query) !== -1 ||
                item.location.toLowerCase().indexOf(query) !== -1;
        });
        items.slice(0, 12).forEach(function (this: any, item?: any) {
            var option: any = document.createElement("button");
            option.type = "button";
            option.className = "sp-entity-option";
            var name: any = document.createElement("span");
            name.className = "sp-entity-option-name";
            name.textContent = item.label;
            option.appendChild(name);
            if (item.location) {
                var location: any = document.createElement("span");
                location.className = "sp-entity-option-location";
                location.textContent = item.location;
                option.appendChild(location);
            }
            if (item.showValue) {
                var id: any = document.createElement("span");
                id.className = "sp-entity-option-id";
                id.textContent = item.value;
                option.appendChild(id);
            }
            option.addEventListener("mousedown", function (this: any, e?: any) {
                e.preventDefault();
                input._entitySuppressDropdown = true;
                input._entityValue = item.value;
                input._entityDisplayValue = item.label;
                input.value = item.value;
                rememberEntityName(item.value, item.label || titleFromEntityId(item.value));
                input.dispatchEvent(new Event("input", { bubbles: true }));
                input.dispatchEvent(new Event("change", { bubbles: true }));
                closeEntityDropdown(input);
                input._entitySuppressDropdown = false;
                input.blur();
                setTimeout(function (this: any) { showSelectedEntityLabel(input); }, 0);
            });
            dropdown.appendChild(option);
        });
        if (input._remoteEntityLoading) {
            var loading: any = document.createElement("div");
            loading.className = "sp-entity-catalog-status";
            loading.textContent = "Searching Home Assistant…";
            dropdown.appendChild(loading);
        } else if (input._remoteEntityError) {
            var error: any = document.createElement("div");
            error.className = "sp-entity-catalog-error";
            error.textContent = String(input._remoteEntityError);
            dropdown.appendChild(error);
            var retry: any = document.createElement("button");
            retry.type = "button";
            retry.className = "sp-entity-catalog-retry";
            retry.textContent = "Retry search";
            retry.addEventListener("mousedown", function (this: any, e?: any) {
                e.preventDefault();
                input._remoteEntityRetryRequired = false;
                input._remoteEntityQuery = "";
                refreshEntityDatalist(input);
            });
            dropdown.appendChild(retry);
        } else if (!items.length) {
            var empty: any = document.createElement("div");
            empty.className = "sp-entity-catalog-status";
            empty.textContent = "No matching Home Assistant entities.";
            dropdown.appendChild(empty);
        }
        var remoteQuery: any = String(input.value || "").trim();
        var remoteQueryComplete: any = remoteQuery && input._remoteEntityCompleteQuery === remoteQuery;
        dropdown.classList.toggle("sp-open", document.activeElement === input &&
            (items.length > 0 || input._remoteEntityLoading || !!input._remoteEntityError || remoteQueryComplete));
        if (!remoteQuery) {
            if (input._remoteEntityTimer) {
                clearTimeout(input._remoteEntityTimer);
                input._remoteEntityTimer = null;
            }
            input._remoteEntityQuery = "";
            input._remoteEntityCompleteQuery = "";
            return;
        }
        if (document.activeElement === input && entityCatalog && !input._remoteEntityRetryRequired &&
            input._remoteEntityQuery !== remoteQuery && !input._remoteEntityRequest &&
            !input._remoteEntityTimer) {
            input._remoteEntityQuery = remoteQuery;
            input._remoteEntityCompleteQuery = null;
            input._remoteEntityLoading = true;
            input._remoteEntityError = null;
            input._remoteEntityGeneration = (input._remoteEntityGeneration || 0) + 1;
            var generation: any = input._remoteEntityGeneration;
            var domains: any[] = (input._entityDomains || []).slice();
            var cacheKey: any = JSON.stringify([remoteQuery, domains]);
            var cached: any = input._entityCatalogCache && input._entityCatalogCache[cacheKey];
            if (cached) {
                cached.forEach(function (this: any, record?: any) { rememberEntityRecord(record); });
                input._remoteEntityLoading = false;
                input._remoteEntityCompleteQuery = remoteQuery;
                refreshEntityDatalist(input);
            } else {
                if (input._remoteEntityTimer)
                    clearTimeout(input._remoteEntityTimer);
                input._remoteEntityTimer = setTimeout(function (this: any) {
                    input._remoteEntityTimer = null;
                    input._remoteEntityRequest = entityCatalog.search(remoteQuery, domains).then(function (this: any, records?: any[]) {
                        if (generation !== input._remoteEntityGeneration)
                            return;
                        input._remoteEntityRequest = null;
                        input._remoteEntityLoading = false;
                        input._remoteEntityError = null;
                        input._remoteEntityCompleteQuery = remoteQuery;
                        var result: any[] = records || [];
                        if (!input._entityCatalogCache)
                            input._entityCatalogCache = {};
                        input._entityCatalogCache[cacheKey] = result;
                        result.forEach(function (this: any, record?: any) {
                            rememberEntityRecord(record);
                        });
                        refreshEntityDatalist(input);
                    }).catch(function (this: any, error?: any) {
                        if (generation !== input._remoteEntityGeneration)
                            return;
                        input._remoteEntityRequest = null;
                        input._remoteEntityLoading = false;
                        input._remoteEntityRetryRequired = true;
                        input._remoteEntityQuery = "";
                        input._remoteEntityCompleteQuery = null;
                        input._remoteEntityError = "Home Assistant entity search is unavailable. Check the ESPHome connection and retry.";
                        refreshEntityDatalist(input);
                    });
                    refreshEntityDatalist(input);
                }, 180);
            }
        }
    }
    function attachEntitySuggestions(this: any, input?: any, domains?: any) {
        if (!input || input._entitySuggestionsAttached)
            return input;
        input._entityDomains = domains || [];
        input._entitySuggestionsAttached = true;
        input._entityCatalogCache = {};
        input._remoteEntityGeneration = 0;
        input._remoteEntityQuery = "";
        input._remoteEntityCompleteQuery = "";
        input._remoteEntityRetryRequired = false;
        input._remoteEntityLoading = false;
        input.addEventListener("focus", function (this: any) {
            if (input._entityValue && input._entityDisplayValue && input.value === input._entityDisplayValue) {
                input.value = input._entityValue;
                input.select();
            }
            refreshEntityDatalist(input);
        });
        input.addEventListener("input", function (this: any) {
            if (input._entityValue && String(input.value || "") !== String(input._entityValue)) {
                input._entityValue = "";
                input._entityDisplayValue = "";
            }
            input._remoteEntityError = null;
            input._remoteEntityRetryRequired = false;
            input._remoteEntityCompleteQuery = null;
            rememberEntityName(input.value, optionLabelForEntity(input.value));
            refreshEntityDatalist(input);
        });
        input.addEventListener("blur", function (this: any) {
            setTimeout(function (this: any) { closeEntityDropdown(input); }, 120);
            setTimeout(function (this: any) { showSelectedEntityLabel(input); }, 0);
        });
        input.addEventListener("keydown", function (this: any, e?: any) {
            if (e.key === "Escape")
                closeEntityDropdown(input);
        });
        refreshEntityDatalist(input);
        return input;
    }
    function entityInput(this: any, id?: any, value?: any, placeholder?: any, domains?: any) {
        var el: any = dependencies.textInput(id, value, placeholder);
        return attachEntitySuggestions(el, domains);
    }
    function entityValue(this: any, input?: any) {
        if (!input)
            return "";
        return input._entityValue || input.value || "";
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
        uniquePush,
        entityDef,
        entityName,
        entityNameForSlot,
        entityObjectIds,
        entityLookupNames,
        entityStateItem,
        entityStateItems,
        entityStateItemsForSlots,
        esphomeObjectId,
        parseEntityId,
        parseHomeAssistantEntity,
        titleFromEntityId,
        rememberEntityName,
        rememberConfiguredButtonEntities,
        rememberConfiguredEntities,
        optionLabelForEntity,
        entitySuggestions,
        ensureEntityDropdown,
        closeEntityDropdown,
        refreshEntityDatalist,
        attachEntitySuggestions,
        entityInput,
        entityValue,
        rememberEntityPostPath,
        rememberedPostUrls,
        hasRememberedPostPath,
        entityPostUrls,
    };
}

export type EntityStateFeature = ReturnType<typeof createEntityStateFeature>;
