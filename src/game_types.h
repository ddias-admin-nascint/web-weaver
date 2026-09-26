#ifndef GAME_TYPES_H
#define GAME_TYPES_H

#include "splashkit.h"

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768

/**
 * A drawable, collidable thing with a position and a size.
 * Used throughout the game to represent players, enemies, bullets, and other objects.
 */
struct entity
{
    point_2d pos;
    double radius;
    color clr;
    bool enabled;
};

#endif