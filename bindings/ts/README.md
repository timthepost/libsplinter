# FFI Bindings & Tests For Deno

Find the FFI function definitions and supporting types in:

 - `ffi_types.ts` < types
 - `splinter_deno_ffi.ts` < symbol definitions
 - `splinter.class.ts` < all functionality in the C library available in TS;
    this is ideally what you use in your projects.
 - `splinter.class_tests.ts` < unit tests for the FFI symbols
 - `ffi_demo.ts` < Old tests, but still good for showing how the symbols work
    without the class wrapping them.

Currently, these are tested, but there's only really minimal coverage. See the
comments in the source files regarding tests that could stand to be written.

The most complicated (structures -> type mapping) + polls + CRUD is covered.

The rest would be 'nice to have', if someone wants to write them up.
