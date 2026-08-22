# Battalion content authoring guide

This is the canonical guide for adding and replacing runtime content. It
covers static and animated models, gameplay entities, creatures, NPCs,
vehicles, structures, devices, effects, perception zones, reactions, trigger
routing, and map placement.

The content system is deliberately renderer-neutral. The same manifest, GLB
models, entity descriptors, and maps are consumed by Metal and classic desktop
OpenGL. A future GLES renderer must consume the same loaded representation;
content must not contain backend-specific alternatives.

## What can be added without changing C code

The engine composes authored data from a fixed set of reusable capabilities.
Without recompiling the game, an author can:

- add or replace a static or animated GLB model;
- add a new `vehicle`, `npc`, `creature`, `object`, `device`, or `structure`;
- make any archetype controllable by the player;
- select ground, air, or stationary movement and free or steered controls;
- select an existing weapon capability and tune its ranges and timing;
- define collision, health, scoring, random spawning, camera, and muzzle data;
- add proximity, vision, hearing, and damage sensors in any combination;
- define enter, stay, exit, and externally triggered reactions;
- emit events and route them to another actor or a group of actors;
- place actors, patrol paths, initial state, and trigger links in a map;
- replace models, textures, animations, sounds, music, UI values, and effect
  parameters through external catalogs.

Data selects and combines capabilities; it does not invent new simulation
operations. A genuinely new movement algorithm, sensor type, reaction verb,
weapon algorithm, or live procedural effect requires one generic engine
capability to be implemented first. After that capability is registered,
individual objects using it must again be definable entirely in `assets/`.
Adding an unknown word or an unused value key never silently creates behavior.

## Runtime content layout

All runtime files live below lowercase `assets/`:

```text
assets/
  manifest.txt                  global asset catalog
  audio/catalog.txt             semantic sound and music catalog
  config/
    animation.values            controller constants
    effects.values              live-effect appearance constants
    palette.values              renderer colours
    render.bindings             stable engine role -> manifest key
    ui.values                   HUD layout
  entities/
    catalog.txt                 entity descriptor catalog
    creatures/*.entity
    npcs/*.entity
    vehicles/*.entity
    objects/*.entity            create when first needed
    devices/*.entity            create when first needed
    structures/*.entity         create when first needed
  fonts/stroke.font
  maps/<map-id>/map.json
  models/
    buildings/*.glb
    effects/*.glb
    monsters/*.glb
    terrain/*.glb
    ui/*.glb
    vehicles/*.glb
  textures/*.tga
  music/*
  sounds/*
```

The category directories are for organization, not behavior. The `.entity`
descriptor defines gameplay capabilities. For example, an aircraft may live
under `models/vehicles/`, but only `movement air` makes its entity airborne.

The checked-in files under `assets/models/` are runtime source assets. Do not
put Blender projects, reference images, temporary exports, or backup files in
that directory: the asset audit rejects unexpected and uncataloged files.

## Naming and path rules

Use lowercase ASCII names. Entity ids, factions, sensor ids, events, groups,
and node names referenced by descriptors may contain only:

```text
a-z  0-9  -
```

They must be non-empty and shorter than 64 bytes. Manifest keys and paths also
accept `_` and `.`, while paths additionally accept `/`. Absolute paths,
backslashes, `..`, uppercase letters, and spaces are rejected. Prefer hyphens
everywhere even where underscores are technically accepted.

Keys have different scopes:

- a **model key** identifies a GLB in `manifest.txt`;
- an **animation key** identifies one named clip in one model;
- an **entity id** identifies a reusable gameplay archetype;
- an **actor id** identifies one map instance;
- a **group** identifies a set of map actors for event routing;
- a **binding slot** is a stable engine role whose asset can be replaced.

Do not reuse one key for unrelated content merely because the names happen to
match.

## The content pipeline

Most new gameplay objects follow this sequence:

1. Author one GLB containing the complete model and its animation clips.
2. Put it in the appropriate `assets/models/<category>/` directory.
3. Add a `model` line and any `animation` lines to `assets/manifest.txt`.
4. Create one `.entity` descriptor under `assets/entities/<category>/`.
5. Add that descriptor to `assets/entities/catalog.txt`.
6. Place an instance in a version 2 map, or configure random spawning.
7. Run `make test` and then test the object in the game.

Replacing an existing render role, such as a terrain tile or explosion model,
uses `assets/config/render.bindings` instead of an entity descriptor. A GLB is
only visual content; a descriptor is what turns a model into a gameplay object.

## GLB model authoring profile

Battalion loads Khronos glTF 2.0 in binary `.glb` form. A GLB must contain its
JSON and binary geometry chunks in one file. External `.gltf` plus `.bin`
packages are not runtime formats.

### Coordinate system and origin

The engine uses:

- `+Y` as up;
- the `X/Z` plane as the battlefield;
- `+Z` as forward when heading is zero;
- radians for map actor headings.

All gameplay positions are persistent map-world coordinates. Movement changes
the moving object's coordinates, never the map, paths, contacts, projectiles,
or other actors. Rendering applies the inverse player X/Z position as a camera
transform, but that transform is not visible to gameplay code or asset data.

Place a ground model's contact point at local `Y = 0` whenever practical.
Use the entity's `vertical-offset` for intentional hovering or for adapting an
existing origin. Keep the visual origin, collision envelope, muzzle offsets,
and camera values consistent. A floating model with a correctly placed shadow
usually means that the model origin or `vertical-offset` is wrong.

Apply intentional object transforms before export unless the transform is part
of a node hierarchy or animation. Test orientation in game; do not rely only
on a DCC application's camera convention.

When an existing animated GLB cannot be re-exported immediately,
`model-yaw-offset` may correct its visual forward axis without changing
movement, aiming, collision, or muzzle calculations. For example, `180` turns
a model authored toward `-Z` to the engine's `+Z` convention. Prefer applying
the transform in the source asset for newly authored models.

### Supported geometry

The loader supports:

- triangle, line, and point primitives;
- indexed and non-indexed geometry;
- float `VEC3` positions;
- normals, `TEXCOORD_0`, and RGB/RGBA `COLOR_0` attributes;
- unsigned-byte, unsigned-short, and unsigned-int indices;
- multiple primitives when every primitive in one glTF mesh uses the same
  primitive mode;
- `pbrMetallicRoughness.baseColorFactor` as the material colour;
- `alphaMode: "BLEND"` and authored material or vertex alpha;
- node hierarchies and the active glTF scene;
- node matrix transforms for static nodes;
- node translation, quaternion rotation, and scale transforms.

Missing normals default to `(0, 1, 0)`, missing UVs to `(0, 0)`, and missing
colours to white. Those defaults make a file load, but they rarely make a good
final asset.

The following are not part of the current runtime profile:

- skins and skeletal deformation;
- morph targets;
- mesh compression extensions;
- embedded or external glTF image textures;
- PBR metallic/roughness lighting beyond the base colour;
- animation weights and arbitrary extension channels;
- cubic-spline animation playback.

Use rigid node animation instead of a skin. Use material base colours and
vertex colours for general entity models. TGA textures are loaded separately
for registered renderer roles; a texture referenced inside a GLB is not bound
automatically.

For historical line and point assets, the first primitive may carry these
optional `extras` values:

```json
{
  "battalionLineWidth": 2.0,
  "battalionPointSize": 3.0,
  "battalionUseCurrentColor": true
}
```

`battalionUseCurrentColor` is intended for engine-coloured geometry such as
ground planes. Ordinary authored objects should carry their own material or
vertex colours.

### Static model

A minimal static model needs at least one supported mesh, one node using it,
and a scene containing that node. Keep the full object in one GLB. Do not split
one tree, building, antenna, vehicle, or creature into separate files merely
to represent its parts.

Add the result to the manifest:

```text
model radar-dish models/buildings/radar-dish.glb
```

At this point the GLB can be referenced by an entity descriptor. It is not yet
placed in the world.

### Compound and animated model

Use named parent/child nodes for moving parts. Parent transforms affect their
children, so hierarchy is gameplay-significant. For a tank, a suitable tree
is:

```text
body
├── tracks
├── upper-hull
└── turret
    └── barrel
```

Only `turret` receives live yaw and only `barrel` receives live pitch. Making
`upper-hull` a turret child would incorrectly rotate the hull plate. Making
the complete turret a barrel child would incorrectly pitch the turret roof.

Animations must be named inside the GLB. Supported channels target node
translation, rotation, or scale and use `LINEAR` or `STEP` interpolation.
Animated nodes must use TRS transforms, not a 4x4 `matrix`. Keyframe times are
floating-point seconds, start at zero or later, are strictly increasing, and
must not be negative.

`CUBICSPLINE` channels are ignored. Do not use them: a clip can exist by name
yet have no playable tracks.

Register a named clip separately from its model:

```text
model radar-dish models/buildings/radar-dish.glb
animation radar-dish-scan radar-dish scan loop
```

The four animation fields are:

```text
animation <animation-key> <model-key> <clip-name> <loop|once>
```

The model and clip must already exist when the complete manifest is validated.
An animation key is what entity descriptors and render bindings reference;
they do not reference the raw clip name directly.

Use `loop` for idle motion, tracks, propellers, rotors, walking, breathing, and
scanning. Use `once` for attacks, destruction, collapse, and explosions. The
entity controller selects idle, movement, and attack clips by state. It does
not blend clips.

### Live articulated controls

An entity can bind aiming to two named nodes:

```text
aim-yaw-node turret
aim-pitch-node barrel
```

Both directives must be present or both absent. The names must exist in the
entity's normal GLB. Mouse movement and IJKL update these nodes after the
current canned pose is sampled, so aim can be combined with a moving clip.

Creatures whose head or eye assembly turns and tilts as one part may name the
same GLB node in both directives:

```text
aim-yaw-node head
aim-pitch-node head
```

This keeps the firing direction and visible head/eyes aligned without adding a
creature-specific branch to the engine.

Keep yaw and pitch pivots at their real mechanical axes. The barrel should be
a descendant of the turret when it must inherit turret yaw.

### Transparency

For translucent geometry, set the material to `alphaMode: "BLEND"` and give
`baseColorFactor` or vertex colour an alpha below `1`. Do not expect alpha alone
to enable blending. Vapour is the reference asset: its particle nodes and
`cloud` clip live in one GLB and its alpha is authored as blended material
data.

Transparent models still need sensible draw ordering. Prefer a small number of
non-intersecting translucent layers and avoid using transparency to repair bad
solid geometry.

## Global asset manifest

`assets/manifest.txt` begins with:

```text
battalion-assets 2
```

Blank lines and lines whose first token starts with `#` are ignored. Every
other line must exactly match one of these forms:

```text
model <key> <relative-glb-path>
texture <key> <relative-tga-path> <nearest|linear> <clamp|repeat>
animation <key> <model-key> <clip-name> <loop|once>
file <key> <relative-path>
```

Models and textures are loaded eagerly. Missing files, duplicate keys,
unsupported GLBs or TGAs, missing animation clips, malformed options, and
unsafe paths stop startup or the test suite. Every `.glb` under
`assets/models/` and every `.tga` under `assets/textures/` must be declared;
orphan files are errors.

### External textures

The texture loader accepts unpaletted 24-bit or 32-bit true-colour TGA, either
uncompressed or RLE-compressed. The manifest chooses nearest/linear sampling
and clamp/repeat wrapping:

```text
texture warning-grid textures/warning-grid.tga nearest repeat
```

A manifest texture is data, not an automatic material assignment. Existing
engine-rendered roles acquire it through `render.bindings`. General GLB entity
meshes currently use material and vertex colours rather than an arbitrary TGA
binding in their `.entity` file.

## Render bindings

`assets/config/render.bindings` begins with:

```text
battalion-bindings 1
```

It maps a stable engine role to a manifest key:

```text
model effect-boom-high boom-high
animation effect-boom-high boom-high-explode
texture road-surface road
file config-effects effect-values
```

Use bindings when replacing content for a role already known by the engine,
such as terrain, bound structures, projectiles, explosions, UI geometry, or a
configuration file. Do not add a new binding slot speculatively: a slot is only
active if an engine capability consumes it.

New generic actors normally reference model and animation manifest keys
directly from their `.entity` descriptor and do not need new render slots.

## Entity catalog

`assets/entities/catalog.txt` begins with:

```text
battalion-entities 1
```

It contains an optional fallback player archetype and one line for each
descriptor:

```text
fallback googelon
entity entities/vehicles/tank-standard.entity
entity entities/npcs/guard.entity
entity entities/structures/radar-station.entity
```

Descriptor paths are relative to `assets/`. They must be lowercase safe paths.
Entity ids, player shortcuts, and network codes must be unique.
The fallback must name a catalog entry. The game also verifies that every
referenced model, clip, and aim node exists.

## Entity descriptor format

Every descriptor begins with exactly:

```text
battalion-entity 1
```

Comments beginning in column one with `#` and blank lines are ignored. Every
ordinary directive has one value; sensors have five arguments after the word
`sensor`, and reaction rules have three. Values cannot contain whitespace.

The four required directives are:

```text
id <lowercase-id>
kind <vehicle|npc|object|device|structure|creature>
model <manifest-model-key>
faction <lowercase-faction>
```

`kind` classifies content for authors and future capability policies. Current
motion, control, combat, and AI are selected by their explicit capability
fields, not by a hidden per-kind switch. Consequently a creature can fly, a
vehicle can be stationary, and an object can have sensors.

### Visual directives

| Directive | Default | Meaning |
| --- | ---: | --- |
| `model` | required | Normal model manifest key. |
| `destroyed-model` | none | Optional model drawn after destruction. |
| `shadow-model` | none | Optional separate shadow model. |
| `idle-animation` | none | Animation key used while idle. |
| `move-animation` | none | Animation key used while moving. |
| `attack-animation` | none | Animation key used while attacking. |
| `animation-speed` | `0.05` | Seconds added to clip time per simulation counter. Must be positive. |
| `scale` | `1` | Uniform render scale. Must be positive. |
| `vertical-offset` | `0` | Model-origin height relative to the battlefield plane. |
| `aim-yaw-node` | none | Named GLB node receiving live yaw. Requires pitch node. |
| `aim-pitch-node` | none | Named GLB node receiving live pitch. Requires yaw node. |
| `render-node` | none | Repeat to draw only named model subtrees; with no entries the active scene is drawn. |

The normal, destroyed, and shadow model names are manifest keys, not paths.
Animation names are manifest animation keys, not raw GLB clip names.

Use `render-node` when one GLB deliberately contains optional variants such as
seasonal parts, LOD nodes, or a separately controlled shadow. The selected
nodes are still sampled from the active animation pose. This keeps variants in
one source model without drawing every optional subtree at once.

### Identity and faction directives

| Directive | Default | Meaning |
| --- | ---: | --- |
| `id` | required | Unique archetype id. |
| `kind` | required | Authoring category. |
| `faction` | required | This archetype's faction. |
| `hostile-faction` | none | Repeat up to eight times to declare enemies. |
| `network-code` | none | Optional numeric wire id for multiplayer. It does not select behavior or rendering. |
| `model-yaw-offset` | `0` | Visual yaw correction in degrees; gameplay forward remains `+Z`. |

Hostility is directional. If `guards` lists `raiders`, that does not make
`raiders` hostile to `guards` unless their descriptor says so too.

### Movement, collision, and control

| Directive | Default | Meaning |
| --- | ---: | --- |
| `movement` | `ground` | `stationary`, `ground`, or `air`. |
| `speed` | `0.03` | Distance advanced per simulation step; zero is allowed. |
| `turn-rate` | `0.2` | Fraction of heading error applied per AI step; must be positive. |
| `collision-mode` | `solid` | `solid` blocks; `phase` passes through structures and actors. |
| `collision-radius` | `0.25` | Horizontal collision radius; must be positive. |
| `collision-height` | `0.3` | Physical height; must be positive. |
| `control-mode` | `free` | `free` strafes; `steered` turns the hull with A/D. |

`movement stationary` prevents AI path movement. It does not make an object
invulnerable, invisible, or non-reactive. Use it for props, buildings, turrets,
sensors, switches, and other fixed actors.

`collision-mode phase` is reusable for vapour, ghosts, smoke, or any other
non-solid actor. It is not tied to a particular entity id.

### Damage, scoring, and spawning

| Directive | Default | Meaning |
| --- | ---: | --- |
| `health` | `1` | Hit points; at least one. |
| `score` | `10` | Score awarded for destruction; non-negative. |
| `spawn-weight` | `0` | Relative random-spawn weight; zero disables random spawning. |
| `spawn-after` | `0` | Simulation counter before random spawning is allowed. |
| `max-active` | `0` | Maximum random/map instances counted for spawning; zero is unlimited. |

Random spawn weights are relative to other eligible descriptors and to the
runtime's no-spawn chance. Do not treat the number as a percentage. Explicit
map actors are the preferred method when placement and behavior matter.

### Weapon and target directives

| Directive | Default | Meaning |
| --- | ---: | --- |
| `weapon` | `none` | `none`, `shell`, `rocket`, `beam`, `fire`, or `plasma`. |
| `weapon-color` | `blue` | Palette key used by the visible weapon effect. |
| `weapon-range` | `6` | Maximum attack range; non-negative. |
| `weapon-min-range` | `0.5` | Minimum attack range; between zero and maximum. |
| `projectile-speed` | `0.08` | Projectile capability speed; must be positive. |
| `fire-interval` | `120` | Simulation steps between shots; at least one. |
| `accuracy` | `10` | Existing weapon spread/accuracy parameter; non-negative. |
| `forget-range` | `12` | Target is forgotten beyond this range; zero disables range forgetting. |
| `lost-frames` | `180` | Steps to retain an unseen target; non-negative. |
| `target-mode` | `nearest` | `nearest` or `all` sensor processing. |
| `max-targets` | `1` | Contacts retained per step, from one through eight. |
| `muzzle-forward` | `0` | Spawn offset in the aimed forward direction. |
| `muzzle-height` | `0` | Spawn height relative to entity origin. |

`target-mode all` applies sensor rules to up to `max-targets` contacts. The
current movement and weapon controller still has one active target at a time;
the other contacts are useful for emitted events and group activation.

An `attack` reaction is invalid with `weapon none`. New weapon names cannot be
created in a descriptor; they must be registered as a reusable engine
capability first.

### Player-control directives

| Directive | Default | Meaning |
| --- | ---: | --- |
| `controllable` | `no` | Include this archetype in player selection. |
| `shortcut` | none | Unique one-character selection shortcut; requires controllable. |
| `player-energy` | `50` | Maximum/start energy; positive. |
| `player-height` | `1.3` | Player/camera envelope height; positive. |
| `player-width` | `0.3` | Player envelope width; positive. |
| `player-bottom` | `0` | Bottom of the player envelope relative to origin. |
| `player-regen` | `0.1` | Energy regenerated per step; non-negative. |
| `player-move-cost` | `0.1` | Movement energy cost; non-negative. |
| `player-fire-cost` | `0.1` | Per-shot energy cost; non-negative. |
| `player-turn-rate` | `30` | Free-control turn rate; positive. |
| `player-steer-rate` | `18` | Steered hull turn rate; positive. |
| `aim-pitch-default` | `0` | Initial pitch in degrees. |
| `aim-pitch-min` | `-40` | Lower pitch limit in degrees. |
| `aim-pitch-max` | `40` | Upper pitch limit in degrees. |
| `first-person-eye-height` | `0` | True first-person eye/cockpit height. Must be positive when controllable. |
| `first-person-visible` | `no` | Draw the player's model in true first-person view. |

For a controllable armed entity, `muzzle-forward` must be non-negative and
`muzzle-height` must be positive. Pitch minimum must be below maximum and the
default must lie inside the interval.

`control-mode free` gives camera-relative WASD movement. `control-mode steered`
gives W/S forward and reverse with A/D hull turning, appropriate for tracked or
wheeled vehicles. Mouse and IJKL aiming are independent from hull steering when
aim nodes are configured.

View 1 is the original close third-person chase view. View 5 is true
first-person. Tune `vertical-offset`, collision/player dimensions, and
`first-person-eye-height` together; do not compensate for a bad model origin by
moving only the camera.

## Sensors and perception zones

An archetype may have up to eight independently configured sensors:

```text
sensor <id> <type> <range> <angle-degrees> <faction-filter>
```

Supported types are:

| Type | Detection rule |
| --- | --- |
| `proximity` | Circular zone around the actor; ignores facing. |
| `vision` | Range plus a forward cone; also requires world line of sight. |
| `hearing` | Range plus an audible contact; not blocked by structures. |
| `damage` | Reacts to the recorded damage source. Use conventional `0 360`. |

Range must be non-negative. Angle must be greater than zero and no more than
360 degrees. The angle is the complete vision cone, not its half angle.
Proximity and hearing normally use 360.

The faction filter is one of:

- `any` for every contact;
- `hostile` for factions listed by `hostile-faction`;
- an exact faction name.

A `hostile` sensor requires at least one `hostile-faction` when reactions are
defined. Vision visibility and hearing audibility come from live world state;
the descriptor configures the zone but does not bypass occlusion or generate
sound by itself.

## Sensor lifecycle and reactions

Rules map sensor transitions to local reactions and optionally emit an event:

```text
on-enter  <sensor-id> <reaction> <emitted-event|none>
on-stay   <sensor-id> <reaction> <emitted-event|none>
on-exit   <sensor-id> <reaction> <emitted-event|none>
```

`on-enter` runs once when a contact enters. `on-stay` runs on later steps while
it remains selected. `on-exit` runs once after it leaves and carries the last
known contact marked inactive.

Supported reactions are:

| Reaction | Effect |
| --- | --- |
| `observe` | Retain the weakest passive state. |
| `alert` | Mark an alert state and target. |
| `investigate` | Move toward the contact's last known position. |
| `pursue` | Follow the contact. |
| `attack` | Approach weapon range and request fire when reloaded. |
| `flee` | Move away from the contact. |
| `activate` | Enable an inactive actor. |
| `deactivate` | Disable the actor and clear its target. |

Behavioral reactions have increasing priority in the order listed through
`flee`; a weaker rule does not replace an already stronger state. `activate`
and `deactivate` are state commands. Multiple rules may consume the same event,
which allows an actor to activate and then attack:

```text
on-trigger base-alarm activate none
on-trigger base-alarm attack none
```

An archetype may define up to 32 sensor and trigger rules. Event names use the
same lowercase naming rules as ids. `none` suppresses event emission.

## External triggers

An actor consumes a routed event with:

```text
on-trigger <event> <reaction> <emitted-event|none>
```

The original perceived or damaging contact travels with the event, so a guard
can tell another actor whom to investigate or attack. Delivery is queued until
the next simulation step after all senders have updated. Behavior therefore
does not depend on linked-list or map actor order.

Inactive actors do not perceive or patrol, but queued triggers are processed
first. An `activate` rule can wake an inactive actor; a second rule for the same
event can immediately assign its target and reaction.

## Complete archetype examples

### Static prop

A static prop is still an entity and can later gain health, sensors, triggers,
or a destroyed model without changing its map representation:

```text
battalion-entity 1
id cargo-crate
kind object
model cargo-crate
faction neutral
movement stationary
weapon none
health 1
score 0
collision-mode solid
collision-radius 0.25
collision-height 0.3
spawn-weight 0
```

Add `model cargo-crate models/terrain/cargo-crate.glb` to the manifest, add the
descriptor to the entity catalog, and place it as a version 2 map actor.

### Animated static device

This radar never moves across the map, but its dish rotates and it can alert a
group when an enemy enters its sight cone:

```text
battalion-entity 1
id radar-station
kind device
model radar-dish
idle-animation radar-dish-scan
animation-speed 0.05
faction army
hostile-faction monster
movement stationary
weapon none
health 3
score 20
collision-radius 0.6
collision-height 1.2
sensor sight vision 14 90 hostile
on-enter sight alert radar-alert
on-trigger power-off deactivate none
on-trigger power-on activate none
spawn-weight 0
```

The GLB contains a named `scan` rotation clip on the dish node. The manifest
maps it to the descriptor's animation key:

```text
model radar-dish models/buildings/radar-dish.glb
animation radar-dish-scan radar-dish scan loop
```

### Patrolling NPC

```text
battalion-entity 1
id rifle-guard
kind npc
model rifle-guard
idle-animation rifle-guard-idle
move-animation rifle-guard-walk
attack-animation rifle-guard-fire
animation-speed 0.05
faction army
hostile-faction monster
movement ground
control-mode free
speed 0.025
turn-rate 0.2
health 3
score 15
collision-radius 0.2
collision-height 0.9
weapon beam
weapon-color red
weapon-range 5
weapon-min-range 0.5
projectile-speed 0.1
fire-interval 30
accuracy 20
forget-range 12
lost-frames 180
target-mode nearest
max-targets 1
sensor nearby proximity 1.5 360 hostile
sensor sight vision 8 100 hostile
sensor noise hearing 10 360 hostile
sensor damaged damage 0 360 hostile
on-enter nearby attack squad-alert
on-enter sight pursue squad-alert
on-enter noise investigate squad-alert
on-enter damaged attack squad-alert
on-exit sight investigate target-lost
on-trigger squad-alert activate none
on-trigger squad-alert attack none
spawn-weight 0
```

The route and patrol path belong to the map instance, not this reusable NPC
archetype.

### Steered vehicle with articulated weapon

```text
battalion-entity 1
id scout-tank
kind vehicle
model scout-tank
destroyed-model scout-tank-destroyed
shadow-model scout-tank-shadow
move-animation scout-tank-tracks
aim-yaw-node turret
aim-pitch-node barrel
animation-speed 0.05
faction army
hostile-faction monster
movement ground
control-mode steered
speed 0.04
turn-rate 0.2
health 2
score 10
collision-radius 0.25
collision-height 0.3
weapon shell
weapon-range 7
weapon-min-range 0.5
projectile-speed 0.3
muzzle-forward 0.3
muzzle-height 0.2
fire-interval 90
accuracy 1000
forget-range 14
lost-frames 180
target-mode nearest
max-targets 1
sensor sight vision 10 120 hostile
sensor damaged damage 0 360 hostile
on-enter sight attack armour-alert
on-enter damaged attack armour-alert
on-trigger armour-alert activate none
on-trigger armour-alert attack none
spawn-weight 0
controllable yes
player-energy 50
player-height 0.6
player-width 0.3
player-bottom 0
player-regen 0.1
player-move-cost 0.05
player-fire-cost 0.05
player-turn-rate 35
player-steer-rate 8
aim-pitch-default 0
aim-pitch-min -20
aim-pitch-max 35
first-person-eye-height 0.28
first-person-visible no
```

The tracks clip belongs inside the same complete tank GLB. Turret and barrel
also remain in that GLB as correctly parented named nodes.

### Flying creature or vehicle

Use `movement air` and a positive `vertical-offset`. The kind can be creature
or vehicle; behavior comes from capabilities:

```text
battalion-entity 1
id scout-drone
kind vehicle
model scout-drone
idle-animation scout-drone-rotors
move-animation scout-drone-rotors
attack-animation scout-drone-rotors
faction army
hostile-faction monster
movement air
control-mode free
vertical-offset 1.2
speed 0.06
turn-rate 0.2
health 2
collision-radius 0.25
collision-height 0.35
weapon rocket
weapon-range 7
weapon-min-range 1
projectile-speed 0.12
fire-interval 100
sensor sight vision 12 140 hostile
on-enter sight attack drone-alert
spawn-weight 0
```

Add the full player parameter block if the drone must be selectable by the
player.

### Phased creature

For a vapour-like creature that may pass through structures and actors, use:

```text
kind creature
movement ground
collision-mode phase
```

Transparency is a GLB material property, not a consequence of phased
collision. Configure both independently.

## Map authoring and actor instances

Battlefields are `assets/maps/<id>/map.json`. A map with generic actors uses:

```json
{
  "format": "battalion-map",
  "version": 2,
  "id": "test-yard",
  "name": "Test Yard",
  "roads": [],
  "structures": [],
  "units": [],
  "actors": []
}
```

All four arrays are required in version 2, even when empty. `roads`,
`structures`, and `units` retain the historical numeric map records. New
extensible objects should normally use `actors`, whose geometry and behavior
come from entity archetypes.

### Actor fields

| Field | Required | Default | Meaning |
| --- | --- | ---: | --- |
| `id` | yes | — | Unique lowercase map-instance id. |
| `archetype` | yes | — | Id from the entity catalog. |
| `x` | yes | — | World X coordinate. |
| `z` | yes | — | World Z coordinate. |
| `group` | no | empty | Lowercase event-routing group. |
| `heading` | no | `0` | Initial heading in radians; zero faces `+Z`. |
| `active` | no | `true` | Whether perception, patrol, and reactions start enabled. |
| `patrol-mode` | no | `loop` | `loop`, `pingpong`, or `once`. |
| `path` | no | empty | Ordered array of `{ "x", "z" }` waypoints. |
| `triggers` | no | empty | Emitted-event routing rules. |

Actor ids must be unique. A trigger target id or group must exist somewhere in
the same map package. A path on `movement stationary` is valid data but cannot
move the actor. Actor positions and every waypoint use map-world coordinates.
The actor, its current target, sensor contacts, queued events, and the complete
path retain those coordinates while the player moves; no path point is
relative to the player or camera.

### Patrol and group alarm example

```json
{
  "id": "guard-a",
  "archetype": "rifle-guard",
  "group": "base-guards",
  "x": 5,
  "z": 5,
  "heading": 0,
  "active": true,
  "patrol-mode": "loop",
  "path": [
    {"x": 5, "z": 5},
    {"x": 8, "z": 5},
    {"x": 8, "z": 8}
  ],
  "triggers": [
    {"event": "squad-alert", "target-group": "base-guards"},
    {"event": "radar-alert", "target-id": "gate-turret"}
  ]
}
```

Each trigger contains exactly one of `target-id` or `target-group`. The event
must be emitted by one of the source archetype's rules to do anything.

An initially inactive reinforcement can be configured as:

```json
{
  "id": "reserve-tank",
  "archetype": "scout-tank",
  "group": "base-guards",
  "x": 12,
  "z": 4,
  "active": false,
  "path": [],
  "triggers": []
}
```

Its descriptor must contain an `on-trigger` rule for the routed event, for
example `on-trigger squad-alert activate none`.

## Effect authoring

Battalion has two effect families and they must not be confused.

### Authored model effects

Projectiles, explosions, fire meshes, spheres, and reusable shadows are GLB
models under `assets/models/effects/`. They follow the same static and animated
GLB profile as every other model.

An animated explosion is registered like this:

```text
model shockwave models/effects/shockwave.glb
animation shockwave-expand shockwave expand once
```

To replace an existing engine effect role, bind the new keys:

```text
model effect-boom-high shockwave
animation effect-boom-high shockwave-expand
```

The binding slot must already be consumed by the engine. Adding a new model
and new arbitrary binding slot does not define when it is spawned, what it
damages, or when it expires.

An effect that should exist as a persistent map object can instead be a normal
`kind object` entity, often with `movement stationary` and `collision-mode
phase`. It may be activated and deactivated by triggers. Timed lifetime and
spawn-on-impact behavior remain properties of registered effect capabilities.

### Live dynamic effects

Beams, scars, HUD quads, and similar geometry derive endpoints or vertices from
current gameplay state every frame. Their topology is emitted through the
renderer-neutral dynamic primitive layer; reusable appearance values live in
`assets/config/effects.values`:

```text
battalion-values 1
value monster-beam-line-width 5.01
value monster-beam-lines 4 25 40
value monster-beam-jitter 0.04
```

Existing keys have required component counts. Preserve those counts when
tuning a value. A new `value` line is inert until a generic engine capability
looks it up. Never hardcode a tunable colour, width, radius, density, or timing
constant in a renderer backend.

### Effect transparency and ordering

Model effects use GLB `alphaMode: "BLEND"` plus authored alpha. Dynamic effects
pass RGBA values through the shared primitive API. Test transparency on Metal
and classic OpenGL; no effect should rely on one backend's implicit blend state.

### Effect audio

Semantic sound and music files are declared by `assets/audio/catalog.txt`.
Entity descriptors currently do not have arbitrary sound-event directives.
Replacing an existing sound is data-only; defining a new gameplay sound event
requires a reusable audio-event capability before descriptors can select it.

## Validation workflow

Run the complete validation suite after every content change:

```sh
make test
```

It checks, among other things:

- strict C89 compatibility of the engine;
- GLB loading, hierarchy, primitives, transparency, and animation sampling;
- manifest/file agreement and lowercase paths;
- absence of orphan GLB and TGA files;
- render bindings and required configuration values;
- entity syntax, referenced models/clips/nodes, and catalog uniqueness;
- map JSON parsing, actor links, and write/read round trips;
- sensors, lifecycle transitions, reactions, patrol, and triggers;
- absence of compiled model geometry and direct asset paths in gameplay code.

Then build and run the intended platform. Examples:

```sh
make PLATFORM=macos
build/macos-metal-none/battalion --map default --player scout-tank

make PLATFORM=linux AUDIO=none
build/linux-opengl-none/battalion --map default --player scout-tank
```

When developing one new actor, create a small version 2 test map containing
that actor, a hostile target, empty historical arrays, and short patrol paths.
Test idle, movement, attack, destruction, all camera views, collision,
occlusion, sensor transitions, event routing, and reload timing.

## Content review checklist

Before committing a new object, verify all of the following:

- every path and key is lowercase and categorized in its own subdirectory;
- one complete object is one GLB, including rigid parts and clips;
- no duplicate part GLBs or temporary exports remain;
- origin is at the intended ground/contact point and heading zero faces `+Z`;
- animated pivots and parent/child relationships are correct;
- clips are named, use TRS, and use only `LINEAR` or `STEP` interpolation;
- translucent materials explicitly use `alphaMode: "BLEND"`;
- the model and every clip are present in `manifest.txt`;
- the descriptor references keys, never file paths;
- the descriptor is listed once in `entities/catalog.txt`;
- collision, muzzle, camera, and vertical offsets match the actual geometry;
- collision bottom/top intervals match the object's real occupied height, so
  an airborne object may pass above a lower solid without intersecting it;
- stationary/ground/air and free/steered/phase capabilities are intentional;
- hostility is defined in every required direction;
- every sensor id used by a rule exists;
- every emitted event either has a map route or is intentionally local-only;
- every map trigger target id/group exists;
- `make test` passes;
- the game process is closed after visual testing.

## Common failures

### `An entity references missing assets`

Check that normal/destroyed/shadow model keys exist, all animation keys resolve
to real named clips, and both aim node names exist in the normal GLB.

### Model loads but is white

The GLB loader uses material base colour and vertex colour, not glTF image
textures. Author colours in `baseColorFactor`/`COLOR_0`, or use an existing
registered external TGA renderer role where appropriate.

### Model is transparent in the authoring tool but opaque in game

Set `alphaMode` to `BLEND`; alpha below one is not sufficient by itself.

### Animation is present but does not move

Check the exact clip name, manifest animation key, interpolation, animated node
transform type, keyframe times, and controller state. Do not animate a node
that has a matrix transform and do not export `CUBICSPLINE` channels.

### Unrelated vehicle parts rotate or pitch

Repair the GLB hierarchy. Only nodes below the live aim node inherit its
transform. Keep fixed hull parts outside the turret and barrel subtree.

### Object floats above its shadow or the ground

Repair the GLB origin first, then tune `vertical-offset`. Verify that the
separate shadow model uses the same local origin.

### Sensor never fires

Check range, full cone angle, faction filter, directional hostility, contact
visibility/audibility, and whether the actor is active. A vision sensor also
requires line of sight; a hearing sensor requires the contact to be audible.

### Event is emitted but another actor does nothing

The source map actor needs a matching route, the target id/group must exist,
and the target archetype needs a matching `on-trigger` rule. Delivery occurs on
the following simulation step.

### New effect value has no visible result

Configuration does not invent behavior. Confirm that an existing effect
capability consumes that exact key and component count, or add a generic engine
capability before exposing its instances as data.
