// ---------------------------------------------------------------------------
// Web Weaver - SIT102 Custom Project
//
// An arcade game in which a spider defends a web built as a graph of anchor
// nodes joined by strands. Insects that hit a strand are held there and load
// it; overloaded strands snap, and a breadth-first search decides which parts
// of the web are still connected to an anchor. Severed sections go slack and
// catch nothing until they are repaired.
// ---------------------------------------------------------------------------

#include "game_types.h"
#include "web.h"
#include "insects.h"
#include "collision.h"
#include "render.h"

#define REPAIR_RECHARGE  6.0    // seconds between free repair charges
#define MAX_REPAIRS      8

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

/**
 * Reset everything needed to begin a run at level one.
 */
static void start_new_game(game_data &game)
{
    build_web(game.web);

    game.player.ent.pos     = point_at(SCREEN_WIDTH / 2.0, SCREEN_HEIGHT / 2.0);
    game.player.ent.radius  = 20.0;
    game.player.ent.enabled = true;
    game.player.speed       = 260.0;

    game.insect_count      = 0;
    game.level_index       = 0;
    game.score             = 0;
    game.lives             = 3;
    game.repairs           = 4;
    game.repair_timer      = 0.0;
    game.spawn_timer       = 0.0;
    game.elapsed           = 0.0;
    game.comparisons       = 0;
    game.naive_comparisons = 0;
    game.entry_name        = "";
    game.screen            = PLAYING;
}

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
    if (player.ent.pos.x < r)                    player.ent.pos.x = r;
    if (player.ent.pos.x > SCREEN_WIDTH - r)     player.ent.pos.x = SCREEN_WIDTH - r;
    if (player.ent.pos.y < 44 + r)               player.ent.pos.y = 44 + r;
    if (player.ent.pos.y > SCREEN_HEIGHT - r)    player.ent.pos.y = SCREEN_HEIGHT - r;
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
 * Insert a score into the ranked table, shifting lower entries down and
 * discarding the last when the table is full.
 */
static void record_score(game_data &game, const string &name, int value)
{
    int slot = game.score_count;
    for (int i = 0; i < game.score_count; i++)
    {
        if (value > game.scores[i].value)
        {
            slot = i;
            break;
        }
    }

    if (slot >= MAX_SCORES) return;

    int last = game.score_count;
    if (last >= MAX_SCORES) last = MAX_SCORES - 1;

    // Walk backwards so each entry is copied before it is overwritten.
    for (int i = last; i > slot; i--)
    {
        game.scores[i] = game.scores[i - 1];
    }

    game.scores[slot].name  = name;
    game.scores[slot].value = value;

    if (game.score_count < MAX_SCORES) game.score_count++;
}

/**
 * Advance one frame of play: input, spawning, movement, web load and
 * collision, then the level target check.
 */
static void update_playing(game_data &game, double dt)
{
    game.elapsed += dt;

    move_spider(game.player, dt);
    recharge_repairs(game, dt);

    if (key_typed(R_KEY)) repair_nearest(game);
    if (key_typed(D_KEY)) game.show_debug = not game.show_debug;
    if (key_typed(G_KEY)) game.show_grid  = not game.show_grid;
    if (key_typed(P_KEY)) { game.screen = PAUSED; return; }

    game.spawn_timer -= dt;
    if (game.spawn_timer <= 0.0)
    {
        spawn_insect(game);
        game.spawn_timer = game.levels[game.level_index].spawn_interval;
    }

    update_insects(game, dt);
    update_web(game, dt);
    broad_phase(game);

    if (game.screen != PLAYING) return;   // broad_phase may have ended the run

    if (game.score >= game.levels[game.level_index].target_score)
    {
        if (game.level_index < LEVEL_COUNT - 1)
        {
            game.level_index++;
            game.repairs += 2;
            game.screen   = LEVEL_COMPLETE;
        }
        else
        {
            game.screen = GAME_OVER;
        }
    }
}

/**
 * Collect up to three initials at game over, then file the score.
 */
static void update_game_over(game_data &game)
{
    for (int k = A_KEY; k <= Z_KEY; k++)
    {
        if (key_typed((key_code)k) and (int)game.entry_name.length() < NAME_LENGTH)
        {
            game.entry_name += (char)('A' + (k - A_KEY));
        }
    }

    if (key_typed(BACKSPACE_KEY) and game.entry_name.length() > 0)
    {
        game.entry_name = game.entry_name.substr(0, game.entry_name.length() - 1);
    }

    if (key_typed(RETURN_KEY))
    {
        string name = game.entry_name;
        if (name.length() == 0) name = "---";
        record_score(game, name, game.score);
        game.screen = SCORES;
    }
}

int main()
{
    open_window("Web Weaver", SCREEN_WIDTH, SCREEN_HEIGHT);

    game_data game;
    load_levels(game);
    game.score_count = 0;
    game.show_debug  = true;
    game.show_grid   = false;
    start_new_game(game);
    game.screen = MENU;

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

        // The screen state decides which update and draw pair runs.
        switch (game.screen)
        {
            case MENU:
                if (key_typed(RETURN_KEY)) start_new_game(game);
                if (key_typed(S_KEY))      game.screen = SCORES;
                draw_menu(game);
                break;

            case PLAYING:
                update_playing(game, dt);
                draw_playing(game);
                break;

            case PAUSED:
                if (key_typed(P_KEY)) game.screen = PLAYING;
                draw_paused(game);
                break;

            case LEVEL_COMPLETE:
                if (key_typed(RETURN_KEY)) game.screen = PLAYING;
                draw_level_complete(game);
                break;

            case GAME_OVER:
                update_game_over(game);
                draw_game_over(game);
                break;

            case SCORES:
                if (key_typed(RETURN_KEY)) game.screen = MENU;
                draw_scores(game);
                break;
        }

        refresh_screen(60);
    }

    close_all_windows();
    return 0;
}