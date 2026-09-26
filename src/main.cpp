// ---------------------------------------------------------------------------
// Web Weaver - SIT102 Custom Project
// Stage 2: the web is now a graph of nodes and strands that loads,
// breaks and is repaired. A breadth-first search decides which parts are
// still attached to an anchor.
// ---------------------------------------------------------------------------

#include "game_types.h"
#include "insects.h"
#include "render.h"
#include "web.h"

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
 * Test the spider against every insect in the array.
 *
 * This is the preliminary approach: the cost grows with the number of insects,
 * and every pair is tested whether or not the two are anywhere near each
 * other. 
 */
static void check_catches(game_data &game)
{
    for (int i = 0; i < game.insect_count; i++)
    {
        insect &bug = game.insects[i];

        double gap = point_point_distance(game.player.ent.pos, bug.ent.pos);
        if (gap > game.player.ent.radius + bug.ent.radius) continue;

        if (bug.kind == WASP)
        {
            game.lives--;
            remove_insect(game, i);
            i--;
            continue;
        }

        game.score += insect_value(bug.kind);
        remove_insect(game, i);
        i--;
    }
}

/**
 * Test every insect against every live strand.
 *
 * This is the naive approach: the cost is the insect count multiplied by
 * the strand count, and every pair is tested whether or not the two are
 * anywhere near each other.
 */
static void check_strand_hits(game_data &game)
{
    for (int i = 0; i < game.insect_count; i++)
    {
        insect &bug = game.insects[i];
        if (bug.stuck) continue;

        for (int s = 0; s < game.web.strand_count; s++)
        {
            if (not strand_is_live(game.web, s)) continue;

            line strand = strand_line(game.web, s);
            if (point_line_distance(bug.ent.pos, strand) > bug.ent.radius) continue;

            if (bug.kind == WASP)
            {
                // Wasps cut through rather than sticking.
                game.web.strands[s].tension += 30.0;
            }
            else
            {
                bug.stuck        = true;
                bug.stuck_timer  = 2.0;
                bug.stuck_strand = s;
                bug.velocity     = vector_to(0, 0);
            }
            break;
        }
    }
}

/** Reset everything needed to begin a run. */
static void start_new_game(game_data &game)
{
    build_web(game.web);

    game.player.ent.pos     = point_at(SCREEN_WIDTH / 2.0, SCREEN_HEIGHT / 2.0);
    game.player.ent.radius  = 20.0;
    game.player.ent.enabled = true;
    game.player.speed       = 260.0;

    game.insect_count = 0;
    game.score        = 0;
    game.lives        = 3;
    game.repairs      = 4;
    game.spawn_timer  = 0.0;
    game.elapsed      = 0.0;
}

int main()
{
    open_window("Web Weaver", SCREEN_WIDTH, SCREEN_HEIGHT);

    game_data game;
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

        if (key_typed(R_KEY)) repair_nearest(game);

        game.spawn_timer -= dt;
        if (game.spawn_timer <= 0.0)
        {
            spawn_insect(game);
            game.spawn_timer = 1.1;
        }

        update_insects(game, dt);
        update_web(game, dt);
        check_strand_hits(game);
        check_catches(game);

        if (game.lives <= 0) start_new_game(game);

        draw_playing(game);
        refresh_screen(60);
    }

    close_all_windows();
    return 0;
}