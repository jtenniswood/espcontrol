import { deviceConfig } from "../device_config";
import { createInitialState } from "./app_state";
import type { AppState } from "./types";

export const state: AppState = createInitialState(deviceConfig);
