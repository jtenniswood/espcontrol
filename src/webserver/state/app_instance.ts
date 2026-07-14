import { deviceConfig } from "../device_config";
import { createInitialState } from "./app_state";
import type { AppState } from "./types";

export let state: AppState;

export function initializeAppState(): void {
  state = createInitialState(deviceConfig);
}
