#include "render.h"

#define BACKGROUND    rgba_color( 28,  43,  31, 255)
#define HUD_BAR       rgba_color( 19,  29,  22, 255)
#define TEXT_BRIGHT   rgba_color(232, 239, 228, 255)
#define TEXT_DIM      rgba_color(159, 181, 153, 255)
#define SPIDER_BODY   rgba_color( 43,  28,  21, 255)

/** Draw every insect, with a dark bar marking the dangerous ones. */
void draw_insects(const game_data &game)
{
    for (int i = 0; i < game.insect_count; i++)
    {
        const insect &bug = game.insects[i];
        fill_circle(bug.ent.clr, bug.ent.pos.x, bug.ent.pos.y, bug.ent.radius);

        if (bug.kind == WASP)
        {
            fill_rectangle(SPIDER_BODY,
                           bug.ent.pos.x - 2, bug.ent.pos.y - bug.ent.radius,
                           4, bug.ent.radius * 2);
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
    fill_rectangle(HUD_BAR, 0, 0, SCREEN_WIDTH, 44);
    draw_text("Score " + to_string(game.score),          TEXT_BRIGHT, 16,  16);
    draw_text("Lives " + to_string(game.lives),          TEXT_DIM,   140, 16);
    draw_text("Insects " + to_string(game.insect_count), TEXT_DIM,   250, 16);
    draw_text("Arrows move   ESC quit",                  TEXT_DIM,   380, 16);
}

/** Draw the whole play screen. */
void draw_playing(const game_data &game)
{
    clear_screen(BACKGROUND);
    draw_insects(game);
    draw_spider(game);
    draw_hud(game);
}