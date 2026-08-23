# Battalion assets

The complete content-creation workflow and format reference is in
[`../docs/content-authoring.md`](../docs/content-authoring.md). It documents
static and animated GLB authoring, every `.entity` directive, creatures, NPCs,
vehicles, objects, devices, structures, effects, map actors, perception zones,
reactions, and triggers.

All runtime content lives below this directory. Directory and file names use
lowercase so the same package works on case-sensitive and case-insensitive
filesystems.

```text
assets/
  manifest.txt       versioned runtime asset catalog
  audio/             backend-neutral audio catalog
  config/            render bindings and renderer-neutral controller values
  entities/          gameplay archetypes grouped by entity kind
  fonts/             external stroke-font glyphs and metrics
  maps/              versioned JSON battlefield packages
  models/            Khronos glTF 2.0 binary models and animation clips
    vehicles/        tanks, aircraft, and vehicle shadows
    monsters/        monsters, Heroes, and Mecha
    buildings/       structures and structure animations
    terrain/         trees, water, hills, ground, and replaceable road tiles
      roads/         textured road surfaces and their lane markers
    effects/         projectiles, explosions, fire, and generic effects
    ui/              title geometry
  textures/          TGA textures
  music/             music tracks
  sounds/            sound effects
  battalion.sho      historical Showcase help document
```

`assets/manifest.txt` is required. Its version 2 schema catalogs models,
textures, animations, and auxiliary authored files. A missing, invalid, or
undeclared model, texture, or animation is an error; the normal executable has
no compiled C geometry or texture fallback. `make test` also rejects duplicate,
uppercase, and orphan model or texture entries.

`config/render.bindings` maps stable engine roles to replaceable manifest keys.
Models, textures, animation clips, configuration files, the stroke font, and
the historical SGI help document are selected through this file instead of
hardcoded paths or catalog keys in gameplay source.

The GLB loader supports triangle, line, and point primitives; indexed and
non-indexed positions; normals, UVs and vertex colours; material base colours;
scene-node hierarchies; named nodes; and named translation, rotation, and scale
clips with `LINEAR` or `STEP` interpolation. It also supports standard glTF
skins, `JOINTS_0`, `WEIGHTS_0`, inverse-bind matrices, and independent skin
instances through a renderer-neutral CPU evaluator shared by Metal/OpenGL/GLES.
Standard glTF `alphaMode: BLEND`
is honoured by every rendering backend, so transparent assets keep authored
material and vertex alpha instead of being drawn opaque. `helo.glb`, `airplane.glb`,
`googelon.glb`, `flutter.glb`, `techs.glb`, `vapour.glb`, both Hero files, and `mecha.glb`
are compound assets. Their
moving parts and reusable variants are nodes in the same file; periodic motion
is stored in named animation clips. Vapour's particle-cloud motion is the
`cloud` clip inside its one GLB rather than a procedural C particle layout;
its original translucent appearance is authored in that GLB as blended
particles with alpha `0.2`.
Projectile rotation, Techs' death motion,
and explosion frames are likewise stored as clips in their GLB files.
The tank's individual tread plates and their looping `tracks` clip are stored
inside `models/vehicles/tank.glb`; the entity descriptor selects that clip only
while the tank is moving.
Gameplay-driven controller values, such as live aiming scales and damage-part
thresholds, live in `config/animation.values` instead of C source.
Dynamic beam endpoints still come from live gameplay, while their widths,
radii, jitter and density live in `config/effects.values`. Road surfaces,
texture coordinates, normals and lane markers are authored GLB assets rather
than procedural geometry in `graphics.c`. The renderer palette lives in
`config/palette.values`, HUD layout values live in `config/ui.values`, and the
complete SGI stroke font lives in `fonts/stroke.font` rather than compiled C
tables. `audio/catalog.txt` maps semantic sound and music names to portable
PCM files and mixer slots. The shared spatial mixer feeds the native CoreAudio,
SGI AL, OSS, and Sun `/dev/audio` outputs; audio file names and music sequence
lengths are not compiled into gameplay code.
Beams, particles, impact scars, and HUD rectangles are runtime primitives
because their vertices depend on live per-frame state; their reusable visual
parameters and layout remain external in `config/`.

## Gameplay entities

`entities/catalog.txt` is the entry point for gameplay archetypes. Each
lowercase `.entity` descriptor selects its GLB model and clips by manifest key
and defines kind, faction, movement, collision, health, weapon, spawning,
perception sensors, reactions, trigger events, and optional player-control
parameters. Vehicles, NPCs, creatures, devices, structures, and ordinary
objects all use the same catalog and runtime representation.

Sensor reactions support `on-enter`, `on-stay`, and `on-exit` transitions.
Maps route emitted events to an actor id or group, including inactive
actors that an `activate` trigger rule can wake.

An archetype with `controllable yes` can be used as the local player without a
new C type or a rebuild:

```sh
build/macos-metal-none/battalion --player tank-standard
build/macos-metal-none/battalion --player helo-standard
build/macos-metal-none/battalion --player beam-soldier
build/macos-metal-none/battalion --player moira-brown
build/macos-metal-none/battalion --player googelon
build/macos-metal-none/battalion --player vapour
```

In the running game Space opens the player selection screen. The arrow keys
select any controllable catalog entry and Enter starts it. A descriptor may
also assign a unique one-character `shortcut`; the historical `6` through `9`
choices are now ordinary data in the creature descriptors. Preview placement
and scale are calculated from the current animated GLB pose and its configured
render nodes, rather than from a model-specific offset.

Entity visual size can be authored with `height` in world units. The loader
measures the transformed rest-pose GLB Y extent and resolves a uniform scale;
Moira Brown uses `height 0.165`, corresponding to 1.65 metres because one world
unit is ten metres. Explicit `scale` is optional, defaults to `1`, and is
mutually exclusive with `height`. Collision and player envelopes remain
explicit descriptor values.

`spawn-weight 0` disables random runtime instances. This is the default for the
checked-in controllable creatures; place them as version 2 map actors or opt in
to spawning explicitly in custom content.

Weapons use reusable capabilities (`none`, `shell`, `rocket`, `beam`, `fire`,
or `plasma`) rather than entity ids. `player-fire-cost` controls the energy
spent per shot independently of `player-move-cost`; `fire-interval` is the
reload period after an immediate shot.

`control-mode free` uses camera-relative WASD movement. `control-mode steered`
uses W/S for forward/reverse and A/D for hull steering. Articulated vehicles
name their yaw and pitch nodes with `aim-yaw-node` and `aim-pitch-node`; mouse
aim and IJKL then rotate only those authored GLB nodes. Pitch limits and hull
steering rate are descriptor values as well.

`vertical-offset` places only the model origin relative to the battlefield
plane. `player-bottom` and `player-height` are respectively the lower and upper
Y bounds of the controllable player's collision envelope, both relative to
`PLANEY`; they do not inherit `vertical-offset`. This keeps ground creatures
solid from the terrain up while allowing an air entity to declare clearance
over low objects explicitly. `collision-height` describes the non-player actor
above its model origin. `first-person-eye-height`
sets the cockpit/eye position and `first-person-visible` controls whether the
player model is drawn in that view. View 1 retains Battalion's original close
third-person camera; true first-person is a separate view selected with 5.
The same choice can be made at launch with `--view 1` or `--view 5`.
`collision-mode solid` blocks against structures and actors, while
`collision-mode phase` passes through them. Vapour declares `phase`; the rule
is a reusable archetype capability and contains no Vapour id check in movement
code.

Gameplay uses stable map-world X/Z coordinates. Moving the player updates only
the player's position; maps, actors, paths, sensor contacts, shots, and effects
are never translated around it. Collision combines the horizontal radius with
the vertical occupied interval. Therefore a flying entity passes over a lower
object when their height intervals do not overlap, while a tall structure can
still block it.

Verify the checked-in content with:

```sh
make test
```
