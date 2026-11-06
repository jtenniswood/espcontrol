export type BackupRestoreStage =
  "idle" | "validating" | "waiting" | "staging" | "applying" | "committing" | "rolling-back";

export interface BackupRestoreAssets<Plan> {
  stage(): Promise<void>;
  remap(plan: Plan): void;
  commit(): Promise<void>;
  rollback(): Promise<void>;
}

export interface BackupRestoreControllerOptions<Plan, Target> {
  readonly plan: (data: unknown, target: Target) => Plan;
  readonly warnings: (plan: Plan) => readonly string[];
  readonly showBanner: (message: string, kind: "warning" | "error" | "success") => void;
  readonly setPostThrottle: (milliseconds: number) => void;
  readonly resetPostQueueError: () => void;
  readonly postQueueIdle: () => Promise<unknown>;
  readonly postQueueHadError: () => boolean;
}

export interface BackupRestoreController<Plan, Target> {
  readonly stage: BackupRestoreStage;
  restore(data: unknown, target: Target, apply: (plan: Plan, progress: BackupRestoreProgress) => unknown,
          assets?: BackupRestoreAssets<Plan>): Promise<boolean>;
}

export interface BackupRestoreProgress {
  /** A lost layout-write response must not authorize destructive compensation. */
  layoutWriteStarted(): void;
  /** Commit assets as soon as their layout is durable, before unrelated settings. */
  layoutPersisted(): Promise<void>;
}

function restoreErrorMessage(error: unknown): string {
  return (error as Error & { backupMessage?: string })?.backupMessage
    || (error instanceof Error && error.message)
    || "Configuration restore failed. Check the connection and try again.";
}

/** Owns validation, asset staging, configuration completion and compensation. */
export function createBackupRestoreController<Plan, Target>(
  options: BackupRestoreControllerOptions<Plan, Target>,
): BackupRestoreController<Plan, Target> {
  let stage: BackupRestoreStage = "idle";
  return {
    get stage() { return stage; },
    async restore(data, target, apply, assets): Promise<boolean> {
      if (stage !== "idle") {
        options.showBanner("A backup restore is already in progress.", "warning");
        return false;
      }
      stage = "validating";
      let plan: Plan;
      try {
        plan = options.plan(data, target);
      } catch (error) {
        options.showBanner((error as { backupMessage?: string })?.backupMessage
          || "Invalid config file — missing required fields", "error");
        stage = "idle";
        return false;
      }
      let assetsStarted = false;
      let configurationPersisted = false;
      let layoutWriteStarted = false;
      let assetsCommitted = false;
      const commitAssets = async () => {
        if (!assets || assetsCommitted) return;
        stage = "committing";
        try { await assets.commit(); }
        catch { await assets.commit(); }
        assetsCommitted = true;
      };
      try {
        const warnings = options.warnings(plan);
        if (warnings.length) options.showBanner(warnings.join(" "), "warning");
        stage = "waiting";
        await options.postQueueIdle();
        options.setPostThrottle(75);
        options.resetPostQueueError();
        if (assets) {
          stage = "staging";
          assetsStarted = true;
          await assets.stage();
          assets.remap(plan);
        }
        stage = "applying";
        await apply(plan, {
          layoutWriteStarted() { layoutWriteStarted = true; },
          async layoutPersisted() {
            configurationPersisted = true;
            await commitAssets();
            stage = "applying";
          },
        });
        await options.postQueueIdle();
        if (options.postQueueHadError()) {
          throw new Error("Configuration restore failed. Check the connection and try again.");
        }
        configurationPersisted = true;
        await commitAssets();
        options.showBanner("Configuration imported successfully", "success");
        return true;
      } catch (error) {
        if (assets && assetsStarted && !configurationPersisted && !layoutWriteStarted) {
          stage = "rolling-back";
          try {
            // Already queued settings must settle before deleting their images.
            await options.postQueueIdle();
            await assets.rollback();
          } catch (rollbackError) {
            options.showBanner(`${restoreErrorMessage(error)} ${restoreErrorMessage(rollbackError)}`, "error");
            return false;
          }
        }
        // A layout write may have succeeded even if its response was lost.
        // Device recovery can inspect durable references; client rollback cannot.
        options.showBanner(restoreErrorMessage(error), "error");
        return false;
      } finally {
        options.setPostThrottle(0);
        stage = "idle";
      }
    },
  };
}
