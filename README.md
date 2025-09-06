# Splinter 🐀

> _“I am always here, my sons. Even when unseen.”_

**Splinter** is a minimalist, high-speed message bus using shared memory
(`memfd`) or memory-mapped files for persistent backing. Designed to support AI
agents and ephemeral memory workflows, Splinter sits behind the scenes —
synchronizing state between inference, retrieval, UI, and coordination layers.

Inspired by the wisdom and discretion of _[Master Splinter](https://en.wikipedia.org/wiki/Splinter_(Teenage_Mutant_Ninja_Turtles))_, this bus never
speaks unless spoken to, but is always ready.

---

## Key Features

- 🧠 Shared memory layout: low-overhead, mmap-based store
- 📥 `set`, `unset`, `get`, `list`, and `poll` operations
- 🔑 Lock-free design (utilizes atomic operations with seqlock for contention, EAGAIN for non-block operation)
- 🧹 Auto-vacuuming (always zeroes out individual keys and value regions on creation, unset and update)
- 🧵 Thread-safe single-writer, multi-reader semantics, resilient even when MRSW contract is broken at huge scale.
- 🕰️ Built-in version tracking via atomic epoch counters
- 🔧 Configurable slot count and max value size
- 💾 Optional persistent mode via file-backed `mmap`
- 🦕 Deno and Rust bindings included!

---

## Directory layout

```
splinter/
├── README.md            ← this doc
├── splinter.h           ← public API
├── splinter.c           ← implementation
├── splinter_test.c      ← example / smoke test
├── splinter_send.c      ← simple tool to send data on the bus
├── splinter_recv.c      ← simple tool to receive data on the bus
├── splinter_logtee.c    ← simple tool to drain bus keys to a file (non-destructively)
├── splinter_perf.c      ← MRSW test harness (16 threads) for read concurrency
├── Makefile             ← build & test (memory + persistent builds)
├── bindings/rust/       ← Rust bindings (auto generated)
└── bindings/ts/
    └── deno_ffi.ts      ← Deno FFI interface (manually updated)
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

- Key (constant-defined max length)
- Atomic Value length + offset
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

For **high-bandwidth** network solutions like 25G RoCE, RDMA becomes very 
practical (albeit latency goes from a few nanoseconds to maybe 10ms). If that's
not a problem, then no need to even use the full 200GB that things like the
NVIDIA/Mellanox Cards go up to. You can also use the persistent mode on 
NVMe/RDMA setups. This won't be as fast as "native", and can still get 
racey if latency is inconsistent. 

Some kind of domain tick mechanism will likely be adopted prior to synchronization
becoming officially supported. Yes, hyperscale problems, but thankfully not hyperscale
prices. This stuff is still off-the-shelf.

---

### Next Major Feature Goals

- [ ] `splinterctl` CLI utility that includes snapshot / dump functionality
- [ ] `splinterd` daemon TTL eviction, policy server

### Next Planned Chores

- [ ] Maybe a TAP-compatible test suite, and scripts to test the tools?
- [ ] Validate correctness of the test harness itself; improve UX of perf test use and output
      usefulness (very cryptic right now)
- [ ] Tools need proper argument validation & parsing, version info, help summary

### Help Wanted

If someone wants to own the Rust bindings (and crate) - please let me know! `timthepost@protonmail.com`
or just open an issue on GH.

---

For developer docs, see `docs/` in the repo root. Doygen users can generate auto
documentation by just typing `doxygen`.
