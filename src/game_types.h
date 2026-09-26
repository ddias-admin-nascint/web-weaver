#ifndef GAME_TYPES_H
#define GAME_TYPES_H

#include "splashkit.h"

#define SCREEN_WIDTH   1024
#define SCREEN_HEIGHT  768
#define MAX_INSECTS    64
#define INSECT_CAP     12

#define MAX_NODES           64
#define MAX_STRANDS         128
#define MAX_EDGES_PER_NODE  8
#define WEB_SPOKES          8
#define WEB_RINGS           3

/**
 * The kinds of insect that can spawn. The kind drives point value, size and
 * speed, selected by a switch rather than by scattered conditionals.
 */
enum insect_kind
{
    FLY,
    BUTTERFLY,
    WASP,
    GOLDEN
};

/** A drawable, collidable thing with a position and a size. */
struct entity
{
    point_2d    pos;
    double      radius;
    color       clr;
    bool        enabled;
};

/** An insect in play. Composes an entity and adds movement state. */
struct insect
{
    entity      ent;
    vector_2d   velocity;
    insect_kind kind;
    bool        stuck;
    double      stuck_timer;
    int         stuck_strand;   // index of the strand holding it, or -1
    double      life_timer;     // seconds remaining before it leaves
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

/** The web as a graph: nodes referencing strands, strands referencing nodes. */
struct web_graph
{
    web_node    nodes[MAX_NODES];
    int         node_count;
    web_strand  strands[MAX_STRANDS];
    int         strand_count;
};

/** The spider the player controls. */
struct spider
{
    entity      ent;
    double      speed;
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

    int         score;
    int         lives;
    int         repairs;
    double      spawn_timer;
    double      elapsed;
};

#endif