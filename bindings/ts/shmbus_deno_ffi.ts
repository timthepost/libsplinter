// proper class and proper TS module for exported
// bindings coming soon-ish; just for testing for now!

const lib = Deno.dlopen("../../libshmbus.so", {
  "shmbus_open": { parameters: ["buffer"], result: "i32" },
  "shmbus_create": { parameters: ["buffer", "usize", "usize"], result: "i32" },
  "shmbus_set": { parameters: ["buffer", "pointer", "usize"], result: "i32" },
  "shmbus_get": {
    parameters: ["buffer", "pointer", "usize", "pointer"],
    result: "i32",
  },
  "shmbus_close": { parameters: [], result: "void" },
});

function cstr(str: string): [Uint8Array, Deno.PointerValue] {
  const buf = new Uint8Array([...new TextEncoder().encode(str), 0]);
  return [buf, Deno.UnsafePointer.of(buf)];
}

export function testDenoFFI() {
  const [nameBuf] = cstr("/runa_test");

  if (lib.symbols.shmbus_create(nameBuf, 32, 1024) !== 0) {
    console.error("❌ Failed to create shmbus");
    Deno.exit(1);
  }

  const [keyBuf] = cstr("hello");
  const valBuf = new Uint8Array(new TextEncoder().encode("from Deno!"));
  const valPtr = Deno.UnsafePointer.of(valBuf);

  if (lib.symbols.shmbus_set(keyBuf, valPtr, valBuf.byteLength) !== 0) {
    console.error("❌ shmbus_set failed");
    Deno.exit(1);
  }

  const out = new Uint8Array(128);
  const outPtr = Deno.UnsafePointer.of(out);
  const outLen = new Uint32Array(1);
  const outLenPtr = Deno.UnsafePointer.of(outLen);

  if (lib.symbols.shmbus_get(keyBuf, outPtr, out.byteLength, outLenPtr) !== 0) {
    console.error("❌ shmbus_get failed");
    Deno.exit(1);
  }

  const result = new TextDecoder().decode(out.subarray(0, outLen[0]));
  console.log("✅ Success:", result); // Should log: "from Deno!"

  lib.symbols.shmbus_close();
}

// auto-run
testDenoFFI();
