// ---------------------------------------------------------------------------
// Web Weaver - SIT102 Custom Project
// Stage 3: insect movement is now blended steering forces, and both
// naive collision loops are replaced by a single narrow-phase routine
// behind a spatial grid, with counters recording what the grid saved.
// ---------------------------------------------------------------------------

#include "game_types.h"
#include "insects.h"
#include "render.h"
#include "web.h"
#include "collision.h"

#define REPAIR_RECHARGE  6.0   // seconds between free repair charges
#define MAX_REPAIRS      8

/**
 * Move the spider with the arrow keys, keeping it inside the play area.
 * Takes the spider by reference so it is modified in place.
 */
static void move_spider(spider &player, double dt)
{
    double step = player.speed * dt;

    if (key_down(LEFT_KEY))  player.ent.pos.x -= step;
    if (key_down(RIGHT_KEY)) player.ent.pos.x += step;
    if (key_down(UP_KEY))    player.ent.pos.y -= step;
    if (key_down(DOWN_KEY))  player.ent.pos.y += step;

    double r = player.ent.radius;
    if (player.ent.pos.x < r)                 player.ent.pos.x = r;
    if (player.ent.pos.x > SCREEN_WIDTH - r)  player.ent.pos.x = SCREEN_WIDTH - r;
    if (player.ent.pos.y < 44 + r)            player.ent.pos.y = 44 + r;
    if (player.ent.pos.y > SCREEN_HEIGHT - r) player.ent.pos.y = SCREEN_HEIGHT - r;
}

/**
 * Trickle repair charges back over time so a damaged web stays recoverable.
 */
static void recharge_repairs(game_data &game, double dt)
{
    if (game.repairs >= MAX_REPAIRS) return;

    game.repair_timer += dt;
    if (game.repair_timer >= REPAIR_RECHARGE)
    {
        game.repairs++;
        game.repair_timer = 0.0;
    }
}

/**
 * Fill the level table. Difficulty is data read by the spawn routine rather
 * than branches written into it, so tuning the curve means editing values.
 */
static void load_levels(game_data &game)
{
    //                     target  spawn   cap  speed
    game.levels[0] = level{   250,   0.70,  16,  0.85 };
    game.levels[1] = level{   600,   0.58,  22,  1.00 };
    game.levels[2] = level{  1100,   0.48,  28,  1.15 };
    game.levels[3] = level{  1800,   0.40,  34,  1.30 };
    game.levels[4] = level{  2800,   0.33,  42,  1.45 };
}

/** Reset everything needed to begin a run. */
static void start_new_game(game_data &game)
{
    build_web(game.web);

    game.player.ent.pos     = point_at(SCREEN_WIDTH / 2.0, SCREEN_HEIGHT / 2.0);
    game.player.ent.radius  = 20.0;
    game.player.ent.enabled = true;
    game.player.speed       = 260.0;

    game.insect_count      = 0;
    game.score             = 0;
    game.lives             = 3;
    game.repairs           = 4;
    game.repair_timer      = 0.0;
    game.spawn_timer       = 0.0;
    game.elapsed           = 0.0;
    game.level_index       = 0;
    game.comparisons       = 0;
    game.naive_comparisons = 0;
}

int main()
{
    open_window("Web Weaver", SCREEN_WIDTH, SCREEN_HEIGHT);

    game_data game;
    load_levels(game);
    game.show_debug = true;
    game.show_grid  = false;
    start_new_game(game);

    create_timer("frame");
    start_timer("frame");
    unsigned int last_tick = timer_ticks("frame");

    while (not quit_requested())
    {
        process_events();

        // Seconds since the previous frame, clamped so a long stall cannot
        // teleport entities across the screen.
        unsigned int now = timer_ticks("frame");
        double dt = (now - last_tick) / 1000.0;
        last_tick = now;
        if (dt > 0.05) dt = 0.05;

        if (key_typed(ESCAPE_KEY)) break;

        game.elapsed += dt;

        move_spider(game.player, dt);
        recharge_repairs(game, dt);

        if (key_typed(R_KEY)) repair_nearest(game);
        if (key_typed(D_KEY)) game.show_debug = not game.show_debug;
        if (key_typed(G_KEY)) game.show_grid  = not game.show_grid;

        game.spawn_timer -= dt;
        if (game.spawn_timer <= 0.0)
        {
            spawn_insect(game);
            game.spawn_timer = game.levels[game.level_index].spawn_interval;
        }

        update_insects(game, dt);
        update_web(game, dt);
        broad_phase(game);

        if (game.lives <= 0) start_new_game(game);

        draw_playing(game);
        refresh_screen(60);
    }

    close_all_windows();
    return 0;
}