import type { CardConfig } from "../contracts/types";
import type { SettingsDraft } from "../state/types";

export interface CardEditorLocation {
  readonly slot: number;
  readonly homeSlot: number | null;
  readonly isSub: boolean;
}

export interface NewCardEditorLocation extends CardEditorLocation {
  readonly pos: number;
}

export interface CardEditorDraftControllerOptions {
  readonly cloneCard: (button: CardConfig) => CardConfig;
  readonly emptyCard: () => CardConfig;
}

/** Owns editor draft identity so the browser adapter can safely rerender cards. */
export class CardEditorDraftController {
  constructor(private readonly options: CardEditorDraftControllerOptions) {}

  keyFor(location: CardEditorLocation): string {
    return `${location.isSub ? `sub:${location.homeSlot}` : "main"}:${location.slot}`;
  }

  newDraft(location: NewCardEditorLocation): SettingsDraft {
    return {
      key: `${location.isSub ? `sub:${location.homeSlot}` : "main"}:new:${location.pos}:${location.slot}`,
      ...location,
      isNew: true,
      dirty: false,
      typeSelected: false,
      button: this.options.emptyCard(),
    };
  }

  matchesNewDraft(draft: SettingsDraft | null, location: CardEditorLocation): boolean {
    return !!draft && !!draft.isNew && draft.slot === location.slot && draft.isSub === location.isSub &&
      (!location.isSub || draft.homeSlot === location.homeSlot);
  }

  ensureExistingDraft(draft: SettingsDraft | null, location: CardEditorLocation, button: CardConfig): SettingsDraft {
    const key = this.keyFor(location);
    if (draft && draft.key === key) return draft;
    return { key, ...location, dirty: false, button: this.options.cloneCard(button) };
  }

  markDirty(draft: SettingsDraft | null, key: string): void {
    if (draft && draft.key === key) draft.dirty = true;
  }
}

export function createCardEditorDraftController(options: CardEditorDraftControllerOptions): CardEditorDraftController {
  return new CardEditorDraftController(options);
}
