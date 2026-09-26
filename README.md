# Web Weaver

An arcade game written in C++ with SplashKit for **SIT102 Introduction to
Programming** - Custom Project tasks D2 and D4.

The player controls a spider defending its web. The web is not a background
image: it is a graph of anchor nodes joined by strands, held in plain arrays
and indexed by `int`. Insects that hit a strand are held there and load it.
A strand pushed past its integrity snaps, and a breadth-first search from the
anchor nodes decides which parts of the web are still attached. Severed
sections go slack, catch nothing, and are drawn greyed out until repaired.

## Build

```
skm g++ src/*.cpp -o web_weaver
./web_weaver
```

The submission build is `web_weaver.cpp`, the modules concatenated in
dependency order:

```
skm g++ web_weaver.cpp -o web_weaver
```

## Tests

```
skm g++ tests/tests.cpp src/web.cpp src/insects.cpp src/collision.cpp -o tests
./tests
```

`tests/sim.cpp` runs 60 seconds of gameplay headlessly and reports collision
cost and web wear. Useful when tuning difficulty.

## Controls

| Key | Action |
|---|---|
| Arrows | Move the spider |
| R | Repair the nearest broken strand |
| D | Toggle diagnostic counters |
| G | Toggle the collision grid (needs D) |
| P | Pause |
| ESC | Quit |

## Structure

| File | Responsibility |
|---|---|
| `src/game_types.h` | Shared structs, enums and limits |
| `src/web.*` | Graph construction, breaking, repair, connectivity search |
| `src/insects.*` | Steering behaviours, spawning, array lifecycle |
| `src/collision.*` | Unified narrow phase, spatial grid broad phase |
| `src/render.*` | Web, entities, HUD, diagnostic overlay, screens |
| `src/main.cpp` | Level table, screen state machine, event loop |

## Development stages

Built iteratively, one branch per stage, each merged to `main` only once it
compiled and ran:

1. **Core** — module structure, shared types, spider movement, insects, HUD
2. **The web** — node and strand graph, tension, breaking, repair,
   connectivity search, severed-section rendering
3. **Behaviour and collision** — steering forces per insect kind, unified
   narrow phase, spatial grid broad phase with comparison counters
4. **Completion** — screen state machine, level progression, high scores

## Attribution

The game draws inspiration from the 'Fly Catch' used in the SIT102 exercises.
However significant and original extention has been conducted by me
to bring it's difficulty up to a level of distinction, as required by the 
assignment criteria. The proposal for this game was approved as part of D2 
task.