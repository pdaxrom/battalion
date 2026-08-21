# Battalion battlefield packages

Each directory contains one versioned `battalion-map` JSON package. `default`
is the historical unnumbered battlefield; `00`, `01`, and `02` are the three
numbered battlefields from the original release.

`map.json` is the canonical source and the only runtime map format. Validate
all packages from the repository root:

```sh
make test
```

The schema is documented in `docs/assets-and-maps.md`.
