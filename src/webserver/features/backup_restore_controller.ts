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
  restore(data: unknown, target: Target, apply: (plan: Plan) => unknown,
          assets?: BackupRestoreAssets<Plan>): Promise<boolean>;
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
      try {
        for (const warning of options.warnings(plan)) options.showBanner(warning, "warning");
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
        await apply(plan);
        await options.postQueueIdle();
        if (options.postQueueHadError()) {
          throw new Error("Configuration restore failed. Check the connection and try again.");
        }
        configurationPersisted = true;
        if (assets) {
          stage = "committing";
          try {
            await assets.commit();
          } catch {
            // A lost response is safe to retry with the same durable session.
            await assets.commit();
          }
        }
        options.showBanner("Configuration imported successfully", "success");
        return true;
      } catch (error) {
        if (assets && assetsStarted && !configurationPersisted) {
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
        // Once configuration is durable, preserve its images for device recovery.
        options.showBanner(restoreErrorMessage(error), "error");
        return false;
      } finally {
        options.setPostThrottle(0);
        stage = "idle";
      }
    },
  };
}
