# Battalion assets

All runtime content lives below this directory. Directory and file names use
lowercase so the same package works on case-sensitive and case-insensitive
filesystems.

```text
assets/
  manifest.txt       model, texture, and animation registry
  maps/              versioned JSON battlefield packages
  models/            Khronos glTF 2.0 binary models and animation clips
    vehicles/        tanks, aircraft, and vehicle shadows
    monsters/        monsters, Heroes, and Mecha
    buildings/       structures and structure animations
    terrain/         trees, water, hills, and ground meshes
    effects/         projectiles, explosions, fire, and generic effects
    ui/              title geometry
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
scene-node hierarchies; named nodes; and named translation, rotation, and scale
clips with `LINEAR` or `STEP` interpolation. `helo.glb`, `airplane.glb`,
`googelon.glb`, `flutter.glb`, `techs.glb`, both Hero files, and `mecha.glb`
are compound assets. Their
moving parts and reusable variants are nodes in the same file; periodic motion
is stored in named animation clips. Standard projectile models likewise carry
their `spin` animation inside their GLB files.

Verify the checked-in content with:

```sh
make test
```
