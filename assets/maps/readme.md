# Battalion battlefield packages

Each directory contains one versioned `battalion-map` JSON package. `default`
corresponds to the unnumbered historical data files; `00`, `01`, and `02`
correspond to the entries in `data/battalion.levels`.

Do not hand-maintain generated numeric records while the legacy files remain
the source of truth. Regenerate every package deterministically from the
repository root:

```sh
make export-maps
make test
```

The schema and map compatibility modes are documented in
`docs/assets-and-maps.md`.
