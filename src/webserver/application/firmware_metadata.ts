import { deviceId } from "../device_config";

export const FIRMWARE_VERSION_METADATA_PATH = "/espcontrol/version";
export const FIRMWARE_PUBLIC_MANIFEST_BASE = "https://jtenniswood.github.io/espcontrol/firmware/";

export function isSpecificFirmwareVersion(this: any, version?: any) {
        version = String(version == null ? "" : version).trim();
        return /^v[0-9]+(\.[0-9]+){2}([-+][0-9A-Za-z.-]+)?$/i.test(version);
    }
export function firmwareVersionFromMetadata(this: any, data?: any) {
        if (!data)
            return "";
        return String(data.firmware_version || data.project_version || data.version || data.current_version || "").trim();
    }
export function firmwareVersionsSame(this: any, a?: any, b?: any) {
        return String(a == null ? "" : a).trim().toLowerCase() ===
            String(b == null ? "" : b).trim().toLowerCase();
    }
export function publicFirmwareManifestUrl(this: any) {
        return FIRMWARE_PUBLIC_MANIFEST_BASE + encodeURIComponent(deviceId) + "/manifest.json";
    }
export function publicFirmwareVersionsUrl(this: any) {
        return FIRMWARE_PUBLIC_MANIFEST_BASE + encodeURIComponent(deviceId) + "/versions.json";
    }
export function publicFirmwareAssetUrl(this: any, assetPath?: any, baseUrl?: any) {
        assetPath = String(assetPath || "").trim();
        if (!assetPath)
            return "";
        try {
            return new URL(assetPath, baseUrl || publicFirmwareManifestUrl()).href;
        }
        catch (err) {
            if (/^https?:\/\//i.test(assetPath))
                return assetPath;
            return FIRMWARE_PUBLIC_MANIFEST_BASE + encodeURIComponent(deviceId) + "/" + assetPath.replace(/^\/+/, "");
        }
    }
export function firmwareInfoFromPublicManifest(this: any, data?: any, baseUrl?: any) {
        if (!data || typeof data !== "object")
            return null;
        var version: any = String(data.version || "").trim();
        if (!isSpecificFirmwareVersion(version))
            return null;
        var builds: any = Array.isArray(data.builds) ? data.builds : [];
        var expectedOta: any = deviceId + ".ota.bin";
        for (var i: any = 0; i < builds.length; i++) {
            var build: any = builds[i] || {};
            var ota: any = build.ota || {};
            var otaPath: any = String(ota.path || "").trim();
            if (otaPath === expectedOta) {
                return {
                    latest_version: version,
                    release_url: String(ota.release_url || "").trim(),
                    ota_url: publicFirmwareAssetUrl(otaPath, baseUrl || publicFirmwareManifestUrl()),
                    ota_filename: expectedOta,
                    ota_md5: String(ota.md5 || "").trim(),
                };
            }
        }
        return null;
    }
export function firmwareInfoFromPublicVersionEntry(this: any, entry?: any, baseUrl?: any) {
        if (!entry || typeof entry !== "object")
            return null;
        var version: any = String(entry.version || entry.latest_version || "").trim();
        if (!isSpecificFirmwareVersion(version))
            return null;
        var ota: any = entry.ota && typeof entry.ota === "object" ? entry.ota : entry;
        var otaPath: any = String(ota.path || entry.ota_path || "").trim();
        var expectedOta: any = deviceId + ".ota.bin";
        if (!otaPath)
            return null;
        var filename: any = otaPath.split("/").pop();
        if (filename !== expectedOta)
            return null;
        return {
            latest_version: version,
            release_url: String(entry.release_url || ota.release_url || "").trim(),
            ota_url: publicFirmwareAssetUrl(otaPath, baseUrl || publicFirmwareVersionsUrl()),
            ota_filename: expectedOta,
            ota_md5: String(ota.md5 || entry.ota_md5 || "").trim(),
        };
    }
export function firmwareInfosFromPublicVersions(this: any, data?: any, baseUrl?: any) {
        if (!data || typeof data !== "object")
            return [];
        var entries: any = Array.isArray(data.versions) ? data.versions : [];
        var seen: any = {};
        var infos: any = [];
        for (var i: any = 0; i < entries.length; i++) {
            var info: any = firmwareInfoFromPublicVersionEntry(entries[i], baseUrl || publicFirmwareVersionsUrl());
            if (!info || seen[info.latest_version.toLowerCase()])
                continue;
            seen[info.latest_version.toLowerCase()] = true;
            infos.push(info);
        }
        return infos;
    }
