#ifndef WEB_H
#define WEB_H

#include "game_types.h"

void   build_web(web_graph &web);
int    add_strand(web_graph &web, int node_a, int node_b, double integrity);
void   remove_strand(web_graph &web, int strand_id);
void   rebuild_reachability(web_graph &web);
line   strand_line(const web_graph &web, int strand_id);
bool   strand_is_live(const web_graph &web, int strand_id);
int    active_strand_count(const web_graph &web);
int    severed_node_count(const web_graph &web);
void   update_web(game_data &game, double dt);
bool   repair_nearest(game_data &game);

#endif