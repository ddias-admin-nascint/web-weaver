#include "insects.h"
#include "web.h"
#include <cmath>

#define MAX_STEER_FORCE 220.0

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

/** @returns the speed ceiling applied after the steering force is added */
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

/** @returns a readable name for this kind, used by the debug overlay */
string insect_name(insect_kind kind)
{
    switch (kind)
    {
        case FLY:       return "Fly";
        case BUTTERFLY: return "Butterfly";
        case WASP:      return "Wasp";
        case GOLDEN:    return "Golden";
    }
    return "Unknown";
}

/**
 * A steering force that pulls towards a point.
 * @param from where the insect is now
 * @param target where it wants to be
 * @param strength the magnitude of the returned force
 */
static vector_2d seek(const point_2d &from, const point_2d &target, double strength)
{
    vector_2d desired = vector_point_to_point(from, target);
    if (vector_magnitude(desired) < 0.0001) return vector_to(0, 0);
    return vector_multiply(unit_vector(desired), strength);
}

/**
 * A steering force that pushes directly away from a point.
 */
static vector_2d flee(const point_2d &from, const point_2d &threat, double strength)
{
    return vector_multiply(seek(from, threat, strength), -1.0);
}

/**
 * A steering force that drifts, turning by a small random amount each frame.
 * The angle is stored on the insect so the drift is continuous rather than
 * re-randomised every frame, which would just produce jitter.
 */
static vector_2d wander(insect &bug, double strength, double turn_rate)
{
    bug.wander_angle += (rnd() - 0.5) * turn_rate;
    return vector_from_angle(bug.wander_angle, strength);
}

/**
 * Blend the steering behaviours for this insect's kind into a single force.
 * The switch decides which behaviours contribute and with what weighting, so
 * a movement pattern is a combination of forces rather than a special case.
 */
void steer_insect(insect &bug, const game_data &game)
{
    bug.force = vector_to(0, 0);

    switch (bug.kind)
    {
        case FLY:
            // Drifts aimlessly, with a mild pull away from the spider.
            bug.force = vector_add(bug.force, wander(bug, 150.0, 55.0));
            bug.force = vector_add(bug.force, flee(bug.ent.pos, game.player.ent.pos, 45.0));
            break;

        case BUTTERFLY:
        {
            // Wanders slowly, with a sideways oscillation that produces the
            // characteristic fluttering path.
            bug.force = vector_add(bug.force, wander(bug, 90.0, 25.0));

            // vector_normal is undefined for a zero-length vector, which a
            // butterfly has on the frame it is released from a strand.
            if (vector_magnitude(bug.velocity) > 0.0001)
            {
                vector_2d side = vector_normal(bug.velocity);
                double sway = sin(game.elapsed * 6.0 + bug.wander_angle) * 130.0;
                bug.force = vector_add(bug.force, vector_multiply(side, sway));
            }
            break;
        }

        case WASP:
            // Hunts the spider directly.
            bug.force = vector_add(bug.force, seek(bug.ent.pos, game.player.ent.pos, 190.0));
            bug.force = vector_add(bug.force, wander(bug, 40.0, 30.0));
            break;

        case GOLDEN:
            // Fast, erratic, and actively avoids the spider.
            bug.force = vector_add(bug.force, wander(bug, 200.0, 90.0));
            bug.force = vector_add(bug.force, flee(bug.ent.pos, game.player.ent.pos, 150.0));
            break;
    }

    // Clamp the force before it reaches the velocity. Without this an insect
    // blending several strong behaviours accelerates far harder than one
    // blending a single behaviour.
    bug.force = vector_limit(bug.force, MAX_STEER_FORCE);
}

/**
 * Add an insect at a random point just outside the screen edge, aimed
 * roughly inward. Kind is weighted so flies are common and golden flies rare.
 */
void spawn_insect(game_data &game)
{
    if (game.insect_count >= MAX_INSECTS) return;
    if (game.insect_count >= game.levels[game.level_index].insect_cap) return;

    insect bug;

    int roll = rnd(100);
    if      (roll < 45) bug.kind = FLY;
    else if (roll < 70) bug.kind = BUTTERFLY;
    else if (roll < 92) bug.kind = WASP;
    else                bug.kind = GOLDEN;

    // Choose an edge, then a position along it.
    int edge = rnd(4);
    double x = 0, y = 0;
    switch (edge)
    {
        case 0: x = rnd(SCREEN_WIDTH); y = -20;                 break;
        case 1: x = rnd(SCREEN_WIDTH); y = SCREEN_HEIGHT + 20;  break;
        case 2: x = -20;               y = rnd(SCREEN_HEIGHT);  break;
        case 3: x = SCREEN_WIDTH + 20; y = rnd(SCREEN_HEIGHT);  break;
    }

    bug.ent.pos      = point_at(x, y);
    bug.ent.radius   = insect_radius(bug.kind);
    bug.ent.clr      = insect_colour(bug.kind);
    bug.ent.enabled  = true;
    bug.velocity     = seek(bug.ent.pos, point_at(SCREEN_WIDTH / 2.0, SCREEN_HEIGHT / 2.0), 60.0);
    bug.force        = vector_to(0, 0);
    bug.stuck        = false;
    bug.stuck_timer  = 0.0;
    bug.stuck_strand = -1;
    bug.wander_angle = rnd(360);
    bug.life_timer   = 9.0 + rnd(6);

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
 * Find the insect closest to the spider within a given range.
 * @returns a pointer to that insect, or nullptr when nothing is in range
 */
insect *nearest_insect(game_data &game, double within)
{
    insect *found    = nullptr;
    double  best_gap = within;

    for (int i = 0; i < game.insect_count; i++)
    {
        double gap = point_point_distance(game.player.ent.pos, game.insects[i].ent.pos);
        if (gap < best_gap)
        {
            best_gap = gap;
            found    = &game.insects[i];
        }
    }
    return found;
}

/**
 * Advance every insect: steer, integrate, clamp, and expire.
 * Stuck insects do not move, but they do load the strand holding them.
 */
void update_insects(game_data &game, double dt)
{
    double speed_mult = game.levels[game.level_index].speed_mult;

    for (int i = 0; i < game.insect_count; i++)
    {
        insect &bug = game.insects[i];

        if (bug.stuck)
        {
            bug.stuck_timer -= dt;

            // A held insect keeps loading the strand it is caught on.
            if (bug.stuck_strand >= 0 and game.web.strands[bug.stuck_strand].active)
            {
                game.web.strands[bug.stuck_strand].tension += 16.0 * dt;
            }
            else
            {
                bug.stuck = false;   // the strand broke underneath it
            }

            if (bug.stuck_timer <= 0.0)
            {
                bug.stuck        = false;
                bug.stuck_strand = -1;
            }
            continue;
        }

        steer_insect(bug, game);

        // Integrate the force into the velocity, then clamp the velocity.
        // Clamping at both stages is what keeps movement smooth: limiting
        // only the force still lets speed accumulate without bound.
        bug.velocity = vector_add(bug.velocity, vector_multiply(bug.force, dt));
        bug.velocity = vector_limit(bug.velocity, insect_max_speed(bug.kind) * speed_mult);

        bug.ent.pos.x += bug.velocity.x * dt;
        bug.ent.pos.y += bug.velocity.y * dt;

        // Keep insects loosely within the play area.
        if (bug.ent.pos.x < -40)                bug.ent.pos.x = -40;
        if (bug.ent.pos.x > SCREEN_WIDTH + 40)  bug.ent.pos.x = SCREEN_WIDTH + 40;
        if (bug.ent.pos.y < -40)                bug.ent.pos.y = -40;
        if (bug.ent.pos.y > SCREEN_HEIGHT + 40) bug.ent.pos.y = SCREEN_HEIGHT + 40;

        bug.life_timer -= dt;
        if (bug.life_timer <= 0.0)
        {
            remove_insect(game, i);
            i--;   // the element now at i has not been processed yet
        }
    }
}