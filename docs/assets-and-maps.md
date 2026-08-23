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
positions, vertex normals/UVs/colors, `baseColorFactor`, embedded PNG
`baseColorTexture` materials, scene nodes with matrix or TRS transforms, and
animation channels for node translation, rotation, and scale. Standard glTF
skins, joints, inverse-bind matrices, `JOINTS_0`, and
`WEIGHTS_0` are evaluated with renderer-neutral linear-blend skinning. Every
mesh-node/skin instance owns its palette, so packed actors may contain
incompatible skeleton copies. `LINEAR` and `STEP` interpolation are supported.
Normal/metallic/occlusion/emissive maps, external image URIs, morph targets,
mesh compression, and cubic-spline playback remain later steps. External TGA
textures used by engine render roles are cataloged separately in the same
manifest.

`assets/manifest.txt` uses the `battalion-assets 2` catalog and declares every
runtime model, texture, animation, and auxiliary authored file. Catalog keys
and paths must be lowercase; unsafe relative paths, duplicates, missing files,
and orphan model or texture files fail validation. Paths are relative to
`assets/`. There is no normal-build fallback to compiled C geometry or texture
arrays. This policy is the same on every supported platform.

`assets/config/render.bindings` is the versioned bridge between stable engine
roles and replaceable manifest entries. It maps model, texture, animation, and
auxiliary-file slots without putting catalog keys or paths in gameplay code.
For example, `ui-title-high` selects the title GLB and `help-sgi-showcase`
selects the historical Showcase document. Authored world models, including
buildings, roads, terrain, and actors, are selected by their entity descriptors
instead of renderer bindings. Replacing a model does not require changing or
recompiling the engine.

Renderer colours, HUD rectangles, the complete stroke font, effect parameters,
and gameplay animation controller values are external assets as well. Audio
file variants, mixer slots, and music ordering live in
`assets/audio/catalog.txt`; an optional `unique` entry flag prevents a looping
or sustained effect from stacking copies of itself. `make test` rejects any return of immediate-mode
model geometry or direct model, texture, sound, and music paths to gameplay
source files.

Transient geometry whose vertices are produced from live state each frame is
renderer output rather than authored content. Beams, impact scars, particles,
and HUD rectangles therefore remain runtime primitives; their reusable widths,
radii, colours, density, and layout values are external configuration assets.

## Time of day and dynamic shadows

The runtime initializes the sky from local time and advances through day,
sunset, night, and sunrise. Sun and moon direction drive procedural projected
shadows; stars fade in at night. No destroyed or shadow model is selected by an
entity archetype, and no shadow mesh is stored beside a vehicle. Destruction
uses the shared explosion plus burning-debris simulation instead.

Press `[` or `]` to move time by one hour. Press `F5` to toggle shadows. The
same switch is available at startup as `--no-shadows` or `--shadows`. SGI,
SunOS, and Solaris builds start with shadows disabled; the explicit switch can
still enable them. The geometry and environment model are shared by Metal and
desktop OpenGL through the renderer-neutral path intended for GLES.

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

The runtime applies destruction and expands some compact map markers. Random
catalog spawning is opt-in through a nonzero archetype `spawn-weight`; the
checked-in playable creatures do not enable it. The map and every live object
keep stable map-world coordinates. Player movement changes the player's
coordinates; a camera-only transform centres the rendered view without moving
gameplay state.

## Unified map package

Each battlefield now also has a self-contained
`assets/maps/<id>/map.json`. `default` is the historical unnumbered map;
numbered maps use two-digit IDs. Version 2 keeps authored record order and
uses one generic `objects` array for every authored world item:

```json
{
  "format": "battalion-map",
  "version": 2,
  "id": "01",
  "name": "Andyville",
  "objects": [
    {"archetype": "road-paved-tile", "x": 17.5, "y": 0, "z": 21.5},
    {"archetype": "building-office-white", "x": -24, "y": 0,
     "z": -19.6},
    {"id": "guard-a", "archetype": "tank-standard", "x": 5,
     "y": 0, "z": 40}
  ]
}
```

Map and model data remain separate: map records identify authored gameplay
archetypes, while geometry and animation live in renderer-neutral GLB files
under `assets/models`. Historical numeric `roads`, `structures`, `units`, and
split `actors` arrays were migrated to catalog archetypes and are rejected to
prevent silent data loss.

JSON packages are required on every platform. `make test` validates every
package and performs JSON write/read round trips.

### Map editor MVP

The build also produces a standalone Nuklear map editor. It edits
`BattalionMapPackage` directly: there is no editor-only document model and no
second map format. On macOS it uses the same fixed-function frontend and Metal
backend as the game; on X11 it uses the corresponding OpenGL path.

```sh
make editor
build/macos-metal-coreaudio/map_editor assets/maps/default/map.json
build/macos-metal-coreaudio/map_editor --validate assets/maps/default/map.json
```

Use the archetype palette to enter object placement mode, then click in the
viewport. Left-drag moves objects or selected route/area points, right- or
middle-drag pans, and the mouse wheel zooms around the cursor. File, Edit, and
View provide new/open/save, deletion, grid/snap, fit, validation, and a
`make test` command. The properties panel edits the same v2 object record used
by runtime: description, transform, faction, typed properties, shared patrol
assignment, route/area points, patrol state, and trigger routes. Roads,
structures, lakes, hills, logic zones/routes, vehicles, creatures, and NPCs
are all catalog archetype instances; there is no compact
numeric side array or editor-only representation.

The editor uses the same `battalionMapPackageValidate` routine as package
loading and writing. Its catalog checks also verify archetypes, faction ids,
route ownership/reference rules, and protected-area geometry, matching runtime
startup behavior.

## Extensible gameplay entities

`assets/entities/catalog.txt` lists authored `.entity` archetypes. This is a
gameplay catalog, not a tank table: `vehicle`, `npc`, `creature`, `object`,
`device`, and `structure` all enter the same generic runtime. An archetype
selects its model plus idle, move, and attack clips through manifest keys.
Destroyed entities use runtime debris and fire, while shadows are projected
from collision bounds. The archetype also declares a default faction, movement,
collision dimensions, health, score, weapon, random-spawn policy, and whether
the player may control it.

For example, the checked-in catalog contains a ground tank, helicopter, armed
NPC, the migrated army vehicle roster, the skinned Moira Brown regression NPC,
Googelon, Flutter, Techs, and Vapour. All controllable catalog entries use the
same controls and can be selected without adding a C enum or switch branch:

```sh
build/macos-metal-none/battalion --map default --player tank-standard
build/macos-metal-none/battalion --map default --player helo-standard
build/macos-metal-none/battalion --map default --player beam-soldier
build/macos-metal-none/battalion --map default --player moira-brown
build/macos-metal-none/battalion --map default --player flutter
build/macos-metal-none/battalion --map default --player vapour
```

Space opens the in-game catalog selector, the arrow keys cycle all descriptors
marked `controllable yes`, and Enter starts the selected entry. A controllable
descriptor may declare a unique single-character `shortcut`; this is how the
historical `6`, `7`, `8`, and `9` selections are retained without hardcoded
entity-id branches. The selector frames the current animated GLB from the
bounds of the nodes it actually renders, so differently sized or offset models
remain centred at a consistent visual size.

The reusable weapon capabilities are `none`, `projectile`, and `beam`.
Shells, rockets, plasma, fire, water, debris, and future projectile visuals are
ordinary `kind projectile` archetypes selected by the weapon descriptor, not
by a C enum or entity id. `tactic standoff|flyby` controls generic approach
behavior and `burst-count` controls projectiles per attack. `player-fire-cost`
is the per-shot energy cost, separate
from `player-move-cost`; `fire-interval` is the post-shot reload period rather
than a global-frame alignment requirement.

Player motion is selected with `control-mode`. `free` gives camera-relative
WASD movement; `steered` gives vehicle-style W/S forward/reverse and A/D hull
turning. `aim-yaw-node` and `aim-pitch-node` bind mouse/IJKL aim to named GLB
parts, with descriptor-defined pitch limits and steering rate. The standard
tank therefore turns its hull with A/D while the turret and barrel aim
independently.

All armed archetypes also require a named GLB `muzzle-node`, regardless of
whether they are vehicles, NPCs, creatures, devices, or structures. The
node's origin is the weapon emission point and its local `+Z` axis is the shot
direction after animation and articulated aiming. Numeric muzzle offsets are
not part of the entity format and there is no fallback for a missing node.

Canonical content scale is one world unit per ten metres. A 1.8-metre human is
therefore `0.18` unit tall, while a typical tank is roughly `0.22`-`0.30` unit
tall and `0.45`-`0.65` unit long. An entity can declare a target `height`; the
loader measures its transformed rest-pose GLB bounds and computes uniform
scale. Explicit `scale` remains optional and defaults to `1`. The two directives
are mutually exclusive. Visual sizing affects attachment nodes only; collision
and player envelopes must still be authored explicitly to match. The complete
sizing table and workflow are in `content-authoring.md`.

Entity `kind` never supplies an implicit size. Prefer `height` for imported
actors, such as Moira Brown's `height 0.165` (1.65 metres); use explicit `scale`
for assets that require direct author control. In either case, write collision
and player dimensions in world units. This keeps a newly added human smaller
than a tank without any hardcoded human-or-vehicle branch in the engine.

The descriptor's `vertical-offset` is only the model-origin height relative to
`PLANEY`. An actor's `collision-height` extends above that model origin. For a
controllable player, `player-bottom` and `player-height` are the lower and upper
collision-envelope bounds relative to `PLANEY`; neither value inherits
`vertical-offset`. Ground actors normally start at `player-bottom 0`, while an
air actor declares both bounds above the terrain so it can clear low objects
without passing through tall structures. Camera framing is derived from these
authored values, so short ground vehicles, humanoids, tall monsters, and
hovering aircraft do not require type-specific camera or placement code. Each
controllable descriptor must provide `view-1-height-fraction` and
`view-2-height-fraction`; the renderer computes camera distance so the authored
collision height occupies that fraction of the viewport. View 1 remains the
original close third-person camera, view 2 is the overview, and view 5 is
first-person. First-person eye height and player-model visibility are also
authored per entity.
`model-yaw-offset` is a data-side correction for imported models whose visual
forward axis does not match the engine's `+Z`; it does not alter physics or aim.

`collision-mode` is either `solid` (the default) or `phase`. A phased player or
phased actor passes through structures, vehicles, and other actors;
Vapour uses this capability. Future smoke, ghosts, or similar entities can opt
in through their descriptor without an engine id check.
Destroyed structures stop participating in movement collision immediately;
their collapse, explosion, or removal animation may continue independently.

An entity may declare multiple named perception sensors. The current sensor
types are `proximity`, forward `vision`, `hearing`, `damage`, and polygonal
`area`. Each sensor
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
on-trigger base-alarm investigate none
```

Multiple rules may consume the same trigger. `activate` and `deactivate` are
state commands, so an inactive actor can first be activated and then assigned
an attack or movement reaction by the same event. Trigger delivery is
two-phase: all actors finish the current simulation step before routed events
are queued for the next one, making behavior independent of actor list order.

Map package version 2 has generic `objects`. Every object references a catalog
archetype and may define an instance id/description/group/faction, transform,
active state, typed property overrides, a shared patrol-route reference, route
or area points, and trigger routes. A `kind route` object owns patrol points;
one or more actors reference its id without copying those points. A trigger sends an
emitted event to a configured target object id or group and carries the
original perceived contact. Thus one guard can raise an alarm that activates
or retargets any number of configured objects. Object positions and points are
authored in map-world coordinates. Those coordinates remain stable while the
player moves; trigger contacts and paths therefore never depend on the camera.

Patrolling actors can select a per-instance `alarm-response`: inherit the
archetype reaction, continue patrolling, attack the transmitted intruder,
retreat to a referenced `kind zone`, or choose randomly between the three
explicit actions for each new alarm. Retreating actors wait for
`safe-wait-seconds` of simulation time and then resume their shared route.
Because the safe area is another catalog/map object, no faction, actor, route,
or zone id is compiled into the AI.

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
  "patrol-route": "base-perimeter-route",
  "triggers": [
    {"event": "base-alarm", "target-group": "base-guards"}
  ]
}
```

To add a new playable or autonomous kind, add its GLB/animation entries to the
manifest, add one descriptor and catalog line, then place it in a v2 map or set
its spawn weight. A nonzero spawn weight is an explicit request for random
runtime instances and is independent of `controllable`. The main game source
does not need to know its id.

## Layout and compatibility

The directory names are deliberately lowercase: `maps`, `models`, `textures`,
`music`, and `sounds`. Models are grouped below `models/` as `vehicles`,
`monsters`, `buildings`, `terrain`, `effects`, and `ui`. Model and animation
files are always GLB assets selected through the manifest. The same
renderer-neutral representation feeds Metal and desktop OpenGL; GLES will
consume that representation when its backend is implemented.
