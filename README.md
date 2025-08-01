# Splinter 🐀

> _“I am always here, my sons. Even when unseen.”_

**Splinter** is a minimalist, high-speed message bus using shared memory
(`memfd`) or memory-mapped files for persistent backing. Designed to support AI
agents and ephemeral memory workflows, Splinter sits behind the scenes —
synchronizing state between inference, retrieval, UI, and coordination layers.

Inspired by the wisdom and discretion of _Master Splinter_, this bus never
speaks unless spoken to, but is always ready.

---

## Key Features

- 🧠 Shared memory layout: low-overhead, mmap-based store
- 📥 `set`, `get`, `list`, and `poll` operations
- 🧵 Thread-safe single-writer, multi-reader semantics
- 🕰️ Built-in version tracking via atomic epoch counters
- 🔧 Configurable slot count and max value size
- 💾 Optional persistent mode via file-backed `mmap`

---

## Directory layout

```
splinter/
├── README.md          ← this doc
├── splinter.h         ← public API
├── splinter.c         ← implementation
├── splinter_test.c    ← example / smoke test
├── Makefile           ← build & test (memory + persistent builds)
├── bindings/rust/     ← Rust bindings (auto generated)
└── bindings/ts/
    └── deno_ffi.ts    ← Deno FFI interface (manually updated)
```

---

## Building

```bash
make        # builds both memory and persistent modes
./splinter_test
```

This builds:

- `libsplinter.so` ← default memory-backed bus
- `libsplinter_p.so` ← persistent file-backed variant

---

## Runtime Design

Each Splinter instance is backed by a named region, either:

- Memory-only (via `memfd_create()` or `shm_open()`)
- Persistent (via `open()` + `mmap()` on a regular file)

Slot region layout includes:

- Key (up to 63 chars)
- Value length + offset
- Atomic epoch version
- FNV-1a 64-bit hash of key

Slot selection uses linear probing; polling uses version-check spin.

---

## Persistence Mode

If you want bus persistence between reboots, Link your application or FFI to
`libsplinter_p.so` instead of `libsplinter.so`.

The persistent version:

- Maps from a file instead of RAM.
- Survives system reboots
- Can be snapshot/dumped with `dd`, `cp`, etc.

However, understand the limitations of the underlying file system and whatever
media is backing it. Network synchronization is currently not supported, but is
being contemplated in a way that:

- nodes run memory-only stores that sync among each other over secure sockets,
  and,
- one (or more) of the memory-only stores also synchronizes to a local
  persistent store.

That may sound simple in theory, but keeping it performant in a way that also
supports workflows like tokenizing input _on the bus_ while in transit adds
rather tight constraints.

---

## Next Goals

- [ ] `splinterctl` CLI utility
- [ ] `splinterd` daemon TTL eviction or replication
- [ ] Optional history ring buffer (per key)
- [ ] QUIC-based (?) bridge protocol

Replication is lowest on the priorities right now, as development mostly matches
work on my local LLM stack Runa.

---

For developer docs, see `docs/msgbus.md` in the repo root.
