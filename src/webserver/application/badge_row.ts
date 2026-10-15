import { state } from "../state/app_instance";
import type { CardConfig } from "../contracts/types";
import { cardContractDefaultConfig, cardContractPickerKey, resolveCardRuntimeSpec } from "../generated/card_contract";
import { cloneCardConfig } from "../model/card";
import { BADGE_SUBTYPES, badgeSubtype, setBadgeSubtype, type BadgeSubtype } from "../model/badge";
import { badgeRowKey, badgeRowsFromSettings, MAX_ROW_BADGES, serializeBadgeRow } from "../model/badge_row";
import { updateNativePanelConfigDocument } from "../features/native_panel_config";
import { PREVIEW_THEME_COLORS, defaultCardBackgroundColor, previewEffectiveTheme } from "../state/preview_theme";
import { WEB_UI_COLORS } from "../state/ui_tokens";
import { escHtml, mdiIcon } from "./ui_primitives";
import { createSettingsModal } from "../components/settings_modal";
import type { NativePanelConfigController } from "../controllers/native_panel_config_controller";
import type { CardRegistry } from "./card_registry";
import type { ControlsFieldsFeature } from "./controls_fields";
import type { ButtonSettingsIconPickerFeature } from "./button_settings_icon_picker";
import type { EntityStateFeature } from "./entity_state";
import type { ControlsShellFeature } from "./controls_shell";
import type { UiRuntimeState } from "./state";
import type { ConfigCodecFeature } from "./config_codec";
import type { BadgeContextMenuActions, PreviewContextMenuFeature } from "./preview_context_menu";
import { isSubpageBadge } from "../model/badge";
import { configOptionValue, setConfigOptionValue, lighterCardColor } from "../model/config_primitives";
import { renderBadgeBulkSettings, type BadgeBulkSettings } from "./badge_bulk_settings";
import { badgeCardType, badgeToCard, updateBadgeFromCard, cardBadgeUsesSlider } from "../model/badge_card";
import { badgeCardDisplay, badgeCardSupportsValue } from "../model/badge_card_display";
import { defaultCardTypeForPicker, cardPreviewTextColor, cardProgressColor, type CardPickerOption } from "../features/preview";

export interface BadgeRowFeature {
  render(): void;
  renderSelectionControl(container: HTMLElement): void;
  renderSelectedBadgeBar(container: HTMLElement): boolean;
  clearSelection(): void;
  hasSelection(): boolean;
  load(): Promise<void>;
  renderEditorIfOpen(): boolean;
}

export function createBadgeRowFeature(deps: {
  document: Document; runtime: UiRuntimeState; cards: CardRegistry;
  native: NativePanelConfigController; deviceProfile: string;
  codec: Pick<ConfigCodecFeature, "normalizeButtonConfig">;
  fields: ControlsFieldsFeature; iconPicker: ButtonSettingsIconPickerFeature;
  entities: Pick<EntityStateFeature, "entityInput">;
  contextMenu: Pick<PreviewContextMenuFeature, "showBadge">;
  showCopyCode(button: CardConfig | CardConfig[]): void;
  shell: Pick<ControlsShellFeature, "isConfigLocked" | "showBanner" | "createActionButton">;
  renderPreview(): void;
  prepareBadgeSelection(): void;
  pickerOptions(): CardPickerOption[];
  renderColours(panel: HTMLElement, button: CardConfig): void;
  imageOptions: Pick<import("./config_image_options").ConfigImageOptionsFeature, "imageCardCountWithCandidate" | "imageSlotCapacity" | "showImageCardLimitBanner">;
}): BadgeRowFeature {
  const { document, fields, runtime, shell } = deps;
  const els = runtime.els;
  let row: HTMLElement | null = null;
  let renderedPage: number | null = null;
  let overlay: HTMLElement | null = null;
  let modal: HTMLElement | null = null;
  let config: HTMLElement | null = null;
  let draft: { page: number; index: number; button: CardConfig; card?: CardConfig } | null = null;
  let selectedBadges: { page: number; indices: number[]; anchor: number } | null = null;
  let bulkDraft: { page: number; indices: number[]; settings: BadgeBulkSettings } | null = null;
  let badgeClipboard: CardConfig[] | null = null;
  let saving = false;
  let badgesHidden = false;
  let draggedIndex = -1;
  let draggedPage = -1;
  let loadGeneration = 0;
  const pageIndex = () => state.editingSubpage || 0;
  const badges = (page = pageIndex()) => state.badgeRows[String(page)] || [];

  function currentSelection() {
    if (!selectedBadges || selectedBadges.page !== pageIndex()) {
      selectedBadges = null;
      return null;
    }
    selectedBadges.indices = selectedBadges.indices.filter(index => index >= 0 && index < badges().length);
    if (!selectedBadges.indices.length) { selectedBadges = null; return null; }
    return selectedBadges;
  }

  function button(label: string, action: (event: MouseEvent) => void, className = "sp-action-btn", icon?: string, ariaLabel?: string): HTMLButtonElement {
    const element = shell.createActionButton(className, label, icon, ariaLabel) as HTMLButtonElement;
    element.type = "button";
    element.addEventListener("click", event => { if (!saving && !shell.isConfigLocked()) action(event); });
    return element;
  }
  function close() { if (saving) return; draft = null; bulkDraft = null; selectedBadges = null; overlay?.classList.remove("sp-visible"); deps.renderPreview(); }

  function ensureUi() {
    if (row || !els.previewScreen) return;
    row = document.createElement("div");
    row.className = "sp-badge-row";
    row.setAttribute("aria-label", "Page badges");
    row.addEventListener("dragover", event => {
      if (!canDropBadge()) return;
      event.preventDefault();
      event.stopPropagation();
      if (event.dataTransfer) event.dataTransfer.dropEffect = "move";
      showInsertionMarker(insertionAt(event.clientX));
    });
    row.addEventListener("dragleave", event => {
      const bounds = row!.getBoundingClientRect();
      if (event.clientX < bounds.left || event.clientX >= bounds.right ||
          event.clientY < bounds.top || event.clientY >= bounds.bottom) hideInsertionMarker();
    });
    row.addEventListener("drop", event => {
      if (!canDropBadge()) return;
      event.preventDefault();
      event.stopPropagation();
      const insertion = insertionAt(event.clientX);
      const from = draggedIndex;
      endDrag();
      reorderBadge(from, from < insertion ? insertion - 1 : insertion);
    });
    els.previewScreen.appendChild(row);
    const settings = createSettingsModal(document, close, "Close badge settings");
    overlay = settings.overlay;
    modal = settings.modal;
    config = settings.config;
    overlay.classList.add("sp-badge-settings-overlay");
    overlay.setAttribute("role", "dialog");
    overlay.setAttribute("aria-label", "Page badges");
    overlay.setAttribute("aria-modal", "true");
    overlay.addEventListener("click", e => { if (e.target === overlay) close(); });
    overlay.addEventListener("keydown", e => { if (e.key === "Escape") close(); });
    els.screenPage.appendChild(overlay);
  }

  function render() {
    if (draft?.card) updateBadgeFromCard(draft.button, draft.card);
    ensureUi();
    if (!row) return;
    const scrollLeft = renderedPage === pageIndex() ? row.scrollLeft : 0;
    renderedPage = pageIndex();
    const items = badges().map(cloneCardConfig);
    if (draft && draft.page === pageIndex()) items[draft.index] = draft.button;
    if (bulkDraft && bulkDraft.page === pageIndex()) {
      for (const index of bulkDraft.indices) items[index] = bulkDraft.settings.apply(items[index]!);
    }
    const badgeRowAvailable = state.badgeRowsSupported && state.badgeRowsReady;
    els.previewScreen.classList.toggle("sp-has-badge-row", badgeRowAvailable && !badgesHidden);
    row.hidden = !badgeRowAvailable || badgesHidden;
    const group = document.createElement("div");
    group.className = "sp-badge-row-items";
    const marker = document.createElement("div");
    marker.className = "sp-badge-insertion-marker";
    marker.setAttribute("aria-hidden", "true");
    marker.hidden = true;
    group.appendChild(marker);
    const colors = PREVIEW_THEME_COLORS[previewEffectiveTheme(state)];
    const selection = currentSelection();
    items.forEach((config, index) => {
      const rawColor = configOptionValue(config.options, "card_off_color").replace(/^#/, "").toUpperCase();
      const offColor = /^[0-9A-F]{6}$/.test(rawColor) ? rawColor : "";
      const cardType = badgeCardType(config);
      const primary = state.onColor || WEB_UI_COLORS.primary;
      const foreground = offColor ? cardPreviewTextColor(offColor) : "#" + colors.textPrimary;
      const preview = deps.cards.definitions.badge!.renderPreview!(config, { escHtml,
        cardBackgroundColor: offColor, cardProgressColor: cardProgressColor(offColor) || primary, cardTextColor: foreground,
      }) as { buttonClass: string; iconHtml: string };
      const selected = selection?.indices.includes(index);
      const item = button("", event => selectBadge(index, event), "sp-btn " + preview.buttonClass + (selected ? " sp-selected" : ""));
      item.innerHTML = preview.iconHtml;
      item.style.backgroundColor = "#" + (offColor || (cardType !== null ? defaultCardBackgroundColor(cardType, state) : colors.surfaceCard));
      const fill = cardType === "media" && cardBadgeUsesSlider(badgeToCard(config))
        ? cardProgressColor(offColor) : lighterCardColor(offColor);
      item.style.setProperty("--card-active-color", "#" + (fill || primary));
      item.style.setProperty("--card-text-color", foreground);
      item.style.color = foreground;
      item.setAttribute("data-badge-index", String(index));
      item.setAttribute("aria-label", "Select badge " + (config.label || index + 1));
      item.setAttribute("aria-pressed", String(!!selected));
      item.draggable = !saving && !shell.isConfigLocked();
      item.disabled = saving || shell.isConfigLocked();
      bindDrag(item, index);
      item.addEventListener("mousedown", event => { if (event.shiftKey || event.ctrlKey || event.metaKey) event.preventDefault(); });
      item.addEventListener("contextmenu", event => {
        event.preventDefault();
        event.stopPropagation();
        if (saving || shell.isConfigLocked()) return;
        selectBadge(index, undefined, true);
        const page = pageIndex();
        // Finish scrolling the wider row into view before opening its menu;
        // a queued scroll event would otherwise immediately dismiss it.
        document.defaultView!.requestAnimationFrame(() => {
          const selected = currentSelection();
          if (!saving && !shell.isConfigLocked() && selected?.page === page && selected.indices.includes(index)) {
            deps.contextMenu.showBadge(event, badgeContextMenuActions());
          }
        });
      });
      group.appendChild(item);
    });
    const add = button("", addBadge, "sp-btn sp-badge-add", "plus", "Add badge");
    add.disabled = saving || shell.isConfigLocked() || items.length >= MAX_ROW_BADGES;
    group.appendChild(add);
    // Replace a complete row so its width never collapses during selection.
    row.replaceChildren(group);
    row.scrollLeft = scrollLeft;
  }

  function selectBadge(index: number, event?: MouseEvent, contextMenu = false) {
    const selected = currentSelection();
    let indices = selected?.indices.slice() || [];
    let anchor = selected?.anchor ?? index;
    if (contextMenu) {
      if (!indices.includes(index)) indices = indices.length > 1 ? [...indices, index] : [index];
    } else if (event?.shiftKey && selected) {
      event.preventDefault();
      indices = Array.from({ length: Math.abs(index - anchor) + 1 }, (_, offset) => Math.min(index, anchor) + offset);
    } else if (event?.ctrlKey || event?.metaKey) {
      event.preventDefault();
      indices = indices.includes(index) ? indices.filter(value => value !== index) : [...indices, index];
      anchor = index;
    } else {
      indices = indices.length === 1 && indices[0] === index ? [] : [index];
      anchor = index;
    }
    selectedBadges = indices.length ? { page: pageIndex(), indices, anchor } : null;
    deps.prepareBadgeSelection();
    deps.renderPreview();
  }

  function addBadge() {
    selectedBadges = null;
    deps.prepareBadgeSelection();
    deps.renderPreview();
    chooseType();
  }

  function bindDrag(item: HTMLButtonElement, index: number) {
    item.addEventListener("dragstart", event => {
      if (saving || shell.isConfigLocked()) { event.preventDefault(); return; }
      draggedIndex = index;
      draggedPage = pageIndex();
      event.dataTransfer && (event.dataTransfer.effectAllowed = "move");
      event.dataTransfer && event.dataTransfer.setData("text/plain", String(index));
      item.classList.add("sp-dragging");
    });
    item.addEventListener("dragend", endDrag);
  }

  function canDropBadge() {
    return draggedIndex >= 0 && draggedIndex < badges().length && draggedPage === pageIndex() &&
      !row?.hidden && !saving && !shell.isConfigLocked();
  }

  function insertionAt(clientX: number) {
    const items = Array.from(row!.querySelectorAll<HTMLElement>("[data-badge-index]"));
    const index = items.findIndex(item => {
      const bounds = item.getBoundingClientRect();
      return clientX < bounds.left + bounds.width / 2;
    });
    return index < 0 ? items.length : index;
  }

  function showInsertionMarker(insertion: number) {
    const group = row!.querySelector<HTMLElement>(".sp-badge-row-items")!;
    const marker = group.querySelector<HTMLElement>(".sp-badge-insertion-marker")!;
    const items = group.querySelectorAll<HTMLElement>("[data-badge-index]");
    const before = items[insertion - 1]?.getBoundingClientRect();
    const after = (items[insertion] || group.querySelector<HTMLElement>(".sp-badge-add"))!.getBoundingClientRect();
    const gap = parseFloat(document.defaultView!.getComputedStyle(items[0]!).marginLeft);
    const x = before ? (before.right + after.left) / 2 : after.left - gap / 2;
    marker.style.left = (x - group.getBoundingClientRect().left) + "px";
    marker.dataset.insertionIndex = String(insertion);
    marker.hidden = false;
  }

  function hideInsertionMarker() {
    const marker = row?.querySelector<HTMLElement>(".sp-badge-insertion-marker");
    if (marker) marker.hidden = true;
  }

  function endDrag() {
    draggedIndex = -1;
    draggedPage = -1;
    hideInsertionMarker();
    row?.querySelectorAll(".sp-dragging").forEach(item => item.classList.remove("sp-dragging"));
  }

  function reorderBadge(from: number, to: number) {
    const page = pageIndex();
    const next = badges(page).map(cloneCardConfig);
    to = Math.min(to, next.length - 1);
    if (from < 0 || from >= next.length || to < 0 || from === to) return;
    const [moved] = next.splice(from, 1);
    next.splice(to, 0, moved!);
    if (selectedBadges?.page === page) {
      const remap = (index: number) => index === from ? to
        : from < index && index <= to ? index - 1 : to <= index && index < from ? index + 1 : index;
      selectedBadges.indices = selectedBadges.indices.map(remap);
      selectedBadges.anchor = remap(selectedBadges.anchor);
    }
    void save(next, page, false);
  }

  function clearSelection() {
    selectedBadges = null;
  }

  function hasSelection() {
    return currentSelection() !== null;
  }

  function badgeContextMenuActions(): BadgeContextMenuActions {
    const selected = currentSelection()!;
    const indices = selected.indices.slice().sort((a, b) => a - b);
    const copied = () => {
      try { return indices.map(index => cloneCardConfig(badges(selected.page)[index]!)); }
      catch (error) { shell.showBanner((error as Error).message, "error"); return null; }
    };
    return {
      selectionCount: indices.length,
      edit: editSelectedBadges,
      duplicate: () => duplicateBadges(indices),
      copy: () => { const copy = copied(); if (copy) badgeClipboard = copy; },
      copyCode: () => deps.showCopyCode(indices.map(index => cloneCardConfig(badges(selected.page)[index]!))),
      cut: () => { const copy = copied(); if (copy) { badgeClipboard = copy; deleteSelectedBadges(); } },
      delete: deleteSelectedBadges,
      ...(badgeClipboard ? { paste: () => pasteBadge(indices[indices.length - 1]! + 1) } : {}),
    };
  }

  function renderSelectedBadgeBar(container: HTMLElement) {
    const selected = currentSelection();
    if (!selected) return false;
    const label = document.createElement("span");
    label.className = "sp-selection-label";
    label.textContent = selected.indices.length === 1 ? "1 badge selected" : selected.indices.length + " badges selected";
    container.appendChild(label);
    const actions = document.createElement("div");
    actions.className = "sp-selection-actions";
    actions.appendChild(button("Edit", editSelectedBadges, "sp-selection-btn sp-selection-btn-primary", "pencil"));
    const menu = shell.createActionButton("sp-selection-btn", "", "dots-horizontal", "Badge actions") as HTMLButtonElement;
    menu.type = "button";
    menu.addEventListener("click", event => {
      event.preventDefault();
      event.stopPropagation();
      if (!saving && !shell.isConfigLocked()) {
        deps.contextMenu.showBadge(event, badgeContextMenuActions());
      }
    });
    actions.appendChild(menu);
    container.appendChild(actions);
    return true;
  }

  function deleteSelectedBadges() {
    const selected = currentSelection();
    if (!selected) return;
    const next = badges(selected.page).filter((_, index) => !selected.indices.includes(index)).map(cloneCardConfig);
    selectedBadges = null;
    void save(next, selected.page, false);
  }

  function duplicateBadges(indices: number[]) {
    const page = pageIndex();
    const next = badges(page).map(cloneCardConfig);
    if (next.length + indices.length > MAX_ROW_BADGES) {
      shell.showBanner("The badge row is full.", "error");
      return;
    }
    const insertion = indices[indices.length - 1]! + 1;
    try { next.splice(insertion, 0, ...indices.map(index => cloneCardConfig(next[index]!))); }
    catch (error) { shell.showBanner((error as Error).message, "error"); return; }
    selectedBadges = { page, indices: indices.map((_, offset) => insertion + offset), anchor: insertion };
    void save(next, page, false);
  }

  function pasteBadge(index: number) {
    if (!badgeClipboard?.length) return;
    const page = pageIndex();
    const next = badges(page).map(cloneCardConfig);
    if (next.length + badgeClipboard.length > MAX_ROW_BADGES) {
      shell.showBanner("The badge row is full.", "error");
      return;
    }
    const destination = Math.max(0, Math.min(index, next.length));
    next.splice(destination, 0, ...badgeClipboard.map(item => cloneCardConfig(item)));
    selectedBadges = { page, indices: badgeClipboard.map((_, offset) => destination + offset), anchor: destination };
    void save(next, page, false);
  }

  function renderSelectionControl(container: HTMLElement) {
    ensureUi();
    if (!state.badgeRowsSupported) return;
    const control = button(badgesHidden ? "Show badges" : "Hide badges", () => {
      badgesHidden = !badgesHidden;
      deps.renderPreview();
    }, "sp-selection-btn", badgesHidden ? "eye-outline" : "eye-off-outline", badgesHidden ? "Show badges" : "Hide badges");
    control.disabled = !state.badgeRowsReady || shell.isConfigLocked();
    container.appendChild(control);
  }

  function header(title: string) {
    config!.replaceChildren();
    modal!.classList.remove("sp-card-type-picker-open");
    const heading = document.createElement("div"); heading.className = "sp-section-title"; heading.textContent = title;
    config!.appendChild(heading);
    overlay!.classList.add("sp-visible");
  }

  function chooseType() {
    header("Choose Badge Type");
    modal!.classList.add("sp-card-type-picker-open");
    const grid = document.createElement("div"); grid.className = "sp-card-type-grid";
    if (badgeClipboard) {
      const pasteLabel = badgeClipboard.length === 1 ? "Paste copied badge" : "Paste copied badges";
      const paste = button("", () => {
        close();
        pasteBadge(badges().length);
      }, "sp-card-type-option");
      paste.setAttribute("aria-label", pasteLabel);
      paste.innerHTML = '<span class="sp-card-type-icon mdi mdi-content-paste"></span><span class="sp-card-type-copy"><span class="sp-card-type-title">' + pasteLabel + '</span><span class="sp-card-type-description">Add the copied badges to the end of this row.</span></span>';
      grid.appendChild(paste);
    }
    const types = deps.pickerOptions().filter(type => type.key !== "badge" && type.key !== "subpage");
    types.push({ key: "badge-modal", label: "Modal", icon: "card-outline", description: "Open an existing card's control modal.", disabled: false });
    for (const type of types) {
      const item = button("", () => {
        if (type.key === "badge-modal") edit(badges().length, "modal");
        else editCardBadge(badges().length, defaultCardTypeForPicker(type.key));
      }, "sp-card-type-option");
      item.setAttribute("data-badge-type", type.key || "switch");
      item.setAttribute("aria-label", type.label + " badge type");
      item.innerHTML = '<span class="sp-card-type-icon mdi mdi-' + type.icon + '"></span><span class="sp-card-type-copy"><span class="sp-card-type-title">' + escHtml(type.label) + '</span><span class="sp-card-type-description">' + escHtml(type.description) + '</span></span>';
      grid.appendChild(item);
    }
    config!.appendChild(grid);
    const actions = document.createElement("div"); actions.className = "sp-btn-row sp-btn-row--save";
    actions.appendChild(button("Cancel", close, "sp-action-btn sp-cancel-btn"));
    config!.appendChild(actions);
  }

  async function save(next: CardConfig[], page: number, closeEditor: boolean) {
    if (saving || shell.isConfigLocked()) return false;
    for (let index = 0; index < next.length; index++) {
      const card = badgeToCard(next[index]!);
      if (deps.imageOptions.imageCardCountWithCandidate({ badgePage: page, badgeIndex: index, button: card, badgeRow: next }) > deps.imageOptions.imageSlotCapacity()) {
        deps.imageOptions.showImageCardLimitBanner();
        return false;
      }
    }
    let value: string;
    try { value = serializeBadgeRow(next); }
    catch (error) { shell.showBanner((error as Error).message, "error"); return false; }
    saving = true;
    if (closeEditor) {
      modal?.setAttribute("aria-busy", "true");
      modal?.querySelectorAll("button,input,select").forEach(input => (input as HTMLInputElement).disabled = true);
    }
    const result = await deps.native.schedule(current => {
      return updateNativePanelConfigDocument(current, deps.deviceProfile, "settings", badgeRowKey(page), value);
    });
    saving = false;
    if (closeEditor) {
      const closeButton = modal?.querySelector(".sp-settings-close") as HTMLButtonElement | null;
      if (closeButton) closeButton.disabled = false;
      modal?.removeAttribute("aria-busy");
      if (bulkDraft) modal?.querySelectorAll("button,input,select").forEach(input => (input as HTMLInputElement).disabled = false);
    }
    if (result === "saved" || result === "mirror-failed") {
      ++loadGeneration;
      state.badgeRowsReady = true;
      state.badgeRows[String(page)] = next;
      if (closeEditor) close();
      else deps.renderPreview();
      return true;
    } else {
      shell.showBanner("Could not save badge rows. Check the connection and firmware, then try again.", "error");
      if (draft) renderEditor();
      else deps.renderPreview();
      return false;
    }
  }

  function edit(index: number, subtype?: BadgeSubtype) {
    if (index >= MAX_ROW_BADGES) return;
    bulkDraft = null;
    config!.replaceChildren();
    draft = { page: pageIndex(), index, button: cloneCardConfig(badges()[index] || cardContractDefaultConfig("badge")) };
    if (subtype) {
      setBadgeSubtype(draft.button, subtype);
      draft.button.icon = BADGE_SUBTYPES.find(type => type.key === subtype)!.defaultIcon;
    }
    renderEditor(); deps.renderPreview();
  }

  function editCardBadge(index: number, type: string) {
    bulkDraft = null;
    draft = { page: pageIndex(), index, button: cardContractDefaultConfig("badge") };
    const card = cardContractDefaultConfig(type);
    deps.cards.definitions[type]?.onSelect?.(card);
    draft!.card = card;
    updateBadgeFromCard(draft!.button, card);
    renderEditor(); deps.renderPreview();
  }

  function editSelectedBadges() {
    const selected = currentSelection();
    if (!selected) return;
    if (selected.indices.length === 1) { edit(selected.indices[0]!); return; }
    draft = null;
    header("Edit " + selected.indices.length + " Badges");
    const note = document.createElement("p");
    note.className = "sp-setting-note";
    note.textContent = "Only fields you change will be applied to the selected badges. Other settings keep their existing values.";
    config!.appendChild(note);
    const panel = document.createElement("div");
    panel.className = "sp-panel";
    const indices = selected.indices.slice();
    const page = selected.page;
    const settings = renderBadgeBulkSettings(panel, indices.map(index => badges(page)[index]!), {
      document, fields, entityInput: deps.entities.entityInput, iconPickerField,
      onChange: () => deps.renderPreview(),
    });
    bulkDraft = { page, indices, settings };
    fields.groupCardSettingsFields(panel, "sp-badge-bulk-", "Badge Settings");
    const actions = document.createElement("div");
    actions.className = "sp-btn-row sp-btn-row--save";
    actions.appendChild(button("Cancel", close, "sp-action-btn sp-cancel-btn"));
    actions.appendChild(button("Save", () => {
      const error = settings.validate();
      if (error) { shell.showBanner(error, "error"); return; }
      const next = badges(page).map((badge, index) => indices.includes(index) ? settings.apply(badge) : cloneCardConfig(badge));
      void save(next, page, true);
    }, "sp-action-btn sp-save-btn"));
    panel.appendChild(actions);
    config!.appendChild(panel);
    deps.renderPreview();
  }

  function iconPickerField(pickerId: string, inputId: string, value: string, onSelect: (value: string) => void, label = "Icon") {
    const wrapper = document.createElement("div"); wrapper.className = "sp-field";
    wrapper.appendChild(fields.fieldLabel(label, inputId));
    const picker = document.createElement("div"); picker.id = pickerId; picker.className = "sp-icon-picker";
    picker.appendChild(mdiIcon(value, "sp-icon-picker-preview mdi"));
    const input = fields.textInput(inputId, value, "Search icons…"); input.className = "sp-icon-picker-input";
    picker.appendChild(input);
    const dropdown = document.createElement("div"); dropdown.className = "sp-icon-dropdown"; picker.appendChild(dropdown);
    wrapper.appendChild(picker); deps.iconPicker.init(picker, value, onSelect); return wrapper;
  }

  function renderEditor() {
    if (!draft) return;
    if (draft.card) updateBadgeFromCard(draft.button, draft.card);
    const cardType = badgeCardType(draft.button);
    if (!isSubpageBadge(draft.button) && cardType !== null && !draft.card) draft.card = badgeToCard(draft.button);
    const settingsOpen = config!.querySelector("#sp-badge-inp-card-settings")?.getAttribute("aria-expanded") === "true";
    const unsupported = isSubpageBadge(draft.button);
    const cardDefinition = !unsupported && cardType !== null ? deps.cards.definitions[cardType]! : null;
    const label = cardDefinition ? deps.pickerOptions().find(type => type.key === (cardContractPickerKey(cardType) || cardType))?.label ||
      (typeof cardDefinition.label === "function" ? cardDefinition.label() : cardDefinition.label)
      : BADGE_SUBTYPES.find(type => type.key === badgeSubtype(draft!.button))?.label || "Badge";
    header(unsupported ? "Badge unavailable" : label + " Badge");
    const current = draft;
    const settingsButton = current.card || current.button;
    const panel = document.createElement("div"); panel.className = "sp-panel";
    const required: Array<{ input: HTMLInputElement; valid: () => boolean; message: string }> = [];
    const helpers = {
      ...fields, idPrefix: "sp-badge-inp-", entityInput: deps.entities.entityInput,
      saveField: () => deps.renderPreview(), renderBadgeSettings: renderEditor,
      isSub: false, cardSize: 1, clearFieldError: () => {},
      makeIconPicker: iconPickerField,
      precisionField: (id: string, value: string, onChange: EventListener) => fields.selectField("Unit Precision", id, [["0", "10"], ["1", "10.2"], ["2", "10.21"]], value || "0", onChange),
      toggleSection: (label: string, id: string, checked: boolean) => ({ toggle: fields.toggleRow(label, id, checked), section: fields.condField() }),
      bindField(input: HTMLInputElement, field: keyof CardConfig) {
        input.addEventListener("input", () => { settingsButton[field] = input.value; deps.renderPreview(); });
        input.addEventListener("change", () => { settingsButton[field] = input.value; deps.renderPreview(); });
      },
      requireField(input: HTMLInputElement, message: string, active = () => true, hasValue = (value: string) => Boolean(value.trim())) {
        required.push({ input, message, valid: () => !active() || hasValue(input.value) });
      },
      requireEntityDomain(input: HTMLInputElement, domains: string[], message: string, active = () => true) {
        required.push({ input, message, valid: () => !active() || !input.value.trim() || domains.includes(input.value.split(".")[0] || "") });
      },
      iconPickerField,
    };
    const entityField = (label: string, id: string, value: string, placeholder: string, domains: string[], bindName?: keyof CardConfig, _rerender?: boolean, requiredMessage?: string) => {
      const input = helpers.entityInput(id, value, placeholder, domains);
      if (bindName) helpers.bindField(input, bindName);
      if (requiredMessage) helpers.requireField(input, requiredMessage);
      return { field: fields.fieldWithControl(label, id, input), input };
    };
    const textField = (label: string, id: string, value: string, placeholder: string, bindName?: keyof CardConfig) => {
      const input = fields.textInput(id, value, placeholder);
      if (bindName) helpers.bindField(input, bindName);
      const field = fields.fieldWithControl(label, id, input);
      if (label === "Label") fields.markCardPrimaryField(field, "name");
      return { field, input };
    };
    Object.assign(helpers, { entityField, textField });
    if (unsupported) {
      const notice = document.createElement("p");
      notice.className = "sp-hint";
      notice.textContent = "Subpage badges are unavailable. Deleting this badge keeps its stored page.";
      panel.appendChild(notice);
    } else if (cardDefinition) {
      cardDefinition.renderSettingsBeforeLabel?.(panel, settingsButton, current.index, helpers);
      if (!cardDefinition.hideLabel) {
        const field = textField("Label", helpers.idPrefix + "label", settingsButton.label, "e.g. Kitchen", "label");
        fields.markCardPrimaryField(field.field, "name");
        panel.appendChild(field.field);
      }
      cardDefinition.renderSettings?.(panel, settingsButton, current.index, helpers);
      const displayChoices: Array<[string, string]> = [["card", "Card display"], ["label", "Label"]];
      if (badgeCardSupportsValue(settingsButton) || badgeCardDisplay(current.button) === "value") displayChoices.push(["value", "Live value"]);
      const display = fields.selectField("Badge Display", helpers.idPrefix + "badge-display", displayChoices,
        badgeCardDisplay(current.button), (event: Event) => {
          const value = (event.target as HTMLSelectElement).value;
          current.button.options = setConfigOptionValue(current.button.options, "badge_display", value === "card" ? "" : value);
          renderEditor();
          deps.renderPreview();
        });
      panel.appendChild(display.field);
    } else deps.cards.definitions.badge!.renderSettings!(panel, current.button, current.index, helpers);
    let nameField = panel.querySelector<HTMLElement>('[data-sp-card-primary="name"]');
    if (current.card && badgeCardDisplay(current.button) === "label" && !["clock", "calendar", "screen_lock"].includes(current.card.type)) {
      if (!nameField) {
        const added = textField("Label", helpers.idPrefix + "label", settingsButton.label, "e.g. Kitchen", "label").field as HTMLElement;
        panel.appendChild(added);
        nameField = added;
      }
      nameField.hidden = false;
      nameField.style.display = "";
    }
    if (nameField && nameField.parentElement !== panel) panel.insertBefore(nameField, panel.querySelector(":scope > .sp-disclosure"));
    const entityFieldElement = panel.querySelector<HTMLElement>(':scope > [data-sp-card-primary="entity"]');
    if (nameField?.parentElement === panel && entityFieldElement) panel.insertBefore(entityFieldElement, nameField);
    for (const section of panel.querySelectorAll<HTMLElement>(":scope > .sp-disclosure")) {
      const toggle = section.querySelector<HTMLElement>(".sp-disclosure-button")!;
      if (toggle.firstElementChild?.textContent === "Card Settings") {
        toggle.firstElementChild.textContent = "Badge Settings";
        toggle.id = "sp-badge-inp-card-settings";
      }
    }
    if (!unsupported) deps.renderColours(panel, settingsButton);
    const settings = unsupported ? null : fields.groupCardSettingsFields(panel, "sp-badge-inp-", "Badge Settings");
    if (settingsOpen) settings?.button.click();
    const actions = document.createElement("div"); actions.className = "sp-btn-row sp-btn-row--save";
    const saveButton = button("Save", () => {
      if (current.card) {
        deps.codec.normalizeButtonConfig(current.card);
        updateBadgeFromCard(current.button, current.card);
      }
      if (!current.card && configOptionValue(current.button.options, "badge_action") === "modal") {
        const configured = [...state.grid.filter(slot => slot > 0).map(slot => state.buttons[slot - 1]!),
          ...Object.values(state.subpages).flatMap((page: any) => (page.grid || []).filter((slot: number) => slot > 0).map((slot: number) => page.buttons[slot - 1]))];
        if (!configured.some(card => card && card.type !== "badge" && card.entity === current.button.entity && ["light_control", "cover_modal", "climate", "fan_control", "option_select", "fan_preset", "media", "image", "alarm", "wifi_qr", "weather"].includes(resolveCardRuntimeSpec(card)?.driver || ""))) {
          shell.showBanner("Choose an entity belonging to a configured card with a control modal.", "error"); return;
        }
      }
      const invalid = required.find(rule => !rule.valid());
      if (invalid) {
        const section = invalid.input.closest(".sp-disclosure:not(.sp-open)");
        (section?.querySelector(".sp-disclosure-button") as HTMLButtonElement | null)?.click();
        shell.showBanner(invalid.message, "error"); invalid.input.focus(); return;
      }
      const next = badges(current.page).map(cloneCardConfig); next[current.index] = cloneCardConfig(current.button); void save(next, current.page, true);
    }, "sp-action-btn sp-save-btn");
    if (current.index < badges(current.page).length) {
      actions.classList.add("sp-has-delete");
      const management = document.createElement("div"); management.className = "sp-badge-management";
      for (const [label, offset] of [["Move left", -1], ["Move right", 1]] as const) {
        const move = button(label, () => { close(); reorderBadge(current.index, current.index + offset); });
        move.disabled = current.index + offset < 0 || current.index + offset >= badges(current.page).length;
        management.appendChild(move);
      }
      management.appendChild(button("", () => {
        const next = badges(current.page).map(cloneCardConfig);
        next.splice(current.index, 1);
        void save(next, current.page, true);
      }, "sp-action-btn sp-delete-btn", "trash-can-outline", "Delete"));
      actions.appendChild(management);
    }
    const rightGroup = document.createElement("div"); rightGroup.className = "sp-btn-group-right";
    rightGroup.appendChild(saveButton); actions.appendChild(rightGroup);
    panel.appendChild(actions); config!.appendChild(panel);
  }

  async function load() {
    const generation = ++loadGeneration;
    state.badgeRowsReady = false;
    try {
      if (await deps.native.waitForDiscovery() !== true) return;
      state.badgeRowsSupported = deps.native.client?.badgeRowsSupported() || false;
      if (!state.badgeRowsSupported) return;
      const document = await deps.native.client!.read();
      if (generation !== loadGeneration || saving || draft || bulkDraft) return;
      state.badgeRows = badgeRowsFromSettings(document.settings);
      state.badgeRowsReady = true;
      deps.renderPreview();
    } catch { shell.showBanner("Could not load badge rows. Reload to try again.", "error"); }
  }
  return { render, renderSelectionControl, renderSelectedBadgeBar, clearSelection, hasSelection, load,
    renderEditorIfOpen() { if (!draft || !overlay?.classList.contains("sp-visible")) return false; renderEditor(); return true; } };
}
