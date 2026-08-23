<!-- Copyright (c) 2026 Battalion contributors. -->

# Data-driven content contract

This contract is a required architectural invariant for Battalion runtime,
tools, tests, and the map editor.

## One content model

Every authored world item is a catalog archetype instance. This includes
actors, vehicles, creatures, NPCs, roads, buildings, trees, hills, lakes,
devices, projectiles, and future content kinds. The editor and game consume
the same catalog and the same `map.json`; there is no editor-only schema and
no duplicated runtime representation in authored data.

Every map instance may have:

- an optional instance id and description;
- an archetype id;
- group and active state;
- an optional faction override selected from the shared faction catalog;
- world transform;
- typed instance property overrides;
- an optional shared patrol-route reference, per-instance alarm response and
  safe-zone wait policy, or owned route/area points;
- trigger routes targeting an instance id or group.

Every archetype may declare:

- display name, description, editor category, editor palette color, placement
  mode, and whether it is placeable;
- model, LOD and seasonal variants, animation, visible nodes, texture,
  material/palette parameters, and shadow participation;
- collision, destructibility, movement, weapon, projectile, sensor, reaction,
  emitter, catalog-selected impact/destruction effects and debris, semantic
  movement/fire/destruction audio, surface-contact audio rules, and other
  reusable components;
- typed property definitions and defaults exposed by the editor.

## Forbidden content hardcoding

First-party C code must not contain concrete archetype ids, manifest keys,
model-specific pose globals, per-object draw functions, numeric map object
types/shapes, or switch statements that select behavior or rendering for a
specific authored object. Moving such values into enums or renderer bindings
does not make them data-driven.

The engine may contain generic component and placement algorithms such as
model, animation, point, tile, polyline, area, solid collision, projectile,
beam, emitter, and trigger routing. Truly engine-owned resources such as UI
fallbacks may use stable bindings. Authored world content may not.

The catalog is the sole registry of authored content. Generic lookup and cache
structures must be keyed by catalog or manifest data and sized dynamically;
they must not mirror the known content roster in source code.

Faction ids and directional `neutral`, `ally`, and `hostile` relationships are
registered only in `assets/factions/catalog.txt`. Patrol points are owned by a
named map route object and may be referenced by any number of actors. Neither
relationship tables nor per-actor copies of shared routes belong in C code.

## Required enforcement

Tests must reject reintroduction of legacy `roads`, `structures`, or `actors`
arrays in canonical version 2 maps, concrete content ids in first-party C, and
content model/animation slots in `render.bindings`.
