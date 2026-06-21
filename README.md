# leveldb-clone

A from-scratch LevelDB clone for learning storage engine internals.
See [STUDY_PLAN.md](./STUDY_PLAN.md) for the phased roadmap.

## Build

```bash
make run
```

## Roadmap

Each phase ratchets one new capability onto a running DB. Components listed are what gets built *that phase* to enable the capability.

### Phase 0 — Skeleton ✅
**Capability:** `DB::Open / Put / Get / Delete` work against `std::map`, single-threaded, no durability.
- [x] `Slice` — non-owning byte view
- [x] `Status` — error returns with zero-alloc success path
- [x] `DB` skeleton over `std::map` + `std::mutex`

### Phase 1 — Real MemTable 🟡
**Capability:** Replace `std::map` with skip-list MemTable keyed by `InternalKey`. MVCC underneath — multiple versions of a user key coexist, newest wins, tombstones are first-class.
- [x] `Arena` — bump allocator for batch-deallocated nodes
- [x] `Comparator` — strategy for key ordering
- [x] `InternalKey` = `user_key || seq:7 || type:1`, ordered `(user asc, seq desc)`
- [ ] `SkipList` — single-writer / many-reader, arena-allocated nodes
- [x] `MemTable` — wrapper with `Add(seq, type, key, value)` / `Get(LookupKey, …)` *(currently backed by `std::map`, swap to skiplist pending)*
- [ ] `LookupKey` — packs user key + seq for lookup path
- [ ] Wire MemTable into `DBImpl` (replace `std::map`)
- [ ] `WriteBatch` + serialized format

### Phase 2 — Durability via WAL 🟡
**Capability:** Writes survive `_exit(0)`. On reopen, the WAL replays into a fresh MemTable.
- [x] `coding.h` — varint + fixed-width encoding
- [x] `CRC32C` — table-driven checksum
- [x] `Env` skeleton — `SequentialFile`, `WritableFile`, etc.
- [x] Log writer — 32KB blocks, framed records (FULL/FIRST/MIDDLE/LAST), CRC per record
- [🟡] Log reader — written, **not yet tested**; corruption handling + block-boundary cases TBD
- [ ] Wire WAL into `DBImpl::Write` (log before memtable, sync before ack)
- [ ] Recovery on `Open` — replay existing log into memtable

### Phase 3 — SSTable on disk ⬜
**Capability:** Standalone SSTable builder/reader pair. Can write a 100K-key sorted run and read it back with seeks + iteration. (Not yet wired into DB — that's Phase 5.)
- [ ] Block builder + block reader (shared-prefix encoding, restart points)
- [ ] `Iterator` interface + `BlockIterator`
- [ ] `TableBuilder` — data blocks + filter block + metaindex + index + 48-byte footer
- [ ] `Table` reader + two-level iterator (index → data block)
- [ ] Bloom filter + filter block (skip block reads on missing keys)

### Phase 4 — Versioning + MANIFEST ⬜
**Capability:** Track which SSTables exist, atomically swap the file set, recover that set after a crash.
- [ ] `VersionEdit` — delta describing file-set changes (files added/deleted per level, new log#, new last-seq)
- [ ] `VersionSet` — refcounted `Version` chain; `LogAndApply` appends edit to MANIFEST
- [ ] `CURRENT` file as the single atomic pointer (swapped via `rename(2)`)
- [ ] `Recover()` — read CURRENT → open MANIFEST → replay edits

### Phase 5 — Background flush to L0 ⬜
**Capability:** When MemTable fills, swap to immutable; background thread spills it to a new L0 SSTable + records a `VersionEdit`. Reads merge memtable + imm + L0 files.
- [ ] Immutable memtable (`imm_`) + atomic swap
- [ ] Background flush thread (`BuildTable` → `LogAndApply`)
- [ ] Multi-source `Get`: memtable → imm → L0 (newest first) → L1, L2…

### Phase 6 — Compaction ⬜
**Capability:** L0 doesn't pile up forever. Background compactions merge overlapping runs into deeper levels, maintaining the LSM invariant (levels ≥ 1 have non-overlapping files, sized exponentially).
- [ ] Merging iterator — N-way merge of sorted iterators
- [ ] L0 → L1 compaction (merge + output rotation by file size)
- [ ] Level-N compaction + picking strategy (`PickCompaction`, `compact_pointer_`, trivial-move)
- [ ] Background scheduler (single bg thread, condvar signaling, L0 write-stall back-pressure)

### Phase 7 — Measure ⬜
**Capability:** Throughput + latency numbers; crash stress test that doesn't diverge; writeup.
- [ ] `db_bench` port — `fillseq`, `fillrandom`, `readrandom`, `readseq`, `overwrite`
- [ ] Crash stress test (kill + reopen vs `std::map` oracle)
- [ ] Reflection / writeup

---

**Legend:** ✅ done · 🟡 in progress · ⬜ not started

See [STUDY_PLAN.md](./STUDY_PLAN.md) for the full design notes per task.
