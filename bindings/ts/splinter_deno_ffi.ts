// deno_ffi.ts
// manually updated when public header changes
// PRs to automate this are welcome!

export const Libsplinter = Deno.dlopen("../../libsplinter.so", {
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
  "splinter_open": { 
    parameters: ["buffer"], 
    result: "i32" 
  },
  "splinter_set": { 
    parameters: ["buffer", "pointer", "usize"], 
    result: "i32" 
  },
  "splinter_unset": { 
    parameters: ["buffer"], 
    result: "i32" 
  },
  "splinter_get": {
    parameters: ["buffer", "pointer", "usize", "pointer"],
    result: "i32",
  },
  "splinter_list": { 
    parameters: ["pointer", "usize", "pointer"], 
    result: "i32" 
  },
  "splinter_poll": { 
    parameters: ["buffer", "u64"], 
    result: "i32" 
  },
  "splinter_close": { 
    parameters: [], 
    result: "void" 
  }
});
