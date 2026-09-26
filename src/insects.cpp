#include "insects.h"

/** @returns the points awarded for catching this kind of insect */
int insect_value(insect_kind kind)
{
    switch (kind)
    {
        case FLY:       return 10;
        case BUTTERFLY: return 25;
        case WASP:      return 40;
        case GOLDEN:    return 100;
    }
    return 0;
}

/** @returns the drawn and collidable radius for this kind of insect */
double insect_radius(insect_kind kind)
{
    switch (kind)
    {
        case FLY:       return 9.0;
        case BUTTERFLY: return 12.0;
        case WASP:      return 12.0;
        case GOLDEN:    return 10.0;
    }
    return 9.0;
}

/** @returns the speed ceiling for this kind of insect */
double insect_max_speed(insect_kind kind)
{
    switch (kind)
    {
        case FLY:       return 105.0;
        case BUTTERFLY: return 85.0;
        case WASP:      return 130.0;
        case GOLDEN:    return 190.0;
    }
    return 100.0;
}

/** @returns the colour used to draw this kind of insect */
color insect_colour(insect_kind kind)
{
    switch (kind)
    {
        case FLY:       return rgba_color(95, 143, 74, 255);
        case BUTTERFLY: return rgba_color(237, 147, 177, 255);
        case WASP:      return rgba_color(239, 159, 39, 255);
        case GOLDEN:    return rgba_color(242, 193, 74, 255);
    }
    return color_white();
}

/**
 * Add an insect just outside a random screen edge, aimed roughly inward.
 * Kind is weighted so flies are common and golden flies rare.
 */
void spawn_insect(game_data &game)
{
    if (game.insect_count >= MAX_INSECTS) return;
    if (game.insect_count >= INSECT_CAP)  return;

    insect bug;

    int roll = rnd(100);
    if      (roll < 45) bug.kind = FLY;
    else if (roll < 70) bug.kind = BUTTERFLY;
    else if (roll < 92) bug.kind = WASP;
    else                bug.kind = GOLDEN;

    int edge = rnd(4);
    double x = 0, y = 0;
    switch (edge)
    {
        case 0: x = rnd(SCREEN_WIDTH); y = -20;                 break;
        case 1: x = rnd(SCREEN_WIDTH); y = SCREEN_HEIGHT + 20;  break;
        case 2: x = -20;               y = rnd(SCREEN_HEIGHT);  break;
        case 3: x = SCREEN_WIDTH + 20; y = rnd(SCREEN_HEIGHT);  break;
    }

    bug.ent.pos     = point_at(x, y);
    bug.ent.radius  = insect_radius(bug.kind);
    bug.ent.clr     = insect_colour(bug.kind);
    bug.ent.enabled = true;
    bug.life_timer  = 9.0 + rnd(6);

    // Aim at the middle of the screen at this kind's speed.
    point_2d centre = point_at(SCREEN_WIDTH / 2.0, SCREEN_HEIGHT / 2.0);
    vector_2d toward = vector_point_to_point(bug.ent.pos, centre);
    bug.velocity = vector_multiply(unit_vector(toward), insect_max_speed(bug.kind) * 0.6);

    game.insects[game.insect_count] = bug;
    game.insect_count++;
}

/**
 * Remove an insect by copying the last active element over it and
 * decrementing the count. Order in the array carries no meaning, so this
 * avoids shuffling every later element down.
 */
void remove_insect(game_data &game, int index)
{
    if (index < 0 || index >= game.insect_count) return;
    game.insects[index] = game.insects[game.insect_count - 1];
    game.insect_count--;
}

/**
 * Advance every insect and expire the ones whose time is up.
 */
void update_insects(game_data &game, double dt)
{
    for (int i = 0; i < game.insect_count; i++)
    {
        insect &bug = game.insects[i];

        bug.ent.pos.x += bug.velocity.x * dt;
        bug.ent.pos.y += bug.velocity.y * dt;

        bug.life_timer -= dt;
        if (bug.life_timer <= 0.0)
        {
            remove_insect(game, i);
            // The element that was last now sits at i and has not been
            // processed, so step back to catch it on the next iteration.
            i--;
        }
    }
}