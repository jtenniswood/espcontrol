import { createBackupRestoreController, type BackupRestoreAssets } from "../../src/webserver/features/backup_restore_controller";

function equal<T>(actual: T, expected: T, message: string): void {
  if (actual !== expected) throw new Error(`${message}: expected ${String(expected)}, received ${String(actual)}`);
}
function deferred() {
  let resolve!: () => void;
  const promise = new Promise<void>((done) => { resolve = done; });
  return { promise, resolve };
}
const tick = async () => { for (let i = 0; i < 8; i++) await Promise.resolve(); };

export async function runBackupRestoreControllerTests(): Promise<void> {
  function fixture() {
    const events: string[] = [];
    let postError = false;
    let invalid = false;
    let commitFailures = 0;
    let rollbackFails = false;
    const controller = createBackupRestoreController<{ warnings: string[] }, undefined>({
      plan: () => {
        events.push("validate");
        if (invalid) throw Object.assign(new Error("invalid"), { backupMessage: "Backup is too new" });
        return { warnings: ["Different device"] };
      },
      warnings: (plan) => plan.warnings,
      showBanner: (message, kind) => events.push(`${kind}:${message}`),
      setPostThrottle: (value) => events.push(`throttle:${value}`),
      resetPostQueueError: () => { events.push("reset"); postError = false; },
      postQueueIdle: async () => { events.push("queue"); },
      postQueueHadError: () => postError,
    });
    const assets: BackupRestoreAssets<{ warnings: string[] }> = {
      stage: async () => { events.push("stage"); },
      remap: () => { events.push("remap"); },
      commit: async () => {
        events.push("commit");
        if (commitFailures-- > 0) throw new Error("Commit response lost");
      },
      rollback: async () => {
        events.push("rollback");
        if (rollbackFails) throw new Error("Rollback incomplete");
      },
    };
    return {
      events, controller, assets,
      failPosts() { postError = true; },
      rejectPlan() { invalid = true; },
      loseCommits(count: number) { commitFailures = count; },
      failRollback() { rollbackFails = true; },
    };
  }

  const success = fixture();
  const pending = deferred();
  const completion = success.controller.restore({}, undefined, () => {
    success.events.push("apply");
    return pending.promise;
  }, success.assets);
  await tick();
  equal(success.controller.stage, "applying", "completion waits for asynchronous configuration save");
  equal(success.events.join(","), "validate,warning:Different device,queue,throttle:75,reset,stage,remap,apply",
    "validation and queue drain precede staging and remapped save");
  equal(await success.controller.restore({}, undefined, () => { throw new Error("must not overlap"); }), false,
    "overlapping restores do not overwrite active transaction state");
  pending.resolve();
  equal(await completion, true, "completion promise reports success");
  equal(success.events.slice(-4).join(","), "queue,commit,success:Configuration imported successfully,throttle:0",
    "assets commit only after settings finish");
  equal(success.controller.stage, "idle", "finished coordinator can be reused");

  const invalid = fixture();
  invalid.rejectPlan();
  equal(await invalid.controller.restore({}, undefined, () => undefined, invalid.assets), false, "reject invalid backup");
  equal(invalid.events.join(","), "validate,error:Backup is too new", "invalid backup never touches images or settings");

  for (const failure of ["stage", "sync-apply", "async-apply", "posts"] as const) {
    const test = fixture();
    if (failure === "stage") test.assets.stage = async () => { throw new Error("Upload interrupted"); };
    const apply = () => {
      if (failure === "sync-apply") throw new Error("Apply failed");
      if (failure === "async-apply") return Promise.reject(new Error("Apply failed"));
      if (failure === "posts") test.failPosts();
      return undefined;
    };
    equal(await test.controller.restore({}, undefined, apply, test.assets), false, `${failure} fails restore`);
    equal(test.events.includes("rollback"), true, `${failure} compensates staged assets`);
    equal(test.events[test.events.indexOf("rollback") - 1], "queue", "queued saves settle before rollback");
    equal(test.events.includes("commit"), false, "failed save never commits assets");
    equal(test.events.some((event) => event.startsWith("success:")), false, "failure never reports success");
    equal(test.events.at(-1), "throttle:0", "failure restores queue throttle");
  }

  const lostResponse = fixture();
  lostResponse.loseCommits(1);
  equal(await lostResponse.controller.restore({}, undefined, () => undefined, lostResponse.assets), true,
    "lost commit response is retried successfully");
  equal(lostResponse.events.filter((event) => event === "commit").length, 2, "retry same transaction once");
  equal(lostResponse.events.includes("rollback"), false, "durable configuration is never rolled back after commit failure");

  const offline = fixture();
  offline.loseCommits(2);
  equal(await offline.controller.restore({}, undefined, () => undefined, offline.assets), false, "report unreachable commit");
  equal(offline.events.includes("rollback"), false, "leave durable images for reboot recovery");

  const rollbackFailure = fixture();
  rollbackFailure.failRollback();
  equal(await rollbackFailure.controller.restore({}, undefined, () => { throw new Error("Save failed"); }, rollbackFailure.assets), false,
    "failed compensation is handled by the completion promise");
  equal(rollbackFailure.events.includes("error:Save failed Rollback incomplete"), true, "report both failures");

  const legacy = fixture();
  equal(await legacy.controller.restore({}, undefined, () => undefined), true, "JSON restores work without assets");
  equal(legacy.events.includes("stage"), false, "JSON restore does not access image storage");
}
