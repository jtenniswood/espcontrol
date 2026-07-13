// ── Entity State Helpers ───────────────────────────────────────────────
// @web-module-requires: state, entity_catalog

function uniquePush(list, value) {
  if (value && list.indexOf(value) === -1) list.push(value);
}

function entityDef(key) {
  return ENTITY_CATALOG.entities[key] || {};
}

function entityName(key) {
  return entityDef(key).name || "";
}

function entityNameForSlot(key, slot) {
  return String(entityDef(key).template || "").replace("{slot}", String(slot));
}

function entityObjectIds(key) {
  return (entityDef(key).objectIds || []).slice();
}

function entityLookupNames(key) {
  var names = [];
  uniquePush(names, entityName(key));
  entityObjectIds(key).forEach(function (objectId) { uniquePush(names, objectId); });
  return names;
}

function entityStateItem(key) {
  var def = entityDef(key);
  return [def.domain, def.name];
}

function entityStateItems(keys) {
  return keys.map(entityStateItem);
}

function entityStateItemsForSlots(keys) {
  var items = [];
  for (var i = 1; i <= TOTAL_SLOTS; i++) {
    keys.forEach(function (key) {
      items.push([entityDef(key).domain, entityNameForSlot(key, i)]);
    });
  }
  return items;
}

function esphomeObjectId(value) {
  return String(value || "").replace(/./g, function (ch) {
    if (ch === " ") return "_";
    var lower = ch.toLowerCase();
    if ((lower >= "a" && lower <= "z") || (ch >= "0" && ch <= "9") || ch === "-" || ch === "_") return lower;
    return "_";
  });
}

function parseEntityId(value) {
  var id = String(value || "");
  if (!id) return null;
  if (id.indexOf("/") !== -1) {
    var parts = id.split("/");
    if (parts.length < 2 || !parts[0] || !parts[parts.length - 1]) return null;
    return {
      raw: id,
      domain: parts[0],
      name: parts[parts.length - 1],
      objectId: esphomeObjectId(parts[parts.length - 1]),
      path: "/" + parts.map(encodeURIComponent).join("/"),
    };
  }
  var dash = id.indexOf("-");
  if (dash <= 0) return null;
  return {
    raw: id,
    domain: id.substring(0, dash),
    objectId: id.substring(dash + 1),
    path: "/" + encodeURIComponent(id.substring(0, dash)) + "/" + encodeURIComponent(id.substring(dash + 1)),
  };
}

function parseHomeAssistantEntity(value) {
  var text = String(value || "").trim();
  var dot = text.indexOf(".");
  if (dot <= 0 || dot >= text.length - 1) return null;
  return {
    id: text,
    domain: text.substring(0, dot),
    objectId: text.substring(dot + 1),
  };
}

function titleFromEntityId(entityId) {
  var parsed = parseHomeAssistantEntity(entityId);
  if (!parsed) return entityId;
  return parsed.objectId.replace(/_/g, " ").replace(/\b\w/g, function (ch) {
    return ch.toUpperCase();
  });
}

function rememberEntityName(entityId, name) {
  var parsed = parseHomeAssistantEntity(entityId);
  if (!parsed || !name) return;
  if (!state.entityNames[parsed.id]) state.entityNames[parsed.id] = [];
  uniquePush(state.entityNames[parsed.id], String(name));
}

function rememberConfiguredButtonEntities(button) {
  if (!button) return;
  var label = button.label || "";
  if (button.entity) rememberEntityName(button.entity, label || titleFromEntityId(button.entity));
  if (button.sensor && parseHomeAssistantEntity(button.sensor)) {
    rememberEntityName(button.sensor, label || titleFromEntityId(button.sensor));
  }
  if (button.type === "action") {
    var stateEntity = actionCardStateEntity(button);
    if (stateEntity) rememberEntityName(stateEntity, titleFromEntityId(stateEntity));
  }
}

function rememberConfiguredEntities() {
  for (var i = 0; i < state.buttons.length; i++) rememberConfiguredButtonEntities(state.buttons[i]);
  for (var slot in state.subpages) {
    var sp = state.subpages[slot];
    if (!sp || !sp.buttons) continue;
    for (var bi = 0; bi < sp.buttons.length; bi++) rememberConfiguredButtonEntities(sp.buttons[bi]);
  }
  rememberEntityName(state.indoorEntity, "Indoor Temperature");
  rememberEntityName(state.outdoorEntity, "Outdoor Temperature");
  clockBarTemperatureEntities().forEach(function (entityId, index) {
    rememberEntityName(entityId, "Clock Bar Temperature " + (index + 1));
  });
  rememberEntityName(state.presenceEntity, "Presence Sensor");
  rememberEntityName(state.coverArtMediaPlayerEntity, "Media Player");
}

function optionLabelForEntity(entityId) {
  var names = state.entityNames[entityId] || [];
  if (!names.length) return titleFromEntityId(entityId);
  return names.join(" / ");
}

function entitySuggestions(domains) {
  rememberConfiguredEntities();
  var allowed = {};
  (domains || []).forEach(function (domain) { allowed[domain] = true; });
  var ids = [];
  for (var id in state.entityNames) {
    var parsed = parseHomeAssistantEntity(id);
    if (!parsed) continue;
    if (domains && domains.length && !allowed[parsed.domain]) continue;
    ids.push(id);
  }
  ids.sort(function (a, b) {
    var al = optionLabelForEntity(a).toLowerCase();
    var bl = optionLabelForEntity(b).toLowerCase();
    if (al === bl) return a.localeCompare(b);
    return al.localeCompare(bl);
  });
  return ids.map(function (id) {
    return { value: id, label: optionLabelForEntity(id) };
  });
}

function ensureEntityDropdown(input) {
  if (!input || input._entityDropdown || !input.parentNode) return;
  var wrap = document.createElement("div");
  wrap.className = "sp-entity-input-wrap";
  input.parentNode.insertBefore(wrap, input);
  wrap.appendChild(input);
  var dropdown = document.createElement("div");
  dropdown.className = "sp-entity-dropdown";
  wrap.appendChild(dropdown);
  input._entityDropdown = dropdown;
}

function closeEntityDropdown(input) {
  if (input && input._entityDropdown) input._entityDropdown.classList.remove("sp-open");
}

function refreshEntityDatalist(input) {
  if (!input) return;
  ensureEntityDropdown(input);
  var dropdown = input._entityDropdown;
  if (!dropdown) return;
  if (input._entitySuppressDropdown) {
    closeEntityDropdown(input);
    return;
  }
  dropdown.innerHTML = "";
  var query = String(input.value || "").trim().toLowerCase();
  var items = entitySuggestions(input._entityDomains || []).filter(function (item) {
    if (!query) return true;
    return item.value.toLowerCase().indexOf(query) !== -1 ||
      item.label.toLowerCase().indexOf(query) !== -1;
  }).slice(0, 12);
  items.forEach(function (item) {
    var option = document.createElement("button");
    option.type = "button";
    option.className = "sp-entity-option";
    option.textContent = item.value;
    option.addEventListener("mousedown", function (e) {
      e.preventDefault();
      input._entitySuppressDropdown = true;
      input.value = item.value;
      rememberEntityName(item.value, item.label || titleFromEntityId(item.value));
      input.dispatchEvent(new Event("input", { bubbles: true }));
      input.dispatchEvent(new Event("change", { bubbles: true }));
      closeEntityDropdown(input);
      input._entitySuppressDropdown = false;
    });
    dropdown.appendChild(option);
  });
  dropdown.classList.toggle("sp-open", document.activeElement === input && items.length > 0);
}

function attachEntitySuggestions(input, domains) {
  if (!input || input._entitySuggestionsAttached) return input;
  input._entityDomains = domains || [];
  input._entitySuggestionsAttached = true;
  input.addEventListener("focus", function () { refreshEntityDatalist(input); });
  input.addEventListener("input", function () {
    rememberEntityName(input.value, optionLabelForEntity(input.value));
    refreshEntityDatalist(input);
  });
  input.addEventListener("blur", function () {
    setTimeout(function () { closeEntityDropdown(input); }, 120);
  });
  input.addEventListener("keydown", function (e) {
    if (e.key === "Escape") closeEntityDropdown(input);
  });
  refreshEntityDatalist(input);
  return input;
}

function entityInput(id, value, placeholder, domains) {
  var el = textInput(id, value, placeholder);
  return attachEntitySuggestions(el, domains);
}

function rememberEntityPostPath(data) {
  var preferred = parseEntityId(data && data.name_id) || parseEntityId(data && data.id);
  if (data && data.domain && data.name) rememberEntityName(data.domain + "." + esphomeObjectId(data.name), data.name);
  if (!preferred || !preferred.path) return;
  entityStateKeys(data).forEach(function (key) {
    state.entityPostPaths[key] = preferred.path;
  });
  if (preferred.domain && preferred.name) state.entityPostPaths[preferred.domain + ":" + preferred.name] = preferred.path;
  if (preferred.domain && preferred.objectId) state.entityPostPaths[preferred.domain + ":" + preferred.objectId] = preferred.path;
}

function rememberedPostUrls(domain, name, objectIds, action) {
  var urls = [];
  var keys = [domain + ":" + name, domain + "-" + esphomeObjectId(name)];
  objectIds.forEach(function (objectId) {
    keys.push(domain + ":" + objectId);
    keys.push(domain + "-" + objectId);
  });
  keys.forEach(function (key) {
    if (state.entityPostPaths[key]) uniquePush(urls, state.entityPostPaths[key] + "/" + action);
  });
  return urls;
}

function hasRememberedPostPath(domain, name, objectIds) {
  var keys = [domain + ":" + name, domain + "-" + esphomeObjectId(name)];
  (objectIds || []).forEach(function (objectId) {
    keys.push(domain + ":" + objectId);
    keys.push(domain + "-" + objectId);
  });
  return keys.some(function (key) {
    return !!state.entityPostPaths[key];
  });
}

function entityPostUrls(domain, name, objectIds, action) {
  var urls = [];
  rememberedPostUrls(domain, name, objectIds || [], action).forEach(function (url) {
    uniquePush(urls, url);
  });
  (objectIds || []).forEach(function (objectId) {
    uniquePush(urls, "/" + domain + "/" + encodeURIComponent(objectId) + "/" + action);
  });
  uniquePush(urls, "/" + domain + "/" + encodeURIComponent(name) + "/" + action);
  uniquePush(urls, "/" + domain + "/" + encodeURIComponent(esphomeObjectId(name)) + "/" + action);
  return urls;
}
