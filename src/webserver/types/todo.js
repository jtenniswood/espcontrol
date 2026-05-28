// Home Assistant todo card.
var TODO_CARD_METADATA = {
  entity: {
    label: "Entity",
    idSuffix: "todo-entity",
    placeholder: "e.g. todo.shopping",
    domains: function () { return cardContractDomains("todo"); },
    bindName: "entity",
    rerender: true,
    requiredMessage: "Add a todo entity before saving.",
  },
  labelField: {
    label: "Label",
    idSuffix: "todo-label",
    field: "label",
    placeholder: "e.g. Shopping",
    rerender: true,
  },
  icon: {
    pickerIdSuffix: "todo-icon-picker",
    idSuffix: "todo-icon",
    field: "icon",
    fallback: "Check",
  },
  iconDisplay: {
    label: "Icon Display",
    options: [
      ["icon", "Icon"],
      ["count", "Counter"],
    ],
  },
  preview: {
    badge: "check",
  },
};

function normalizeTodoConfig(b) {
  if (!b) return;
  b.sensor = "";
  b.unit = "";
  b.precision = "";
  b.options = normalizeTodoOptions(b.options);
  b.icon_on = "Auto";
  if (!b.icon || b.icon === "Auto") b.icon = "Check";
}

registerButtonType("todo", {
  label: function () { return cardContractCardLabel("todo"); },
  allowInSubpage: function () { return cardContractAllowInSubpage("todo"); },
  pickerKey: function () { return cardContractPickerKey("todo"); },
  experimental: function () { return cardContractExperimental("todo"); },
  hidden: function () { return cardContractHidden("todo"); },
  showSelectedWhenExperimentalHidden: false,
  hideLabel: true,
  defaultConfig: function () { return cardContractDefaultConfig("todo"); },
  cardMetadata: TODO_CARD_METADATA,
  onSelect: function (b) {
    b.entity = "";
    b.label = "";
    b.icon = "Check";
    normalizeTodoConfig(b);
  },
  renderSettings: function (panel, b, slot, helpers) {
    normalizeTodoConfig(b);
    helpers.renderCardEntityField(panel, b, helpers, TODO_CARD_METADATA);
    helpers.renderCardTextField(panel, b, helpers, TODO_CARD_METADATA.labelField);
    var iconField = condField();
    helpers.renderCardIconPicker(iconField, b, helpers, TODO_CARD_METADATA.icon);
    function syncIconField() {
      iconField.classList.toggle("sp-visible", todoCardStatusMode(b) === "icon");
    }
    helpers.renderCardSegmentControl(panel, b, helpers, {
      segment: Object.assign({}, TODO_CARD_METADATA.iconDisplay, {
        value: function () { return todoCardStatusMode(b); },
        onSelect: function (button, cardHelpers, value) {
          setTodoCardStatusMode(button, value);
          cardHelpers.saveField("options", button.options);
          syncIconField();
          scheduleRender();
        },
      }),
    });
    syncIconField();
    panel.appendChild(iconField);
  },
  renderPreview: function (b, helpers) {
    var label = b.label || b.entity || "Todo";
    return {
      iconHtml: todoCardShowCount(b)
        ? cardSensorPreviewHtml(b, helpers, "3", null)
        : '<span class="sp-btn-icon mdi mdi-' + iconSlug(b.icon && b.icon !== "Auto" ? b.icon : "Check") + '"></span>',
      labelHtml: cardBadgeLabelHtml(helpers, label, TODO_CARD_METADATA.preview.badge),
    };
  },
});
