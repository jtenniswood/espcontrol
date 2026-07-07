// Garage door card: cover toggle or one-tap open/close commands.
var GARAGE_MODE_OPTIONS = [
  ["", "Toggle"],
  ["open", "Open"],
  ["close", "Close"],
];

function garageCommandMode(mode) {
  return mode === "open" || mode === "close";
}

function garageModeOptionValues() {
  return coverLikeModeValues("garage", "garage_mode", GARAGE_MODE_OPTIONS);
}

function normalizeGarageMode(mode) {
  return normalizeCoverLikeMode(mode, garageModeOptionValues());
}

function garageModeDefaultIcon(mode) {
  return mode === "open" ? "Garage Open" : "Garage";
}

function garageModeDefaultLabel(mode) {
  if (mode === "open") return "Open";
  if (mode === "close") return "Close";
  return "Garage Door";
}

function garageUsesDefaultIcon(icon) {
  return !icon || icon === "Auto" || icon === "Garage" || icon === "Garage Open";
}

var GARAGE_CARD_METADATA = {
  mode: {
    label: "Interaction",
    idSuffix: "garage-interaction",
    options: GARAGE_MODE_OPTIONS,
    value: function (b) {
      return normalizeGarageMode(b.sensor);
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
    placeholder: "e.g. cover.garage_door",
    domains: function () { return cardContractDomains("garage"); },
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
    badge: "garage",
  },
};

registerCoverLikeCardType({
  type: "garage",
  optionName: "garage_mode",
  metadata: GARAGE_CARD_METADATA,
  commandModes: ["open", "close"],
  closedIcon: "Garage",
  openIcon: "Garage Open",
  shortLabel: "Garage",
  defaultCardLabel: "Garage Door",
  labelPlaceholder: "e.g. Garage Door",
  defaultIcon: garageModeDefaultIcon,
  defaultLabel: garageModeDefaultLabel,
  usesDefaultIcon: garageUsesDefaultIcon,
  normalizeOptions: normalizeGarageOptions,
  labelDisplayMode: garageLabelDisplayMode,
  setLabelDisplayMode: setGarageLabelDisplayMode,
});
