// Webhook card: sends a direct HTTP request from the panel.
var WEBHOOK_HEADERS_OPTION = "webhook_headers";
var WEBHOOK_METHODS = [
  ["GET", "GET"],
  ["POST", "POST"],
  ["PUT", "PUT"],
  ["PATCH", "PATCH"],
  ["DELETE", "DELETE"],
];

function webhookMethod(value) {
  value = String(value || "").trim().toUpperCase();
  for (var i = 0; i < WEBHOOK_METHODS.length; i++) {
    if (WEBHOOK_METHODS[i][0] === value) return value;
  }
  return "GET";
}

function webhookHeaders(b) {
  return configOptionValue(b && b.options, WEBHOOK_HEADERS_OPTION);
}

function setWebhookHeaders(b, value) {
  if (!b) return "";
  b.options = setConfigOptionValue(b.options, WEBHOOK_HEADERS_OPTION, value || "");
  return b.options;
}

function normalizeWebhookConfig(b) {
  if (!b) return;
  b.sensor = webhookMethod(b.sensor);
  b.icon_on = "Auto";
  b.precision = "";
  if (b.sensor === "GET" || b.sensor === "DELETE") b.unit = "";
  if (!b.icon) b.icon = "Auto";
  var headers = webhookHeaders(b);
  b.options = headers ? setConfigOptionValue("", WEBHOOK_HEADERS_OPTION, headers) : "";
}

var WEBHOOK_CARD_METADATA = {
  url: {
    label: "URL",
    idSuffix: "webhook-url",
    placeholder: "e.g. http://jeedom.local/core/api/jeeApi.php?...",
  },
  method: {
    label: "Method",
    idSuffix: "webhook-method",
    options: WEBHOOK_METHODS,
  },
  icon: {
    pickerIdSuffix: "webhook-icon-picker",
    idSuffix: "webhook-icon",
    field: "icon",
    fallback: "Auto",
  },
  preview: {
    badge: "webhook",
  },
};

registerButtonType("webhook", {
  label: function () { return cardContractCardLabel("webhook"); },
  allowInSubpage: function () { return cardContractAllowInSubpage("webhook"); },
  pickerKey: function () { return cardContractPickerKey("webhook"); },
  hidden: function () { return cardContractHidden("webhook"); },
  labelPlaceholder: "e.g. Gate Open",
  defaultConfig: function () { return cardContractDefaultConfig("webhook"); },
  cardMetadata: WEBHOOK_CARD_METADATA,
  onSelect: function (b) {
    b.entity = "";
    b.sensor = "GET";
    b.unit = "";
    b.icon = "Auto";
    b.icon_on = "Auto";
    b.precision = "";
    b.options = "";
  },
  renderSettingsBeforeLabel: function (panel, b, slot, helpers) {
    normalizeWebhookConfig(b);

    var methodField = helpers.selectField(
      WEBHOOK_CARD_METADATA.method.label,
      helpers.idPrefix + WEBHOOK_CARD_METADATA.method.idSuffix,
      WEBHOOK_CARD_METADATA.method.options,
      webhookMethod(b.sensor),
      function () {
        b.sensor = webhookMethod(this.value);
        helpers.saveField("sensor", b.sensor);
        if (b.sensor === "GET" || b.sensor === "DELETE") {
          b.unit = "";
          helpers.saveField("unit", "");
        }
        renderButtonSettings();
      }
    );
    panel.appendChild(methodField.field);
  },
  renderSettings: function (panel, b, slot, helpers) {
    normalizeWebhookConfig(b);

    var urlField = helpers.textField(
      WEBHOOK_CARD_METADATA.url.label,
      helpers.idPrefix + WEBHOOK_CARD_METADATA.url.idSuffix,
      b.entity,
      WEBHOOK_CARD_METADATA.url.placeholder,
      "entity",
      true
    );
    panel.appendChild(urlField.field);
    helpers.requireField(urlField.input, "Add a webhook URL before saving.");

    if (b.sensor !== "GET" && b.sensor !== "DELETE") {
      var bodyField = helpers.textField(
        "Body",
        helpers.idPrefix + "webhook-body",
        b.unit,
        "e.g. {\"value1\":\"Gate\"}",
        "unit",
        false
      );
      panel.appendChild(bodyField.field);
    }

    var headersField = helpers.textField(
      "Headers",
      helpers.idPrefix + "webhook-headers",
      webhookHeaders(b),
      "e.g. Content-Type: application/json; Authorization: Bearer token",
      null,
      false
    );
    panel.appendChild(headersField.field);
    headersField.input.addEventListener("input", saveHeaders);
    headersField.input.addEventListener("change", saveHeaders);
    headersField.input.addEventListener("blur", saveHeaders);
    headersField.input.addEventListener("keydown", function (e) {
      if (e.key === "Enter") {
        saveHeaders();
        this.blur();
      }
    });

    helpers.renderBasicCardFields(panel, b, helpers, WEBHOOK_CARD_METADATA, {
      entity: false,
      label: false,
    });

    function saveHeaders() {
      helpers.saveField("options", setWebhookHeaders(b, headersField.input.value));
    }
  },
  renderPreview: function (b, helpers) {
    var label = b.label || b.entity || "Webhook";
    return cardBadgePreview(b, helpers, {
      label: label,
      iconFallback: "Flash",
      badge: WEBHOOK_CARD_METADATA.preview.badge,
    });
  },
});
