import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installControlsFieldsModule(): GlobalDescriptors {
    // ── Settings helpers ───────────────────────────────────────────────────
    function makeCollapsibleCard(this: any, title?: any, bodyElement?: any, defaultCollapsed?: any, badgeElement?: any, actionElement?: any) {
        var card: any = document.createElement("div");
        card.className = "card";
        var header: any = document.createElement("div");
        header.className = "card-header";
        var h3: any = document.createElement("h3");
        h3.textContent = title;
        var rightWrap: any = document.createElement("div");
        rightWrap.className = "card-header-right";
        var chevron: any = createDisclosureChevron("card-chevron");
        if (badgeElement)
            rightWrap.appendChild(badgeElement);
        if (actionElement)
            rightWrap.appendChild(actionElement);
        rightWrap.appendChild(chevron);
        header.appendChild(h3);
        header.appendChild(rightWrap);
        var body: any = document.createElement("div");
        body.className = "card-body";
        body.appendChild(bodyElement);
        card.appendChild(header);
        card.appendChild(body);
        if (defaultCollapsed)
            card.classList.add("collapsed");
        header.onclick = function (this: any) { card.classList.toggle("collapsed"); };
        return card;
    }
    function fieldLabel(this: any, text?: any, forId?: any) {
        var el: any = document.createElement("label");
        el.className = "sp-field-label";
        el.textContent = text;
        if (forId)
            el.htmlFor = forId;
        return el;
    }
    function textInput(this: any, id?: any, value?: any, placeholder?: any) {
        var el: any = document.createElement("input");
        el.type = "text";
        el.className = "sp-input";
        if (id)
            el.id = id;
        el.value = value;
        el.placeholder = placeholder || "";
        return el;
    }
    function fieldWithControl(this: any, labelText?: any, inputId?: any, control?: any) {
        var field: any = document.createElement("div");
        field.className = "sp-field";
        field.appendChild(fieldLabel(labelText, inputId));
        if (control)
            field.appendChild(control);
        return field;
    }
    function optionValue(this: any, option?: any) {
        if (Array.isArray(option))
            return option[0];
        if (option && typeof option === "object")
            return option.value;
        return option;
    }
    function optionLabel(this: any, option?: any) {
        if (Array.isArray(option))
            return option[1];
        if (option && typeof option === "object")
            return option.label;
        return option;
    }
    function selectField(this: any, labelText?: any, inputId?: any, options?: any, value?: any, onChange?: any) {
        var select: any = document.createElement("select");
        select.className = "sp-select";
        if (inputId)
            select.id = inputId;
        (options || []).forEach(function (this: any, entry?: any) {
            var option: any = document.createElement("option");
            option.value = optionValue(entry);
            option.textContent = optionLabel(entry);
            select.appendChild(option);
        });
        select.value = value == null ? "" : String(value);
        if (onChange)
            select.addEventListener("change", onChange);
        return {
            field: fieldWithControl(labelText, inputId, select),
            select: select,
        };
    }
    function segmentControl(this: any, options?: any, value?: any, onSelect?: any, className?: any) {
        var segment: any = document.createElement("div");
        segment.className = className || "sp-segment";
        var buttons: any = {};
        (options || []).forEach(function (this: any, entry?: any) {
            var optValue: any = optionValue(entry);
            var button: any = document.createElement("button");
            button.type = "button";
            button.textContent = optionLabel(entry);
            button.classList.toggle("active", optValue === value);
            button.addEventListener("click", function (this: any) {
                for (var key in buttons)
                    buttons[key].classList.toggle("active", key === optValue);
                if (onSelect)
                    onSelect(optValue, button);
            });
            segment.appendChild(button);
            buttons[optValue] = button;
        });
        return {
            segment: segment,
            buttons: buttons,
        };
    }
    function disclosureSection(this: any, labelText?: any, inputId?: any, open?: any) {
        var panel: any = document.createElement("div");
        panel.className = "sp-disclosure" + (open ? " sp-open" : "");
        var button: any = document.createElement("button");
        button.type = "button";
        button.className = "sp-disclosure-button";
        if (inputId)
            button.id = inputId;
        button.setAttribute("aria-expanded", open ? "true" : "false");
        var label: any = document.createElement("span");
        label.textContent = labelText;
        button.appendChild(label);
        button.appendChild(createDisclosureChevron("sp-disclosure-chevron"));
        var section: any = document.createElement("div");
        section.className = "sp-disclosure-body";
        button.addEventListener("click", function (this: any) {
            open = !panel.classList.contains("sp-open");
            panel.classList.toggle("sp-open", open);
            button.setAttribute("aria-expanded", open ? "true" : "false");
        });
        panel.appendChild(button);
        panel.appendChild(section);
        return {
            panel: panel,
            button: button,
            section: section,
        };
    }
    function colorField(this: any, id?: any, value?: any, onChange?: any) {
        var row: any = document.createElement("div");
        row.className = "sp-color-row";
        var swatch: any = document.createElement("div");
        swatch.className = "sp-color-swatch";
        swatch.style.backgroundColor = "#" + (value.length === 6 ? value : "000000");
        var picker: any = document.createElement("input");
        picker.type = "color";
        picker.value = "#" + (value.length === 6 ? value : "000000");
        swatch.appendChild(picker);
        row.appendChild(swatch);
        var inp: any = document.createElement("input");
        inp.type = "text";
        inp.className = "sp-input";
        inp.id = id;
        inp.value = value;
        inp.placeholder = "6-digit hex e.g. 0073FF";
        row.appendChild(inp);
        picker.addEventListener("input", function (this: any) {
            var hex: any = this.value.replace("#", "").toUpperCase();
            inp.value = hex;
            swatch.style.backgroundColor = "#" + hex;
            onChange(hex);
        });
        inp.addEventListener("blur", function (this: any) {
            var hex: any = this.value.replace(/^#/, "").toUpperCase();
            if (/^[0-9A-F]{6}$/i.test(hex)) {
                swatch.style.backgroundColor = "#" + hex;
                picker.value = "#" + hex;
            }
            onChange(hex);
        });
        inp.addEventListener("keydown", function (this: any, e?: any) {
            if (e.key === "Enter")
                this.blur();
        });
        row._syncColor = function (this: any, hex?: any) {
            if (document.activeElement !== inp)
                inp.value = hex;
            swatch.style.backgroundColor = "#" + (hex.length === 6 ? hex : "000000");
            picker.value = "#" + (hex.length === 6 ? hex : "000000");
        };
        return row;
    }
    function toggleRow(this: any, label?: any, id?: any, checked?: any) {
        var row: any = document.createElement("div");
        row.className = "sp-toggle-row";
        var lbl: any = document.createElement("label");
        lbl.className = "sp-toggle-label";
        lbl.htmlFor = id;
        lbl.textContent = label;
        row.appendChild(lbl);
        var toggle: any = document.createElement("label");
        toggle.className = "sp-toggle";
        var inp: any = document.createElement("input");
        inp.type = "checkbox";
        inp.id = id;
        inp.checked = !!checked;
        var track: any = document.createElement("span");
        track.className = "sp-toggle-track";
        toggle.appendChild(inp);
        toggle.appendChild(track);
        row.appendChild(toggle);
        return { row: row, input: inp };
    }
    function cardMetadataValue(this: any, value?: any, b?: any, helpers?: any) {
        return typeof value === "function" ? value(b, helpers) : value;
    }
    function cardLargeNumbersSupportsCardSize(this: any, b?: any, helpers?: any, metadata?: any) {
        helpers = helpers || {};
        metadata = metadata || {};
        var large: any = metadata.largeNumbers || {};
        var cardSize: any = helpers.cardSize || 1;
        if (large.supportedCardSizes) {
            return large.supportedCardSizes.indexOf(cardSize) !== -1;
        }
        if (large.supportedCardSize) {
            return !!cardMetadataValue(large.supportedCardSize, b, helpers);
        }
        return cardSize === CARD_SIZE_LARGE;
    }
    function cardLargeNumbersMetadata(this: any, b?: any) {
        var typeDef: any = BUTTON_TYPES[(b && b.type) || ""] || null;
        return typeDef && typeDef.cardMetadata ? typeDef.cardMetadata : {};
    }
    function cardLargeNumbersActiveForCardSize(this: any, b?: any, helpers?: any, metadata?: any) {
        helpers = helpers || {};
        if (!cardLargeNumbersSupported(b) ||
            !cardLargeNumbersSupportsCardSize(b, helpers, metadata || cardLargeNumbersMetadata(b))) {
            return false;
        }
        if (largeNumbersExplicitlyDisabled(b && b.options))
            return false;
        return (helpers.cardSize || CARD_SIZE_SINGLE) === CARD_SIZE_LARGE || cardLargeNumbersEnabled(b);
    }
    function cardLargeNumbersHidePreviewLabel(this: any, b?: any, helpers?: any, metadata?: any) {
        if (!cardLargeNumbersActiveForCardSize(b, helpers, metadata))
            return false;
        metadata = metadata || cardLargeNumbersMetadata(b);
        var large: any = metadata.largeNumbers || {};
        var cardSize: any = (helpers && helpers.cardSize) || 1;
        if (large.hideLabelCardSizes)
            return large.hideLabelCardSizes.indexOf(cardSize) !== -1;
        if (large.hideLabel)
            return !!cardMetadataValue(large.hideLabel, b, helpers || {});
        return false;
    }
    function applyCardMetadataFields(this: any, b?: any, helpers?: any, fields?: any) {
        fields = fields || {};
        for (var key in fields) {
            var value: any = cardMetadataValue(fields[key], b, helpers);
            b[key] = value;
            helpers.saveField(key, value);
        }
    }
    function renderCardModeSelector(this: any, panel?: any, b?: any, helpers?: any, metadata?: any) {
        metadata = metadata || {};
        var mode: any = metadata.mode || {};
        var field: any = helpers.selectField(mode.label || "Type", helpers.idPrefix + (mode.idSuffix || "mode"), cardMetadataValue(mode.options, b, helpers) || [], cardMetadataValue(mode.value, b, helpers) || "", function (this: any) {
            if (mode.onChange)
                mode.onChange.call(this, b, helpers);
        });
        panel.appendChild(field.field);
        return field;
    }
    function renderCardLargeNumbersToggle(this: any, panel?: any, b?: any, helpers?: any, metadata?: any) {
        metadata = metadata || {};
        var large: any = metadata.largeNumbers || {};
        if (!large.showSettingForAnyCardSize && !cardLargeNumbersSupportsCardSize(b, helpers, metadata))
            return null;
        if (large.isVisible && !large.isVisible(b, helpers))
            return null;
        var toggle: any = helpers.toggleRow(cardMetadataValue(large.label, b, helpers) || "Large Numbers", helpers.idPrefix + (large.idSuffix || "large-numbers"), cardLargeNumbersActiveForCardSize(b, helpers, metadata));
        panel.appendChild(toggle.row);
        toggle.input.addEventListener("change", function (this: any) {
            setSensorLargeNumbersEnabled(b, this.checked);
            helpers.saveField("options", b.options);
            if (large.onChange)
                large.onChange.call(this, b, helpers);
        });
        return toggle;
    }
    function syncCardLargeNumbersToggle(this: any, toggle?: any, b?: any, helpers?: any, visible?: any) {
        if (!toggle)
            return;
        toggle.row.style.display = visible ? "" : "none";
        if (!visible && (cardLargeNumbersEnabled(b) || largeNumbersExplicitlyDisabled(b && b.options))) {
            b.options = setConfigOption(b.options, SENSOR_LARGE_NUMBERS_OPTION, false);
            b.options = setConfigOptionValue(b.options, SENSOR_LARGE_NUMBERS_OPTION, "");
            toggle.input.checked = false;
            helpers.saveField("options", b.options);
        }
    }
    // ── Calendar list editor ───────────────────────────────────────────
    // One row per calendar: an entity box plus the colour its events take on
    // the panel. Serialises to the "calendar.family:#66BB6A, calendar.work"
    // form the firmware parses, so the stored value stays a plain string.
    // Positions match the firmware's default palette in
    // button_grid_agenda_cards.h.
    var AGENDA_CALENDAR_PALETTE: any = ["66BB6A", "EF5350", "42A5F5", "AB47BC", "FFB300", "26C6DA"];
    // The first six are the firmware's own palette, so the presets a calendar
    // is offered match the colours the panel assigns by position.
    var AGENDA_COLOR_PRESETS: any = [
        { name: "Green", hex: "66BB6A" },
        { name: "Red", hex: "EF5350" },
        { name: "Blue", hex: "42A5F5" },
        { name: "Purple", hex: "AB47BC" },
        { name: "Amber", hex: "FFB300" },
        { name: "Cyan", hex: "26C6DA" },
        { name: "Pink", hex: "EC407A" },
        { name: "Indigo", hex: "5C6BC0" },
        { name: "Teal", hex: "26A69A" },
        { name: "Orange", hex: "FF7043" },
        { name: "Lime", hex: "9CCC65" },
        { name: "Slate", hex: "90A4AE" },
    ];
    var agendaColorMenuCleanup: any = null;
    function closeAgendaColorMenu(this: any) {
        if (agendaColorMenuCleanup) {
            agendaColorMenuCleanup();
            agendaColorMenuCleanup = null;
        }
        var open: any = document.querySelector(".sp-cal-colors");
        if (open && open.parentNode)
            open.parentNode.removeChild(open);
    }
    // The menu lives on document.body with fixed positioning: the settings
    // modal scrolls its own content, and would otherwise clip it.
    function openAgendaColorMenu(this: any, anchor?: any, current?: any, onPick?: any) {
        closeAgendaColorMenu();
        var menu: any = document.createElement("div");
        menu.className = "sp-cal-colors";
        var grid: any = document.createElement("div");
        grid.className = "sp-cal-colors-grid";
        AGENDA_COLOR_PRESETS.forEach(function (this: any, preset?: any) {
            var dot: any = document.createElement("button");
            dot.type = "button";
            dot.className = "sp-cal-color" +
                (String(current || "").toUpperCase() === preset.hex ? " sp-selected" : "");
            dot.style.backgroundColor = "#" + preset.hex;
            dot.title = preset.name;
            dot.addEventListener("click", function (this: any) {
                onPick(preset.hex);
                closeAgendaColorMenu();
            });
            grid.appendChild(dot);
        });
        menu.appendChild(grid);
        var customRow: any = document.createElement("label");
        customRow.className = "sp-cal-custom";
        customRow.textContent = "Custom";
        var custom: any = document.createElement("input");
        custom.type = "color";
        custom.value = "#" + (String(current || "").length === 6 ? current : "66BB6A");
        custom.addEventListener("input", function (this: any) {
            onPick(this.value.replace("#", "").toUpperCase());
        });
        customRow.appendChild(custom);
        menu.appendChild(customRow);
        document.body.appendChild(menu);
        var rect: any = anchor.getBoundingClientRect();
        var below: any = (window.innerHeight || 0) - rect.bottom;
        // Measure after insertion rather than assuming a width, so the clamp
        // keeps the menu on screen whatever the theme sizes it to.
        var width: any = menu.offsetWidth || 200;
        menu.style.left =
            Math.max(8, Math.min(rect.left, (window.innerWidth || 0) - width - 8)) + "px";
        if (below < 180 && rect.top > below)
            menu.style.bottom = ((window.innerHeight || 0) - rect.top + 6) + "px";
        else
            menu.style.top = (rect.bottom + 6) + "px";
        function onDocumentMouseDown(this: any, e?: any) {
            if (!menu.contains(e.target))
                closeAgendaColorMenu();
        }
        // Capture, so Escape dismisses this menu before the document-level
        // handler that would close the whole settings dialog.
        function onKeyDown(this: any, e?: any) {
            if (e.key !== "Escape")
                return;
            e.stopPropagation();
            closeAgendaColorMenu();
        }
        // Fixed positioning does not follow the modal's own scrolling.
        function onViewportChange(this: any) {
            closeAgendaColorMenu();
        }
        // Deferred: this very click would otherwise close the menu at once.
        setTimeout(function (this: any) {
            document.addEventListener("mousedown", onDocumentMouseDown);
        }, 0);
        document.addEventListener("keydown", onKeyDown, true);
        window.addEventListener("scroll", onViewportChange, true);
        window.addEventListener("resize", onViewportChange);
        agendaColorMenuCleanup = function (this: any) {
            document.removeEventListener("mousedown", onDocumentMouseDown);
            document.removeEventListener("keydown", onKeyDown, true);
            window.removeEventListener("scroll", onViewportChange, true);
            window.removeEventListener("resize", onViewportChange);
        };
    }
    function parseCalendarList(this: any, value?: any) {
        var rows: any = [];
        String(value || "").split(",").forEach(function (this: any, part?: any) {
            var text: any = String(part || "").trim();
            if (!text)
                return;
            var at: any = text.indexOf(":#");
            var entity: any = at === -1 ? text : text.slice(0, at).trim();
            var color: any = at === -1 ? "" : text.slice(at + 2).trim().toUpperCase();
            if (!/^[0-9A-F]{6}$/.test(color))
                color = "";
            rows.push({ entity: entity, color: color });
        });
        return rows;
    }
    function serializeCalendarList(this: any, rows?: any) {
        return rows.filter(function (this: any, row?: any) {
            return String(row.entity || "").trim();
        }).map(function (this: any, row?: any) {
            var entity: any = String(row.entity).trim();
            return row.color ? entity + ":#" + row.color : entity;
        }).join(", ");
    }
    function calendarPaletteColor(this: any, index?: any) {
        return AGENDA_CALENDAR_PALETTE[index % AGENDA_CALENDAR_PALETTE.length];
    }
    // Calendars are few, so the picker lists them by friendly name rather than
    // asking anyone to type an entity id. Where the Home Assistant entity
    // lookup is available it fills this list; otherwise it falls back to the
    // calendars already configured on the panel.
    function refreshKnownCalendars(this: any) {
        var refresh: any = (globalThis as any).refreshHaEntitiesIfStale;
        if (typeof refresh === "function")
            refresh();
        return entitySuggestions(["calendar"]);
    }
    function fillCalendarOptions(this: any, select?: any, current?: any) {
        var known: any = refreshKnownCalendars();
        var seen: any = {};
        select.innerHTML = "";
        var placeholder: any = document.createElement("option");
        placeholder.value = "";
        placeholder.textContent = known.length ? "Select a calendar\u2026" : "No calendars found";
        select.appendChild(placeholder);
        known.forEach(function (this: any, item?: any) {
            var option: any = document.createElement("option");
            option.value = item.value;
            option.textContent = item.label || titleFromEntityId(item.value);
            seen[item.value] = true;
            select.appendChild(option);
        });
        // Keep a configured calendar selectable even when it is not in the
        // known list yet, so opening the editor cannot silently drop it.
        if (current && !seen[current]) {
            var kept: any = document.createElement("option");
            kept.value = current;
            kept.textContent = titleFromEntityId(current);
            select.appendChild(kept);
        }
        select.value = current || "";
    }
    function calendarListField(this: any, idPrefix?: any, value?: any, onChange?: any) {
        var rows: any = parseCalendarList(value);
        // Always keep one row on screen: it gives the picker an empty state to
        // type into, and a real input for required-field validation to target.
        if (!rows.length)
            rows.push({ entity: "", color: "" });
        var list: any = document.createElement("div");
        list.className = "sp-cal-list";
        var firstInput: any = null;
        function commit(this: any) {
            if (onChange)
                onChange(serializeCalendarList(rows));
        }
        function render(this: any) {
            list.innerHTML = "";
            firstInput = null;
            rows.forEach(function (this: any, row?: any, index?: any) {
                var line: any = document.createElement("div");
                line.className = "sp-cal-row";
                var input: any = document.createElement("select");
                input.className = "sp-input";
                input.id = index === 0 ? idPrefix : idPrefix + "-" + index;
                fillCalendarOptions(input, row.entity);
                line.appendChild(input);
                if (index === 0)
                    firstInput = input;
                // Repopulate on open so calendars discovered after this row was
                // drawn still appear.
                input.addEventListener("mousedown", function (this: any) {
                    fillCalendarOptions(input, row.entity);
                });
                input.addEventListener("change", function (this: any) {
                    row.entity = this.value;
                    // A calendar with no colour yet takes its palette slot, so
                    // the swatch always shows what the panel will draw.
                    if (!row.color && String(this.value || "").trim())
                        row.color = calendarPaletteColor(index);
                    swatch.style.backgroundColor = "#" + (row.color || calendarPaletteColor(index));
                    commit();
                });

                var swatch: any = document.createElement("button");
                swatch.type = "button";
                swatch.className = "sp-cal-swatch";
                swatch.title = "Event colour";
                swatch.style.backgroundColor = "#" + (row.color || calendarPaletteColor(index));
                swatch.addEventListener("click", function (this: any) {
                    openAgendaColorMenu(swatch, row.color || calendarPaletteColor(index),
                        function (this: any, hex?: any) {
                            row.color = hex;
                            swatch.style.backgroundColor = "#" + hex;
                            commit();
                        });
                });
                line.appendChild(swatch);

                var remove: any = document.createElement("button");
                remove.type = "button";
                remove.className = "sp-cal-remove";
                remove.textContent = "\u00D7";
                remove.title = "Remove calendar";
                remove.addEventListener("click", function (this: any) {
                    if (rows.length > 1)
                        rows.splice(index, 1);
                    else
                        rows[0] = { entity: "", color: "" };
                    render();
                    commit();
                });
                line.appendChild(remove);
                list.appendChild(line);
            });
        }
        render();

        var field: any = document.createElement("div");
        field.className = "sp-field";
        field.appendChild(fieldLabel("Calendars", idPrefix));
        field.appendChild(list);
        var add: any = document.createElement("button");
        add.type = "button";
        add.className = "sp-cal-add";
        add.textContent = "+ Add calendar";
        add.addEventListener("click", function (this: any) {
            rows.push({ entity: "", color: "" });
            render();
        });
        field.appendChild(add);
        return {
            field: field,
            list: list,
            firstInput: function (this: any) { return firstInput; },
            // Rebuild from a device-pushed value. Skipped while a row has
            // focus so an incoming update cannot yank the field mid-edit.
            setValue: function (this: any, next?: any) {
                if (list.contains(document.activeElement))
                    return;
                if (serializeCalendarList(rows) === String(next || "").trim())
                    return;
                rows = parseCalendarList(next);
                if (!rows.length)
                    rows.push({ entity: "", color: "" });
                render();
            },
        };
    }
    function renderCardEntityField(this: any, panel?: any, b?: any, helpers?: any, metadata?: any) {
        metadata = metadata || {};
        var entity: any = metadata.entity || {};
        var bindName: any = Object.prototype.hasOwnProperty.call(entity, "bindName") ? entity.bindName : "entity";
        var value: any = entity.value != null ? cardMetadataValue(entity.value, b, helpers) : (bindName ? b[bindName] : "");
        var domains: any = cardMetadataValue(entity.domains, b, helpers) || [];
        var field: any = helpers.entityField(cardMetadataValue(entity.label, b, helpers) || "Entity", helpers.idPrefix + (entity.idSuffix || "entity"), value || "", cardMetadataValue(entity.placeholder, b, helpers) || "", domains, bindName, entity.rerender !== false, cardMetadataValue(entity.requiredMessage, b, helpers) || "", !!entity.multi);
        panel.appendChild(field.field);
        return field;
    }
    function renderCardTextField(this: any, panel?: any, b?: any, helpers?: any, metadata?: any) {
        metadata = metadata || {};
        var text: any = metadata.text || metadata;
        var hasBindName: any = Object.prototype.hasOwnProperty.call(text, "bindName");
        var bindName: any = hasBindName ? text.bindName : (text.field || "label");
        var value: any = text.value != null ? cardMetadataValue(text.value, b, helpers) : b[bindName];
        var control: any = helpers.textField(text.label || "Label", helpers.idPrefix + (text.idSuffix || bindName), value || "", cardMetadataValue(text.placeholder, b, helpers) || "", bindName, text.rerender !== false);
        panel.appendChild(control.field);
        return control;
    }
    function renderCardNumberField(this: any, panel?: any, b?: any, helpers?: any, metadata?: any) {
        metadata = metadata || {};
        var number: any = metadata.number || metadata;
        var inputId: any = helpers.idPrefix + (number.idSuffix || "number");
        var field: any = document.createElement("div");
        field.className = "sp-field";
        field.appendChild(helpers.fieldLabel(number.label || "Number", inputId));
        var input: any = document.createElement("input");
        input.type = "number";
        input.className = "sp-input";
        input.id = inputId;
        if (number.min != null)
            input.min = String(number.min);
        if (number.max != null)
            input.max = String(number.max);
        if (number.step != null)
            input.step = String(number.step);
        input.placeholder = number.placeholder || "";
        input.value = cardMetadataValue(number.value, b, helpers) || "";
        field.appendChild(input);
        panel.appendChild(field);
        return { field: field, input: input };
    }
    function renderCardIconPicker(this: any, panel?: any, b?: any, helpers?: any, metadata?: any) {
        metadata = metadata || {};
        var icon: any = metadata.icon || metadata;
        var fieldName: any = icon.field || "icon";
        var fallback: any = cardMetadataValue(icon.fallback, b, helpers) || "Auto";
        var currentValue: any = icon.value != null ? cardMetadataValue(icon.value, b, helpers) : (b[fieldName] || fallback);
        var picker: any = helpers.iconPickerField(helpers.idPrefix + (icon.pickerIdSuffix || fieldName + "-picker"), helpers.idPrefix + (icon.idSuffix || fieldName), currentValue, function (this: any, opt?: any) {
            b[fieldName] = opt || fallback;
            helpers.saveField(fieldName, b[fieldName]);
            if (icon.onChange)
                icon.onChange(b, helpers, b[fieldName]);
        }, icon.label || "Icon");
        panel.appendChild(picker);
        return picker;
    }
    function renderCardOptionToggle(this: any, panel?: any, b?: any, helpers?: any, metadata?: any) {
        metadata = metadata || {};
        var toggle: any = metadata.toggle || metadata;
        var row: any = helpers.toggleRow(toggle.label || "Enabled", helpers.idPrefix + (toggle.idSuffix || "toggle"), !!cardMetadataValue(toggle.checked, b, helpers));
        panel.appendChild(row.row);
        row.input.addEventListener("change", function (this: any) {
            if (toggle.onChange)
                toggle.onChange.call(this, b, helpers, this.checked);
        });
        return row;
    }
    function renderCardIconPair(this: any, panel?: any, b?: any, helpers?: any, offMetadata?: any, onMetadata?: any) {
        return {
            off: helpers.renderCardIconPicker(panel, b, helpers, offMetadata),
            on: helpers.renderCardIconPicker(panel, b, helpers, onMetadata),
        };
    }
    function renderCardActiveColorToggle(this: any, panel?: any, b?: any, helpers?: any, metadata?: any, setEnabled?: any) {
        return helpers.renderCardOptionToggle(panel, b, helpers, Object.assign({}, metadata, {
            onChange: function (this: any, button?: any, cardHelpers?: any, checked?: any) {
                setEnabled(button, checked);
                cardHelpers.saveField("options", button.options);
            },
        }));
    }
    function renderBasicCardFields(this: any, panel?: any, b?: any, helpers?: any, metadata?: any, options?: any) {
        options = options || {};
        if (options.entity !== false && metadata.entity) {
            helpers.renderCardEntityField(panel, b, helpers, metadata);
        }
        if (options.label !== false && metadata.labelField) {
            helpers.renderCardTextField(panel, b, helpers, metadata.labelField);
        }
        if (options.icon !== false && metadata.icon) {
            helpers.renderCardIconPicker(panel, b, helpers, metadata.icon);
        }
        if (options.iconPair !== false && (metadata.iconOff || metadata.iconOn)) {
            renderCardIconPair(panel, b, helpers, metadata.iconOff, metadata.iconOn);
        }
    }
    function renderCardSegmentControl(this: any, panel?: any, b?: any, helpers?: any, metadata?: any) {
        metadata = metadata || {};
        var segment: any = metadata.segment || metadata;
        var control: any = helpers.segmentControl(segment.options || [], cardMetadataValue(segment.value, b, helpers) || "", function (this: any, value?: any, button?: any) {
            if (segment.onSelect)
                segment.onSelect(b, helpers, value, button, control);
        });
        panel.appendChild(helpers.fieldWithControl(segment.label || "Type", segment.inputId || null, control.segment));
        return control;
    }
    function cardSensorPreviewHtml(this: any, b?: any, helpers?: any, value?: any, unit?: any, extraClass?: any, valueClass?: any) {
        var className: any = "sp-sensor-preview" + (extraClass ? " " + extraClass : "") +
            (cardLargeNumbersActiveForCardSize(b, helpers) ? " sp-sensor-preview-large" : "");
        return '<span class="' + className + '">' +
            '<span class="sp-sensor-value' + (valueClass ? " " + valueClass : "") + '">' + helpers.escHtml(value) + '</span>' +
            (unit != null ? '<span class="sp-sensor-unit">' + helpers.escHtml(unit) + '</span>' : "") +
            '</span>';
    }
    function cardBadgeLabelHtml(this: any, helpers?: any, label?: any, badgeIcon?: any) {
        return '<span class="sp-btn-label-row"><span class="sp-btn-label">' +
            helpers.escHtml(label) +
            '</span><span class="sp-type-badge mdi mdi-' + badgeIcon + '"></span></span>';
    }
    function cardIconHtml(this: any, iconSlugName?: any, extraHtml?: any) {
        return '<span class="sp-btn-icon mdi mdi-' + iconSlugName + '"></span>' + (extraHtml || "");
    }
    function cardIconSlug(this: any, b?: any, helpers?: any, fallback?: any, field?: any) {
        field = field || "icon";
        var value: any = b && b[field];
        if (value && value !== "Auto")
            return iconSlug(value);
        return iconSlug(cardMetadataValue(fallback, b, helpers) || "Auto");
    }
    function cardBadgePreview(this: any, b?: any, helpers?: any, options?: any) {
        options = options || {};
        return {
            iconHtml: cardIconHtml(cardIconSlug(b, helpers, options.iconFallback, options.iconField), options.iconExtraHtml || ""),
            labelHtml: cardBadgeLabelHtml(helpers, options.label || "Configure", options.badge),
        };
    }
    function condField(this: any) {
        var el: any = document.createElement("div");
        el.className = "sp-cond-field";
        return el;
    }
    function createRangeSlider(this: any, label?: any, initial?: any, postName?: any) {
        var wrap: any = document.createElement("div");
        wrap.className = "sp-field";
        wrap.appendChild(fieldLabel(label));
        var row: any = document.createElement("div");
        row.className = "sp-range-row";
        var range: any = document.createElement("input");
        range.type = "range";
        range.className = "sp-range";
        range.min = "10";
        range.max = "100";
        range.step = "5";
        range.value = String(initial);
        var val: any = document.createElement("span");
        val.className = "sp-range-val";
        val.textContent = initial + "%";
        range.addEventListener("input", function (this: any) { val.textContent = this.value + "%"; });
        range.addEventListener("change", function (this: any) {
            if (typeof postName === "function")
                postName(this.value);
            else if (postName)
                postNumber(postName, this.value);
        });
        row.appendChild(range);
        row.appendChild(val);
        wrap.appendChild(row);
        return { wrap: wrap, range: range, val: val };
    }
    return {
        "makeCollapsibleCard": staticGlobal(makeCollapsibleCard),
        "fieldLabel": staticGlobal(fieldLabel),
        "textInput": staticGlobal(textInput),
        "fieldWithControl": staticGlobal(fieldWithControl),
        "optionValue": staticGlobal(optionValue),
        "optionLabel": staticGlobal(optionLabel),
        "selectField": staticGlobal(selectField),
        "segmentControl": staticGlobal(segmentControl),
        "disclosureSection": staticGlobal(disclosureSection),
        "colorField": staticGlobal(colorField),
        "toggleRow": staticGlobal(toggleRow),
        "cardMetadataValue": staticGlobal(cardMetadataValue),
        "cardLargeNumbersSupportsCardSize": staticGlobal(cardLargeNumbersSupportsCardSize),
        "cardLargeNumbersMetadata": staticGlobal(cardLargeNumbersMetadata),
        "cardLargeNumbersActiveForCardSize": staticGlobal(cardLargeNumbersActiveForCardSize),
        "cardLargeNumbersHidePreviewLabel": staticGlobal(cardLargeNumbersHidePreviewLabel),
        "applyCardMetadataFields": staticGlobal(applyCardMetadataFields),
        "renderCardModeSelector": staticGlobal(renderCardModeSelector),
        "renderCardLargeNumbersToggle": staticGlobal(renderCardLargeNumbersToggle),
        "syncCardLargeNumbersToggle": staticGlobal(syncCardLargeNumbersToggle),
        "calendarListField": staticGlobal(calendarListField),
        "parseCalendarList": staticGlobal(parseCalendarList),
        "serializeCalendarList": staticGlobal(serializeCalendarList),
        "renderCardEntityField": staticGlobal(renderCardEntityField),
        "renderCardTextField": staticGlobal(renderCardTextField),
        "renderCardNumberField": staticGlobal(renderCardNumberField),
        "renderCardIconPicker": staticGlobal(renderCardIconPicker),
        "renderCardOptionToggle": staticGlobal(renderCardOptionToggle),
        "renderCardIconPair": staticGlobal(renderCardIconPair),
        "renderCardActiveColorToggle": staticGlobal(renderCardActiveColorToggle),
        "renderBasicCardFields": staticGlobal(renderBasicCardFields),
        "renderCardSegmentControl": staticGlobal(renderCardSegmentControl),
        "cardSensorPreviewHtml": staticGlobal(cardSensorPreviewHtml),
        "cardBadgeLabelHtml": staticGlobal(cardBadgeLabelHtml),
        "cardIconHtml": staticGlobal(cardIconHtml),
        "cardIconSlug": staticGlobal(cardIconSlug),
        "cardBadgePreview": staticGlobal(cardBadgePreview),
        "condField": staticGlobal(condField),
        "createRangeSlider": staticGlobal(createRangeSlider),
    };
}
