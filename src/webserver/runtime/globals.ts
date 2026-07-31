export type GlobalDescriptors = PropertyDescriptorMap;

export function liveGlobal(get: () => unknown, set: (value: unknown) => void): PropertyDescriptor {
  return { configurable: true, enumerable: false, get, set };
}

export function staticGlobal(value: unknown): PropertyDescriptor {
  return { configurable: true, enumerable: false, writable: true, value };
}

export function installGlobals(descriptors: GlobalDescriptors): void {
  Object.defineProperties(globalThis, descriptors);
}

export function installStaticGlobals(values: Record<string, unknown>): void {
  for (const [name, value] of Object.entries(values)) {
    Object.defineProperty(globalThis, name, {
      configurable: true,
      enumerable: false,
      writable: true,
      value,
    });
  }
}
