# Splinter 🐀

**Splinter** is a minimalist, high-speed message bus and subscribable KV store
that operates in shared memory (`memfd`) or uses memory-mapped files for
persistent backing.

Designed to support AI ephemeral memory workflows and IPC, Splinter synchronizes 
state between inference, retrieval, UI, and coordination layers through convenient
but guarded shared memory regions. Rust, TypeScript and even Bash scripts can all
communicate using the same memory region, with a simple and convenient
interface.

But, Splinter isn't limited to inference, training and generation workflows; it
can power advanced IPC and messaging systems at extreme scales, or just act as a
lightweight, subscribe-able and reliable KV store. Splinter is ideal for backing
inter-process or even inter-service communication through RDMA.

Rust and TypeScript (Deno) bindings are included, and thoughtful design means
other languages are easy to support as well through dynamic import. Splinter is
designed to be used in code, but ships with a production-quality CLI for
debugging and administration.

Inspired by the wisdom and discretion of _[Master Splinter][4]_, this bus never
speaks unless spoken to, but is always ready.

---

## Key Features

- 🧠 Shared memory layout: low-overhead, mmap-based store
- 📥 `set`, `unset`, `get`, `list`, and `poll` operations
- 🔑 Lock-free atomic ops, seqlock for contention, EAGAIN for non-block i/o)
- 🧹 Auto-vacuuming for hygienic memory mode; toggle on/off instantly.
- 🧵 Thread-safe single-writer, multi-reader semantics.
- ✨ 100% Valgrind clean! Well-tested and easy to integrate.
- 🕰️ Built-in version tracking via atomic epoch counters
- 🔧 Configurable slot count and max value size
- 💾 Optional persistent mode via file-backed `mmap`
- 🦕 Deno and Rust bindings included!
- ⚙️ Embeddable and extendable; easy to build upon!
- 🔎 Epoch-based semantics provides performant watches & easy pub/sub.

Splinter is _entirely self-contained_; it has no external dependencies other than the GNU C 
library. The CLI uses [line noise](https://github.com/antirez/linenoise) to manage history,
completion and suggestions, but it ships in the repo with Splinter.

---

## Quick Start:

After downloading the code, run the following commands:

```bash
make
sudo make install
splinterctl init demo
alias splinterctl="splinterctl --use demo"
splinterctl list
splinterctl set foo "hello, world"
splinterctl get foo
splinterctl head foo
splinterctl unset foo
splinterctl list
```

These commands also work if you just use `splinter_cli` interactively.

You can now explore [the code docs][5] to understand the C api, or check out the
counter part [TypeScript bindings][6]. Note, not all hosts provide access to
`memfd()` in their security layer, so you may need to use persistent mode for
TypeScript projects.

## How Does Splinter Compare With Other Stores?

It's important to first re-state: Splinter isn’t a database — it’s a
**shared-memory exchange**.

It’s designed for the **multi-reader, single-writer (MRSW)** case where speed
matters more than schema, flow is anticipated, there just needs to be a bridge
for information to flow through. Splinter was created to provide _just enough_
safety for things like `llama.cpp`, `Deno`, `Rust` and `btrfs` to be able to
share memory together and synchronize work.

It was created when the author was thinking

> "_I need something like `/proc` but that I can do in user space, preferably
> without mounting anything new. Something like XenStore but no hypervisor ..._"

Essentially, Splinter provides _auditable_, _consistent_ and _safe_ access to 
shared memory, across languages and entire workflows, with no other opinions imposed.

And, because it was developed specifically _**for LLM workloads**_, like:

- Combining and feeding output from sanitation workflows
- Providing short-term, manageable, auditable and persistent memory capabilities
  to serverless MCP servers (see [Tieto][3] for a companion project)
- Providing real-time auditable and replay-able feedback mechanisms for NLP
  training in assistive-use scenarios,
- Ephemeral caching where mutations need to trigger or happen as part of other
  reactions.

Splinter is designed to (by default) not ever allow old information to leak into new,
which is something very innovative for a store that (by primary design) operates
in shared memory without persistence. But: _**not everyone needs "hyper scale
LLM levels" of paranoia in their engineering**_, so Splinter gives you a choice:

- **Sterile mode (`auto_vacuum=on`)** — every write zeroes old contents before
  reuse. Splinter uses static geometry, so there's no row reclamation needed.
  This is perfect for LLM scratchpads and training contexts where stale data must
  never leak back. It's like boiling a hotel room every time a new guest arrives
  (if only that were possible!!!). No contamination, but it takes time to write
  twice.

- **Throughput mode (`auto_vacuum=off`)** — skips scrubbing for maximum raw
  speed. Ideal for message buses, ephemeral caches, or event streams where
  under-reading will not happen (or matter). Reading past the atomic advertised
  value length (up to the max value length) could result in fetching random
  stale data. This isn't possible through normal use of the library, but could
  happen if you've got synchronization issues going on in your code.

Splinter was written because nothing else was lean enough with the very specific
set of desired features present and verifiable. The choice was to eviscerate and
contort SQLite, or just write Splinter. The latter made so much more sense.

Here's a table to help you see where Splinter lands in contrast with what's
around, and why it's novel:

| Feature / System      | **Splinter**                                  | SQLite (`:memory:` / shm)            | LMDB                                  | Tokyo/Kyoto Cabinet       | Kernel Seqlock/RCU                    |
| --------------------- | --------------------------------------------- | ------------------------------------ | ------------------------------------- | ------------------------- | ------------------------------------- |
| Language / API        | C (simple, flat API)                          | C (SQL layer)                        | C (B+tree API)                        | C                         | Kernel only                           |
| Persistence           | Optional (mem-only focus)                     | Yes (journaling)                     | Yes (copy-on-write)                   | Yes (files/dbs)           | No                                    |
| Concurrency Model     | **Single-writer, many-reader (MRSW)**         | Serialized via locks/transactions    | **Single-writer, many-reader (MRSW)** | Basic locks               | **Single-writer, many-reader (MRSW)** |
| Lock-Free Reads       | ✅ Yes (seqlock snapshot)                     | ❌ No                                | ✅ Yes (MVCC pages)                   | ❌ No                     | ✅ Yes                                |
| Write Safety          | Atomic + seqlock                              | Transaction journal                  | Page copy-on-write                    | Record lock               | Sequence lock                         |
| Hygiene / Auto-Vacuum | **Toggleable (scrub or not)**                 | Heavyweight VACUUM                   | None (stale pages reclaimed)          | None                      | N/A                                   |
| Typical Use           | **LLM scratchpad, message bus, ephemeral KV** | Relational queries, structured cache | Embedded KV DB with persistence       | Lightweight persistent KV | Kernel timekeeping, counters          |
| Overhead per Write    | Low (memcpy + atomics)                        | High (SQL parse + journaling)        | Medium (page churn)                   | Medium                    | Low (in-kernel)                       |
| Throughput Focus      | **Yes, bus-scale**                            | ❌ No                                | Balanced (read-heavy)                 | Balanced                  | Yes (in kernel)                       |

Splinter is the only user-space C library that the author knows of that
combines:

- Lock-free reads (seqlock snapshots)
- Single-writer atomicity without global mutexes
- Optional "sterile memory" mode for training-safe hygiene
- Lightweight enough to double as a pub/sub message bus
- Auto vacuuming you can toggle cheaply like a light switch
- Persistence / Ephemera treated as unique and different first-class features

Sharing address space between languages (and entire workflows) is easier when
you have something like splinter managing the guard rails you need to do it
dynamically. Just figure out your key space naming and go!

---

## Programs Included:

Splinter ships with (in addition to the shared and static objects) several
programs to help illustrate how to use the C version of the library, as well as
a comprehensive CLI for managing splinter stores in debugging / production
monitoring.

### TAP-Compatible Tests & Stress / Perf Tests:

- `splinter_test`: Unit tests (`make test` or `make valtest`) which also
  illustrate the C library quite well, and,

- `splinter_stress`: MRSW-contract stress test (many millions of ops / sec with
  random writes). At the level of torture introduced, Less than ~.5% integrity
  failures is normal for lock-free schemes and why we use serialization.

Be advised that `splinterp_stress` could cause premature wear on rotating media
and older solid state drives . It should also be used with extreme care over a
network.

### Robust `splinterctl` and `splinter_cli` Tools:

*For persistent mode, use `splinterpctl` and `splinterp_cli`.

The biggest things to remember about the CLI are that invoking it via
`splinterctl` will cause it to turn into non-interactive mode (so you can use
the commands it offers in scripts), and that it's self- documenting in that
`help` is online.

#### Typical `splinter_cli` Interactive Use:

```cli
splinter_cli version 0.5.0 build 40e3633
To quit, press ctrl-c or ctrl-d.
no-conn # use splinter_debug
use: now connected to splinter_debug
splinter_debug # list
Key Name                          | Epoch           | Value Length   
------------------------------------------------------------------
__debug                           | 2               | 4              

splinter_debug # set foo "{I think this is a perfectly reasonable value for foo.}"
splinter_debug # get foo
55:{I think this is a perfectly reasonable value for foo.}

splinter_debug # list
Key Name                          | Epoch           | Value Length   
------------------------------------------------------------------
__debug                           | 2               | 4              
foo                               | 2               | 55             

splinter_debug # list ^__
Key Name                          | Epoch           | Value Length   
------------------------------------------------------------------
__debug                           | 2               | 4              

splinter_debug # watch foo
Press Ctrl-] To Stop ...

(any changes get printed here)

splinter_debug #
```

### TypeScript Bindings

In the `bindings/ts/` directory you'll find FFI bindings for Deno as well as a
utility class and some simple tests to show you that the installation was
successful.

The _**primary driver**_ for this project was a ned to connect TypeScript (Deno)
with Inference (in my case, llama.cpp) in a way that was close to bare-metal
`select() / poll()` style latency. Sockets were the very first thing to get
thrown out the window, and, well, here we are.

Use whatever license you want with the TS code; the library is Apache 2.0.

There's always special 3> for Deno (the author used to work there).

### Rust Bindings

Feel free to include the generated bindings in `bindings/rust` (run `make world`
or `make rust_bindings` to generate them) under whatever license your project
uses. Libsplinter itself is Apache 2.0.

> _**Help Wanted**_

> I could really use someone's help in publishing / managing the Rust crate for
> the project. If you'd like to help, please reach out to me at
> `timthepost@protonmail.com` or through GH issues.

---

## Building

```bash
make        # builds both memory and persistent modes
make tests
```

Prompts for additional tests will be shown after the build.

This builds:

- `libsplinter.so` : default memory-backed bus
- `libsplinter_p.so` : persistent file-backed variant
- `libsplinter.a` : static version of the default bus
- `libsplinter_p.a` : static version of the persistent bus

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

Slot selection uses linear probing; polling uses version-check spin. The library
includes utilities to take snapshots of slot structures by key name, as well as
the global bus configuration and operation structure.

Splinter doesn't try to 'hide' much from the user behind its API; only what's
essential for safety.

---

## Persistence Mode

If you want bus persistence, use the persistent CLI tool or link your
application (or set dlopen()) to `libsplinter_p.so` instead of `libsplinter.so`.

The persistent version:

- Maps from a file instead of RAM.
- Survives system reboots
- Can be snapshot/dumped with `dd`, `cp`, etc.

However, understand the limitations of the underlying file system and whatever
media is backing it. NVMe storage will be quite fast, rotating media will be
much slower.

The MRSW stress tool will give you some indication of throughput, however you
should scale it down thread-wise as well as payload.

---

## Network Synchronization

Network synchronization is currently not _officially_ supported and probably
will not be unless the project picks up equipment donations or people willing to
give access to machines with high-bandwidth cards in them for testing and
development.

If you'd like to help in this way, please email me directly at
`timthepost@protonmail.com` with libsplinter somewhere in the subject line.

That said, you have some options you can try right now, and I'll do my best to
help if I can:

### Funded Startup Budget

For **high-bandwidth** network solutions like 25G RoCE, RDMA becomes very
practical (albeit latency goes from a few nanoseconds to maybe 10ms). If that's
not a problem, then no need to even use the full 200GB that things like the
NVIDIA/Mellanox Cards go up to. You can also use the persistent mode on
NVMe/RDMA setups. This won't be as fast as "native", and can still get racey if
latency is inconsistent.

### Broke College Student Self-Funding A Research Project Budget

You have options! They just require a little more time on your part.

#### NFS (Best Bet)

Your easiest route is going to be to use persistent mode and NFS.

You can have a setup where a network server hosts the stores, and all compute
nodes mount via NFS. They all mmap the same region, so while it's a little
slower, it's definitely sound and will work probably without issue.

Use small block sizes on the client side to lower latency and think about the
size of your writes / reads, something like:

```bash
mount -t nfs -o rsize=8192,wsize=8192,timeo=14,intr,vers=3 \
  server:/export/splinter /mnt/splinter
```

You may have to play with / tune this, but it should work reliably.

#### Make A UDP Bridge

You can broadcast key updates over UDP and have nodes write them upon receiving
the packets. Hey, it's UDP - it is what it is. But it's an option and on a
decent local network it should be fine.

I'd love a PR with whatever you come up with.

#### Make A Deno / Oak -> REST Gateway

Use the Deno FFI bindings + Oak to make a very simple REST interface. I'll
include a sample one at some point, but any LLM can roll this for you in a few
seconds with the included FFI bindings and TS class for Splinter.

---

### Next Major Feature Goals:

- [ ] `splinter_multi_poll()` to watch multiple keys simultaneously

### Long-term Feature Goals:

- [ ] `splinterd` daemon to enforce ttl eviction (tedious, but not hard)
- [ ] Better support for using ephemeral + persistent mode simultaneously?
      (hard)
- [ ] Ability to merge stores and manage multiple stores separately and
      concurrently (for 'big memory' systems and those with large persistent
      stores) (also hard)

For developer docs, see `docs/` in the repo root, the CONTRIBUTING info and
please give the code of conduct a read if you'd like to send patches.

[1]: https://github.com/HelenOS/helenos/tree/master/uspace/app/bdsh
[2]: https://helenos.ogr
[3]: https://github.com/timthepost/tieto
[4]: https://en.wikipedia.org/wiki/Splinter_(Teenage_Mutant_Ninja_Turtles)
[5]: https://github.com/timthepost/libsplinter/tree/main/docs
[6]: https://github.com/timthepost/libsplinter/tree/main/bindings/ts
