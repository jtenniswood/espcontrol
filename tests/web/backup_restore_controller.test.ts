import { createBackupRestoreController } from "../../src/webserver/features/backup_restore_controller";

function equal<T>(actual: T, expected: T, message: string): void {
  if (actual !== expected) throw new Error(`${message}: expected ${String(expected)}, received ${String(actual)}`);
}

export async function runBackupRestoreControllerTests(): Promise<void> {
  const events: string[] = [];
  let resolveIdle: (() => void) | undefined;
  const controller = createBackupRestoreController<{ warnings: string[] }, { device: string }>({
    plan: () => ({ warnings: ["Different device"] }),
    warnings: (plan) => plan.warnings,
    showBanner: (message, kind) => events.push(`${kind}:${message}`),
    setPostThrottle: (milliseconds) => events.push(`throttle:${milliseconds}`),
    resetPostQueueError: () => events.push("reset"),
    postQueueIdle: () => new Promise<void>((resolve) => { resolveIdle = resolve; }),
    postQueueHadError: () => false,
  });

  equal(controller.restore({}, { device: "panel" }, () => events.push("apply")), true, "valid backups start a restore");
  equal(
    events.join(","),
    "warning:Different device,throttle:75,reset,apply",
    "restore preserves warning and queued-write ordering",
  );
  resolveIdle?.();
  await Promise.resolve();
  await Promise.resolve();
  equal(events.at(-1), "success:Configuration imported successfully", "successful queue completion is reported");

  const delayedEvents: string[] = [];
  let finishApply: (() => void) | undefined;
  let finishDelayedQueue: (() => void) | undefined;
  const delayed = createBackupRestoreController<{}, undefined>({
    plan: () => ({}),
    warnings: () => [],
    showBanner: (message, kind) => delayedEvents.push(`${kind}:${message}`),
    setPostThrottle: (milliseconds) => delayedEvents.push(`throttle:${milliseconds}`),
    resetPostQueueError: () => delayedEvents.push("reset"),
    postQueueIdle: () => {
      delayedEvents.push("capture-queue");
      return new Promise<void>((resolve) => { finishDelayedQueue = resolve; });
    },
    postQueueHadError: () => false,
  });
  equal(delayed.restore({}, undefined, () => new Promise<void>((resolve) => {
    delayedEvents.push("apply-pending");
    finishApply = resolve;
  })), true, "asynchronous backup application starts a restore");
  equal(delayedEvents.join(","), "throttle:75,reset,apply-pending",
    "restore retains its throttle and does not capture the queue while native discovery is pending");
  finishApply?.();
  await Promise.resolve();
  await Promise.resolve();
  equal(delayedEvents.join(","), "throttle:75,reset,apply-pending,capture-queue",
    "restore captures delayed legacy posts while retaining the device-stability throttle");
  finishDelayedQueue?.();
  await Promise.resolve();
  await Promise.resolve();
  await Promise.resolve();
  equal(delayedEvents.at(-1), "success:Configuration imported successfully",
    "delayed fallback posts finish before restore success is reported");
  equal(delayedEvents.includes("throttle:0"), true,
    "restore resets the device-stability throttle after delayed fallback posts finish");

  const rejected = createBackupRestoreController<never, undefined>({
    plan: () => { throw Object.assign(new Error("invalid"), { backupMessage: "Backup is too new" }); },
    warnings: () => [],
    showBanner: (message, kind) => events.push(`${kind}:${message}`),
    setPostThrottle: () => { throw new Error("must not enqueue invalid backup"); },
    resetPostQueueError: () => { throw new Error("must not reset invalid backup"); },
    postQueueIdle: async () => undefined,
    postQueueHadError: () => false,
  });
  equal(rejected.restore({}, undefined, () => { throw new Error("must not apply invalid backup"); }), false,
    "invalid backups are rejected before writes");
  equal(events.at(-1), "error:Backup is too new", "validation error stays user-readable");

  const failedEvents: string[] = [];
  const failed = createBackupRestoreController<{}, undefined>({
    plan: () => ({}),
    warnings: () => [],
    showBanner: (message, kind) => failedEvents.push(`${kind}:${message}`),
    setPostThrottle: (milliseconds) => failedEvents.push(`throttle:${milliseconds}`),
    resetPostQueueError: () => undefined,
    postQueueIdle: async () => undefined,
    postQueueHadError: () => false,
  });
  equal(failed.restore({}, undefined, async () => {
    throw Object.assign(new Error("verification failed"), { backupMessage: "Layout verification failed" });
  }), true, "an asynchronous restore failure is handled");
  await Promise.resolve();
  await Promise.resolve();
  await Promise.resolve();
  equal(failedEvents.includes("error:Layout verification failed"), true,
    "verification failures are shown to the user");
  equal(failedEvents.some((event) => event.startsWith("success:")), false,
    "verification failures suppress the success banner");
  equal(failedEvents.includes("throttle:0"), true, "verification failures reset throttling");
}
