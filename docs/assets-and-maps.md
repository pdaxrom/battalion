# External assets and battlefield maps

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
scale. `LINEAR` and `STEP` interpolation are supported. Textures, skins, morph
targets, mesh compression, and cubic-spline playback remain later steps.

`assets/manifest.txt` declares every runtime model, texture, and animation.
Paths in the manifest are relative to `assets/`. Missing or invalid declared
content is fatal; there is no normal-build fallback to compiled C geometry or
texture arrays. This policy is the same on every supported platform.

The checked-in GLB files are the canonical source assets. The one-time legacy
converter and the old C model factories have been removed, so geometry is not
duplicated between code and assets.

The tank is a static one-node scene. Compound vehicles and monsters keep their
parts as named nodes in one GLB. The helicopter carries its `rotors` clip, the
airplane its `propellers` clip, Googelon its `tail-sway` clip, Flutter its
`wing-flap` clip, the two Hero variants their `walk` and `fire` clips, and Mecha
its `eye-scan` clip. Projectile variants likewise keep their named `spin` clips
inside their GLB files.

## Existing battlefield maps

The maps were already external; they were not procedurally generated. Their
historical three-file representation has been converted to one JSON package
per battlefield and removed. A map can be selected without changing files:

```sh
build/macos-metal-none/battalion --map 1
build/macos-metal-none/battalion --map Andyville
```

The runtime still dynamically spawns enemies, applies destruction, expands
some compact map markers, and shifts world coordinates around the player. That
runtime behavior is separate from the authored map layout.

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

Map and model data remain separate: map records identify the legacy gameplay
type/shape, while migrated geometry and animation live in renderer-neutral GLB
files under `assets/models`. Runtime expansion of compound hills, initial occupants,
destruction state, and enemy spawning is intentionally still gameplay logic.

JSON packages are required on every platform. `make test` validates every
package and performs JSON write/read round trips.
## Layout and compatibility

The directory names are deliberately lowercase: `maps`, `models`, `textures`,
`music`, and `sounds`. Models are grouped below `models/` as `vehicles`,
`monsters`, `buildings`, `terrain`, `effects`, and `ui`. Model and animation
files are always GLB assets selected through the manifest. The same
renderer-neutral representation feeds Metal and desktop OpenGL; GLES will
consume that representation when its backend is implemented.
