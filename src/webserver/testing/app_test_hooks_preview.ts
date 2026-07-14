import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installAppTestHooksPreview(): GlobalDescriptors {
    if (typeof globalThis !== "undefined" && globalThis.__ESPCONTROL_TEST_HOOKS__) {
        registerEspControlTestHookGroup("preview", {
            clockBarVisibleInPreviewFor: function (this: any, clockBarOn?: any, screensaverAction?: any) {
                var oldClockBarOn: any = state.clockBarOn;
                var oldScreensaverAction: any = state.screensaverAction;
                state.clockBarOn = !!clockBarOn;
                state.screensaverAction = normalizeScreensaverAction(screensaverAction);
                var visible: any = clockBarVisibleInPreview();
                state.clockBarOn = oldClockBarOn;
                state.screensaverAction = oldScreensaverAction;
                return visible;
            },
            webserverMockNow: webserverMockNow,
            webserverNow: webserverNow,
            buttonTypePreviewFor: function (this: any, type?: any, button?: any, options?: any) {
                var oldTimezone: any = state.timezone;
                var oldActiveTimezone: any = state.activeTimezone;
                var oldUnit: any = state.temperatureUnit;
                var oldClockFormat: any = state.clockFormat;
                var oldLanguage: any = state.language;
                options = options || {};
                if (options.timezone != null)
                    state.timezone = options.timezone;
                if (options.activeTimezone != null)
                    state.activeTimezone = options.activeTimezone;
                if (options.temperatureUnit != null) {
                    state.temperatureUnit = normalizeTemperatureUnit(options.temperatureUnit);
                }
                if (options.clockFormat != null)
                    state.clockFormat = options.clockFormat;
                if (options.language != null)
                    state.language = normalizeLanguage(options.language);
                var typeDef: any = BUTTON_TYPES[type || ""];
                var preview: any = typeDef && typeDef.renderPreview
                    ? typeDef.renderPreview(button || {}, { escHtml: escHtml, cardSize: options.cardSize || 1 })
                    : null;
                state.timezone = oldTimezone;
                state.activeTimezone = oldActiveTimezone;
                state.temperatureUnit = oldUnit;
                state.clockFormat = oldClockFormat;
                state.language = oldLanguage;
                return preview;
            },
            buttonTypePreviewForMockNow: function (this: any, type?: any, button?: any, options?: any) {
                return withWebserverMockNow(function (this: any) {
                    return globalThis.__ESPCONTROL_TEST_HOOKS__.config.buttonTypePreviewFor(type, button, options);
                });
            },
            networkPreviewIconSlug: networkPreviewIconSlug,
            displayFirmwareVersion: displayFirmwareVersion,
            firmwareVersionLabelFor: function (this: any, version?: any, pending?: any) {
                var oldVersion: any = state.firmwareVersion;
                var oldPending: any = state.firmwareVersionRefreshPending;
                state.firmwareVersion = version;
                state.firmwareVersionRefreshPending = !!pending;
                var label: any = firmwareVersionLabel();
                state.firmwareVersion = oldVersion;
                state.firmwareVersionRefreshPending = oldPending;
                return label;
            },
            importedButtonOrderFor: function (this: any, orderStr?: any, existingSizes?: any) {
                var oldSizes: any = state.sizes;
                var oldGrid: any = state.grid;
                state.sizes = existingSizes || {};
                state.grid = [];
                for (var i: any = 0; i < NUM_SLOTS; i++)
                    state.grid.push(0);
                applyImportedButtonOrder(orderStr, {});
                var sizes: any = {};
                for (var k in state.sizes)
                    sizes[k] = state.sizes[k];
                var grid: any = state.grid.slice();
                state.sizes = oldSizes;
                state.grid = oldGrid;
                return { grid: grid, sizes: sizes };
            },
        });
    }
    return {};
}
