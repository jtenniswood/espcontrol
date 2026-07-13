export type SavedConfigField =
  | "entity"
  | "label"
  | "icon"
  | "icon_on"
  | "sensor"
  | "unit"
  | "type"
  | "precision"
  | "options";

export type CardConfig = Record<SavedConfigField, string>;

export interface NormalizationCondition {
  source: "field" | "option";
  name: string;
  operator: "equals" | "in" | "present";
  value?: string | readonly string[];
  negate?: boolean;
}

export type FieldNormalizationPolicy =
  | { policy: "keep" | "clear" }
  | { policy: "default"; value: string }
  | { policy: "allowed"; values: readonly string[]; fallback: string }
  | { policy: "alias"; aliases: Readonly<Record<string, string>> }
  | { policy: "hook"; hook: string };

export interface CardNormalizationSpec {
  fields: Readonly<Record<SavedConfigField, FieldNormalizationPolicy>>;
  unknownOptions: "drop";
  canonicalOptionOrder: readonly string[];
  optionHook?: string;
  migrationActions?: readonly string[];
}

export interface MigrationActionSpec {
  when: readonly NormalizationCondition[];
  set: Partial<CardConfig>;
  hook?: string;
}

export interface CardOptionSpec {
  name: string;
  label: string;
  kind?: "choice" | "flag" | "number" | "text";
  values?: readonly string[];
  defaultValue?: string;
  min?: number;
  max?: number;
  step?: number;
  defaultValueByMode?: Readonly<Record<string, string>>;
  storage?: readonly string[];
  hidden?: boolean;
  docsHidden?: boolean;
  migration?: "drop";
  omitDefault?: boolean;
  storageField?: SavedConfigField;
  aliases?: Readonly<Record<string, string>>;
  applicability?: readonly NormalizationCondition[];
  applicabilityHook?: string;
  supportedWhen?: {
    precision?: readonly string[];
    precisionNot?: readonly string[];
    entityDomains?: readonly string[];
    never?: boolean;
  };
}

export interface CardTypeSpec {
  label: string;
  allowInSubpage: boolean;
  default: CardConfig;
  domains: readonly string[];
  pickerKey?: string;
  hidden?: boolean;
  options?: readonly CardOptionSpec[];
  normalization?: CardNormalizationSpec;
  behavior?: {
    lightTemperature?: {
      defaultRange: string;
      min: number;
      max: number;
      minMax: number;
      step: number;
      legacySensorValues?: readonly string[];
    };
    pushAction?: {
      defaultIcon: string;
      defaultIconOn: string;
    };
    internalRelay?: {
      defaultIcons: Readonly<Record<string, string>>;
      defaultIconOn: string;
    };
    media?: {
      defaultMode: string;
      legacyModes?: Readonly<Record<string, string>>;
      stateDisplayModes?: readonly string[];
      playbackServices?: Readonly<Record<string, string>>;
    };
    alarm?: {
      controlPanelValue: string;
      defaultActions: readonly string[];
      maxVisibleActions: number;
      actions: readonly {
        value: string;
        label: string;
        service: string;
        icon: string;
        legacyIcon?: string;
      }[];
    };
    climate?: {
      defaultLabelDisplay: string;
      defaultNumberDisplay: string;
      defaultTemperatureStep: string;
      precisionValues: readonly string[];
    };
    cover?: {
      commandServices: Readonly<Record<string, string>>;
    };
    lock?: {
      commandServices: Readonly<Record<string, string>>;
      toggleServices: Readonly<Record<string, string>>;
    };
  };
}

export interface GridLayout {
  slots: number;
  cols: number;
  rows: number;
  grid: readonly number[];
  sizes: Readonly<Record<string, number>>;
}

export interface SubpageConfig {
  order: readonly string[];
  buttons: readonly CardConfig[];
  backLabel?: string;
  grid?: readonly number[];
  sizes?: Readonly<Record<string, number>>;
}

export interface PanelSettings {
  button_on_color: string;
  temperature_unit: string;
  clock_bar: boolean;
  clock_bar_time?: boolean;
  network_status_icon: boolean;
  voice_services?: boolean;
  timezone: string;
  language: string;
  clock_format: string;
  screensaver_mode: string;
  screen_rotation: string;
}

export interface DeviceProfile {
  slug?: string;
  slots: number;
  public?: {
    name: string;
    docsPath: string;
    screenSize: string;
    resolution: string;
    orientation: string;
  };
  layout: {
    cols: number;
    rows: number;
    firmwareGrid: string;
    portraitCols?: number;
  };
  rotation?: {
    enabled: boolean;
    options: readonly string[];
    displayOffset?: number;
    rotateWidthCompensation?: boolean;
  };
  internalRelays?: readonly {
    key: string;
    label: string;
  }[];
  web: {
    dragMode: "swap" | "displace";
    dragAnimation: boolean;
    screen: {
      width: string;
      aspect: string;
    };
    portrait?: {
      cols: number;
      rows: number;
      screen: {
        width: string;
        aspect: string;
      };
    };
    topbar?: Readonly<Record<string, string | number>>;
    grid?: Readonly<Record<string, string | number>>;
    btn?: Readonly<Record<string, number>>;
    emptyCell?: Readonly<Record<string, number>>;
    sensorBadge?: Readonly<Record<string, number>>;
    subpageBadge?: Readonly<Record<string, number>>;
  };
  firmware: {
    build: {
      chip: string;
    };
    fonts: Readonly<Record<string, string>>;
    display?: Readonly<Record<string, string | number | boolean>>;
    package: {
      firmwareVersion: string;
      substitutions: Readonly<Record<string, string>>;
      deviceFontPackageKey?: string;
      networkCoprocessor?: boolean;
      ethernetSelectable?: boolean;
      improvSerial?: boolean;
      backlightPwmFrequency?: {
        wifi: string;
        ethernet: string;
      };
    };
  };
}

export interface BackupConfig {
  version: number;
  format?: string;
  device?: string;
  source?: {
    device: string;
    slots: number;
  };
  button_order: string;
  buttons: readonly Partial<CardConfig>[];
  subpages?: Readonly<Record<string, string | SubpageConfig>>;
  settings?: Partial<PanelSettings>;
  screen?: Readonly<Record<string, string | number | boolean>>;
}
