// deno_ffi.ts
// manually updated when public header changes
// PRs to automate this are welcome!

const lib = Deno.dlopen("../../libsplinter.so", {
  "splinter_create": {
    parameters: ["buffer", "usize", "usize"],
    result: "i32",
  },
    "splinter_create_or_open": {
    parameters: ["buffer", "usize", "usize"],
    result: "i32",
  },
    "splinter_open_or_create": {
    parameters: ["buffer", "usize", "usize"],
    result: "i32",
  },
  "splinter_open": { parameters: ["buffer"], result: "i32" },
  "splinter_set": { parameters: ["buffer", "pointer", "usize"], result: "i32" },
  "splinter_get": {
    parameters: ["buffer", "pointer", "usize", "pointer"],
    result: "i32",
  },
  "splinter_close": { parameters: [], result: "void" },
});

// You can just export lib here if all you need are the bindings.
// The rest of this code is just to test them.

function cstr(str: string): [Uint8Array, Deno.PointerValue] {
  const buf = new Uint8Array([...new TextEncoder().encode(str), 0]);
  return [buf, Deno.UnsafePointer.of(buf)];
}

export function testSplinterFFI() {
  const [nameBuf] = cstr("/splinter_test");
  if (lib.symbols.splinter_create(nameBuf, BigInt(32), BigInt(1024)) !== 0) {
    console.error("❌ Failed to create splinter instance");
    Deno.exit(1);
  }

  const [keyBuf] = cstr("hello");
  const valBuf = new Uint8Array(new TextEncoder().encode("from Deno!"));
  const valPtr = Deno.UnsafePointer.of(valBuf);

  if (lib.symbols.splinter_set(keyBuf, valPtr, BigInt(valBuf.byteLength)) !== 0) {
    console.error("❌ splinter_set failed");
    Deno.exit(1);
  }

  const out = new Uint8Array(128);
  const outPtr = Deno.UnsafePointer.of(out);
  const outLen = new Uint32Array(1);
  const outLenPtr = Deno.UnsafePointer.of(outLen);

  if (
    lib.symbols.splinter_get(keyBuf, outPtr, BigInt(out.byteLength), outLenPtr) !== 0
  ) {
    console.error("❌ splinter_get failed");
    Deno.exit(1);
  }

  const result = new TextDecoder().decode(out.subarray(0, outLen[0]));
  console.log("✅ Success:", result); // → should log: from Deno!

  lib.symbols.splinter_close();
}

// Auto-run for local smoke test
if (import.meta.main) testSplinterFFI();
