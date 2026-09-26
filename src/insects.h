#ifndef INSECTS_H
#define INSECTS_H

#include "game_types.h"

int     insect_value(insect_kind kind);
double  insect_radius(insect_kind kind);
double  insect_max_speed(insect_kind kind);
color   insect_colour(insect_kind kind);

void    spawn_insect(game_data &game);
void    remove_insect(game_data &game, int index);
void    update_insects(game_data &game, double dt);

#endif