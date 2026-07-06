// Gate card: cover toggle or one-tap open/close/stop commands.
var GATE_MODE_OPTIONS = [
  ["", "Toggle"],
  ["open", "Open"],
  ["close", "Close"],
  ["stop", "Stop"],
];

function gateCommandMode(mode) {
  return mode === "open" || mode === "close" || mode === "stop";
}

function gateModeOptionValues() {
  return coverLikeModeValues("gate", "gate_mode", GATE_MODE_OPTIONS);
}

function normalizeGateMode(mode) {
  return normalizeCoverLikeMode(mode, gateModeOptionValues());
}

function gateModeDefaultIcon(mode) {
  if (mode === "open") return "Gate Open";
  if (mode === "stop") return "Stop";
  return "Gate";
}

function gateModeDefaultLabel(mode) {
  if (mode === "open") return "Open";
  if (mode === "close") return "Close";
  if (mode === "stop") return "Stop";
  return "Gate";
}

function gateUsesDefaultIcon(icon) {
  return !icon || icon === "Auto" || icon === "Gate" || icon === "Gate Open" || icon === "Stop";
}

var GATE_CARD_METADATA = {
  mode: {
    label: "Interaction",
    idSuffix: "gate-interaction",
    options: GATE_MODE_OPTIONS,
    value: function (b) {
      return normalizeGateMode(b.sensor);
    },
  },
  display: {
    label: "Display",
    options: [
      ["label", "Label"],
      ["status", "Status"],
    ],
  },
  entity: {
    label: "Entity",
    idSuffix: "entity",
    placeholder: "e.g. cover.driveway_gate",
    domains: function () { return cardContractDomains("gate"); },
    bindName: "entity",
    rerender: true,
    requiredMessage: "Add an entity before saving.",
  },
  labelField: {
    label: "Label",
    idSuffix: "label",
    field: "label",
    rerender: true,
  },
  preview: {
    badge: "gate",
  },
};

registerCoverLikeCardType({
  type: "gate",
  optionName: "gate_mode",
  metadata: GATE_CARD_METADATA,
  commandModes: ["open", "close", "stop"],
  closedIcon: "Gate",
  openIcon: "Gate Open",
  shortLabel: "Gate",
  defaultCardLabel: "Gate",
  labelPlaceholder: "e.g. Gate",
  defaultIcon: gateModeDefaultIcon,
  defaultLabel: gateModeDefaultLabel,
  usesDefaultIcon: gateUsesDefaultIcon,
  normalizeOptions: normalizeGateOptions,
  labelDisplayMode: gateLabelDisplayMode,
  setLabelDisplayMode: setGateLabelDisplayMode,
});
