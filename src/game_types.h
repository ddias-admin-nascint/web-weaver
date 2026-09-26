#ifndef GAME_TYPES_H
#define GAME_TYPES_H

#include "splashkit.h"

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768
#define MAX_INSECTS 64
#define INSECT_CAP 12

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
    point_2d pos;
    double radius;
    color clr;
    bool enabled;
};

/** An insect in play. Composes an entity and adds movement state. */
struct insect
{
    entity ent;
    vector_2d velocity;
    insect_kind kind;
    double life_timer; // seconds remaining before it leaves
};

/** The spider the player controls. */
struct spider
{
    entity ent;
    double speed;
};

/**
 * Everything the game owns. Passing this by reference keeps parameter lists
 * short and lets procedures modify state in place.
 */
struct game_data
{
    spider player;
    insect insects[MAX_INSECTS];
    int insect_count;

    int score;
    int lives;
    double spawn_timer;
    double elapsed;
};

#endif