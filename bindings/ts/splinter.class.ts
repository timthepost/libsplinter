/**
 * TS Lib to make access to Splinter more convenient from Deno.
 * License: MIT
 */
import { Libsplinter } from "./splinter_deno_ffi.ts";

export class Splinter {
  private isOpen = false;

  /**
   * Creates and initializes a new splinter store.
   * @param nameOrPath The name of the shared memory object or path to the file
   * @param slots The total number of key-value slots to allocate
   * @param maxValueSize The maximum size in bytes for any single value
   * @throws Error if creation fails (e.g., store already exists)
   */
  static create(nameOrPath: string, slots: number, maxValueSize: number): Splinter {
    const nameBuffer = new TextEncoder().encode(nameOrPath + '\0');
    const result = Libsplinter.symbols.splinter_create(nameBuffer, BigInt(slots), BigInt(maxValueSize));
    
    if (result !== 0) {
      throw new Error(`Failed to create splinter store: ${nameOrPath}`);
    }
    
    const splinter = new Splinter();
    splinter.isOpen = true;
    return splinter;
  }

  /**
   * Opens an existing splinter store.
   * @param nameOrPath The name of the shared memory object or path to the file
   * @throws Error if opening fails (e.g., store does not exist)
   */
  static open(nameOrPath: string): Splinter {
    const nameBuffer = new TextEncoder().encode(nameOrPath + '\0');
    const result = Libsplinter.symbols.splinter_open(nameBuffer);
    
    if (result !== 0) {
      throw new Error(`Failed to open splinter store: ${nameOrPath}`);
    }
    
    const splinter = new Splinter();
    splinter.isOpen = true;
    return splinter;
  }

  /**
   * Opens an existing splinter store, or creates it if it does not exist.
   * @param nameOrPath The name of the shared memory object or path to the file
   * @param slots The total number of key-value slots if creating
   * @param maxValueSize The maximum value size in bytes if creating
   * @throws Error if operation fails
   */
  static openOrCreate(nameOrPath: string, slots: number, maxValueSize: number): Splinter {
    const nameBuffer = new TextEncoder().encode(nameOrPath + '\0');
    const result = Libsplinter.symbols.splinter_open_or_create(nameBuffer, BigInt(slots), BigInt(maxValueSize));
    
    if (result !== 0) {
      throw new Error(`Failed to open or create splinter store: ${nameOrPath}`);
    }
    
    const splinter = new Splinter();
    splinter.isOpen = true;
    return splinter;
  }

  /**
   * Creates a new splinter store, or opens it if it already exists.
   * @param nameOrPath The name of the shared memory object or path to the file
   * @param slots The total number of key-value slots if creating
   * @param maxValueSize The maximum value size in bytes if creating
   * @throws Error if operation fails
   */
  static createOrOpen(nameOrPath: string, slots: number, maxValueSize: number): Splinter {
    const nameBuffer = new TextEncoder().encode(nameOrPath + '\0');
    const result = Libsplinter.symbols.splinter_create_or_open(nameBuffer, BigInt(slots), BigInt(maxValueSize));
    
    if (result !== 0) {
      throw new Error(`Failed to create or open splinter store: ${nameOrPath}`);
    }
    
    const splinter = new Splinter();
    splinter.isOpen = true;
    return splinter;
  }

  private constructor() {}

  private checkOpen(): void {
    if (!this.isOpen) {
      throw new Error("Splinter store is not open");
    }
  }

  /**
   * Sets or updates a key-value pair in the store.
   * @param key The key string
   * @param value The value data (string, Uint8Array, or any serializable object)
   * @throws Error if operation fails (e.g., store is full)
   */
  set(key: string, value: string | Uint8Array | unknown): void {
    this.checkOpen();
    
    const keyBuffer = new TextEncoder().encode(key + '\0');
    let valueData: Uint8Array;
    
    if (typeof value === 'string') {
      valueData = new TextEncoder().encode(value);
    } else if (value instanceof Uint8Array) {
      valueData = value;
    } else {
      // Serialize as JSON for other types
      valueData = new TextEncoder().encode(JSON.stringify(value));
    }
    
    const result = Libsplinter.symbols.splinter_set(
      keyBuffer,
      Deno.UnsafePointer.of(<BufferSource> valueData),
      BigInt(valueData.length)
    );
    
    if (result !== 0) {
      throw new Error(`Failed to set key: ${key}`);
    }
  }

  /**
   * Removes a key-value pair from the store.
   * @param key The key to remove
   * @returns The length of the deleted value, or null if key not found
   * @throws Error if operation fails
   */
  unset(key: string): number | null {
    this.checkOpen();
    
    const keyBuffer = new TextEncoder().encode(key + '\0');
    const result = Libsplinter.symbols.splinter_unset(keyBuffer);
    
    if (result === -2) {
      throw new Error("Invalid key or store not open");
    }
    
    return result === -1 ? null : result;
  }

  /**
   * Retrieves the raw bytes for a key.
   * @param key The key to look up
   * @returns The raw value data as Uint8Array, or null if key not found
   * @throws Error if operation fails
   */
  getRaw(key: string): Uint8Array | null {
    this.checkOpen();
    
    const keyBuffer = new TextEncoder().encode(key + '\0');
    const outSizePtr = new BigUint64Array(1);
    
    // First call to get the size
    let result = Libsplinter.symbols.splinter_get(
      keyBuffer,
      null,
      BigInt(0),
      Deno.UnsafePointer.of(outSizePtr)
    );
    
    if (result !== 0) {
      return null; // Key not found or other error
    }
    
    const size = Number(outSizePtr[0]);
    if (size === 0) {
      return new Uint8Array(0);
    }
    
    // Second call to get the actual data
    const buffer = new Uint8Array(size);
    result = Libsplinter.symbols.splinter_get(
      keyBuffer,
      Deno.UnsafePointer.of(buffer),
      BigInt(size),
      Deno.UnsafePointer.of(outSizePtr)
    );
    
    if (result !== 0) {
      throw new Error(`Failed to get value for key: ${key}`);
    }
    
    return buffer;
  }

  /**
   * Retrieves a value as a string.
   * @param key The key to look up
   * @returns The value as a UTF-8 string, or null if key not found
   */
  getString(key: string): string | null {
    const data = this.getRaw(key);
    return data ? new TextDecoder().decode(data) : null;
  }

  /**
   * Retrieves a value and parses it as JSON.
   * @param key The key to look up
   * @returns The parsed JSON value, or null if key not found
   * @throws Error if the value is not valid JSON
   */
  getJSON<T = unknown>(key: string): T | null {
    const str = this.getString(key);
    return str ? JSON.parse(str) : null;
  }

  /**
   * Lists all keys currently in the store.
   * @param maxKeys Maximum number of keys to return (default: 1000)
   * @returns Array of key strings
   * @throws Error if operation fails
   */
 list(maxKeys = 1000): string[] {
    this.checkOpen();
    
    const outKeysPtr = new BigUint64Array(maxKeys);
    const outCountPtr = new BigUint64Array(1);
    
    const result = Libsplinter.symbols.splinter_list(
      Deno.UnsafePointer.of(outKeysPtr),
      BigInt(maxKeys),
      Deno.UnsafePointer.of(outCountPtr)
    );
    
    if (result !== 0) {
      throw new Error("Failed to list keys");
    }
    
    const count = Number(outCountPtr[0]);
    const keys: string[] = [];
    
    for (let i = 0; i < count; i++) {
      const strPtr = Deno.UnsafePointer.create(outKeysPtr[i]);
      if (strPtr === null) {
        throw new Error(`Invalid pointer at index ${i}`);
      }
      const cString = new Deno.UnsafePointerView(strPtr).getCString();
      keys.push(cString);
    }
    
    return keys;
  }

  /**
   * Waits for a key's value to be changed.
   * @param key The key to monitor for changes
   * @param timeoutMs The maximum time to wait in milliseconds
   * @returns true if the value changed, false on timeout
   * @throws Error if the key doesn't exist or other error occurs
   */
  poll(key: string, timeoutMs: number): boolean {
    this.checkOpen();
    
    const keyBuffer = new TextEncoder().encode(key + '\0');
    const result = Libsplinter.symbols.splinter_poll(keyBuffer, BigInt(timeoutMs));
    
    if (result === 0) {
      return true; // Value changed
    } else if (result === -1) {
      return false; // Timeout or key doesn't exist
    } else {
      throw new Error(`Poll failed for key: ${key}`);
    }
  }

  /**
   * Closes the splinter store and unmaps the shared memory region.
   * After calling this, the instance cannot be used anymore.
   */
  close(): void {
    if (this.isOpen) {
      Libsplinter.symbols.splinter_close();
      this.isOpen = false;
    }
  }

  /**
   * Checks if the store is currently open.
   */
  get opened(): boolean {
    return this.isOpen;
  }
}