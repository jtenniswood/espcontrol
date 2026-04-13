registerButtonType("subpage", {
  label: "Subpage",
  allowInSubpage: false,
  labelPlaceholder: "e.g. Lighting",
  onSelect: function (b) {
    b.entity = ""; b.sensor = ""; b.unit = ""; b.icon_on = "Auto";
  },
  renderSettings: function (panel, b, slot, helpers) {
    panel.appendChild(helpers.makeIconPicker(
      helpers.idPrefix + "icon-picker", helpers.idPrefix + "icon",
      b.icon || "Auto", function (opt) {
        b.icon = opt;
        helpers.saveField("icon", opt);
        renderPreview();
      }
    ));
    var configBtn = document.createElement("button");
    configBtn.className = "sp-action-btn";
    configBtn.style.background = "var(--accent)";
    configBtn.style.color = "#fff";
    configBtn.style.width = "100%";
    configBtn.textContent = "Configure Subpage";
    configBtn.addEventListener("click", function () { enterSubpage(slot); });
    panel.appendChild(configBtn);
  },
  renderPreview: function (b, helpers) {
    var label = b.label || b.entity || "Configure";
    return {
      labelHtml:
        '<span class="sp-btn-label-row"><span class="sp-btn-label">' + helpers.escHtml(label) + '</span>' +
        '<span class="sp-subpage-badge mdi mdi-chevron-right"></span></span>',
    };
  },
  contextMenuItems: function (slot, b, helpers) {
    helpers.addCtxItem("cog", "Edit Subpage", function () { enterSubpage(slot); });
  },
});
