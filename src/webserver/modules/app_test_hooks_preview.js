// @web-module-requires: language_state, environment_state, screensaver_state, firmware_version_state, clock_bar_state

if (typeof globalThis !== "undefined" && globalThis.__ESPCONTROL_TEST_HOOKS__) {
  registerEspControlTestHookGroup("preview", {
    clockBarVisibleInPreviewFor: function (clockBarOn, screensaverAction) {
      var oldClockBarOn = state.clockBarOn;
      var oldScreensaverAction = state.screensaverAction;
      state.clockBarOn = !!clockBarOn;
      state.screensaverAction = normalizeScreensaverAction(screensaverAction);
      var visible = clockBarVisibleInPreview();
      state.clockBarOn = oldClockBarOn;
      state.screensaverAction = oldScreensaverAction;
      return visible;
    },
    webserverMockNow: webserverMockNow,
    webserverNow: webserverNow,
    previewHtmlValue: previewHtmlValue,
    buttonTypePreviewFor: function (type, button, options) {
      var oldTimezone = state.timezone;
      var oldActiveTimezone = state.activeTimezone;
      var oldUnit = state.temperatureUnit;
      var oldClockFormat = state.clockFormat;
      var oldLanguage = state.language;
      options = options || {};
      if (options.timezone != null) state.timezone = options.timezone;
      if (options.activeTimezone != null) state.activeTimezone = options.activeTimezone;
      if (options.temperatureUnit != null) {
        state.temperatureUnit = normalizeTemperatureUnit(options.temperatureUnit);
      }
      if (options.clockFormat != null) state.clockFormat = options.clockFormat;
      if (options.language != null) state.language = normalizeLanguage(options.language);
      var typeDef = BUTTON_TYPES[type || ""];
      var preview = typeDef && typeDef.renderPreview
        ? typeDef.renderPreview(button || {}, { escHtml: escHtml, cardSize: options.cardSize || 1 })
        : null;
      state.timezone = oldTimezone;
      state.activeTimezone = oldActiveTimezone;
      state.temperatureUnit = oldUnit;
      state.clockFormat = oldClockFormat;
      state.language = oldLanguage;
      return preview;
    },
    buttonTypePreviewForMockNow: function (type, button, options) {
      return withWebserverMockNow(function () {
        return globalThis.__ESPCONTROL_TEST_HOOKS__.config.buttonTypePreviewFor(type, button, options);
      });
    },
    networkPreviewIconSlug: networkPreviewIconSlug,
    displayFirmwareVersion: displayFirmwareVersion,
    firmwareVersionLabelFor: function (version, pending) {
      var oldVersion = state.firmwareVersion;
      var oldPending = state.firmwareVersionRefreshPending;
      state.firmwareVersion = version;
      state.firmwareVersionRefreshPending = !!pending;
      var label = firmwareVersionLabel();
      state.firmwareVersion = oldVersion;
      state.firmwareVersionRefreshPending = oldPending;
      return label;
    },
    findDuplicatePlacementFor: function (grid, start, size, maxSlots) {
      return findDuplicatePlacement(grid.slice(), start, size, maxSlots || NUM_SLOTS);
    },
    importedButtonOrderFor: function (orderStr, existingSizes) {
      var oldSizes = state.sizes;
      var oldGrid = state.grid;
      state.sizes = existingSizes || {};
      state.grid = [];
      for (var i = 0; i < NUM_SLOTS; i++) state.grid.push(0);
      applyImportedButtonOrder(orderStr, {});
      var sizes = {};
      for (var k in state.sizes) sizes[k] = state.sizes[k];
      var grid = state.grid.slice();
      state.sizes = oldSizes;
      state.grid = oldGrid;
      return { grid: grid, sizes: sizes };
    },
  });
}
