import { CARD_RUNTIME_SPECS } from "../generated/card_contract";

export type CardDefinitionValue<T> = T | (() => T);

export interface CardDefinition {
  readonly key?: string;
  readonly label?: CardDefinitionValue<string>;
  readonly allowInSubpage?: CardDefinitionValue<boolean>;
  readonly hideLabel?: boolean;
  readonly labelPlaceholder?: string | null;
  readonly pickerKey?: CardDefinitionValue<string | null>;
  readonly hidden?: CardDefinitionValue<boolean>;
  readonly isAvailable?: (() => boolean) | null;
  readonly onSelect?: ((button: any) => void) | null;
  readonly renderSettingsBeforeLabel?: ((...args: any[]) => unknown) | null;
  readonly renderSettings?: ((...args: any[]) => unknown) | null;
  readonly renderPreview?: ((...args: any[]) => unknown) | null;
  readonly contextMenuItems?: ((...args: any[]) => unknown) | null;
  readonly cardMetadata?: Record<string, any> | null;
  readonly runtimeSpec?: unknown;
  readonly defaultConfig?: CardDefinitionValue<unknown> | null;
  readonly normalizeConfig?: ((...args: any[]) => unknown) | null;
  readonly [name: string]: unknown;
}

export type CardDefinitions = Record<string, CardDefinition>;

export interface CardRegistry {
  readonly definitions: CardDefinitions;
  readonly typedDefinitionCount: number;
  register(key: string, definition: CardDefinition): CardDefinition;
}

export interface CardUiServices {
    renderPreview(): void;
    renderButtonSettings(force?: boolean): void;
}

const DEFAULT_DEFINITION: CardDefinition = {
  label: "Toggle",
  allowInSubpage: false,
  hideLabel: false,
  labelPlaceholder: null,
  pickerKey: null,
  isAvailable: null,
  onSelect: null,
  renderSettingsBeforeLabel: null,
  renderSettings: null,
  renderPreview: null,
  contextMenuItems: null,
  cardMetadata: null,
  runtimeSpec: null,
  defaultConfig: null,
  normalizeConfig: null,
};

export function createCardRegistry(): CardRegistry {
  let definitions: CardDefinitions = {};
  let typedDefinitionCount = 0;

  const registerDefinition = (key: string, definition: CardDefinition): CardDefinition => {
    const registered = {
      ...DEFAULT_DEFINITION,
      key,
      label: key || "Toggle",
      ...definition,
      runtimeSpec: definition.runtimeSpec || CARD_RUNTIME_SPECS[key] || null,
    };
    definitions[key] = registered;
    return registered;
  };

  return {
    get definitions() {
      return definitions;
    },
    get typedDefinitionCount() {
      return typedDefinitionCount;
    },
    register(key, definition) {
      typedDefinitionCount += 1;
      return registerDefinition(key, definition);
    },
  };
}
