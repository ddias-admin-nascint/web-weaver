#include "render.h"
#include "web.h"
#include "insects.h"
#include "collision.h"

#define BACKGROUND    rgba_color( 28,  43,  31, 255)
#define HUD_BAR       rgba_color( 19,  29,  22, 255)
#define TEXT_BRIGHT   rgba_color(232, 239, 228, 255)
#define TEXT_DIM      rgba_color(159, 181, 153, 255)
#define STRAND_OK     rgba_color(107, 127,  94, 255)
#define STRAND_WARM   rgba_color(239, 159,  39, 255)
#define STRAND_HOT    rgba_color(226,  75,  74, 255)
#define STRAND_SLACK  rgba_color( 90,  95,  86, 255)
#define NODE_ANCHOR   rgba_color(143, 168, 138, 255)
#define NODE_LIVE     rgba_color( 95, 115,  88, 255)
#define NODE_DEAD     rgba_color( 74,  79,  70, 255)
#define SPIDER_BODY   rgba_color( 43,  28,  21, 255)
#define DEBUG_TEXT    rgba_color(127, 212, 168, 255)

// The default SplashKit font is a fixed-width bitmap font, eight pixels per
// character. There is no text_width for it, so centring is calculated from
// the string length instead.
#define CHAR_WIDTH    8.0

/**
 * Draw text centred horizontally on the screen.
 * @param text the string to draw
 * @param clr its colour
 * @param y the top of the text
 */
static void draw_centred(const string &text, const color &clr, double y)
{
    draw_text(text, clr, SCREEN_WIDTH / 2.0 - (text.length() * CHAR_WIDTH) / 2.0, y);
}

/**
 * Draw a heading centred on the screen.
 */
static void draw_heading(const string &text, const color &clr, double y)
{
    draw_text(text, clr, SCREEN_WIDTH / 2.0 - (text.length() * CHAR_WIDTH) / 2.0, y);
}

/**
 * Pick a strand's colour from its state: slack strands are greyed out, and
 * live strands shift from green through amber to red as tension rises, so
 * the player can see which parts of the web are about to fail.
 */
static color strand_colour(const web_graph &web, int strand_id)
{
    if (not strand_is_live(web, strand_id)) return STRAND_SLACK;

    double load = web.strands[strand_id].tension / web.strands[strand_id].integrity;
    if (load > 0.66) return STRAND_HOT;
    if (load > 0.33) return STRAND_WARM;
    return STRAND_OK;
}

/** Draw every active strand and node. */
void draw_web(const game_data &game)
{
    for (int i = 0; i < game.web.strand_count; i++)
    {
        if (not game.web.strands[i].active) continue;

        point_2d a = game.web.nodes[game.web.strands[i].node_a].pos;
        point_2d b = game.web.nodes[game.web.strands[i].node_b].pos;
        draw_line(strand_colour(game.web, i), a.x, a.y, b.x, b.y);
    }

    for (int i = 0; i < game.web.node_count; i++)
    {
        color node_clr = NODE_DEAD;
        if (game.web.nodes[i].reachable)
        {
            node_clr = game.web.nodes[i].is_anchor ? NODE_ANCHOR : NODE_LIVE;
        }
        double r = game.web.nodes[i].is_anchor ? 6.0 : 4.0;
        fill_circle(node_clr, game.web.nodes[i].pos.x, game.web.nodes[i].pos.y, r);
    }
}

/** Draw every insect, ringing the ones currently held on a strand. */
void draw_insects(const game_data &game)
{
    for (int i = 0; i < game.insect_count; i++)
    {
        const insect &bug = game.insects[i];
        fill_circle(bug.ent.clr, bug.ent.pos.x, bug.ent.pos.y, bug.ent.radius);

        if (bug.kind == WASP)
        {
            // A dark bar makes the dangerous insect readable at a glance.
            fill_rectangle(SPIDER_BODY,
                           bug.ent.pos.x - 2, bug.ent.pos.y - bug.ent.radius,
                           4, bug.ent.radius * 2);
        }

        if (bug.stuck)
        {
            draw_circle(TEXT_BRIGHT, bug.ent.pos.x, bug.ent.pos.y, bug.ent.radius + 5);
        }
    }
}

/** Draw the spider as a body with legs. */
void draw_spider(const game_data &game)
{
    double x = game.player.ent.pos.x;
    double y = game.player.ent.pos.y;
    double r = game.player.ent.radius;

    for (int i = 0; i < 3; i++)
    {
        double offset = (i - 1) * 8.0;
        draw_line(SPIDER_BODY, x - r * 0.5, y + offset, x - r * 1.7, y + offset - 6);
        draw_line(SPIDER_BODY, x + r * 0.5, y + offset, x + r * 1.7, y + offset - 6);
    }

    fill_circle(SPIDER_BODY, x, y, r);
    fill_circle(rgba_color(59, 40, 32, 255), x, y - r * 0.45, r * 0.55);
}

/** Draw the status bar across the top of the play area. */
void draw_hud(const game_data &game)
{
    const level &lv = game.levels[game.level_index];

    fill_rectangle(HUD_BAR, 0, 0, SCREEN_WIDTH, 44);
    draw_text("Score " + to_string(game.score), TEXT_BRIGHT, 16, 16);
    draw_text("Target " + to_string(lv.target_score), TEXT_DIM, 140, 16);
    draw_text("Level " + to_string(game.level_index + 1), TEXT_DIM, 270, 16);
    draw_text("Lives " + to_string(game.lives), TEXT_DIM, 370, 16);
    draw_text("Repairs " + to_string(game.repairs), TEXT_DIM, 470, 16);
    draw_text("R repair  D debug  G grid  P pause", TEXT_DIM, 600, 16);
}

/**
 * Draw the diagnostic counters, and optionally the grid itself.
 *
 * The two comparison figures are the evidence for the broad phase: the
 * first is what the frame actually cost, the second is what testing every
 * insect against every live strand would have cost.
 */
void draw_debug(const game_data &game)
{
    fill_rectangle(rgba_color(13, 22, 16, 220), 12, SCREEN_HEIGHT - 86, 330, 74);

    draw_text("DEBUG", DEBUG_TEXT, 24, SCREEN_HEIGHT - 78);
    draw_text("overlap tests this frame : " + to_string(game.comparisons),
              DEBUG_TEXT, 24, SCREEN_HEIGHT - 60);
    draw_text("all-pairs would have cost : " + to_string(game.naive_comparisons),
              DEBUG_TEXT, 24, SCREEN_HEIGHT - 42);
    draw_text("severed nodes : " + to_string(severed_node_count(game.web))
              + "   strands : " + to_string(active_strand_count(game.web)),
              DEBUG_TEXT, 24, SCREEN_HEIGHT - 24);

    if (not game.show_grid) return;

    color grid_clr = rgba_color(127, 212, 168, 45);
    for (int c = 1; c < GRID_COLS; c++)
    {
        double x = c * (SCREEN_WIDTH / (double)GRID_COLS);
        draw_line(grid_clr, x, 44, x, SCREEN_HEIGHT);
    }
    for (int r = 1; r < GRID_ROWS; r++)
    {
        double y = r * (SCREEN_HEIGHT / (double)GRID_ROWS);
        if (y < 44) continue;
        draw_line(grid_clr, 0, y, SCREEN_WIDTH, y);
    }
}

/** Draw the title screen. */
void draw_menu(const game_data &game)
{
    clear_screen(BACKGROUND);
    draw_web(game);
    draw_heading("WEB WEAVER", TEXT_BRIGHT, 180);
    draw_centred("Catch insects. Keep the web attached to its anchors.", TEXT_DIM, 220);
    draw_centred("ENTER to start      S for high scores      ESC to quit", TEXT_DIM, 250);
}

/** Draw the play screen. */
void draw_playing(const game_data &game)
{
    clear_screen(BACKGROUND);
    draw_web(game);
    draw_insects(game);
    draw_spider(game);
    draw_hud(game);
    if (game.show_debug) draw_debug(game);
}

/** Draw the play screen dimmed, with a pause notice. */
void draw_paused(const game_data &game)
{
    draw_playing(game);
    fill_rectangle(rgba_color(0, 0, 0, 150), 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    draw_heading("PAUSED", TEXT_BRIGHT, SCREEN_HEIGHT / 2);
    draw_centred("P to resume", TEXT_DIM, SCREEN_HEIGHT / 2 + 26);
}

/** Draw the between-levels screen. */
void draw_level_complete(const game_data &game)
{
    clear_screen(BACKGROUND);
    draw_web(game);
    draw_heading("LEVEL " + to_string(game.level_index + 1) + " COMPLETE", TEXT_BRIGHT, 250);
    draw_centred("Score " + to_string(game.score), TEXT_DIM, 285);
    draw_centred("ENTER to continue", TEXT_DIM, 315);
}

/** Draw the game over screen, including initials entry when the score qualifies. */
void draw_game_over(const game_data &game)
{
    clear_screen(BACKGROUND);
    draw_web(game);
    draw_heading("GAME OVER", TEXT_BRIGHT, 240);
    draw_centred("Final score " + to_string(game.score), TEXT_DIM, 275);
    draw_heading("Initials: " + game.entry_name + "_", TEXT_BRIGHT, 315);
    draw_centred("Type three letters, then ENTER", TEXT_DIM, 345);
}

/** Draw the ranked high score table. */
void draw_scores(const game_data &game)
{
    clear_screen(BACKGROUND);
    draw_heading("HIGH SCORES", TEXT_BRIGHT, 160);

    if (game.score_count == 0)
    {
        draw_centred("No scores yet.", TEXT_DIM, 210);
    }

    for (int i = 0; i < game.score_count; i++)
    {
        draw_text(to_string(i + 1) + ".", TEXT_DIM, SCREEN_WIDTH / 2 - 110, 210 + i * 26);
        draw_text(game.scores[i].name, TEXT_BRIGHT, SCREEN_WIDTH / 2 - 70, 210 + i * 26);
        draw_text(to_string(game.scores[i].value), TEXT_BRIGHT,
                  SCREEN_WIDTH / 2 + 30, 210 + i * 26);
    }

    draw_centred("ENTER to return", TEXT_DIM, 210 + MAX_SCORES * 26 + 30);
}