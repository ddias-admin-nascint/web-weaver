# Web Weaver

An arcade game written in C++ with SplashKit for SIT102 Introduction to 
Programming - Custom Project tasks D2 and D4.

The player controls a spider defending its web. The web is not a background
image: it is a graph of 32 nodes joined by 48 strands, held in plain arrays
and indexed by `int`, with 8 of those nodes fixed as anchors. Insects that
hit a strand are held there and load it. A strand pushed past its integrity
snaps, and a breadth-first search from the anchor nodes decides which parts
of the web are still attached. Severed sections go slack, catch nothing, and
are drawn greyed out until the player spends a repair charge on them.

## Build

```
skm g++ src/*.cpp -o web_weaver
./web_weaver
```

`web_weaver.cpp` in the project root is the same program with all modules
concatenated into one file, as required by the D4 submission:

```
skm g++ web_weaver.cpp -o web_weaver
```

## Controls

| Key | Action |
|---|---|
| Arrows | Move the spider |
| R | Repair the nearest broken strand |
| D | Toggle diagnostic counters |
| G | Toggle the collision grid (needs D on) |
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

## Diagnostics

Pressing `D` shows two counters: the overlap tests the current frame
actually performed, and the number an all-pairs approach would have needed.
`G` additionally draws the 16x12 collision grid. The reduction is large
because insects are spatially sparse — most are nowhere near a strand, so
their grid cell returns nothing to test.

## Development stages

Built iteratively, one branch per stage, each merged to `main` only once it
compiled and ran. Every commit on `main` is a working version.

| Stage | Contents |
|---|---|
| 0 | Project structure, empty window |
| 1 | Insect array, spawning, expiry, spider movement, catching, HUD. Naive collision. |
| 2 | Web graph, tension, breaking, repair, connectivity search. Naive strand collision. |
| 3 | Steering behaviours; both naive routines replaced by the grid broad phase |
| 4 | Screen state machine, level progression, ranked high scores |
| 5 | Balance changes |

Stages 1 and 2 use deliberately naive collision. Stage 3 deletes it and
replaces it with the spatial grid, so the optimisation is a visible change
in the history rather than an assertion.

## Attribution

The game draws inspiration from 'Fly Catch' game used in the SIT102 exercises. However 
significant and original extention has been conducted by me to bring it's difficulty 
up to a level of distinction, as required by the assignment criteria. The proposal 
for this game was approved as part of D2 task.