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
  restore(data: unknown, target: Target, apply: (plan: Plan) => unknown): boolean;
}

function restoreErrorMessage(error: unknown): string {
  return (error as Error & { backupMessage?: string })?.backupMessage
    || (error instanceof Error && error.message)
    || "Configuration restore failed. Check the connection and try again.";
}

/** Coordinates a restore so all entity posts use the same safe queue lifecycle. */
export function createBackupRestoreController<Plan, Target>(
  options: BackupRestoreControllerOptions<Plan, Target>,
): BackupRestoreController<Plan, Target> {
  return {
    restore(data: unknown, target: Target, apply: (plan: Plan) => unknown): boolean {
      let backupPlan: Plan;
      try {
        backupPlan = options.plan(data, target);
      } catch (error) {
        const message = (error as Error & { backupMessage?: string }).backupMessage
          || "Invalid config file \u2014 missing required fields";
        options.showBanner(message, "error");
        return false;
      }

      for (const warning of options.warnings(backupPlan)) options.showBanner(warning, "warning");

      options.setPostThrottle(75);
      options.resetPostQueueError();
      let applyCompletion: unknown;
      try {
        applyCompletion = apply(backupPlan);
      } catch (error) {
        options.setPostThrottle(0);
        throw error;
      }
      const finishApply = (): Promise<unknown> => {
        const queueCompletion = options.postQueueIdle();
        return queueCompletion.then(
          (result) => {
            options.setPostThrottle(0);
            return result;
          },
          (error) => {
            options.setPostThrottle(0);
            throw error;
          },
        );
      };
      const queueCompletion = applyCompletion && typeof (applyCompletion as PromiseLike<unknown>).then === "function"
        ? Promise.resolve(applyCompletion).then(finishApply, (error) => {
          options.setPostThrottle(0);
          throw error;
        })
        : finishApply();
      queueCompletion.then(
        () => {
          if (!options.postQueueHadError()) options.showBanner("Configuration imported successfully", "success");
        },
        (error) => options.showBanner(restoreErrorMessage(error), "error"),
      );
      return true;
    },
  };
}
