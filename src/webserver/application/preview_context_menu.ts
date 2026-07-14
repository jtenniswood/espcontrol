import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installPreviewContextMenuModule(): GlobalDescriptors {
    // ── Preview Context Menu ──────────────────────────────────────────
    // ── Context menu (unified) ─────────────────────────────────────────────
    var ctxMenu: any = null;
    function positionMenu(this: any, menu?: any, e?: any) {
        var position: any = PreviewFeature.clampMenuPosition({ x: e.clientX, y: e.clientY }, menu.offsetWidth, menu.offsetHeight, window.innerWidth, window.innerHeight);
        menu.style.left = position.x + "px";
        menu.style.top = position.y + "px";
    }
    function addCtxItem(this: any, icon?: any, text?: any, handler?: any, danger?: any) {
        var item: any = document.createElement("div");
        item.className = "sp-ctx-item" + (danger ? " sp-ctx-danger" : "");
        item.appendChild(mdiIcon(icon));
        item.appendChild(document.createTextNode(text));
        item.addEventListener("mousedown", function (this: any, ev?: any) {
            ev.preventDefault();
            ev.stopPropagation();
            hideContextMenu();
            handler();
        });
        ctxMenu.appendChild(item);
    }
    function addCtxDivider(this: any) {
        var div: any = document.createElement("div");
        div.className = "sp-ctx-divider";
        ctxMenu.appendChild(div);
    }
    function addCtxSubmenu(this: any, icon?: any, text?: any, buildFn?: any) {
        var wrapper: any = document.createElement("div");
        wrapper.className = "sp-ctx-item sp-ctx-sub";
        wrapper.appendChild(mdiIcon(icon));
        wrapper.appendChild(document.createTextNode(text));
        var sub: any = document.createElement("div");
        sub.className = "sp-ctx-submenu";
        buildFn(sub);
        wrapper.appendChild(sub);
        wrapper.addEventListener("mouseenter", function (this: any) {
            sub.style.left = "100%";
            sub.style.right = "auto";
            var r: any = sub.getBoundingClientRect();
            if (r.right > window.innerWidth - 4) {
                sub.style.left = "auto";
                sub.style.right = "100%";
            }
        });
        wrapper.addEventListener("mousedown", function (this: any, ev?: any) { ev.preventDefault(); ev.stopPropagation(); });
        ctxMenu.appendChild(wrapper);
    }
    function addSubItem(this: any, container?: any, icon?: any, text?: any, handler?: any, active?: any) {
        var item: any = document.createElement("div");
        item.className = "sp-ctx-item";
        if (active) {
            item.appendChild(mdiIcon("check", "sp-ctx-check mdi"));
        }
        else {
            var spacer: any = document.createElement("span");
            spacer.style.width = "18px";
            item.appendChild(spacer);
        }
        item.appendChild(document.createTextNode(text));
        item.addEventListener("mousedown", function (this: any, ev?: any) {
            ev.preventDefault();
            ev.stopPropagation();
            hideContextMenu();
            handler();
        });
        container.appendChild(item);
    }
    function resizeSlot(this: any, slot?: any, targetSz?: any) {
        if (isConfigLocked())
            return;
        var c: any = ctx();
        var slotPos: any = slot === -2 ? c.grid.indexOf(-2) : c.grid.indexOf(slot);
        if (slotPos < 0)
            return;
        var button: any = slot === -2 ? null : c.buttons[slot - 1];
        targetSz = normalizeCardSizeForConfig(button, targetSz);
        var curSz: any = c.sizes[slot] || 1;
        if (curSz === targetSz)
            return;
        var oldCells: any = coveredCells(slotPos, curSz, c.maxSlots, false);
        for (var oi: any = 0; oi < oldCells.length; oi++) {
            if (c.grid[oldCells[oi]] === -1)
                c.grid[oldCells[oi]] = 0;
        }
        if (targetSz > 1 && !sizeFitsAt(slotPos, targetSz, c.maxSlots)) {
            delete c.sizes[slot];
            return;
        }
        var need: any = coveredCells(slotPos, targetSz, c.maxSlots, false);
        for (var i: any = 0; i < need.length; i++) {
            var p: any = need[i];
            if (c.grid[p] > 0 || c.grid[p] === -2) {
                if (c.isSub && c.grid[p] > 0)
                    return;
                var displaced: any = c.grid[p];
                c.grid[p] = 0;
                if (c.isSub) {
                    for (var j: any = 0; j < c.maxSlots; j++) {
                        if (c.grid[j] === 0 && need.indexOf(j) === -1) {
                            c.grid[j] = displaced;
                            break;
                        }
                    }
                }
                else {
                    var fc: any = firstFreeCell(p + 1);
                    if (fc >= 0)
                        c.grid[fc] = displaced;
                }
            }
        }
        for (var i: any = 0; i < need.length; i++)
            c.grid[need[i]] = -1;
        if (targetSz === 1)
            delete c.sizes[slot];
        else
            c.sizes[slot] = targetSz;
        if (c.isSub) {
            var sp: any = getSubpage(state.editingSubpage);
            sp.order = serializeSubpageGrid(sp);
            saveSubpageConfig(state.editingSubpage);
        }
        else {
            postText(entityName("button_order"), serializeGrid(state.grid));
        }
        renderPreview();
        renderButtonSettings();
    }
    function addBulkCardMenuItems(this: any, slots?: any) {
        addCtxItem("clipboard-outline", "Copy " + slots.length + " Cards", function (this: any) { copyButtons(slots); });
        addCtxItem("content-cut", "Cut " + slots.length + " Cards", function (this: any) { cutButtons(slots); });
        addCtxItem("delete", "Delete " + slots.length + " Cards", function (this: any) { deleteButtons(slots); }, true);
    }
    function addSingleCardMenuItems(this: any, slot?: any) {
        if (slot === -2) {
            addBackButtonMenuItems();
            return;
        }
        var c: any = ctx();
        var b: any = c.buttons[slot - 1];
        addCtxItem("pencil", "Edit Card", function (this: any) { openCardSettings(slot); });
        var ctxTypeDef: any = BUTTON_TYPES[(b && b.type) || ""];
        if (ctxTypeDef && ctxTypeDef.contextMenuItems &&
            (!c.isSub || buttonTypeRegistryValue(ctxTypeDef, "allowInSubpage", false))) {
            ctxTypeDef.contextMenuItems(slot, b, { addCtxItem: addCtxItem });
        }
        var sz: any = c.sizes[slot] || 1;
        addCtxSubmenu("arrow-expand-all", "Size", function (this: any, sub?: any) {
            addSubItem(sub, "", "Single (1x1)", function (this: any) { resizeSlot(slot, 1); }, sz === 1);
            if (!cardRequiresSquareSize(b)) {
                addSubItem(sub, "", "Tall (2x1)", function (this: any) { resizeSlot(slot, 2); }, sz === 2);
                addSubItem(sub, "", "Extra Tall (3x1)", function (this: any) { resizeSlot(slot, 5); }, sz === 5);
                addSubItem(sub, "", "Wide (1x2)", function (this: any) { resizeSlot(slot, 3); }, sz === 3);
                addSubItem(sub, "", "Extra Wide (1x3)", function (this: any) { resizeSlot(slot, 6); }, sz === 6);
            }
            addSubItem(sub, "", "Large (2x2)", function (this: any) { resizeSlot(slot, 4); }, sz === 4);
            if (cardRequiresSquareSize(b))
                addSubItem(sub, "", "Extra Large (3x3)", function (this: any) { resizeSlot(slot, 7); }, sz === 7);
        });
        addCtxDivider();
        addCtxItem("content-copy", "Duplicate", function (this: any) {
            if (c.isSub) {
                duplicateSubpageButton(slot);
            }
            else {
                duplicateButton(slot);
            }
        });
        addCtxItem("clipboard-outline", "Copy", function (this: any) { copySlot(slot); });
        addCtxItem("content-cut", "Cut", function (this: any) { cutSlot(slot); });
        addCtxItem("delete", "Delete", function (this: any) { deleteSlot(slot); }, true);
    }
    function addClockBarMenuItems(this: any, item?: any) {
        if (isClockBarTemperatureItem(item)) {
            addCtxItem("pencil", "Edit Temperature", function (this: any) { openClockBarTemperatureSettings(); });
            addCtxDivider();
        }
        else if (item === "voice") {
            addCtxItem("pencil", "Edit Voice Services", function (this: any) { openVoiceServicesSettings(); });
            addCtxDivider();
        }
        var visible: any = clockBarItemActive(item);
        var label: any = clockBarItemLabel(item);
        addCtxItem(visible ? "eye-off-outline" : "eye-outline", (visible ? "Hide " : "Show ") + label, function (this: any) {
            setClockBarItemVisible(item, !visible);
        });
    }
    function showSelectionMenu(this: any, e?: any) {
        if (isConfigLocked())
            return;
        hideContextMenu();
        var c: any = ctx();
        if (!c.selected.length)
            return;
        ctxMenu = document.createElement("div");
        ctxMenu.className = "sp-ctx-menu";
        if (c.selected.length > 1) {
            addBulkCardMenuItems(c.selected.slice());
        }
        else {
            addSingleCardMenuItems(c.selected[0]);
        }
        document.body.appendChild(ctxMenu);
        positionMenu(ctxMenu, e);
    }
    function showClockBarContextMenu(this: any, e?: any, item?: any) {
        if (isConfigLocked() || clockBarItems().indexOf(item) === -1)
            return;
        hideContextMenu();
        var c: any = ctx();
        if (state.clockBarSelectedItem !== item) {
            c.setSelected([]);
            c.setLastClicked(-1);
            state.clockBarSelectedItem = item;
            hideSettingsOverlay();
            updateClockBarItemUi();
            renderPreview();
            renderButtonSettings();
        }
        ctxMenu = document.createElement("div");
        ctxMenu.className = "sp-ctx-menu";
        addClockBarMenuItems(item);
        document.body.appendChild(ctxMenu);
        positionMenu(ctxMenu, e);
    }
    function showContextMenu(this: any, e?: any, slot?: any) {
        if (isConfigLocked())
            return;
        hideContextMenu();
        var c: any = ctx();
        if (c.selected.indexOf(slot) === -1) {
            if (c.selected.length > 1) {
                c.selected.push(slot);
            }
            else {
                c.setSelected([slot]);
                c.setLastClicked(slot);
            }
            renderPreview();
            renderButtonSettings();
            c = ctx();
        }
        ctxMenu = document.createElement("div");
        ctxMenu.className = "sp-ctx-menu";
        if (c.selected.length > 1 && c.selected.indexOf(slot) !== -1) {
            addBulkCardMenuItems(c.selected.slice());
        }
        else {
            addSingleCardMenuItems(slot);
        }
        document.body.appendChild(ctxMenu);
        positionMenu(ctxMenu, e);
    }
    function showBackContextMenu(this: any, e?: any) {
        if (isConfigLocked())
            return;
        hideContextMenu();
        ctxMenu = document.createElement("div");
        ctxMenu.className = "sp-ctx-menu";
        addBackButtonMenuItems();
        document.body.appendChild(ctxMenu);
        positionMenu(ctxMenu, e);
    }
    function addBackButtonMenuItems(this: any) {
        var sp: any = getSubpage(state.editingSubpage);
        var bkSz: any = sp.sizes[-2] || 1;
        addCtxItem("pencil", "Edit Label", function (this: any) { openCardSettings(-2); });
        addCtxItem("keyboard-return", "Exit Subpage", function (this: any) { exitSubpage(); });
        addCtxDivider();
        addCtxSubmenu("arrow-expand-all", "Size", function (this: any, sub?: any) {
            addSubItem(sub, "", "Single (1x1)", function (this: any) { resizeSlot(-2, 1); }, bkSz === 1);
            addSubItem(sub, "", "Tall (2x1)", function (this: any) { resizeSlot(-2, 2); }, bkSz === 2);
            addSubItem(sub, "", "Extra Tall (3x1)", function (this: any) { resizeSlot(-2, 5); }, bkSz === 5);
            addSubItem(sub, "", "Wide (1x2)", function (this: any) { resizeSlot(-2, 3); }, bkSz === 3);
            addSubItem(sub, "", "Extra Wide (1x3)", function (this: any) { resizeSlot(-2, 6); }, bkSz === 6);
            addSubItem(sub, "", "Large (2x2)", function (this: any) { resizeSlot(-2, 4); }, bkSz === 4);
        });
    }
    function showEmptySlotMenu(this: any, e?: any, pos?: any) {
        if (isConfigLocked())
            return;
        hideContextMenu();
        ctxMenu = document.createElement("div");
        ctxMenu.className = "sp-ctx-menu";
        var c: any = ctx();
        if (state.clipboard) {
            var count: any = state.clipboard.buttons.length;
            addCtxItem("content-paste", count > 1 ? "Paste " + count + " Cards" : "Paste", function (this: any) {
                if (c.isSub) {
                    pasteSubpageButton(pos);
                }
                else {
                    pasteButton(pos);
                }
            });
            addCtxDivider();
        }
        addCtxItem("plus", "Create Card", function (this: any) { addSlot(pos); });
        if (!c.isSub) {
            addCtxItem("folder-plus", "Create Subpage", function (this: any) { addSubpageSlot(pos); });
        }
        document.body.appendChild(ctxMenu);
        positionMenu(ctxMenu, e);
    }
    function hideContextMenu(this: any) {
        if (ctxMenu && ctxMenu.parentNode) {
            ctxMenu.parentNode.removeChild(ctxMenu);
        }
        ctxMenu = null;
    }
    return {
        "ctxMenu": liveGlobal(() => ctxMenu, (value?: any) => { ctxMenu = value; }),
        "positionMenu": staticGlobal(positionMenu),
        "addCtxItem": staticGlobal(addCtxItem),
        "addCtxDivider": staticGlobal(addCtxDivider),
        "addCtxSubmenu": staticGlobal(addCtxSubmenu),
        "addSubItem": staticGlobal(addSubItem),
        "resizeSlot": staticGlobal(resizeSlot),
        "addBulkCardMenuItems": staticGlobal(addBulkCardMenuItems),
        "addSingleCardMenuItems": staticGlobal(addSingleCardMenuItems),
        "addClockBarMenuItems": staticGlobal(addClockBarMenuItems),
        "showSelectionMenu": staticGlobal(showSelectionMenu),
        "showClockBarContextMenu": staticGlobal(showClockBarContextMenu),
        "showContextMenu": staticGlobal(showContextMenu),
        "showBackContextMenu": staticGlobal(showBackContextMenu),
        "addBackButtonMenuItems": staticGlobal(addBackButtonMenuItems),
        "showEmptySlotMenu": staticGlobal(showEmptySlotMenu),
        "hideContextMenu": staticGlobal(hideContextMenu),
    };
}
