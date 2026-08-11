import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
import { createNativePanelConfigClient, updateNativePanelConfigDocument } from "../features/native_panel_config";

export function installNativePanelConfigMigrationModule(): GlobalDescriptors {
    // New panels store the card configuration as one atomic document. Older
    // firmware remains on the established per-entity API until it advertises
    // the native document capability.
    var _nativePanelConfigClient: any = createNativePanelConfigClient(function (path: any, request: any) {
        return fetch(path, request);
    });
    var _nativePanelConfigSaveQueue: any = Promise.resolve("saved");
    var NATIVE_PANEL_CONFIG_RETRY_DELAY_MS: any = 250;

    function beginNativePanelConfigMigration(this: any) {
        if (typeof fetch !== "function")
            return Promise.resolve(false);
        return _nativePanelConfigClient.discover();
    }
    function nativePanelConfigMigrationSupported(this: any) {
        return _nativePanelConfigClient.supported();
    }
    function reportNativePanelConfigSave(this: any, result: any) {
        if (result === "conflict")
            showBanner("Configuration changed in another browser. Reload before saving again.", "error");
        else if (result === "mirror-failed")
            showBanner("The configuration saved, but its older-firmware copy did not. Do not downgrade this panel yet.", "error");
        else if (result === "failed")
            showBanner("Could not save the configuration. Check the connection and try again.", "error");
        return result;
    }
    function waitForNativePanelConfigDiscovery(this: any) {
        return beginNativePanelConfigMigration().then(function (supported: any) {
            if (supported || !_nativePanelConfigClient.retryable())
                return supported;
            return new Promise(function (resolve: any) {
                setTimeout(resolve, NATIVE_PANEL_CONFIG_RETRY_DELAY_MS);
            }).then(waitForNativePanelConfigDiscovery);
        });
    }
    function scheduleNativePanelConfigSave(this: any, update?: any) {
        if (!nativePanelConfigMigrationSupported() && !_nativePanelConfigClient.retryable()) {
            // A restarting panel can serve the editor before its deferred
            // native endpoints exist. Keep this edit on the established
            // entity route until discovery has positively confirmed native
            // support, rather than consuming it with a temporary 404.
            beginNativePanelConfigMigration();
            return null;
        }
        // Serializing complete-document writes keeps each GET/PUT pair tied
        // to its own promise and prevents a later edit from resolving an
        // earlier save's queue entry.
        var save: any = _nativePanelConfigSaveQueue
            .then(function () {
                if (nativePanelConfigMigrationSupported())
                    return true;
                return waitForNativePanelConfigDiscovery();
            })
            .then(function (supported: any) {
                return supported ? _nativePanelConfigClient.save(update) : "unsupported";
            })
            .then(function (result: any) {
                if (result !== "unsupported" || !_nativePanelConfigClient.retryable())
                    return result;
                return waitForNativePanelConfigDiscovery().then(function (supported: any) {
                    return supported ? _nativePanelConfigClient.save(update) : result;
                });
            })
            .then(reportNativePanelConfigSave, function () { return reportNativePanelConfigSave("failed"); });
        _nativePanelConfigSaveQueue = save;
        return save;
    }
    function nativePanelConfigTextWrite(this: any, name?: any, value?: any) {
        var entity: any = String(name || "");
        var text: any = String(value || "");
        if (entity === entityName("button_order")) {
            return scheduleNativePanelConfigSave(function (current: any) {
                return updateNativePanelConfigDocument(current, DEVICE_ID, "settings", "button_order", text);
            });
        }
        if (entity === entityName("button_on_color")) {
            text = normalizeHexColor(text, "");
            // Let the established text-entity path retain responsibility for
            // invalid input, rather than storing a value it would reject.
            if (!text)
                return false;
            return scheduleNativePanelConfigSave(function (current: any) {
                return updateNativePanelConfigDocument(current, DEVICE_ID, "settings", "button_on_color", text);
            });
        }
        for (var slot: any = 1; slot <= NUM_SLOTS; slot++) {
            if (entity === entityNameForSlot("button_config", slot)) {
                return scheduleNativePanelConfigSave(function (current: any) {
                    return updateNativePanelConfigDocument(current, DEVICE_ID, "buttons", slot, text);
                });
            }
        }
        return false;
    }
    function nativePanelConfigSubpageWrite(this: any, slot?: any, value?: any) {
        var numericSlot: any = parseInt(slot, 10);
        if (!numericSlot || numericSlot < 1 || numericSlot > NUM_SLOTS)
            return false;
        var text: any = String(value || "");
        return scheduleNativePanelConfigSave(function (current: any) {
            return updateNativePanelConfigDocument(current, DEVICE_ID, "subpages", numericSlot, text);
        });
    }
    // Browser smoke tests and non-browser consumers deliberately do not
    // install fetch. The normal editor probes as soon as it is available.
    if (typeof fetch === "function")
        beginNativePanelConfigMigration();
    return {
        "_nativePanelConfigClient": liveGlobal(() => _nativePanelConfigClient, (value?: any) => { _nativePanelConfigClient = value; }),
        "_nativePanelConfigSaveQueue": liveGlobal(() => _nativePanelConfigSaveQueue, (value?: any) => { _nativePanelConfigSaveQueue = value; }),
        "NATIVE_PANEL_CONFIG_RETRY_DELAY_MS": liveGlobal(() => NATIVE_PANEL_CONFIG_RETRY_DELAY_MS, (value?: any) => { NATIVE_PANEL_CONFIG_RETRY_DELAY_MS = value; }),
        "beginNativePanelConfigMigration": staticGlobal(beginNativePanelConfigMigration),
        "nativePanelConfigMigrationSupported": staticGlobal(nativePanelConfigMigrationSupported),
        "nativePanelConfigSubpageWrite": staticGlobal(nativePanelConfigSubpageWrite),
        "nativePanelConfigTextWrite": staticGlobal(nativePanelConfigTextWrite),
        "scheduleNativePanelConfigSave": staticGlobal(scheduleNativePanelConfigSave),
        "waitForNativePanelConfigDiscovery": staticGlobal(waitForNativePanelConfigDiscovery),
    };
}
