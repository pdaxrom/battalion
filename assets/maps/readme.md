# Battalion battlefield packages

Each directory contains one versioned `battalion-map` JSON package. `default`
is the historical unnumbered battlefield; `00`, `01`, and `02` are the three
numbered battlefields from the original release.

`map.json` is the canonical source and the only runtime map format. Validate
all packages from the repository root:

```sh
make test
```

Build or run the standalone Nuklear editor from the repository root:

```sh
make editor
build/macos-metal-coreaudio/map_editor assets/maps/default/map.json
build/macos-metal-coreaudio/map_editor --validate assets/maps/default/map.json
```

The platform/configuration portion of the build directory follows the normal
Makefile settings; use the path printed by `make print-config` when it differs
from the macOS example above.

In the editor, choose an archetype and press `Place actor`, or use the explicit
`Road`, `Tree`, `Building`, `Lake`, and `Hill` tools. Click the map to
place an item; the map-tile tools stay active for repeated placement. Press
`Esc` or `Select / move`, then click and drag an item to move it. The right
panel edits the selected item's runtime fields, including the compact
road/structure `type`/`shape`; Delete removes it. Lakes and hills are the existing structure
types 2 and 3, not a separate terrain format. Right- or middle-drag pans, the
wheel zooms, and File -> Save writes the same `map.json`. Saving a new map also
creates its missing `assets/maps/<id>/` directory.

The package overview is documented in `docs/assets-and-maps.md`. The complete
version 2 actor schema, patrol, perception, reaction, and trigger-routing
workflow is documented in `docs/content-authoring.md`.

Version 2 packages may also contain `actors`. An actor references an archetype
from `assets/entities/catalog.txt`, supplies an instance id/group, initial
transform and active state, and may define a waypoint path plus event routes to
another actor id or group. Geometry and behavior stay in the entity descriptor;
the map only places and connects instances.
