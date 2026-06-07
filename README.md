# leveldb-clone

A from-scratch LevelDB clone for learning storage engine internals.
See [STUDY_PLAN.md](./STUDY_PLAN.md) for the phased roadmap.

## Build

```bash
make run
```

## Progress

- [x] Phase 0 — Skeleton (Slice, Status, DB over std::map)
- [ ] Phase 1 — MemTable (Arena, Comparator, InternalKey, SkipList)
- [ ] Phase 2 — WAL
- [ ] Phase 3 — SSTable
- [ ] Phase 4 — MANIFEST / VersionSet
- [ ] Phase 5 — Flush
- [ ] Phase 6 — Compaction
