# Study Plan: Build a LevelDB Clone in C++

## Context

You want to learn advanced systems techniques by re-building a real-world project from scratch. LevelDB is dense with techniques rarely seen in app code — LSM trees, skip lists, custom allocators, on-disk format design, WAL, crash recovery, MVCC, Bloom filters. Small enough (~20K LOC) to read end-to-end, self-contained, and the upstream code is *readable* (Sanjay/Jeff are good writers).

**Realistic frame for this summer.** Your time is fragmented: leaving Kway, moving to Atlanta, recruiting kicking off in July, school starting in August. So this plan is **not a checklist to complete 100%**. It's a *map of a trunk* (the actual deliverable) plus *stretch goals* you can skip without hurting the project.

The trunk alone — a working LSM with WAL, crash recovery, compaction, and benchmark numbers — is already a strong DB-internals portfolio piece for interviews. You don't need a sharded LRU cache and bidirectional iteration to impress anyone; you need the hard middle, done well.

---

## What is LevelDB?

Embedded ordered key-value store from Google (Sanjay Ghemawat, Jeff Dean, 2011). "Embedded" means it's a library, not a server — `db.Put(k, v)` / `db.Get(k)`.

**Real-world users:** Chrome (IndexedDB), Bitcoin Core (UTXO set), Ethereum clients, internal Google systems. RocksDB is the Facebook fork.

**Why it matters:**
- Random writes on disk are 100× slower than sequential — LSM trees turn random into sequential via batched in-memory buffers + background merges.
- Sorted SSTables + Bloom filters give both fast point lookups *and* ordered iteration.
- Atomic manifest updates + WAL → crash safety without per-write fsync.

**What it deliberately is not:** no SQL, no network, no multi-process. It's a primitive you build databases on top of.

---

## The Approach: Tracer-Bullet, Just-in-Time Primitives

This plan is **top-down**, not bottom-up. The two traps to avoid:

1. **Bottom-up — "build all the primitives first."** Spend two weeks on `Slice`, `Status`, `coding.h`, `CRC32C`, `Arena` etc. before writing a single `Put/Get`. Momentum dies because nothing runs. You can't tell whether your `Arena` is right because nothing exercises it. When you finally wire it together you discover your earlier choices don't fit the layer above.
2. **Header-copy — "stub upstream headers, fill in bodies."** Headers contain ~5% of LevelDB's complexity. The hard parts — LSM mechanics, compaction picker, MVCC, recovery protocol — live entirely in the `.cc` files. Stubbing headers leaves all the hard parts unspecified and forces you to invent the algorithm under someone else's API.

**Instead, ratchet the running DB forward one capability at a time.** Each phase upgrades what your DB can *do*, and the primitives that phase needs get built *that phase*, with motivation already in hand:

- Phase 0: a `DB` backed by `std::map`. Demands `Slice`, `Status`.
- Phase 1: replace `std::map` with a real MemTable. Demands `Arena`, `Comparator`, `InternalKey`, `SkipList`.
- Phase 2: writes survive `_exit(0)`. Demands `coding.h`, `CRC32C`, `Env`, log format.
- Phase 3: spill MemTable to disk. Demands the SSTable format + iterators.
- Phase 4: track which SSTables exist across restarts. Demands `VersionEdit`, `VersionSet`.
- Phase 5: flush in the background. Demands `imm_` swap, bg thread.
- Phase 6: keep SSTables from piling up forever. Demands merging iter, compaction picker.
- Phase 7: prove it works. `db_bench`, crash stress, writeup.

Every primitive arrives with the question "what new capability does this enable in the running DB?" already answered. That's the difference between learning the architecture and copying it.

**Working setup:** You've chosen to rewrite in place. Work on a branch:

```bash
git checkout -b clone
# main stays as the reference you diff against
```

When you want to peek at upstream while implementing, `git show main:path/to/file.cc`. Do not rebase clone onto main — main is your reference, not a target.

**Operating discipline:**

- **Headers (`.h`) before each task:** Read the interface, not the implementation. The header tells you what class to build, what methods to expose, what the lifetime/ownership contract is. The `.cc` you write yourself — that's where the learning is.
- **Wire formats** (log format, SSTable format, internal key layout, MANIFEST records): **read the spec/doc first**. These are inventions, not derivations. You can't guess the 48-byte footer.
- **Algorithms** (skip list, merging iterator, compaction picker, DBIter state machine, LRU): **try yourself first.** Read upstream `.cc` only when stuck. The struggle is where the skill compounds.
- **Exceptions where headers contain implementation:** A few upstream files put the algorithm in the header (e.g. `db/skiplist.h` is a template with the body inline). For those, prefer reading the *paper or doc* and looking at the header only for the class signature — flagged per-task below.
- **Predict before peeking:** Before you read an upstream `.cc`, write down in three sentences how you'd implement it. Then read. The places where reality diverged from your prediction are where the non-obvious design lives.

---

## Task Format

Every task uses this structure:

> **Source files to read first (interface only):** Specific `.h` paths in upstream. These give you the class signature, method declarations, ownership/lifetime contract — the *what*, not the *how*. Do not open the `.cc` until you've implemented your own (unless flagged otherwise).  
> **What:** One-sentence component description.  
> **Why this exists in LevelDB:** The problem it solves.  
> **Approach:** "Read first" (wire format / spec) or "Try first, read when stuck" (algorithm).  
> **Concept primer:** Specific docs/papers/blog posts.  
> **Build:** Step-by-step implementation goals.  
> **Skills:** What you internalize.  
> **Validation:** What test proves it works.  
> **Time estimate (first-timer):** Honest range.

---

## Phase 0 — Skeleton (1 day)

**Capability gained at end of phase:** `DB::Open/Put/Get/Delete` work against a `std::map`. Single-threaded, no durability, no concurrency. You can write a unit test and run it.

**Why this phase exists:** Calibrate the API and your test harness before any storage decisions matter. The minute Phase 0 is green, you have something that ratchets forward; every later phase keeps the tests green or adds new ones. You will never again be more than half a day away from a working database. That's the whole point: momentum is the most important resource on an 8-week solo project.

**New concepts:** Public-API design with non-owning views (`Slice`) and rich error returns (`Status`).

### 0.1 — `Slice` and `Status` (a few hours)
- **Source files to read first:** `include/leveldb/slice.h`, `include/leveldb/status.h`
- **What:** Non-owning byte view; success-or-error return type with zero-allocation success path.
- **Why this exists in LevelDB:** Every API takes `Slice` instead of `std::string` so callers don't pay for copies. `Status` returns instead of exceptions so the library composes cleanly with embedded use. The `state_ == nullptr` trick makes `ok()` a pointer check, so the common success case allocates nothing.
- **Approach:** Read both headers fully; they're short. Implement bodies yourself.
- **Skills:** Non-owning view ergonomics; lifetime contracts in API design; allocation-free success paths.
- **Time:** 1–2 h each.

### 0.2 — `DB` skeleton with `std::map` backing (half day)
- **Source files to read first:** `include/leveldb/db.h` (just the abstract class, ignore options for now).
- **What:** `DB::Open/Put/Get/Delete` working with `std::map<std::string,std::string>` underneath.
- **Why this exists in LevelDB:** The public surface is fixed early because everything downstream binds to it. You want to feel where `Slice`, `Status`, and the return shapes get awkward — those frictions are what later phases polish away.
- **Build:** A single `.cc` implementing `DB::Open` (factory returning a concrete subclass), `Put/Get/Delete` with a `std::map` and a `std::mutex`. No options, no write batches yet.
- **Validation:** GoogleTest unit test: `Put("a","1"); Get("a") == "1"; Delete("a"); Get("a")` returns `NotFound`.
- **Time:** half day.

**Exit Phase 0 when:** the trunk runs. You can `make && ./mini_db_test` and it passes.

---

## Phase 1 — Real MemTable (5–7 days)

**Capability gained at end of phase:** `std::map` replaced with a skip-list MemTable keyed by `InternalKey`. Same Put/Get/Delete behavior on the outside, but now you have MVCC underneath: multiple versions of the same user key coexist, newest wins, tombstones are first-class.

**Why this phase exists:** This is where the LSM model actually begins. A naive map overwrites; an LSM accumulates versions and lets readers see a consistent snapshot while writers are mid-write. The whole rest of the project depends on the comparator getting this ordering right.

**New concepts:** MVCC via sequence numbers; probabilistic balancing (skip list); arena allocation for batch-deallocated structures; lock-free readers with a single writer.

### 1.1 — `Arena` (1–2 h)
- **Source files to read first:** `util/arena.h`
- **What:** Bump allocator for batch-deallocated objects.
- **Why this exists in LevelDB (motivation in your DB *now*):** The skip list you're about to build allocates many small nodes that all die at once when the MemTable retires. `malloc/free` per node is wasteful and fragments. An arena bumps a pointer per alloc and frees the whole region at MemTable death.
- **Approach:** You've written memory pools — same shape. Try, then peek.
- **Skills:** Bump allocation, alignment, "lifetime-coupled" allocator design.
- **Time:** 1–2 h.

### 1.2 — `Comparator` + bytewise default (30 min)
- **Source files to read first:** `include/leveldb/comparator.h`
- **What:** Strategy pattern for key ordering.
- **Why this exists in LevelDB (motivation now):** The skip list takes a comparator; the same skip list needs to support both raw-bytes user keys and LevelDB's `InternalKey` ordering (next task).
- **Time:** 30 min.

### 1.3 — `InternalKey` + sequence numbers + key type (2–3 h)
- **Source files to read first:** `db/dbformat.h` — read fully (it *is* the spec).
- **What:** `InternalKey = user_key || seq:7 bytes || type:1 byte`. Comparator orders by `(user_key asc, seq desc)` so newest version comes first.
- **Why this exists in LevelDB (motivation now):** Without this, `Put("a","1"); Put("a","2")` would overwrite, and you couldn't implement MVCC reads (a `Get` at sequence number N should see only writes ≤ N). The seq-desc ordering means iterating user key "a" yields newest first; the first non-tombstone you hit is the answer.
- **Approach:** Sketch on paper how three writes of "a" with seq numbers 5, 7, 12 sort under this comparator. Don't proceed until that's intuitive.
- **Skills:** MVCC-via-composite-key. This single trick underpins half the rest of LevelDB.
- **Time:** 2–3 h.

### 1.4 — SkipList (3–4 days)
- **Source files to read first (interface only):** **`db/skiplist.h` is an exception** — the entire implementation lives in this header (template). Read Pugh's paper, attempt your own skip list from scratch, and only open `db/skiplist.h` once you have something working (or you're truly stuck). When you do open it, focus on the *concurrency invariants* (atomic next-pointers, memory ordering).
- **What:** Single-writer / many-reader skip list with arena-allocated nodes.
- **Why this exists in LevelDB:** Memtable needs O(log n) insert/lookup + sorted iteration + lock-free reads while a writer inserts. Balanced BSTs need rotations + locks. Skip lists are probabilistically balanced with dead-simple insert.
- **Approach:** Try first, then read for concurrency invariants.
- **Concept primer:** Pugh, "Skip Lists: A Probabilistic Alternative to Balanced Trees" (1990, 8 pages).
- **Build:** Templated `SkipList<Key, Comparator>`. Random level via geometric distribution, max 12. `Insert`, `Contains`, internal `Iterator`. Nodes from Arena.
- **Skills:** Probabilistic balancing, memory ordering (`std::atomic` with `acquire`/`release`), why this is *actually* safe lock-free for readers.
- **Validation:** Concurrent test: 1 writer + 4 reader threads, 100K inserts, no crash, all keys eventually visible.
- **Time:** 3–4 days. Expect the concurrency invariants to take a full day on their own.

### 1.5 — MemTable (1–2 days)
- **Source files to read first (interface only):** `db/memtable.h`
- **What:** Wrapper around skip list keyed by `InternalKey`, with `Add(seq, type, key, value)` and `Get(LookupKey, ...)`.
- **Why this exists in LevelDB:** In-memory level-0 of the LSM. All writes land here after WAL.
- **Approach:** Header gives you the class shape; the body is straightforward composition over 1.4.
- **Build:** `MemTable` owns Arena + SkipList. Entries encoded as `[varint internal_key_size][internal_key][varint value_size][value]`. `LookupKey` packs user key + seq for lookup. (You're hand-rolling the varint here; it's tiny. The real `coding.h` arrives next phase.)
- **Skills:** Composing higher-level structures on a primitive, ref-counted lifetime (memtables outlive operations).
- **Validation:** Insert 10K with increasing seq, Get returns latest. Insert a deletion, Get returns `NotFound`.
- **Time:** 1–2 days.

### 1.6 — Tracer bullet: replace `std::map` (1 day)
- **Source files to read first (interface only):** `include/leveldb/write_batch.h`, `db/write_batch_internal.h`, `db/db_impl.h` (just the class declaration — ignore log/compaction fields for now).
- **What:** `DBImpl` now uses `MemTable*` + monotonic sequence counter + writer mutex. Phase 0's tests still pass, plus new MVCC tests.
- **Why this exists in LevelDB:** Validates the foundation E2E before adding I/O. Catches API/threading bugs in the smallest possible system.
- **Build:** `DBImpl` holds `MemTable*` + sequence counter + mutex. Implement `WriteBatch` serialized format yourself from the header signatures.
- **Skills:** Writer serialization, lock-free reads, when to take snapshots of internal state, visitor pattern (`WriteBatch::Iterate`).
- **Validation:** Phase 0 tests still pass. Plus: 4 writer + 4 reader threads, 5 seconds, no crashes.
- **Time:** 1 day.

---

## Phase 2 — Durability via WAL (5–7 days)

**Capability gained at end of phase:** Writes survive `_exit(0)`. On reopen, the WAL replays into a fresh MemTable. This is the actual durability promise of an embedded DB.

**Why this phase exists:** Up to now, everything is in RAM. A crash loses all writes. The WAL is the simplest possible durability mechanism — write to a log first, then to MemTable; if crashed, replay log.

**New concepts:** Wire-format design (framing, checksums, blocks); the WAL pattern; "write-ahead, then apply" ordering; recovery semantics ("liberal on read, strict on report").

### 2.1 — `coding.h` (varint + fixed) (1–2 h)
- **Source files to read first:** `util/coding.h`
- **What:** Endian-neutral fixed-width encoding + variable-length integer encoding + length-prefixed slices.
- **Why this exists in LevelDB (motivation now):** WAL records (next task) are framed as `[length][type][checksum][payload]`. Lengths are stored as varints (most records are small) and payloads contain length-prefixed slices. You need these primitives before you can build the log format.
- **Approach:** Implement bodies yourself; the header is the spec.
- **Skills:** Endian-neutral encoding, varint as a compression scheme for small integers, why bit-shifts beat `memcpy` for portability.
- **Time:** 1–2 h.

### 2.2 — `CRC32C` (Castagnoli) (2–3 h)
- **Source files to read first:** `util/crc32c.h`
- **What:** Table-driven CRC32C; SSE4.2 intrinsic optional.
- **Why this exists in LevelDB (motivation now):** Every WAL record carries a checksum so the reader can detect torn writes (the last record after a crash is normally half-written). CRC32C is fast, well-understood, and has hardware support on x86.
- **Approach:** Table-driven first. The intrinsic is a 30-minute speedup later if you want.
- **Skills:** Why CRC matters at storage boundaries; the difference between detection and correction.
- **Time:** 2–3 h.

### 2.3 — `Env` skeleton (2–3 h)
- **Source files to read first:** `include/leveldb/env.h`
- **What:** Abstract base class + POSIX impl for the file types you need now: `SequentialFile` (log reader), `WritableFile` (log writer + later SSTable writer), `RandomAccessFile` (later SSTable reader).
- **Why this exists in LevelDB (motivation now):** The WAL needs file I/O. Abstracting it now means you can swap in a fault-injecting `Env` for crash tests later.
- **Approach:** Don't over-engineer. Just the file interfaces, plus `NewLogger`, `GetFileSize`, `RenameFile`, `LockFile` (stub if needed).
- **Skills:** Interface design for testability; portability layer separation.
- **Time:** 2–3 h.

### 2.4 — Log writer (1–2 days)
- **Source files to read first (interface only):** `db/log_writer.h`, `db/log_format.h`
- **Doc to read first (this is a wire format):** `doc/log_format.md` — read twice. The 32KB block + record framing is a spec, not a derivation.
- **What:** Append-only log, 32KB blocks, framed records (FULL/FIRST/MIDDLE/LAST), CRC32C per record.
- **Why this exists in LevelDB:** WAL writes precede memtable; crash → replay WAL to rebuild memtable. Framing handles records that straddle 32KB boundaries.
- **Approach:** Read the format doc + header. Write the body yourself.
- **Build:** `Writer(WritableFile*)` with `AddRecord(Slice)`. Track block offset, switch to FIRST/MIDDLE/LAST when records straddle.
- **Skills:** Block-aligned I/O, framing protocols, why fixed-size blocks aid recovery.
- **Validation:** Write 10K random-sized records (1B–100KB), file size matches expected blocks.
- **Time:** 1–2 days.

### 2.5 — Log reader with corruption handling (1–2 days)
- **Source files to read first (interface only):** `db/log_reader.h`
- **What:** Read records, handle corruption via a reporter callback, skip to next block boundary on bad CRC.
- **Why this exists in LevelDB:** Recovery must be liberal: a half-written final record (the crash itself) is *normal*. Corruption mid-file must be reported, not crash.
- **Approach:** Try first. The edge cases (torn writes, truncated tails, block-boundary records) you'll discover from validation tests below. *Then* read `db/log_reader.cc` to compare your handling of edge cases — this is one place where comparing to upstream after attempting teaches a lot.
- **Build:** `Reader(SequentialFile*, Reporter*, checksum)`. `ReadRecord(Slice*, scratch)`. CRC failure → report + advance.
- **Skills:** Recovery semantics, "best-effort + report" vs "fail-fast" on persistence.
- **Validation:** Round-trip 10K records. Flip random bytes → reader reports, skips, continues. Truncate mid-record → stops cleanly.
- **Time:** 1–2 days.

### 2.6 — Wire WAL into write path + recovery on Open (1–2 days)
- **Source files to read first (interface only):** Re-read `db/db_impl.h`. No new headers — you're extending the existing `Write` path.
- **What:** `DBImpl::Write` does serialize batch → append to log → (optional fsync) → apply to memtable. On `Open`, replay existing log into memtable.
- **Why this exists in LevelDB:** This is the actual durability promise.
- **Build:** Hook log writer into write path. On Open, find log file, replay.
- **Skills:** Order of operations: WAL *before* memtable, sync before ack.
- **Validation:** Write 1000 keys, `_exit(0)`, reopen, Get all 1000.
- **Time:** 1–2 days.

---

## Phase 3 — SSTable on disk (8–11 days)

**Capability gained at end of phase:** A standalone SSTable builder/reader pair. You can write a 100K-key sorted run to a file and read it back with seeks and iteration. No DB integration yet — that's Phase 5. You're building the brick before you build the wall.

**Why this phase exists:** MemTables can't grow forever — RAM is finite. The LSM spills full MemTables to immutable sorted files on disk. Those files need a format that supports both random point lookups (binary search via index) and ordered iteration (sequential block reads), all from a file that's too big to load into memory.

**New concepts:** Block-structured files with index; shared-prefix encoding within blocks; lazy block loading; probabilistic membership testing (Bloom filters); the iterator pattern as a universal LSM abstraction.

### 3.1 — Block builder + block reader (2–3 days)
- **Source files to read first (interface only):** `table/block_builder.h`, `table/block.h`
- **Doc to read first (this is a wire format):** `doc/table_format.md` (block section). The shared-prefix entry encoding is a spec.
- **What:** Block = sorted run with shared-prefix compression and restart points every 16 entries.
- **Why this exists in LevelDB:** Sorted keys share prefixes (`user:1234:name`, `user:1234:age`). Encoding only the suffix saves space; restart points enable binary search within a block.
- **Approach:** Spec for encoding (read). Binary-search-on-restart-points logic (try yourself).
- **Build:** `BlockBuilder::Add(key, value)`. Per-entry: `[shared][unshared][value_len][key_delta][value]`. Restart array + count at end.
- **Skills:** Prefix compression, sparse in-block index, why 16 is the magic interval.
- **Validation:** Build a block with 10K sorted keys, iterate. Seek to a key — binary search on restart points works.
- **Time:** 2–3 days.

### 3.2 — Iterator interface + block iterator (1 day)
- **Source files to read first (interface only):** `include/leveldb/iterator.h`
- **What:** Abstract `Iterator` (`SeekToFirst/Seek/Next/Prev/Valid/key/value/status`), with cleanup-function registration.
- **Why this exists in LevelDB:** Every layer exposes an iterator (block → table → memtable → version → merged). The most reused abstraction in the codebase.
- **Build:** Pure-virtual interface + `EmptyIterator` + `BlockIterator` on top of 3.1.
- **Skills:** Iterator pattern, two-phase invalidation (`Valid()` before `key()`), cleanup-function chaining (used for cache integration later).
- **Time:** 1 day.

### 3.3 — Table builder (1–2 days)
- **Source files to read first (interface only):** `include/leveldb/table_builder.h`, `table/format.h`
- **Doc to read first (this is a wire format):** `doc/table_format.md` (full). Footer is fixed 48 bytes with magic number at end-of-file.
- **What:** Complete SSTable: data blocks + filter block + metaindex + index + footer.
- **Why this exists in LevelDB:** Immutable on-disk form of a sorted key range. Immutability is what makes compaction concurrency-safe.
- **Approach:** Spec for layout (read). The build/finish state machine you write yourself.
- **Build:** `Add`, `Flush` (finalize current data block, record `BlockHandle`), `Finish` (filter + metaindex + index + footer).
- **Skills:** On-disk format design, fixed-position footers, magic numbers for format detection.
- **Validation:** Build a 100K-key table; hex-dump footer; verify magic + block handles point to valid offsets.
- **Time:** 1–2 days.

### 3.4 — Table reader + two-level iterator (1–2 days)
- **Source files to read first (interface only):** `include/leveldb/table.h`, `table/two_level_iterator.h`
- **What:** Open table, parse footer + index, on-demand load data blocks. Two-level iterator: outer = index, inner = data block.
- **Why this exists in LevelDB:** Don't load a 64MB SSTable into memory. Index lives in RAM; data blocks (4KB) load on demand.
- **Approach:** Try the two-level iterator yourself first — given the iterator interface, the composition is elegant once you see it. After attempting, read `table/two_level_iterator.cc` to compare. It's a small, beautiful piece of code.
- **Build:** `Table::Open(file)` reads footer + index. `Table::NewIterator()` returns two-level. `Table::InternalGet(key)` for point lookup.
- **Skills:** Lazy loading, iterator composition, two-level vs flat indexing tradeoffs.
- **Validation:** Build a 100K-key table, reopen fresh, seek to random keys, values match.
- **Time:** 1–2 days.

### 3.5 — Bloom filter + filter block (1–2 days)
- **Source files to read first (interface only):** `include/leveldb/filter_policy.h`, `table/filter_block.h`
- **What:** Per-SSTable Bloom filter; `KeyMayMatch == false` → skip block read entirely.
- **Why this exists in LevelDB:** Without Bloom, every `Get` for a missing key reads one block per SSTable. With ~10 bits/key, 99% of those reads vanish. Massive win.
- **Approach:** Try first. Bloom is short; the math (false-positive rate, choosing k) is what you should derive yourself.
- **Concept primer:** Wikipedia "Bloom filter" (basics + math). Double-hashing trick: `h_i = h1 + i*h2`.
- **Build:** `FilterPolicy` interface + `BloomFilterPolicy(bits_per_key)`. `FilterBlockBuilder` groups filters per 2KB of data.
- **Skills:** Probabilistic structures, false-positive math `(1 - e^(-kn/m))^k`, choosing k from bits_per_key.
- **Validation:** 100K keys; query 100K *missing* keys; false-positive rate ≈ 1% at 10 bits/key.
- **Time:** 1–2 days. High-impact-for-effort — keep this in trunk.

**Exit Phase 3 when:** standalone tests for SSTable build/read/seek/Bloom pass. The DB binary doesn't use any of this yet — that wiring is Phase 5.

---

## Phase 4 — Versioning + MANIFEST (4–7 days)

**Capability gained at end of phase:** The DB can track which SSTables exist, atomically swap the file set, and recover that file set after a crash. Standalone tests; not yet wired into the write path (Phase 5 does that).

**Why this phase exists:** Once you have SSTables on disk, you need an authoritative answer to "which files exist right now and which level is each on?" Naively storing this in a single file rewritten on every change risks corruption mid-write. The MANIFEST + CURRENT pattern solves this via atomic `rename(2)`.

**New concepts:** Delta-encoded state changes; the "WAL for metadata" pattern; atomic filesystem swaps; refcounted snapshots (MVCC at the file-set level).

**Don't rush this phase.** It's conceptually the most underrated.

### 4.1 — VersionEdit (1–2 days)
- **Source files to read first (interface only):** `db/version_edit.h`
- **Approach:** The tagged serialization format is a spec — read it from the header carefully (the tag constants are in `version_edit.cc` but you can deduce them from setter signatures, or briefly peek at the tag enum in the `.cc`).
- **What:** Delta describing changes to the file set: files added per level, files deleted, new log#, new last-sequence.
- **Why this exists in LevelDB:** Atomic and crash-safe file-set mutation. Append edits to MANIFEST (a WAL for metadata). Recovery replays.
- **Build:** `VersionEdit` with `AddFile(level, file_meta)`, `DeleteFile(level, num)`. `EncodeTo(string*)` / `DecodeFrom(Slice)`.
- **Skills:** Delta encoding, forward-compatible tagged serialization, "edits not snapshots" pattern.
- **Time:** 1–2 days.

### 4.2 — VersionSet + MANIFEST + CURRENT (3–5 days)
- **Source files to read first (interface only):** `db/version_set.h` — read the entire public interface of `Version`, `VersionSet`, `Compaction`. This file is dense — go slow.
- **Doc to read first:** `doc/impl.md` (manifest + recovery sections).
- **What:** `VersionSet` maintains current `Version` (refcounted file set per level). MANIFEST = log of VersionEdits. CURRENT file holds the active MANIFEST's filename.
- **Why this exists in LevelDB:** CURRENT is the **single atomic pointer**, swapped via `rename(2)`. Everything else is append/replay. This is how the DB recovers consistent state after a crash mid-compaction.
- **Approach:** Sketch the algorithm yourself first (refcounted version chain, atomic CURRENT swap). Then peek at `version_set.cc` for tricky cases — recovery edge cases especially. This is the hardest file in LevelDB.
- **Build:** `Version` (refcounted list of files per level). `VersionSet::Recover()`: read CURRENT → open MANIFEST → replay edits. `LogAndApply()` appends edit, atomic CURRENT swap.
- **Skills:** Atomic state transitions via filesystem primitives (`rename(2)`), refcounted snapshots, MVCC at the file-set level.
- **Validation:** 100 LogAndApply calls with random edits, simulate crash mid-write, recover, verify file set matches.
- **Time:** 3–5 days. The recovery edge cases (half-written last edit, CURRENT pointing to nonexistent MANIFEST) eat real time.

---

## Phase 5 — Background flush to L0 (3–4 days)

**Capability gained at end of phase:** When the MemTable fills, it becomes immutable, a fresh one takes new writes, and a background thread spills the immutable one to a new L0 SSTable + records a VersionEdit. Reads merge memtable + imm + all L0 files. The DB now persists across restarts via SSTables, not just via WAL replay.

**Why this phase exists:** Until now, all reads still come from RAM and durability is WAL-only. This phase is where the L of "LSM" first shows up: the in-memory level overflows into level 0 on disk. Writes never block on disk I/O.

**New concepts:** Producer/consumer between foreground and background threads; the imm_ swap pattern; multi-source `Get` (memtable → imm → L0).

### 5.1 — Immutable memtable + background flush (2–3 days)
- **Source files to read first (interface only):** `db/builder.h`, plus re-read `db/db_impl.h` to see the `imm_` and `background_*` fields/methods.
- **What:** Memtable fills (~4MB) → becomes immutable, new active memtable created. Background thread writes immutable memtable to a new L0 SSTable.
- **Why this exists in LevelDB:** Writes never block on disk I/O. Flush is async.
- **Approach:** Try first — you've designed producer/consumer in trading systems. Then read `DBImpl::CompactMemTable` and `db/builder.cc` to compare.
- **Build:** Add `imm_` field. On memtable full, swap `mem_` → `imm_`, create new `mem_`. Bg thread reads `imm_`, `BuildTable` (uses Phase 3's `TableBuilder`), then `LogAndApply` (uses Phase 4's `VersionSet`) adds new L0 file.
- **Skills:** Two-memtable producer/consumer, consistent reads during swap, background scheduling primitives.
- **Validation:** Write 50MB; multiple L0 files appear; all keys readable.
- **Time:** 2–3 days.

### 5.2 — Multi-source Get (1 day)
- **Source files to read first (interface only):** No new headers — extending `Version::Get` and `DBImpl::Get`. Re-skim `db/version_set.h` for `Version::Get` signature.
- **What:** Get reads memtable → imm → L0 files (newest first) → L1, L2… Stops at first hit (including tombstones).
- **Why this exists in LevelDB:** A key may exist in multiple places. Newest version wins. Deletions must be respected.
- **Approach:** Try first. The "first hit wins, with seq-number ordering" logic is the heart of LSM correctness. Then check `Version::Get` in `db/version_set.cc`.
- **Build:** Extend `DBImpl::Get`: memtable → imm → `current_->Get()`. L0 scans all overlapping files; L1+ is binary search to one file per level.
- **Skills:** Layered lookup, version ordering, why L0 is special.
- **Validation:** Write same key 3 times across memtable/L0 files, Get returns newest. Delete it, Get returns `NotFound`.
- **Time:** 1 day.

---

## Phase 6 — Compaction (10–18 days)

**Capability gained at end of phase:** L0 files don't pile up forever. Background compactions merge overlapping runs into deeper levels with the LSM invariant (each level except L0 has non-overlapping files, sized exponentially). Continuous writes don't degrade reads. Write-stall back-pressure prevents L0 from blowing up under sustained load.

**Why this phase exists:** Without compaction, every `Get` for a missing key scans every L0 file. After a few hours of writes you have hundreds of L0 files and reads grind to a halt. Compaction is the *whole point* of an LSM tree — it's how you turn fast random writes into reasonable read performance over time.

**New concepts:** K-way merge as a streaming algorithm; level invariants and the write-amplification analysis; compaction triggering heuristics; write-stall as back-pressure; the single-bg-thread serialization model.

**The hump.** This is where actual learning compounds and where time estimates lie. Expect: bugs that surface only at scale, races you didn't anticipate, correctness invariants that turn out to be load-bearing. Budget honestly. If you finish in 10 days, great — that's stretch-goal time.

### 6.1 — Merging iterator (1 day)
- **Source files to read first (interface only):** `table/merger.h`
- **What:** N-way merge of sorted iterators → single sorted stream.
- **Approach:** Try first. K-way merge is a known algorithm; you'll likely get something working, then check `table/merger.cc` for the heap-vs-linear-scan tradeoff (linear is fine for N ≤ 12).
- **Build:** `MergingIterator(Iterator** children, int n)`.
- **Validation:** Merge 4 streams of 1000 keys each, output sorted, no duplicates lost.
- **Time:** 1 day.

### 6.2 — L0 → L1 compaction (3–5 days)
- **Source files to read first (interface only):** `db/version_set.h` (`Compaction` class — re-read with fresh eyes). No `db_impl.h` re-read needed beyond what you have.
- **What:** Pick a set of L0 files + all overlapping L1 files → merge → write new L1 files (≤ 2MB each) → VersionEdit removing inputs, adding outputs.
- **Why this exists in LevelDB:** L0 has overlapping ranges (one file per memtable flush). Reads degrade as L0 grows. Compaction sorts into non-overlapping L1.
- **Approach:** Try first. The merging logic is straightforward; the *output rotation* (when to close one output file and start the next, and how to choose split keys cleanly) is where you'll learn most. Read `DBImpl::DoCompactionWork` after attempting.
- **Build:** Merging iterator over input files → `TableBuilder` → rotate when output reaches target size. Drop entries shadowed by snapshots.
- **Skills:** Sort-merge join, output rotation, GC of old versions during compaction.
- **Validation:** Flush 8 overlapping L0 files, trigger compaction, L1 has non-overlapping files, L0 empty.
- **Time:** 3–5 days.

### 6.3 — Level-N compaction + picking strategy (4–7 days)
- **Source files to read first (interface only):** `db/version_set.h` (the `Compaction` class plus `VersionSet::PickCompaction` declaration). You've read this header before — re-read the picking-related parts.
- **What:** When level-K exceeds 10^K MB, pick one file from level-K + all overlapping files from level-(K+1), compact.
- **Why this exists in LevelDB:** Levels grow exponentially; compaction maintains the invariant that levels ≥ 1 have non-overlapping files. Per-level round-robin distributes work.
- **Approach:** Try first for the picker (it's a heuristic — you can invent reasonable ones). Then compare to `VersionSet::PickCompaction` and ponder why upstream chose what it did. Includes `IsTrivialMove` (one file, no L+1 overlap → just move it).
- **Build:** `PickCompaction()` returns level + files. `compact_pointer_[k]` for per-level round-robin. Handle trivial-move separately.
- **Skills:** LSM-tree level invariants, compaction trigger heuristics, write-amplification analysis.
- **Validation:** Insert 100MB random data. Verify compactions happen. All levels respect size limits. All reads still work.
- **Time:** 4–7 days. "All reads still work" hides hours of subtle bugs.

### 6.4 — Background scheduling (2–4 days)
- **Source files to read first (interface only):** No new headers — `db/db_impl.h` re-read for `MaybeScheduleCompaction`, `BGWork`, write-stall fields.
- **What:** Single background thread runs flushes + compactions. Foreground signals via condvar. Write-stall back-pressure when L0 has too many files.
- **Why this exists in LevelDB:** Compactions can't block writes. Two simultaneous compactions on overlapping ranges would corrupt — one bg thread serializes.
- **Approach:** You know condvars from trading systems, but the *write-stall logic* (when to throttle writers based on L0 file count) is LevelDB-specific. Try, then read `MaybeScheduleCompaction` + the write-stall code in `DBImpl::Write`.
- **Build:** `std::thread` + `std::mutex` + `std::condition_variable`. Fg schedules bg work, waits when write-stalling.
- **Skills:** Condvar work queue, write-stall back-pressure, distinguishing "should schedule" from "currently running."
- **Validation:** Stress test — continuous writes 5 minutes, no starvation, no deadlock, compactions progress.
- **Time:** 2–4 days. Deadlocks here can eat a day on their own.

---

## Phase 7 — Measure it (3–5 days)

**Capability gained at end of phase:** Throughput and latency numbers under multiple workloads, crash stress test that runs without divergence, and a writeup that consolidates what you learned. This is what turns the project from "I built an LSM" into "I built an LSM and here's the evidence."

**Why this phase exists:** You're not done until you have numbers. A working DB without measurements is a claim; a working DB with throughput curves and a crash-fuzz log is a portfolio piece.

### 7.1 — Port db_bench (1–2 days)
- **Source files to read first (interface only):** `benchmarks/db_bench.cc` (here the `.cc` *is* the reference — `db_bench` is a single-file program. Skim it for argument parsing + the 5 benchmark loops you care about, lift the structure.)
- **What:** Minimal `db_bench`: `fillseq`, `fillrandom`, `readrandom`, `readseq`, `overwrite`.
- **Build:** Throughput (MB/s) + latency percentiles (p50/p99/p999).
- **Skills:** Microbenchmarking pitfalls, warm-up vs steady-state, percentile latency.
- **Validation:** Each benchmark runs to completion; numbers within 2–4× of upstream on the same hardware (slower is fine).
- **Time:** 1–2 days.

### 7.2 — Crash stress test (1–2 days)
- **Source files to read first (interface only):** Skim upstream `db/db_test.cc` for the `Randomized` test as a structural template. You don't need to port it — write your own.
- **What:** Randomized fuzz: many threads doing Put/Get/Delete/Iterate against your DB vs a reference `std::map`. Periodically kill the process; on reopen, verify state matches the last fsynced batch.
- **Why this exists in real DBs:** This is how real databases are tested. Bugs in compaction/recovery only surface here.
- **Build:** 4 threads × random ops × small key pool. Reference: `std::map` + mutex. Kill-and-reopen cycles between phases.
- **Skills:** Model-based testing, reference-implementation oracles.
- **Validation:** 10 minutes of continuous fuzz + kill cycles, no divergence.
- **Time:** 1–2 days.

### 7.3 — Reflection + writeup (1 day)
- **What:** 2–3 page doc: what surprised you, where your design diverged and why, where it's slow and why, what you'd build next.
- **Why:** Reflection consolidates the learning, *and* this doc is itself an interview asset. Bring it.
- **Time:** 1 day.

---

## Stretch Goals (prioritized)

If you finish trunk with time before recruiting/Atlanta-move/school, add in this order:

1. **Snapshots (`GetSnapshot()` + sequence-number-based reads)** — half day.  
   Read first: `db/snapshot.h`. Highest ROI in this list.
2. **LRU block cache** — 2–3 days.  
   Read first: `include/leveldb/cache.h`. Sharded LRU with intrusive list. Real new technique (refcount-driven eviction). Skip if compaction took 3 weeks.
3. **Reverse iteration in DBIter** — 2 days.  
   Read first: `db/db_iter.h`. Forward/reverse asymmetry is interesting but won't change interview reaction.
4. **Snappy compression integration** — half day. Mostly wiring an existing lib.
5. **Full db_bench port** — 1 day. Polish.

---

## Total Effort, Honest

| Phase | Range |
|---|---|
| 0 — Skeleton | 1 day |
| 1 — Real MemTable | 5–7 days |
| 2 — Durability via WAL | 5–7 days |
| 3 — SSTable on disk | 8–11 days |
| 4 — Versioning + MANIFEST | 4–7 days |
| 5 — Background flush | 3–4 days |
| 6 — Compaction | **10–18 days** |
| 7 — Measure | 3–5 days |
| **Trunk total** | **~39–60 days** |
| Stretch goals (optional) | up to +10 days |

At 6–8 hours/day with fragmented summer time, **plan for 7–10 weeks of trunk**. If recruiting + the Atlanta move eats more time, ruthlessly cut stretch.

---

## Done Definition (trunk only)

- [ ] Put/Get/Delete work with full durability (synced writes survive `_exit(0)`)
- [ ] Forward iteration over memtable + multiple levels returns correct user-visible state
- [ ] Background compactions run without blocking writes
- [ ] Bloom filter measurably reduces missing-key reads (show the number)
- [ ] Crash stress test passes 10 minutes without divergence
- [ ] `db_bench` numbers documented vs upstream

That's a hard, complete project. Stretch goals are bonuses, not requirements.

---

## Cheat Sheet: Headers to Read by Phase

Quick-reference index — what to open *before* writing each task's `.cc`:

| Phase | Headers (read first, interface only) | Format docs (read first) |
|---|---|---|
| 0.1 Slice/Status | `include/leveldb/{slice,status}.h` | — |
| 0.2 DB skeleton | `include/leveldb/db.h` | — |
| 1.1 Arena | `util/arena.h` | — |
| 1.2 Comparator | `include/leveldb/comparator.h` | — |
| 1.3 InternalKey | `db/dbformat.h` *(read fully — it's spec)* | — |
| 1.4 SkipList | `db/skiplist.h` *(impl-in-header — read Pugh's paper first, peek at header for invariants)* | — |
| 1.5 MemTable | `db/memtable.h` | — |
| 1.6 Tracer | `include/leveldb/{db,write_batch}.h`, `db/{write_batch_internal,db_impl}.h` | — |
| 2.1 Coding | `util/coding.h` | — |
| 2.2 CRC32C | `util/crc32c.h` | — |
| 2.3 Env | `include/leveldb/env.h` | — |
| 2.4 Log Writer | `db/log_writer.h`, `db/log_format.h` | `doc/log_format.md` |
| 2.5 Log Reader | `db/log_reader.h` | — |
| 2.6 Wire WAL | re-read `db/db_impl.h` | — |
| 3.1 Block | `table/block_builder.h`, `table/block.h` | `doc/table_format.md` (block section) |
| 3.2 Iterator | `include/leveldb/iterator.h` | — |
| 3.3 Table Builder | `include/leveldb/table_builder.h`, `table/format.h` | `doc/table_format.md` (full) |
| 3.4 Table Reader | `include/leveldb/table.h`, `table/two_level_iterator.h` | — |
| 3.5 Bloom | `include/leveldb/filter_policy.h`, `table/filter_block.h` | — |
| 4.1 VersionEdit | `db/version_edit.h` | — |
| 4.2 VersionSet | `db/version_set.h` | `doc/impl.md` (manifest + recovery) |
| 5.1 Flush | `db/builder.h`, re-read `db/db_impl.h` | — |
| 5.2 Multi-source Get | re-read `db/version_set.h` (`Version::Get`) | — |
| 6.1 Merger | `table/merger.h` | — |
| 6.2–6.3 Compaction | `db/version_set.h` (`Compaction` class, `PickCompaction`) | `doc/impl.md` (compaction section) |
| 6.4 Bg scheduling | re-read `db/db_impl.h` | — |
| 7.1 db_bench | `benchmarks/db_bench.cc` (skim as template) | — |
| 7.2 Stress test | skim `db/db_test.cc` | — |
