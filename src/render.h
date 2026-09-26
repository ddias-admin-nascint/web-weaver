#ifndef RENDER_H
#define RENDER_H

#include "game_types.h"

void draw_web(const game_data &game);
void draw_insects(const game_data &game);
void draw_spider(const game_data &game);
void draw_hud(const game_data &game);
void draw_debug(const game_data &game);

void draw_menu(const game_data &game);
void draw_playing(const game_data &game);
void draw_paused(const game_data &game);
void draw_level_complete(const game_data &game);
void draw_game_over(const game_data &game);
void draw_scores(const game_data &game);

#endif