import type { CardConfig } from "../contracts/types";
import type { SettingsDraft } from "../state/types";

export interface CardEditorSaveContext {
  readonly slot: number;
  readonly maxSlots: number;
  readonly isSubpage: boolean;
  readonly grid: number[];
  readonly buttons: CardConfig[];
}

export interface CardEditorSaveResult {
  readonly accepted: boolean;
  readonly isNew: boolean;
  readonly button: CardConfig | null;
  readonly saveGrid: boolean;
  readonly saveButton: boolean;
  readonly saveSubpage: boolean;
}

export interface CardEditorSaveControllerOptions {
  readonly emptyCard: () => CardConfig;
  readonly copyCard: (target: CardConfig, source: CardConfig) => void;
}

/** Applies a valid draft once and describes the persistence work the adapter must perform. */
export class CardEditorSaveController {
  constructor(private readonly options: CardEditorSaveControllerOptions) {}

  apply(draft: SettingsDraft | null, context: CardEditorSaveContext): CardEditorSaveResult {
    const rejected = (): CardEditorSaveResult => ({
      accepted: false, isNew: false, button: null, saveGrid: false, saveButton: false, saveSubpage: false,
    });
    if (!draft || draft.slot !== context.slot || draft.isSub !== context.isSubpage) return rejected();
    if (draft.isNew) {
      const pos = draft.pos ?? -1;
      if (pos < 0 || pos >= context.maxSlots || context.grid[pos] !== 0) return rejected();
      while (context.buttons.length < context.slot) context.buttons.push(this.options.emptyCard());
      const button = context.buttons[context.slot - 1]!;
      this.options.copyCard(button, draft.button);
      context.grid[pos] = context.slot;
      return {
        accepted: true, isNew: true, button,
        saveGrid: !context.isSubpage, saveButton: !context.isSubpage, saveSubpage: context.isSubpage,
      };
    }
    const button = context.buttons[context.slot - 1];
    if (!button) return rejected();
    this.options.copyCard(button, draft.button);
    return {
      accepted: true, isNew: false, button,
      saveGrid: false, saveButton: !context.isSubpage, saveSubpage: context.isSubpage,
    };
  }
}

export function createCardEditorSaveController(options: CardEditorSaveControllerOptions): CardEditorSaveController {
  return new CardEditorSaveController(options);
}
