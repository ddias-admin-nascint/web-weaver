#ifndef COLLISION_H
#define COLLISION_H

#include "game_types.h"

collider circle_collider(const point_2d &centre, double radius);
collider segment_collider(const point_2d &a, const point_2d &b);
bool     overlaps(const collider &a, const collider &b);

void     cell_for_point(const point_2d &p, int &col, int &row);
void     build_grid(game_data &game);
void     broad_phase(game_data &game);

#endif