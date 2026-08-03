import { createTransactionId } from "../../src/webserver/api/transaction_id";

function equal(actual: number, expected: number, message: string): void {
  if (actual !== expected) throw new Error(`${message}: expected ${expected}, received ${actual}`);
}

export function runTransactionIdTests(): void {
  equal(
    createTransactionId((values) => {
      values[0] = 0x10203040;
    }),
    0x10203040,
    "transaction IDs retain all random bits",
  );
  equal(
    createTransactionId((values) => {
      values[0] = 0;
    }),
    1,
    "zero is remapped away from the firmware sentinel",
  );
}
