import { Libsplinter } from "./splinter_deno_ffi.ts";

function cstr(str: string): [Uint8Array, Deno.PointerValue] {
  const buf = new Uint8Array([...new TextEncoder().encode(str), 0]);
  return [buf, Deno.UnsafePointer.of(buf)];
}

export function testSplinterFFI() {
  const [nameBuf] = cstr("/splinter_debug");
  if (Libsplinter.symbols.splinter_open_or_create(nameBuf, BigInt(32), BigInt(1024)) !== 0) {
    console.error("❌ Failed to create splinter instance");
    Deno.exit(1);
  }

  const [keyBuf] = cstr("hello");
  const valBuf = new Uint8Array(new TextEncoder().encode("from Deno!"));
  const valPtr = Deno.UnsafePointer.of(valBuf);

  if (Libsplinter.symbols.splinter_set(keyBuf, valPtr, BigInt(valBuf.byteLength)) !== 0) {
    console.error("❌ splinter_set failed");
    Deno.exit(1);
  }

  const out = new Uint8Array(128);
  const outPtr = Deno.UnsafePointer.of(out);
  const outLen = new Uint32Array(1);
  const outLenPtr = Deno.UnsafePointer.of(outLen);

  if (
    Libsplinter.symbols.splinter_get(keyBuf, outPtr, BigInt(out.byteLength), outLenPtr) !== 0
  ) {
    console.error("❌ splinter_get failed");
    Deno.exit(1);
  }

  const result = new TextDecoder().decode(out.subarray(0, outLen[0]));
  console.log("✅ Success:", result); // → should log: from Deno!

  Libsplinter.symbols.splinter_close();
}

// Auto-run for local smoke test
testSplinterFFI();
