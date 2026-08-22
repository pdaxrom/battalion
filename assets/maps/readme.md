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

Version 2 packages may also contain `actors`. An actor references an archetype
from `assets/entities/catalog.txt`, supplies an instance id/group, initial
transform and active state, and may define a waypoint path plus event routes to
another actor id or group. Geometry and behavior stay in the entity descriptor;
the map only places and connects instances.
