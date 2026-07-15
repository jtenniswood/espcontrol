import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installFirmwareUpdatePostApiModule(): GlobalDescriptors {
    // ── Firmware Update Post API ──────────────────────────────────────────
    function postFirmwareUpdateInstall(this: any) {
        var urls: any = [];
        rememberedPostUrls("button", entityName("firmware_install_update"), entityObjectIds("firmware_install_update"), "press")
            .forEach(function (this: any, url?: any) { uniquePush(urls, url); });
        entityLookupNames("firmware_install_update").forEach(function (this: any, name?: any) {
            uniquePush(urls, "/button/" + encodeURIComponent(name) + "/press");
        });
        rememberedPostUrls("update", entityName("firmware_update"), entityObjectIds("firmware_update"), "install")
            .forEach(function (this: any, url?: any) { uniquePush(urls, url); });
        entityLookupNames("firmware_update").forEach(function (this: any, name?: any) {
            uniquePush(urls, "/update/" + encodeURIComponent(name) + "/install");
        });
        post(urls, null, "Could not start firmware update.");
    }
    function postFirmwareUpdateCheck(this: any) {
        var urls: any = [];
        rememberedPostUrls("button", entityName("firmware_check_for_update"), entityObjectIds("firmware_check_for_update"), "press")
            .forEach(function (this: any, url?: any) { uniquePush(urls, url); });
        entityLookupNames("firmware_check_for_update").forEach(function (this: any, name?: any) {
            uniquePush(urls, "/button/" + encodeURIComponent(name) + "/press");
        });
        post(urls, null, "Could not check for firmware update.");
    }
    function postC6FirmwareUpdateInstall(this: any) {
        var urls: any = [];
        rememberedPostUrls("button", entityName("esp32_c6_install_update"), entityObjectIds("esp32_c6_install_update"), "press")
            .forEach(function (this: any, url?: any) { uniquePush(urls, url); });
        entityLookupNames("esp32_c6_install_update").forEach(function (this: any, name?: any) {
            uniquePush(urls, "/button/" + encodeURIComponent(name) + "/press");
        });
        post(urls, null, "Could not start WiFi firmware update.");
    }
    function postC6FirmwareUpdateCheck(this: any) {
        var urls: any = [];
        rememberedPostUrls("button", entityName("esp32_c6_check_for_update"), entityObjectIds("esp32_c6_check_for_update"), "press")
            .forEach(function (this: any, url?: any) { uniquePush(urls, url); });
        entityLookupNames("esp32_c6_check_for_update").forEach(function (this: any, name?: any) {
            uniquePush(urls, "/button/" + encodeURIComponent(name) + "/press");
        });
        post(urls, null, "Could not check WiFi firmware update.");
    }
    return {
        "postFirmwareUpdateInstall": staticGlobal(postFirmwareUpdateInstall),
        "postFirmwareUpdateCheck": staticGlobal(postFirmwareUpdateCheck),
        "postC6FirmwareUpdateInstall": staticGlobal(postC6FirmwareUpdateInstall),
        "postC6FirmwareUpdateCheck": staticGlobal(postC6FirmwareUpdateCheck),
    };
}
