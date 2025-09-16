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
- 🔑 Lock-free design (utilizes atomic operations with seqlock for contention,
     EAGAIN for non-block operation)
- 🧹 Auto-vacuuming by default for hygienic memory mode; toggle instantly for
     hyper-scale no-scrubbing mode without reloading. 
- 🧵 Thread-safe single-writer, multi-reader semantics, resilient even when MRSW
     contract is broken at huge scale.
- ☢️ Atomic-seqlock-guaranteed integrity - no torn reads, even under severe stress
     with vacuuming disabled.
- ✨ 100% Valgrind clean! Well-tested and easy to integrate.
- 🕰️ Built-in version tracking via atomic epoch counters
- 🔧 Configurable slot count and max value size
- 💾 Optional persistent mode via file-backed `mmap`
- 🦕 Deno and Rust bindings included!

---

## How Does Splinter Compare With Other Stores?

It's important to first re-state: Splinter isn’t a database — it’s a **shared-memory exchange**.  It’s designed for the 
**multi-reader, single-writer (MRSW)** case where speed matters more than schema (though it holds up surprisingly well
with multiple concurrent writers, we don't design for / suggest it). 

Unlike traditional KV stores, Splinter gives you a choice:  

- **Sterile mode (`auto_vacuum=on`)** — every write zeroes old contents before reuse. Perfect for LLM scratchpads and
  training contexts where stale data must never leak back.
  
- **Throughput mode (`auto_vacuum=off`)** — skips scrubbing for maximum raw speed. Ideal for message buses, ephemeral
  caches, or event streams where yesterday’s payload doesn’t matter.

Note that even with `auto_vacuum` disabled, leaks are only possible when a reader _deliberately_ under-reads the bus;
that is to say reads less than the informed value length (which is atomic). It's a very unlikely scenario that's only
caused in the real world by synchronization problems, but because contamination in training is on the table, we make
a big deal about it. ***Most non-scientific users won't ever care about or notice scrubbing and vacuum settings.***

This toggle makes Splinter unique: the same lightweight library can behave like a data autoclave or like a firehose, 
depending on your workload.

Here's a table to help you see where Splinter lands in contrast with what's around:

| Feature / System          | **Splinter**                | SQLite (`:memory:` / shm) | LMDB                 | Tokyo/Kyoto Cabinet | Kernel Seqlock/RCU |
|---------------------------|------------------------------|---------------------------|----------------------|---------------------|--------------------|
| Language / API            | C (simple, flat API)        | C (SQL layer)             | C (B+tree API)       | C                   | Kernel only        |
| Persistence               | Optional (mem-only focus)   | Yes (journaling)          | Yes (copy-on-write)  | Yes (files/dbs)     | No                 |
| Concurrency Model         | **Single-writer, many-reader (MRSW)** | Serialized via locks/transactions | **Single-writer, many-reader (MRSW)** | Basic locks         | **Single-writer, many-reader (MRSW)** |
| Lock-Free Reads           | ✅ Yes (seqlock snapshot)   | ❌ No                      | ✅ Yes (MVCC pages)  | ❌ No                | ✅ Yes             |
| Write Safety              | Atomic + seqlock            | Transaction journal        | Page copy-on-write   | Record lock         | Sequence lock      |
| Hygiene / Auto-Vacuum     | **Toggleable (scrub or not)** | Heavyweight VACUUM        | None (stale pages reclaimed) | None               | N/A                |
| Typical Use               | **LLM scratchpad, message bus, ephemeral KV** | Relational queries, structured cache | Embedded KV DB with persistence | Lightweight persistent KV | Kernel timekeeping, counters |
| Overhead per Write        | Low (memcpy + atomics)      | High (SQL parse + journaling) | Medium (page churn) | Medium              | Low (in-kernel)    |
| Throughput Focus          | **Yes, bus-scale**          | ❌ No                      | Balanced (read-heavy) | Balanced            | Yes (in kernel)    |

Splinter is the only user-space C library (I/we know of) that combines:  

- Lock-free reads (seqlock snapshots)  
- Single-writer atomicity without global mutexes  
- Optional "sterile memory" mode for training-safe hygiene  
- Lightweight enough to double as a pub/sub message bus

This is easily shared and synchronized between languages through Splinter's bindings - you can easily share
address space between Inference (in C / llama.cpp) and TypeScript (Deno / Oak) for instance. Moving model
context histories around can really become just changing pointer locations, and so many other high-performance
pub/sub workflows!

---

## Directory layout

```
splinter/
├── README.md            ← this doc
├── splinter.h           ← public API
├── splinter.c           ← implementation
├── splinter_test.c      ← TAP-compatible unit tests
├── splinter_send.c      ← simple tool to send data on the bus
├── splinter_recv.c      ← simple tool to receive data on the bus
├── splinter_logtee.c    ← simple tool to drain bus keys to a file (non-destructively)
├── splinter_stress.c    ← MRSW stress harness (16 threads default) for read concurrency
├── Makefile             ← build & test (memory + persistent builds)
├── bindings/rust/       ← Rust bindings (auto generated)
└── bindings/ts/         ← TypeScript (Deno) bindings & class, tests (manually curated)
```

---

## Building

```bash
make        # builds both memory and persistent modes
make tests
```

Prompts for additional tests will be shown after the build.

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
media is backing it. NVMe storage will be quite fast, rotating media will be 
much slower.

---

## Network Synchronization

Network synchronization is currently not _officially_ supported and probably will
not be unless the project picks up equipment donations or people willing to give 
access to machines with high-bandwidth cards in them for testing and development.

If you'd like to help in this way, please email me directly at 
`timthepost@protonmail.com` with libsplinter somewhere in the subject line. 

That said, you have some options you can try right now, and I'll do my best to 
help if I can:

### Funded Startup Budget

For **high-bandwidth** network solutions like 25G RoCE, RDMA becomes very 
practical (albeit latency goes from a few nanoseconds to maybe 10ms). If that's
not a problem, then no need to even use the full 200GB that things like the
NVIDIA/Mellanox Cards go up to. You can also use the persistent mode on 
NVMe/RDMA setups. This won't be as fast as "native", and can still get 
racey if latency is inconsistent. 

### Broke College Student Self-Funding A Research Project Budget

You have options! They just require a little more time on your part.

#### NFS (Best Bet)

Your easiest route is going to be to use persistent mode and NFS.

You can have a setup where a network server hosts the stores, and all compute 
nodes mount via NFS. They all mmap the same region, so while it's a little slower, 
it's definitely sound and will work probably without issue. 

Use small block sizes on the client side to lower latency and think about the
size of your writes / reads, something like:

```bash
mount -t nfs -o rsize=8192,wsize=8192,timeo=14,intr,vers=3 \
  server:/export/splinter /mnt/splinter
```

You may have to play with / tune this, but it should work reliably.

#### Make A UDP Bridge

You can broadcast key updates over UDP and have nodes write them upon
receiving the packets. Hey, it's UDP - it is what it is. But it's an
option and on a decent local network it should be fine. 

I'd love a PR with whatever you come up with.

#### Make A Deno / Oak -> REST Gateway

Use the Deno FFI bindings + Oak to make a very simple REST interface. I'll include
a sample one at some point, but any LLM can roll this for you in a few seconds with
the included FFI bindings and TS class for Splinter.

---

### Next Major Feature Goals

"*" = In Progress

- [ ] `splinterctl` CLI utility that includes snapshot / dump functionality
- [ ] `splinterd` daemon TTL eviction, policy server

### Next Planned Chores

"*" = In Progress

- [✅] Maybe a TAP-compatible test suite, and scripts to test the tools?
- [✅] Validate correctness of the test harness itself; improve UX of perf test use and output
      usefulness (very cryptic right now)
- [*] Tools need proper argument validation & parsing, version info, help summary

### Help Wanted

If someone wants to own the Rust bindings (and crate) - please let me know! `timthepost@protonmail.com`
or just open an issue on GH.

---

For developer docs, see `docs/` in the repo root. Doygen users can generate auto
documentation by just typing `doxygen`.
