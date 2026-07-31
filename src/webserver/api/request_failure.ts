import type { DeviceResult } from "./device_api";

export interface RequestFailureInfo {
  readonly message: string;
  readonly reconnect: boolean;
}

export function requestFailureInfo(result: DeviceResult, errorMessage?: string): RequestFailureInfo | null {
  if (result.ok) return null;
  if (result.kind === "network-error") {
    return { message: "Cannot reach device — is it connected?", reconnect: true };
  }
  if (result.kind === "http-error") {
    return { message: errorMessage || `Request failed: ${result.status}`, reconnect: false };
  }
  return { message: errorMessage || "Device returned an invalid response.", reconnect: false };
}
