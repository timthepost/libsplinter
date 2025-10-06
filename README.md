# Splinter 🐀

> _“I am always here, my sons. Even when unseen.”_

**Splinter** is a minimalist, high-speed message bus and subscribable KV store 
that operates in shared memory (`memfd`) or uses memory-mapped files for 
persistent backing. 

Designed to support AI agents and ephemeral memory workflows, Splinter sits 
behind the scenes — synchronizing state between inference, retrieval, UI, 
and coordination layers.

But, Splinter isn't limited to inference, training and generation workflows;
it can power advanced IPC and messaging systems at extreme scales, or just 
act as a lightweight, subscribeable and reliable KV store.

Rust and TypeScript (Deno) bindings included, designed to be FFI-friendly
for other languages, too. Splinter is a bridge to share address space 
between otherwise incompatible environments.

Inspired by the wisdom and discretion of _[Master Splinter][4]_, this bus never
speaks unless spoken to, but is always ready.

---

## Key Features

- 🧠 Shared memory layout: low-overhead, mmap-based store
- 📥 `set`, `unset`, `get`, `list`, and `poll` operations
- 🔑 Lock-free design (utilizes atomic operations with seqlock for contention,
     EAGAIN for non-block operation)
- 🧹 Auto-vacuuming by default for hygienic memory mode; toggle instantly without
     reloading or restarting anything. 
- 🧵 Thread-safe single-writer, multi-reader semantics, resilient even when MRSW
     contract is broken at huge scale.
- ☢️ Atomic-seqlock-guaranteed integrity - no torn reads, even under severe stress
     with vacuuming disabled.
- ✨ 100% Valgrind clean! Well-tested and easy to integrate.
- 🕰️ Built-in version tracking via atomic epoch counters
- 🔧 Configurable slot count and max value size
- 💾 Optional persistent mode via file-backed `mmap`
- 🦕 Deno and Rust bindings included!
- ⚙️ Embeddable and extendable; easy to build upon! Apache 2 license by default,
     dual-MIT license available at low-cost / free (reach out to
     `timthepost@protonmail.com`)

---

## How Does Splinter Compare With Other Stores?

It's important to first re-state: Splinter isn’t a database — it’s a **shared-memory exchange**.  
It’s designed for the **multi-reader, single-writer (MRSW)** case where speed matters more than 
schema, flow is anticipated, there just needs to be a bridge for information to flow through. 
Splinter was created to provide _just enough_ safety for things like `llama.cpp`, `Deno`, `Rust` 
and `btrfs` to be able to share memory together and synchronize work.

It was created when the author was thinking 

> "_I need something like `/proc` but that I can do in userspace, preferably without mounting 
anything new. Something like XenStore but no hypervisor ..._"

There are limits to what a very creative GNU/Linux hacker can do with core utilities and access 
to `/dev/shm`; Splinter is there to pick up where those tools leave off, and aren't specialized 
enough to accommodate "hyper scale" lock-free design constraints. Splinter is just enough code
to make a high-volume / high-traffic sanitized store safe and practical.

And, because it was developed specifically ***for LLM workloads***, like:

 - Caching, comnbining and feeding output from sanitation workflows
 - Providing short-term, manageable, auditable and persistent memory capabilities to serverless 
   MCP servers (see [Tieto][3] for a companion project)
 - Providing real-time auditable and replay-able feedback mechanisms for NLP training in assistive-use 
   (cognitive assist for brain cancer survivors),
 - Ephemeral caching of multiple linear vector states (RWKV) as well as namespaced for multi-head
   transformer-based attention schemes.

It is designed to not ever allow old information to leak into new, which is something very innovative 
for a store that (by primary design) operates in shared memory without persistence. Splinter also has
no build dependencies outside of glibc (including parsers, etc) and this is very intentional, because
it makes splinter much easier to include in larger applications that must undergo rigorous requirements
testing and certifications.

But: ***not everyone needs "hyperscale LLM levels" of paranoia in their engineering***, so Splinter gives 
you a choice: 

- **Sterile mode (`auto_vacuum=on`)** — every write zeroes old contents before reuse. Perfect for LLM 
  scratchpads and training contexts where stale data must never leak back. It's like boiling a hotel 
  room every time a new guest arrives (if only that were possible!!!). No contamination, but it takes 
  time to write twice.
  
- **Throughput mode (`auto_vacuum=off`)** — skips scrubbing for maximum raw speed. Ideal for message 
  buses, ephemeral caches, or event streams where under-reading will not happen (or matter). Reading 
  past the atomic advertised value length (up to the max value length) could result in fetching random 
  stale data. This isn't possible through normal use of the library, but could happen if you've got 
  synchronization issues going on in your code.

Leaks are a very unlikely scenario, but it would be irresponsible not to call it out as a possibility. 
***Most non-scientific users won't ever care about or notice scrubbing and vacuum settings.*** Splinter 
"just works" - it was built to be intuitive and (relatively) fool-proof. 

It's easy (and cheap) to toggle between auto / no vacuum mode using code; just a single call either way. 
The author did not write Splinter because he wanted to, he wrote it because he ***had*** to, because 
nothing else available did quite what splinter does.

Here's a table to help you see where Splinter lands in contrast with what's around, and why it's novel:

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

Splinter is the only user-space C library that the author knows of that combines:  

- Lock-free reads (seqlock snapshots)  
- Single-writer atomicity without global mutexes  
- Optional "sterile memory" mode for training-safe hygiene  
- Lightweight enough to double as a pub/sub message bus

This is easily shared and synchronized between languages through Splinter's bindings - you can 
easily share address space between Inference (in C / llama.cpp) and TypeScript (Deno / Oak) 
for instance. Moving model context histories around can really become just changing pointer 
locations, and so many other high-performance pub/sub workflows!

---

## Programs Included:

Splinter ships with (in addition to the shared and static objects) several programs to help
illustrate how to use the C version of the library, as well as a comprehensive CLI for managing
splinter stores in debugging / production monitoring.

### Simple / Demonstration Programs:

  - `splinter_send`: writes a value to a key on a given store. Creates a store if necessary.
  - `splinter_recv`: watches a key for updates, then writes the new data to standard output 
     (indefinitely, if wanted).
  - `splinter_test`: Unit tests (`make test` or `make valtest`) which also illustrate the C 
     library quite well.
  - `splinter_stress`: MRSW-contract stress test (many millions of ops / sec with random writes) 
     The defaults take several minutes to run; please be patient. At the level of torture
     introduced, Less than ~.5% integrity failures is normal for lock-free schemes and why we
     use serialization. However: we frequently score ***zero*** `:)`.

### Robust `splinterctl` and `splinter_cli` Tools:

The simple programs are great for showing how the library works and for basic scripting automation
tasks. If that's all you need, then you probably don't need the full-blown CLI/REPL. However, if you
plan to use Splinter for a large workload and need a way to keep an eye on things, this tool has you
covered.

`splinter_cli` is a pure C REPL which I wrote in a very simliar way that I wrote full-blown shells, 
such as [BDSH][1] for [HelenOS][2]. 

The biggest things to remember about the CLI are that invoking it via `splinterctl` will cause it to 
turn into non-interactive mode (so you can use the commands it offers in scripts), and that it's self-
documenting in that `help` is online. 

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
55 : {I think this is a perfectly reasonable value for foo.}

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
splinter_debug # 

```

#### Typical `splinterctl` Non-Interactive Use:

The `splinterctl` command is a symbolic link to the installed `splinter_cli`. It 
(safely) changes behavior slightly to accommodate script requests. A common way 
to use it is with `alias`, so that you can automatically use the same store 
through a succession of commands.

You can also define functions in `.zshrc` / `.bashrc` / etc.

```bash

user@host:$ alias splinterctl="splinterctl --use splinter_debug"
user@host:$ splinterctl list
Key Name                          | Epoch           | Value Length   
------------------------------------------------------------------
__debug                           | 2               | 3  

user@host:$ splinterctl head __debug
hash:     1273363460105448522
epoch:    2
val_off:  75776
val_len:  3
key:      __debug

user@host:$ splinterctl set keyspace::keyname "{Value One}"
user@host:$ splinterctl set otherspace::keyname "{Value Two}" 
user@host:$ splinterctl list
Key Name                          | Epoch           | Value Length   
------------------------------------------------------------------
otherspace::keyname               | 2               | 11             
keyspace::keyname                 | 2               | 11             
__debug

user@host:$ splinterctl list ^keyspace::
Key Name                          | Epoch           | Value Length   
------------------------------------------------------------------
keyspace::keyname                 | 2               | 11

user@host:$ splinterctl get keyspace::keyname
11 : {Value One}

user@host:$ splinterctl get otherspace::keyname
11 : {Value Two}

user@host:$ splinterctl watch __debug
Press Ctrl-] To Stop ...
Hello from another terminal!

user@host:$ splinterctl config
magic:       1397509716
version:     1
slots:       128
max_val_sz:  1024
epoch:       5
auto_vacuum: 1

```

### TypeScript Bindings

In the `bindings/ts/` directory you'll find FFI bindings for Deno as well as a utility 
class and some simple tests to show you that the installation was successful. 

The ***primary driver*** for this project was a ned to connect TypeScript (Deno) with 
Inference (in my case, llama.cpp) in a way that was close to bare-metal `select() / poll()` 
style latency. Sockets were the very first thing to get thrown out the window, and,
well, here we are.

Use whatever license you want with the TS code; the library is Apache 2.0.

There's always special 3> for Deno (the author used to work there).

### Rust Bindings

Feel free to include the generated bindings in `bindings/rust` (run `make world` or 
`make rust_bindings` to generate them) under whatever license your project uses. Libsplinter
itself is Apache 2.0.

>***Help Wanted***

> I could really use someone's help in publishing / managing the Rust crate for the project.
If you'd like to help, please reach out to me at `timthepost@protonmail.com` or through GH 
issues.

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

Slot selection uses linear probing; polling uses version-check spin. The
library includes utilities to take snapshots of slot structures by key name,
as well as the global bus configuration and operation structure.

Splinter doesn't try to 'hide' much from the user behind its API; only what's 
essential for safety. 

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

### Next Major Feature Goals:

- [ ] `splinterd` daemon to enforce ttl eviction

---

For developer docs, see `docs/` in the repo root. Doygen users can generate auto
documentation by just typing `doxygen`.

  [1]: https://github.com/HelenOS/helenos/tree/master/uspace/app/bdsh
  [2]: https://helenos.ogr
  [3]: https://github.com/timthepost/tieto
  [4]: https://en.wikipedia.org/wiki/Splinter_(Teenage_Mutant_Ninja_Turtles)
