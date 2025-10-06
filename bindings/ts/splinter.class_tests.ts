/**
 * Test suite for Splinter class (not yet comprehensive, but getting there!)
 * Run with: deno test --allow-ffi
 */
import { assertEquals, assertThrows, assert } from "https://deno.land/std@0.208.0/assert/mod.ts";
import { Splinter } from "./splinter.class.ts";

const TEST_STORE = "splinter_unit_tests";
const TEST_SLOTS = 100;
const TEST_MAX_VALUE_SIZE = 1024;

// Helper to clean up test stores
function cleanup() {
  try {
    // in case persistent
    Deno.removeSync(TEST_STORE);
  } catch {
    // Ignore if file doesn't exist
  }
}

// cropen / set / get / 
Deno.test({
  name: "Splinter CRUD Battery (4 Operations / 3 Tests)",
  fn: () => {
    cleanup();
    const splinter = Splinter.createOrOpen(TEST_STORE, TEST_SLOTS, TEST_MAX_VALUE_SIZE);
    assert(splinter.opened);
    splinter.set("__test", "stage:1");
    const stageOne = splinter.getString("__test");
    assertEquals(stageOne, "stage:1");
    const removed = splinter.unset("__test");
    assertEquals(removed, 7);
    splinter.close();  
  },
});

// get splinter bus header 
Deno.test({
  name: "Get global atomic bus config snapshot - returns header information (2 Operations / 5 Tests)",
  fn: () => {
    cleanup();
    const splinter = Splinter.createOrOpen(TEST_STORE, TEST_SLOTS, TEST_MAX_VALUE_SIZE);
    
    const snapshot = splinter.getBusHeaderSnapshot();
    assert(snapshot.magic !== undefined);
    assert(snapshot.version !== undefined);
    assertEquals(snapshot.slots, TEST_SLOTS);
    assertEquals(snapshot.max_val_sz, TEST_MAX_VALUE_SIZE);
    assert(snapshot.epoch !== undefined);
    
    splinter.close();
    cleanup();
  },
});

// Set a key and get the slot metadata
Deno.test({
  name: "Get slot metadata - returns slot metadata (2 Operations / 4 Tests)",
  fn: () => {
    cleanup();
    const splinter = Splinter.createOrOpen(TEST_STORE, TEST_SLOTS, TEST_MAX_VALUE_SIZE);
    
    splinter.set("__test", "stage:2");
    const snapshot = splinter.getSlotSnapshot("__test");
    
    assert(snapshot.hash !== undefined);
    assert(snapshot.epoch !== undefined);
    assertEquals(snapshot.key, "__test");
    assert(snapshot.val_len > 0);
    
    splinter.close();
    cleanup();
  },
});

// set up a watch and let it timeout

// list keys, unset a key, list keys again

// set av off , set a key , set av on, get the key


