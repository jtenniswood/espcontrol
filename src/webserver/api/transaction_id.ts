export type RandomValues = (values: Uint32Array<ArrayBuffer>) => void;

export function createTransactionId(
  fillRandom: RandomValues = (values) => {
    crypto.getRandomValues(values);
  },
): number {
  const values = new Uint32Array(new ArrayBuffer(Uint32Array.BYTES_PER_ELEMENT));
  fillRandom(values);
  // Firmware reserves zero as the inactive transaction sentinel.
  return values[0] || 1;
}
