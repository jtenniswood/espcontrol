import type { BackupFeature, BackupImportPlan, BackupTargetDevice } from "../features/backup";
import { buttonConfigDisabledForDevice } from "../features/preview";
import type { ApplicationLayoutState } from "./application_context";
import type { CardRegistry } from "./card_registry";
import type { ConfigCodecFeature } from "./config_codec";

export interface BackupContractFeature extends BackupFeature {
    planBackupImport(data: unknown, targetDevice?: BackupTargetDevice): BackupImportPlan;
}

export function createBackupContractFeature(
    backupFeature: BackupFeature,
    codec: Pick<ConfigCodecFeature, "parseSubpageConfig">,
    cards: CardRegistry,
    layout: ApplicationLayoutState,
): BackupContractFeature {
    const assertButtonSupported = (button: any): void => {
        if (!buttonConfigDisabledForDevice(
            cards.definitions,
            layout.config.disabledCardTypes || [],
            button,
        )) return;
        const type = button?.type || "";
        const label = type ? type.replace(/_/g, " ") : "switch";
        const error = new Error(`This controller does not support the ${label} card type in this backup.`) as Error & { backupMessage?: string };
        error.backupMessage = error.message;
        throw error;
    };
    const planBackupImport = (data: unknown, targetDevice?: BackupTargetDevice): BackupImportPlan => {
        const plan = backupFeature.planBackupImport(data, targetDevice);
        (plan.config.buttons || []).forEach(assertButtonSupported);
        for (const serialized of Object.values(plan.config.subpages || {})) {
            const subpage = codec.parseSubpageConfig(serialized);
            (subpage.buttons || []).forEach(assertButtonSupported);
        }
        return plan;
    };
    return {
        ...backupFeature,
        planBackupImport,
    };
}
