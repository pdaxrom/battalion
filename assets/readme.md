# Battalion assets

All runtime content lives below this directory. Directory and file names use
lowercase so the same package works on case-sensitive and case-insensitive
filesystems.

```text
assets/
  manifest.txt       model, texture, and animation registry
  data/              historical level source data
  maps/              versioned JSON battlefield packages
  models/            Khronos glTF 2.0 binary models and animation clips
  textures/          TGA textures
  music/             music tracks
  sounds/            sound effects
  battalion.sho      historical Showcase help document
```

`assets/manifest.txt` is required. A missing, invalid, or undeclared model,
texture, or animation is an error; the normal executable has no compiled C
geometry or texture fallback.

The GLB loader supports triangle, line, and point primitives; indexed and
non-indexed positions; normals, UVs and vertex colours; material base colours;
scene-node hierarchies; and named translation, rotation, and scale clips with
`LINEAR` or `STEP` interpolation. `helo.glb` is one compound asset containing
the body, both rotors, and the named `rotors` animation. Standard projectile
models likewise carry their `spin` animation inside their GLB files.

Regenerate converted content and verify it with:

```sh
make export-assets
make export-maps
make test
```
