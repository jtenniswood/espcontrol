// Firmware version and public release metadata helpers.

var FIRMWARE_VERSION_METADATA_PATH = "/espcontrol/version";
var FIRMWARE_PUBLIC_MANIFEST_BASE = "https://jtenniswood.github.io/espcontrol/firmware/";

function isSpecificFirmwareVersion(version) {
  version = String(version == null ? "" : version).trim();
  return /^v[0-9]+(\.[0-9]+){2}([-+][0-9A-Za-z.-]+)?$/i.test(version);
}

function firmwareVersionFromMetadata(data) {
  if (!data) return "";
  return String(data.firmware_version || data.project_version || data.version || data.current_version || "").trim();
}

function firmwareVersionsSame(a, b) {
  return String(a == null ? "" : a).trim().toLowerCase() ===
    String(b == null ? "" : b).trim().toLowerCase();
}

function publicFirmwareManifestUrl() {
  return FIRMWARE_PUBLIC_MANIFEST_BASE + encodeURIComponent(DEVICE_ID) + "/manifest.json";
}

function publicFirmwareVersionsUrl() {
  return FIRMWARE_PUBLIC_MANIFEST_BASE + encodeURIComponent(DEVICE_ID) + "/versions.json";
}

function publicFirmwareAssetUrl(assetPath, baseUrl) {
  assetPath = String(assetPath || "").trim();
  if (!assetPath) return "";
  try {
    return new URL(assetPath, baseUrl || publicFirmwareManifestUrl()).href;
  } catch (err) {
    if (/^https?:\/\//i.test(assetPath)) return assetPath;
    return FIRMWARE_PUBLIC_MANIFEST_BASE + encodeURIComponent(DEVICE_ID) + "/" + assetPath.replace(/^\/+/, "");
  }
}

function firmwareInfoFromPublicManifest(data, baseUrl) {
  if (!data || typeof data !== "object") return null;
  var version = String(data.version || "").trim();
  if (!isSpecificFirmwareVersion(version)) return null;
  var builds = Array.isArray(data.builds) ? data.builds : [];
  var expectedOta = DEVICE_ID + ".ota.bin";
  for (var i = 0; i < builds.length; i++) {
    var build = builds[i] || {};
    var ota = build.ota || {};
    var otaPath = String(ota.path || "").trim();
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

function firmwareInfoFromPublicVersionEntry(entry, baseUrl) {
  if (!entry || typeof entry !== "object") return null;
  var version = String(entry.version || entry.latest_version || "").trim();
  if (!isSpecificFirmwareVersion(version)) return null;
  var ota = entry.ota && typeof entry.ota === "object" ? entry.ota : entry;
  var otaPath = String(ota.path || entry.ota_path || "").trim();
  var expectedOta = DEVICE_ID + ".ota.bin";
  if (!otaPath) return null;
  var filename = otaPath.split("/").pop();
  if (filename !== expectedOta) return null;
  return {
    latest_version: version,
    release_url: String(entry.release_url || ota.release_url || "").trim(),
    ota_url: publicFirmwareAssetUrl(otaPath, baseUrl || publicFirmwareVersionsUrl()),
    ota_filename: expectedOta,
    ota_md5: String(ota.md5 || entry.ota_md5 || "").trim(),
  };
}

function firmwareInfosFromPublicVersions(data, baseUrl) {
  if (!data || typeof data !== "object") return [];
  var entries = Array.isArray(data.versions) ? data.versions : [];
  var seen = {};
  var infos = [];
  for (var i = 0; i < entries.length; i++) {
    var info = firmwareInfoFromPublicVersionEntry(entries[i], baseUrl || publicFirmwareVersionsUrl());
    if (!info || seen[info.latest_version.toLowerCase()]) continue;
    seen[info.latest_version.toLowerCase()] = true;
    infos.push(info);
  }
  return infos;
}
