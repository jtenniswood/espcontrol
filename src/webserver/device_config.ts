import type { DeviceConfig } from "./state/types";

declare const __ESPCONTROL_DEVICE_ID__: string;
declare const __ESPCONTROL_DEVICE_CONFIG__: DeviceConfig;

export const deviceId = __ESPCONTROL_DEVICE_ID__;
export const deviceConfig = __ESPCONTROL_DEVICE_CONFIG__;
