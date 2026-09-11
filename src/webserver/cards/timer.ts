import type { CardRegistry } from "../application/card_registry";
import { cardContractDefaultConfig, cardContractCardLabel, cardContractAllowInSubpage } from "../generated/card_contract";

// Timer card: counts down a Home Assistant timer.* entity.
// Tap while idle = start, tap while running = cancel (optionally confirmed),
// tap while paused = resume.
// Storage: sensor="confirm" (enables confirm step), unit="<seconds>" (timeout, default 3).

function timerParseConfirmTimeout(unit: string) {
  var n = parseInt(unit, 10);
  if (!isFinite(n) || n < 1) n = 3;
  if (n > 30) n = 30;
  return n;
}

export function registerTimerCardTypes(registry: CardRegistry): void {
registry.register("timer", {
  label: () => cardContractCardLabel("timer"),
  allowInSubpage: () => cardContractAllowInSubpage("timer"),
  hideLabel: true,
  cardMetadata: { preview: { badge: "timer-outline" } },
  defaultConfig: () => cardContractDefaultConfig("timer"),
  onSelect: function (b) {
    b.entity = "";
    b.icon = "Auto";
    b.icon_on = "Auto";
    b.sensor = "";
    b.unit = "3";
    b.precision = "";
  },
  renderSettings: function (panel, b, slot, helpers) {
    // Entity ID
    var ef = document.createElement("div");
    ef.className = "sp-field";
    ef.appendChild(helpers.fieldLabel("Timer Entity", helpers.idPrefix + "entity"));
    var entityInp = helpers.textInput(helpers.idPrefix + "entity", b.entity, "e.g. timer.kitchen");
    ef.appendChild(entityInp);
    panel.appendChild(ef);
    helpers.markCardPrimaryField(ef, "entity");
    helpers.bindField(entityInp, "entity", true);

    // Label
    var lf = document.createElement("div");
    lf.className = "sp-field";
    lf.appendChild(helpers.fieldLabel("Label", helpers.idPrefix + "label"));
    var labelInp = helpers.textInput(helpers.idPrefix + "label", b.label, "e.g. Kitchen");
    lf.appendChild(labelInp);
    panel.appendChild(lf);
    helpers.bindField(labelInp, "label", true);

    // Confirm before cancel
    var confirmRow = helpers.toggleRow("Confirm before cancel",
      helpers.idPrefix + "confirm", b.sensor === "confirm");

    // Confirm timeout
    var tf = document.createElement("div");
    tf.className = "sp-field";
    tf.appendChild(helpers.fieldLabel("Confirmation Time Out (Seconds)", helpers.idPrefix + "confirm-timeout"));
    var timeoutInp = document.createElement("input");
    timeoutInp.type = "number";
    timeoutInp.className = "sp-input";
    timeoutInp.id = helpers.idPrefix + "confirm-timeout";
    timeoutInp.min = "1";
    timeoutInp.max = "30";
    timeoutInp.step = "1";
    timeoutInp.placeholder = "3";
    timeoutInp.value = String(timerParseConfirmTimeout(b.unit));
    tf.appendChild(timeoutInp);

    panel.appendChild(confirmRow.row);
    panel.appendChild(tf);

    function applyConfirmVisibility() {
      tf.style.display = (b.sensor === "confirm") ? "" : "none";
    }
    applyConfirmVisibility();

    confirmRow.input.addEventListener("change", function () {
      b.sensor = confirmRow.input.checked ? "confirm" : "";
      helpers.saveField("sensor", b.sensor);
      applyConfirmVisibility();
    });

    function saveTimeout() {
      var n = timerParseConfirmTimeout(timeoutInp.value);
      timeoutInp.value = String(n);
      b.unit = String(n);
      helpers.saveField("unit", b.unit);
    }
    timeoutInp.addEventListener("change", saveTimeout);
    timeoutInp.addEventListener("blur", saveTimeout);
  },
  renderPreview: function (b, helpers) {
    var label = b.label || b.entity || "Timer";
    return {
      iconHtml:
        '<span class="sp-sensor-preview">' +
          '<span class="sp-sensor-value">0:00</span>' +
        '</span>',
      labelHtml:
        '<span class="sp-btn-label-row"><span class="sp-btn-label">' + helpers.escHtml(label) + '</span>' +
        '<span class="sp-type-badge mdi mdi-timer-outline"></span></span>',
    };
  },
});

}
