import type { CardConfig } from "../contracts/types";
import { configOptionEnabled, configOptionValue } from "./config_primitives";

export const MEDIA_CARD_CONFIG_VERSION = 1 as const;

export type MediaCardMode =
  | "control_modal"
  | "play_pause"
  | "previous"
  | "next"
  | "volume"
  | "position"
  | "now_playing"
  | "cover_art"
  | "playlist";

export type MediaStateDisplay = "label" | "state";
export type MediaNowPlayingControl = "none" | "progress" | "play_pause";
export type MediaCoverArtAction = "play_pause" | "control_modal";
export type MediaControlLabelDisplay = "label" | "status";
export type MediaControlNumberDisplay = "icon" | "volume";

export interface MediaCardConfigV1 {
  version: typeof MEDIA_CARD_CONFIG_VERSION;
  entity: string;
  mode: MediaCardMode;
  stateDisplay: MediaStateDisplay;
  nowPlayingControl: MediaNowPlayingControl;
  coverArtAction: MediaCoverArtAction;
  showTrackDetails: boolean;
  secondaryEntity: string;
  controlLabelDisplay: MediaControlLabelDisplay;
  controlNumberDisplay: MediaControlNumberDisplay;
  maxVolumePercent: number;
  playlist: {
    contentId: string;
    contentType: string;
    playerSource: string;
  };
  largeNumbers: boolean;
}

const MEDIA_CARD_MODES: readonly MediaCardMode[] = [
  "control_modal",
  "play_pause",
  "previous",
  "next",
  "volume",
  "position",
  "now_playing",
  "cover_art",
  "playlist",
];

function mediaMode(value: string): MediaCardMode {
  const canonical = value === "controls" ? "play_pause" : value;
  return MEDIA_CARD_MODES.includes(canonical as MediaCardMode)
    ? canonical as MediaCardMode
    : "play_pause";
}

function boundedVolumePercent(value: string): number {
  const parsed = Number.parseInt(value, 10);
  if (!Number.isFinite(parsed)) return 100;
  return Math.max(1, Math.min(100, parsed));
}

/**
 * Converts the compact, backwards-compatible saved shape into the typed model
 * used by Media-card behaviour. Storage details must not leak past this seam.
 */
export function decodeMediaCardConfigV1(config: Partial<CardConfig>): MediaCardConfigV1 | null {
  if (config.type !== "media") return null;
  const options = config.options || "";
  let mode = mediaMode(config.sensor || "");
  if (mode === "now_playing" && configOptionEnabled(options, "media_cover_art")) {
    mode = "cover_art";
  }
  const precision = config.precision || "";
  return {
    version: MEDIA_CARD_CONFIG_VERSION,
    entity: config.entity || "",
    mode,
    stateDisplay:
      (mode === "play_pause" || mode === "position") && precision === "state"
        ? "state"
        : "label",
    nowPlayingControl:
      mode === "now_playing" && (precision === "progress" || precision === "play_pause")
        ? precision
        : "none",
    coverArtAction:
      configOptionValue(options, "cover_art_action") === "control_modal"
        ? "control_modal"
        : "play_pause",
    showTrackDetails: configOptionEnabled(options, "cover_art_details"),
    secondaryEntity: configOptionValue(options, "cover_art_secondary_entity").trim(),
    controlLabelDisplay:
      configOptionValue(options, "label_display") === "label" ? "label" : "status",
    controlNumberDisplay:
      configOptionValue(options, "number_display") === "volume" ? "volume" : "icon",
    maxVolumePercent: boundedVolumePercent(configOptionValue(options, "volume_max")),
    playlist: {
      contentId: configOptionValue(options, "playlist_content_id").trim(),
      contentType: configOptionValue(options, "playlist_content_type").trim() || "playlist",
      playerSource: configOptionValue(options, "playlist_player_source").trim(),
    },
    largeNumbers: configOptionEnabled(options, "large_numbers"),
  };
}
