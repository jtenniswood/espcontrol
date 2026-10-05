import { initializeDeviceConfig } from "../../src/webserver/device_config";
import { initializeAppState, state } from "../../src/webserver/state/app_instance";
import { createFirmwareUpdateFeature } from "../../src/webserver/application/firmware_update_state";
import { createStateLoaderFeature } from "../../src/webserver/application/state_loader_api";
import { createFirmwareVersionFeature } from "../../src/webserver/application/firmware_version_state";
import { createPublicFirmwareInstallFeature } from "../../src/webserver/application/public_firmware_install";

function equal(actual: unknown, expected: unknown, message: string) {
  if (actual !== expected) throw new Error(`${message}: expected ${expected}, got ${actual}`);
}
export async function runFirmwareUpdateTests() {
  const globals = globalThis as any;
  globals.__ESPCONTROL_DEFAULT_DEVICE_ID__ = "test";
  globals.__ESPCONTROL_DEVICE_PROFILES__ = { test: { slots: 1 } };
  globals.__ESPCONTROL_TIMEZONE_OPTIONS__ = [];
  initializeDeviceConfig();
  initializeAppState();
  const realTimeout = globalThis.setTimeout;
  const realClearTimeout = globalThis.clearTimeout;
  const realNow = Date.now;
  let now = 1000;
  let poll: (() => void | Promise<void>) | undefined;
  let refreshTimeout: (() => void) | undefined;
  let pollsScheduled = 0;
  globals.setTimeout = (callback: () => void, delay: number) => {
    if (delay === 5000) { poll = callback; pollsScheduled++; }
    if (delay === 15000) refreshTimeout = callback;
    return 1;
  };
  globals.clearTimeout = () => {};
  Date.now = () => now;
  const status = { style: {}, innerHTML: "", className: "" };
  const runtime = { els: { fwStatus: status } } as any;
  let updates: ReturnType<typeof createFirmwareUpdateFeature>;
  const version = createFirmwareVersionFeature(runtime, {
    syncVersionSelect() {}, renderUpdateStatus() {},
    stopInstallRefreshIfComplete() { updates.stopInstallRefreshIfComplete(); },
  });
  let nativeInstalls = 0;
  let refreshVersion: () => Promise<void> = async () => {};
  updates = createFirmwareUpdateFeature(runtime, "test", version, {
    postInstall() { nativeInstalls++; }, refreshVersion: () => refreshVersion(), installViaWebOta() {}, c6UpdateKnownAvailable: () => false,
  });
  try {
    version.set("dev");
    updates.setInfo({ state: "NO UPDATE", latest_version: "v2.11.0" });
    updates.setPublicInfo({ latest_version: "v2.11.0" });
    equal(state.firmwareVersion, "Dev build", "public metadata must not invent an installed release");
    state.firmwareInstallTargetVersion = "v2.11.0";
    state.firmwareInstallPostPending = true;
    updates.startInstallRefresh();
    updates.setInfo({ state: "NO UPDATE", current_version: "dev", latest_version: "v2.11.0" });
    equal(nativeInstalls, 0, "NO UPDATE must preserve the browser fallback, not trigger native install");
    equal(state.firmwareInstallPostPending, true, "latest-release fallback remains pending");
    updates.stopInstallRefresh();
    version.set("v2.11.0");
    state.firmwareInstallTargetVersion = "v2.8.6";
    state.firmwareInstallStatus = "Uploading firmware v2.8.6…";
    updates.startInstallRefresh();
    updates.setInfo({ state: "NO UPDATE", current_version: "v2.11.0" });
    equal(state.firmwareUpdateState, "INSTALLING", "latest-release status must not cancel a downgrade");
    equal(state.firmwareVersion, "v2.11.0", "requested version is not proof of installation");
    equal(status.innerHTML, state.firmwareInstallStatus, "upload progress is visible");
    now += 170000;
    updates.setInfo({ state: "NO UPDATE", current_version: "v2.11.0" });
    now += 10001;
    await poll!();
    equal(state.firmwareUpdateState, "", "routine updates must not postpone the deadline");
    const error = state.firmwareInstallError;
    equal(error.includes("could not be confirmed"), true, "timeout reports a visible failure");
    updates.setInfo({ state: "NO UPDATE", current_version: "v2.11.0" });
    equal(state.firmwareInstallError, error, "polling preserves the failure");
    state.firmwareInstallTargetVersion = "v2.8.6";
    updates.startInstallRefresh();
    updates.setInfo({ state: "UPDATE AVAILABLE", current_version: "v2.8.6", latest_version: "v2.11.0" });
    equal(state.firmwareInstallTargetVersion, "", "actual version confirms a downgrade despite a newer release");
    equal(state.firmwareInstallStatus, "Firmware v2.8.6 installed.", "confirmed result is visible");
    // A last response arriving after the nominal deadline can still confirm success.
    version.set("v2.11.0");
    state.firmwareInstallError = "Previous failure";
    state.firmwareInstallTargetVersion = "v2.8.6";
    updates.startInstallRefresh();
    equal(state.firmwareInstallError, "", "a fresh native attempt clears an old error");
    let finishRefresh!: () => void;
    refreshVersion = () => new Promise<void>(resolve => { finishRefresh = resolve; });
    now += 180001;
    const finalPoll = poll!();
    equal(state.firmwareInstallTargetVersion, "v2.8.6", "deadline waits for the final response");
    updates.setInfo({ state: "NO UPDATE", current_version: "v2.11.0" });
    equal(state.firmwareUpdateState, "INSTALLING", "final check keeps controls busy");
    version.set("v2.8.6");
    finishRefresh();
    await finalPoll;
    equal(state.firmwareInstallError, "", "late success must not leave a timeout error");
    equal(state.firmwareInstallStatus, "Firmware v2.8.6 installed.", "late response confirms installation");

    // A pending check from a stopped attempt must not time out a replacement attempt.
    version.set("v2.11.0");
    state.firmwareInstallTargetVersion = "v2.8.6";
    updates.startInstallRefresh();
    now += 180001;
    const oldPoll = poll!();
    updates.stopInstallRefresh();
    state.firmwareInstallTargetVersion = "v2.9.0";
    updates.startInstallRefresh();
    const scheduledBeforeCompletion = pollsScheduled;
    finishRefresh();
    await oldPoll;
    equal(state.firmwareInstallTargetVersion, "v2.9.0", "old poll cannot clear a new target");
    equal(pollsScheduled, scheduledBeforeCompletion, "old poll cannot schedule duplicate polling");

    // A panel that never responds must still reach a visible, bounded timeout.
    now += 180001;
    const offlinePoll = poll!();
    refreshTimeout!();
    await offlinePoll;
    equal(state.firmwareInstallError.includes("could not be confirmed"), true, "hung final request times out visibly");

    // The production loader's promise includes the actual version request.
    const responses: (() => void)[] = [];
    const request = (_path: unknown, callback?: (data: any) => void) => {
      const index = responses.length;
      return new Promise(resolve => responses.push(() => {
        if (index === 0) callback?.({ firmware_version: "v2.8.6" });
        resolve(null);
      }));
    };
    const loader = createStateLoaderFeature(runtime, {} as any, {} as any, version, updates, {} as any,
      { entityLookupNames: () => [], rememberEntityPostPath() {} } as any, {} as any,
      { getJsonQuietly: request, getJsonFirst: request, entityDetailPaths: () => [] } as any,
      {} as any, { subpageEntityKeys: () => [], connectEvents() {} });
    state.firmwareInstallTargetVersion = "v2.8.6";
    updates.startInstallRefresh();
    let refreshed = false;
    const loaded = loader.refreshFirmwareVersion().then(() => { refreshed = true; });
    responses.slice(1).forEach(resolve => resolve());
    await Promise.resolve();
    await Promise.resolve();
    equal(refreshed, false, "loader waits for the outstanding device version response");
    responses[0]!();
    await loaded;
    equal(state.firmwareInstallStatus, "Firmware v2.8.6 installed.", "loader applies version before completing");

    // A slow firmware transfer does not consume the confirmation window.
    updates.stopInstallRefresh();
    version.set("v2.11.0");
    const pollsBeforeTransfer = pollsScheduled;
    let finishDownload!: (result: any) => void;
    let finishUpload!: (result: any) => void;
    const delayedTransfer = createPublicFirmwareInstallFeature({ request: (url: string) => url === "/update"
      ? new Promise(resolve => { finishUpload = resolve; })
      : new Promise(resolve => { finishDownload = resolve; }) } as any, "test", updates,
      { setConfigLocked() {}, showBanner() {} } as any,
      { getJsonQuietly: async () => {} } as any, { connect() {} });
    const installing = delayedTransfer.installPublicFirmwareViaWebOta({
      latest_version: "v2.8.6", ota_url: "https://example.test/fw.bin",
    });
    for (let i = 0; i < 10 && !finishDownload; i++) await Promise.resolve();
    finishDownload({ kind: "success", value: { ok: true, blob: async () => new Blob(["firmware"]) } });
    for (let i = 0; i < 10 && !finishUpload; i++) await Promise.resolve();
    equal(typeof finishUpload, "function", "firmware download proceeds to the device upload");
    now += 181000;
    equal(pollsScheduled, pollsBeforeTransfer, "transfer does not start the confirmation timer");
    equal(state.firmwareInstallError.includes("could not be confirmed"), false, "long transfer cannot time out confirmation");
    finishUpload({ kind: "success", value: { ok: true, text: async () => "Update Successful!" } });
    await installing;
    equal(pollsScheduled, pollsBeforeTransfer + 2, "confirmation and reconnect timers start when upload completes");

    let banner = "";
    const upload = createPublicFirmwareInstallFeature({ request: async (url: string) => url === "/update"
      ? { kind: "network-error", error: new Error("Load failed") }
      : { kind: "success", value: new Response("firmware") } } as any, "test", updates,
      { setConfigLocked() {}, showBanner(message: string) { banner = message; } } as any,
      { getJsonQuietly: async () => {} } as any, { connect() {} });
    version.set("v2.11.0");
    equal(await upload.installPublicFirmwareViaWebOta({ latest_version: "v2.8.6", ota_url: "https://example.test/fw.bin" }), false,
      "a dropped upload connection must not report success");
    equal(banner, "", "a dropped connection must not claim firmware was uploaded");
    equal(status.innerHTML.includes("Upload connection lost"), true, "uncertain upload is visible");
    upload.failPublicFirmwareUpload("Device rejected firmware upload (500).");
    updates.setInfo({ state: "NO UPDATE", current_version: "v2.11.0" });
    equal(status.innerHTML.includes("Device rejected"), true, "rejected upload remains visible after refresh");
  } finally {
    updates.stopInstallRefresh();
    globals.setTimeout = realTimeout;
    globals.clearTimeout = realClearTimeout;
    Date.now = realNow;
  }
}
