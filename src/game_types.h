#ifndef GAME_TYPES_H
#define GAME_TYPES_H

#include "splashkit.h"

// ---------------------------------------------------------------------------
// Window and world limits.
// Fixed-size arrays are used throughout in place of dynamic containers, so
// every collection needs a compile-time maximum.
// ---------------------------------------------------------------------------
#define SCREEN_WIDTH        1024
#define SCREEN_HEIGHT       768

#define MAX_INSECTS         64
#define MAX_NODES           64
#define MAX_STRANDS         128
#define MAX_EDGES_PER_NODE  8
#define LEVEL_COUNT         5

// Broad-phase collision grid. Cell size is derived from these.
#define GRID_COLS           16
#define GRID_ROWS           12
#define MAX_PER_CELL        24

// Web layout.
#define WEB_SPOKES          8
#define WEB_RINGS           3

/**
 * The kinds of insect that can spawn. The kind drives point value, size,
 * speed and which steering behaviours are blended, so behaviour is selected
 * by a switch on this value rather than by scattered conditionals.
 */
enum insect_kind
{
    FLY,
    BUTTERFLY,
    WASP,
    GOLDEN
};

/**
 * The shape a collider represents. A single collision routine reads this to
 * decide which geometric test to perform.
 */
enum shape_kind
{
    SHAPE_CIRCLE,
    SHAPE_SEGMENT
};

/**
 * A drawable, collidable thing with a position and a size.
 * Retained from the Fly Catch worked example.
 */
struct entity
{
    point_2d    pos;
    double      radius;
    color       clr;
    bool        enabled;
};

/**
 * An insect in play. Composes an entity and adds the movement state needed
 * for steering: a current velocity and the force accumulated this frame.
 */
struct insect
{
    entity      ent;
    vector_2d   velocity;
    vector_2d   force;
    insect_kind kind;
    bool        stuck;
    double      stuck_timer;
    int         stuck_strand;   // index of the strand holding it, or -1
    double      wander_angle;   // degrees, carried between frames
    double      life_timer;     // seconds remaining before it leaves
};

/**
 * The spider the player controls. Moves freely across the play area.
 */
struct spider
{
    entity      ent;
    double      speed;
};

/**
 * A junction in the web. Holds the indices of the strands attached to it,
 * which is what makes the node and strand arrays an adjacency structure.
 */
struct web_node
{
    point_2d    pos;
    int         edges[MAX_EDGES_PER_NODE];
    int         edge_count;
    bool        is_anchor;      // fixed to the screen edge
    bool        reachable;      // result of the connectivity search
};

/**
 * A strand of web between two nodes, referenced by node index rather than
 * by pointer. Breaks when tension passes integrity.
 */
struct web_strand
{
    int         node_a;
    int         node_b;
    double      tension;
    double      integrity;
    bool        active;
};

/**
 * The web as a graph: nodes that reference strands, strands that reference
 * nodes, both held in plain arrays indexed by int.
 */
struct web_graph
{
    web_node    nodes[MAX_NODES];
    int         node_count;
    web_strand  strands[MAX_STRANDS];
    int         strand_count;
};

/**
 * A shape to be tested for overlap. Carrying the shape kind alongside the
 * geometry lets one routine resolve every pairing.
 */
struct collider
{
    shape_kind  kind;
    point_2d    a;          // circle centre, or first endpoint of a segment
    point_2d    b;          // second endpoint of a segment (unused for circles)
    double      radius;     // circle radius (unused for segments)
};

/**
 * One cell of the broad-phase grid. Holds the indices of the insects and
 * strands that currently occupy it.
 */
struct grid_cell
{
    int         insects[MAX_PER_CELL];
    int         insect_count;
    int         strands[MAX_PER_CELL];
    int         strand_count;
};

/**
 * Difficulty settings for one level, read from a table rather than being
 * hard-coded into the spawn routine.
 */
struct level
{
    int         target_score;
    double      spawn_interval;
    int         insect_cap;
    double      speed_mult;
};

/**
 * Everything the game owns. Passing this by reference keeps parameter lists
 * short and lets procedures modify state in place.
 */
struct game_data
{
    spider      player;

    insect      insects[MAX_INSECTS];
    int         insect_count;

    web_graph   web;
    grid_cell   grid[GRID_COLS][GRID_ROWS];

    level       levels[LEVEL_COUNT];
    int         level_index;

    int         score;
    int         lives;
    int         repairs;
    double      repair_timer;   // counts up to the next free repair charge

    double      spawn_timer;
    double      elapsed;

    int         comparisons;        // overlap tests actually performed
    int         naive_comparisons;  // tests an all-pairs approach would need
    bool        show_debug;
    bool        show_grid;
};

#endif