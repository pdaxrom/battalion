# External assets and battlefield maps

For the complete author workflow and directive reference, including static and
animated objects, creatures, NPCs, vehicles, structures, devices, effects,
sensors, reactions, trigger routing, and validation, see
[`content-authoring.md`](content-authoring.md).

Battalion keeps runtime content in the required lowercase `assets/` tree. The
asset loader and animation sampler are renderer-neutral and are shared by the
Metal, desktop OpenGL, and planned GLES backends.

## Model and animation format

The external model format is Khronos glTF 2.0 in its single-file binary form,
`.glb`. It carries meshes, materials, a node hierarchy, and named animation
clips. The engine-side `AssetModel` is renderer-neutral: neither the loader nor
the animation sampler depends on OpenGL, Metal, or the future GLES backend.

The import profile supports triangle, line, and point primitives; indexed and non-indexed
positions, vertex normals/UVs/colors, `baseColorFactor`, scene nodes with matrix
or TRS transforms, and animation channels for node translation, rotation, and
scale. `LINEAR` and `STEP` interpolation are supported. GLB image textures,
skins, morph targets, mesh compression, and cubic-spline playback remain later
steps. External TGA textures used by the current assets are cataloged
separately in the same manifest.

`assets/manifest.txt` uses the `battalion-assets 2` catalog and declares every
runtime model, texture, animation, and auxiliary authored file. Catalog keys
and paths must be lowercase; unsafe relative paths, duplicates, missing files,
and orphan model or texture files fail validation. Paths are relative to
`assets/`. There is no normal-build fallback to compiled C geometry or texture
arrays. This policy is the same on every supported platform.

`assets/config/render.bindings` is the versioned bridge between stable engine
roles and replaceable manifest entries. It maps model, texture, animation, and
auxiliary-file slots without putting catalog keys or paths in gameplay code.
For example, `building-satellite-dish` selects the current satellite-dish GLB,
and `help-sgi-showcase` selects the historical Showcase document. Replacing a
model does not require changing or recompiling the engine.

Renderer colours, HUD rectangles, the complete stroke font, effect parameters,
and gameplay animation controller values are external assets as well. Audio
file variants, mixer slots, and music ordering live in
`assets/audio/catalog.txt`. `make test` rejects any return of immediate-mode
model geometry or direct model, texture, sound, and music paths to gameplay
source files.

Transient geometry whose vertices are produced from live state each frame is
renderer output rather than authored content. Beams, impact scars, particles,
and HUD rectangles therefore remain runtime primitives; their reusable widths,
radii, colours, density, and layout values are external configuration assets.

The checked-in GLB files are the canonical source assets. The one-time
converter and the old C model factories have been removed, so geometry is not
duplicated between code and assets.

The tank keeps `tracks` and `upper-hull` as fixed siblings of its articulated
`turret`; only the turret's `barrel` child receives pitch. Compound vehicles
and monsters keep their parts as named nodes in one GLB. The helicopter carries its `rotors` clip, the
airplane its `propellers` clip, Googelon its `tail-sway` clip, Flutter its
`wing-flap` clip, the two Hero variants their `walk` and `fire` clips, and Mecha
its `eye-scan` clip. Vapour is a 45-node particle cloud whose translation and
scale tracks are the named `cloud` clip inside `vapour.glb`; no runtime Vapour
particle positions are compiled into the engine. Its particles use standard
glTF `alphaMode: BLEND` and authored alpha `0.2`; the OpenGL, GLES, and Metal
backends preserve that transparency. Projectile variants likewise
keep their named `spin` clips inside their GLB files.

## Existing battlefield maps

The maps were already external; they were not procedurally generated. Their
historical three-file representation has been converted to one JSON package
per battlefield and removed. A map can be selected without changing files:

```sh
build/macos-metal-none/battalion --map 1
build/macos-metal-none/battalion --map Andyville
```

The runtime still dynamically spawns enemies, applies destruction, and expands
some compact map markers. The map and every live object keep stable map-world
coordinates. Player movement changes the player's coordinates; a camera-only
transform centres the rendered view without moving gameplay state.

## Unified map package

Each battlefield now also has a self-contained
`assets/maps/<id>/map.json`. `default` is the historical unnumbered map;
numbered maps use two-digit IDs. The v1 schema keeps authored record order and
uses named fields:

```json
{
  "format": "battalion-map",
  "version": 1,
  "id": "01",
  "name": "Andyville",
  "roads": [{"x": 17.5, "z": 21.5, "type": 3}],
  "structures": [{"x": -24, "z": -19.6, "type": 2, "shape": 3}],
  "units": [{"x": 5, "z": 40, "type": 0}]
}
```

Map and model data remain separate: map records identify authored gameplay
type/shape data, while geometry and animation live in renderer-neutral GLB
files under `assets/models`. Runtime expansion of compound hills, initial occupants,
destruction state, and enemy spawning is intentionally still gameplay logic.

JSON packages are required on every platform. `make test` validates every
package and performs JSON write/read round trips.

## Extensible gameplay entities

`assets/entities/catalog.txt` lists authored `.entity` archetypes. This is a
gameplay catalog, not a tank table: `vehicle`, `npc`, `creature`, `object`,
`device`, and `structure` all enter the same generic runtime. An archetype
selects its normal, destroyed, and shadow models plus idle, move, and attack
clips through manifest keys. It also declares faction relationships, movement,
collision dimensions, health, score, weapon, random-spawn policy, and whether
the player may control it.

For example, the checked-in catalog contains a ground tank, helicopter, armed
NPC, Googelon, Flutter, Techs, and Vapour. All controllable catalog entries use the
same controls and can be selected without adding a C enum or switch branch:

```sh
build/macos-metal-none/battalion --map default --player tank-standard
build/macos-metal-none/battalion --map default --player helo-standard
build/macos-metal-none/battalion --map default --player beam-soldier
build/macos-metal-none/battalion --map default --player flutter
build/macos-metal-none/battalion --map default --player vapour
```

Space opens the in-game catalog selector, the arrow keys cycle all descriptors
marked `controllable yes`, and Enter starts the selected entry. A controllable
descriptor may declare a unique single-character `shortcut`; this is how the
historical `6`, `7`, `8`, and `9` selections are retained without hardcoded
entity-id branches.

The reusable weapon capabilities are `none`, `shell`, `rocket`, `beam`, `fire`,
and `plasma`. Projectile choice and player firing are based on that capability,
not on an archetype id. `player-fire-cost` is the per-shot energy cost, separate
from `player-move-cost`; `fire-interval` is the post-shot reload period rather
than a global-frame alignment requirement.

Player motion is selected with `control-mode`. `free` gives camera-relative
WASD movement; `steered` gives vehicle-style W/S forward/reverse and A/D hull
turning. `aim-yaw-node` and `aim-pitch-node` bind mouse/IJKL aim to named GLB
parts, with descriptor-defined pitch limits and steering rate. The standard
tank therefore turns its hull with A/D while the turret and barrel aim
independently.

The descriptor's `vertical-offset` is only the model-origin height relative to
`PLANEY`. An actor's `collision-height` extends above that model origin. For a
controllable player, `player-bottom` and `player-height` are the lower and upper
collision-envelope bounds relative to `PLANEY`; neither value inherits
`vertical-offset`. Ground actors normally start at `player-bottom 0`, while an
air actor declares both bounds above the terrain so it can clear low objects
without passing through tall structures. Camera framing is derived from these
authored values, so short ground vehicles, humanoids, tall monsters, and
hovering aircraft do not require type-specific camera or placement code. View 1 remains the original
close third-person camera, while view 5 is first-person. First-person eye height and
player-model visibility are also authored per entity.
`model-yaw-offset` is a data-side correction for imported models whose visual
forward axis does not match the engine's `+Z`; it does not alter physics or aim.

`collision-mode` is either `solid` (the default) or `phase`. A phased player or
phased actor passes through structures, vehicles, and other actors;
Vapour uses this capability. Future smoke, ghosts, or similar entities can opt
in through their descriptor without an engine id check.
Destroyed structures stop participating in movement collision immediately;
their collapse, explosion, or removal animation may continue independently.

An entity may declare multiple named perception sensors. The current sensor
types are `proximity`, forward `vision`, `hearing`, and `damage`. Each sensor
has its own range, cone angle, and faction filter. Vision is blocked by intact
world structures; hearing reacts to moving or recently firing contacts and is
not blocked by those structures.

Sensor rules can distinguish zone lifecycle transitions. `on-enter` fires once
when a contact enters a zone, `on-stay` fires on later simulation steps while
it remains selected by the zone, and `on-exit` fires once when it leaves.
Each rule maps the transition to a local reaction such as observe, alert,
investigate, pursue, attack, flee, activate, or deactivate, and may emit a
named event:

```text
sensor sight vision 8 120 hostile
on-enter sight attack base-alarm
on-exit sight investigate target-lost
on-trigger base-alarm activate none
on-trigger base-alarm attack none
```

Multiple rules may consume the same trigger. `activate` and `deactivate` are
state commands, so an inactive actor can first be activated and then assigned
an attack or movement reaction by the same event. Trigger delivery is
two-phase: all actors finish the current simulation step before routed events
are queued for the next one, making behavior independent of actor list order.

Map package version 2 adds generic `actors`. Every actor references an entity
archetype and defines an instance id/group, position, heading, active state,
optional patrol mode/waypoints, and zero or more trigger routes. A route sends
an emitted event to a configured target actor id or group and carries the
original perceived contact. Thus one guard can raise an alarm that activates or
retargets any number of other configured actors. Actor positions and paths are
authored in map-world coordinates. Those coordinates remain stable while the
player moves; trigger contacts and paths therefore never depend on the camera.

A minimal actor looks like this:

```json
{
  "id": "guard-a",
  "archetype": "tank-standard",
  "group": "base-guards",
  "x": 5,
  "z": 5,
  "heading": 0,
  "active": true,
  "patrol-mode": "loop",
  "path": [{"x": 5, "z": 5}, {"x": 8, "z": 5}],
  "triggers": [
    {"event": "base-alarm", "target-group": "base-guards"}
  ]
}
```

To add a new playable or autonomous kind, add its GLB/animation entries to the
manifest, add one descriptor and catalog line, then place it in a v2 map or set
its spawn weight. The main game source does not need to know its id.

## Layout and compatibility

The directory names are deliberately lowercase: `maps`, `models`, `textures`,
`music`, and `sounds`. Models are grouped below `models/` as `vehicles`,
`monsters`, `buildings`, `terrain`, `effects`, and `ui`. Model and animation
files are always GLB assets selected through the manifest. The same
renderer-neutral representation feeds Metal and desktop OpenGL; GLES will
consume that representation when its backend is implemented.
