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

The checked-in GLB files are generated deterministically from the original
source material:

```sh
make export-assets
```

The export command builds a separate developer-only converter, captures the
source geometry through the renderer compatibility frontend, writes
deterministic GLBs, and exits. Normal builds do not compile the converter or
the old model factories.

The tank is a static one-node scene. The helicopter is the first animated
asset: its body, main rotor, and tail rotor are separate meshes/nodes, and its
named `rotors` clip contains the two rotation tracks. `drawHelo()` samples that
clip using the original game counter, so the speed and phase remain compatible
with the original animation. Projectile variants likewise keep their named
`spin` clips inside the GLB files.

## Existing battlefield maps

The maps were already external; they were not procedurally generated. The
historical representation splits each battlefield across three text files:

- `roadNN.data`: road placement;
- `treeNN.data`: buildings, trees, and other static structures;
- `tankNN.data`: initial units;
- `battalion.levels`: numeric indices and display names.

The unnumbered `road.data`, `tree.data`, and `tank.data` retain the historical
default. A map can now be selected without changing files:

```sh
build/macos-metal-none-hybrid/battalion --map 1
build/macos-metal-none-hybrid/battalion --map Andyville
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

Map build modes mirror the model migration policy:

- `MAPS=legacy`: compile only the original three-file reader;
- `MAPS=hybrid`: load `map.json`, falling back to the original files;
- `MAPS=json`: require a valid v1 package.

Linux and macOS default to `hybrid`. Historical Unix targets default to
`legacy`, keeping the original map reader available while model, texture,
animation, audio, and map files all remain below the same `assets/` root.

Regenerate all packages from them with:

```sh
make export-maps
```

The converter stops at the same all-zero terminators as the 1996 loader and
reproduces its token-stream interpretation, including unusual legacy input.
`make test` compares every generated package against its three source files and
also performs a JSON write/read round trip.

## Layout and compatibility

The directory names are deliberately lowercase: `data`, `maps`, `models`,
`textures`, `music`, and `sounds`. The map reader can still select `legacy`,
`hybrid`, or `json` input, but all three modes resolve their files below
`assets/`. Model and animation files are always GLB assets selected through the
manifest. The same renderer-neutral representation feeds Metal and desktop
OpenGL; GLES will consume that representation when its backend is implemented.
