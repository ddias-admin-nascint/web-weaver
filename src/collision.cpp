#include "collision.h"
#include "insects.h"
#include "web.h"
#include <cmath>

#define CELL_WIDTH  (SCREEN_WIDTH  / (double)GRID_COLS)
#define CELL_HEIGHT (SCREEN_HEIGHT / (double)GRID_ROWS)
#define STICK_TIME  2.0

/** Build a circular collider. */
collider circle_collider(const point_2d &centre, double radius)
{
    collider c;
    c.kind   = SHAPE_CIRCLE;
    c.a      = centre;
    c.b      = centre;
    c.radius = radius;
    return c;
}

/** Build a line-segment collider between two points. */
collider segment_collider(const point_2d &a, const point_2d &b)
{
    collider c;
    c.kind   = SHAPE_SEGMENT;
    c.a      = a;
    c.b      = b;
    c.radius = 0.0;
    return c;
}

/**
 * Test two colliders for overlap. One routine resolves every pairing by
 * reading the shape kind, so adding a shape means extending this function
 * rather than adding a parallel collision path elsewhere.
 */
bool overlaps(const collider &a, const collider &b)
{
    if (a.kind == SHAPE_CIRCLE and b.kind == SHAPE_CIRCLE)
    {
        return point_point_distance(a.a, b.a) <= a.radius + b.radius;
    }

    // Circle against segment, in whichever order the pair arrived.
    const collider &circle  = (a.kind == SHAPE_CIRCLE) ? a : b;
    const collider &segment = (a.kind == SHAPE_CIRCLE) ? b : a;

    if (circle.kind == segment.kind) return false;   // segment against segment

    return point_line_distance(circle.a, line_from(segment.a, segment.b)) <= circle.radius;
}

/**
 * Work out which grid cell a point falls in, clamped to the grid bounds so
 * that entities just off screen still land in a valid cell.
 */
void cell_for_point(const point_2d &p, int &col, int &row)
{
    col = (int)(p.x / CELL_WIDTH);
    row = (int)(p.y / CELL_HEIGHT);

    if (col < 0) col = 0;
    if (row < 0) row = 0;
    if (col >= GRID_COLS) col = GRID_COLS - 1;
    if (row >= GRID_ROWS) row = GRID_ROWS - 1;
}

/** Add an insect index to a cell, ignoring it if the cell is full. */
static void add_insect_to_cell(grid_cell &cell, int index)
{
    if (cell.insect_count < MAX_PER_CELL)
    {
        cell.insects[cell.insect_count] = index;
        cell.insect_count++;
    }
}

/** Add a strand index to a cell unless it is already listed there. */
static void add_strand_to_cell(grid_cell &cell, int index)
{
    for (int i = 0; i < cell.strand_count; i++)
    {
        if (cell.strands[i] == index) return;
    }
    if (cell.strand_count < MAX_PER_CELL)
    {
        cell.strands[cell.strand_count] = index;
        cell.strand_count++;
    }
}

/**
 * Sort every insect and strand into the grid.
 *
 * Insects occupy a single cell. Strands are line segments that can cross
 * several, so each strand is walked in short steps and registered in every
 * cell it passes through.
 */
void build_grid(game_data &game)
{
    for (int c = 0; c < GRID_COLS; c++)
    {
        for (int r = 0; r < GRID_ROWS; r++)
        {
            game.grid[c][r].insect_count = 0;
            game.grid[c][r].strand_count = 0;
        }
    }

    for (int i = 0; i < game.insect_count; i++)
    {
        int col, row;
        cell_for_point(game.insects[i].ent.pos, col, row);
        add_insect_to_cell(game.grid[col][row], i);
    }

    for (int s = 0; s < game.web.strand_count; s++)
    {
        if (not strand_is_live(game.web, s)) continue;

        point_2d a = game.web.nodes[game.web.strands[s].node_a].pos;
        point_2d b = game.web.nodes[game.web.strands[s].node_b].pos;

        double span  = point_point_distance(a, b);
        int    steps = (int)(span / (CELL_WIDTH / 2.0)) + 1;

        for (int step = 0; step <= steps; step++)
        {
            double t = (double)step / steps;
            point_2d along = point_at(a.x + (b.x - a.x) * t,
                                      a.y + (b.y - a.y) * t);
            int col, row;
            cell_for_point(along, col, row);
            add_strand_to_cell(game.grid[col][row], s);
        }
    }
}

/**
 * Run collision for the frame.
 *
 * Testing every insect against every strand grows with the product of the
 * two counts. The grid exists to avoid those tests: each insect is only
 * tested against the strands registered in its own cell and the eight
 * around it. Two counters record the tests performed and the number an
 * all-pairs approach would have required, so the reduction is measured
 * rather than claimed.
 */
void broad_phase(game_data &game)
{
    game.comparisons = 0;

    // The all-pairs figure must count the same strands the grid can return,
    // otherwise the comparison flatters the grid once parts of the web go
    // slack. Only live strands are ever tested, so only those are counted.
    int live_strands = 0;
    for (int s = 0; s < game.web.strand_count; s++)
    {
        if (strand_is_live(game.web, s)) live_strands++;
    }
    game.naive_comparisons = game.insect_count * live_strands + game.insect_count;

    build_grid(game);

    collider spider_col = circle_collider(game.player.ent.pos, game.player.ent.radius);

    // --- Spider against the insects in its own cell and the eight around it.
    int scol, srow;
    cell_for_point(game.player.ent.pos, scol, srow);

    for (int dc = -1; dc <= 1; dc++)
    {
        for (int dr = -1; dr <= 1; dr++)
        {
            int col = scol + dc;
            int row = srow + dr;
            if (col < 0 or row < 0 or col >= GRID_COLS or row >= GRID_ROWS) continue;

            grid_cell &cell = game.grid[col][row];
            for (int k = 0; k < cell.insect_count; k++)
            {
                int index = cell.insects[k];
                if (index >= game.insect_count) continue;

                insect &bug = game.insects[index];
                collider bug_col = circle_collider(bug.ent.pos, bug.ent.radius);

                game.comparisons++;
                if (not overlaps(spider_col, bug_col)) continue;

                if (bug.kind == WASP and not bug.stuck)
                {
                    game.lives--;
                    remove_insect(game, index);
                    return;
                }

                game.score += insect_value(bug.kind);
                remove_insect(game, index);
            }
        }
    }

    // --- Insects against the strands registered near them.
    for (int i = 0; i < game.insect_count; i++)
    {
        insect &bug = game.insects[i];
        if (bug.stuck) continue;

        int col, row;
        cell_for_point(bug.ent.pos, col, row);
        collider bug_col = circle_collider(bug.ent.pos, bug.ent.radius);

        grid_cell &cell = game.grid[col][row];
        for (int k = 0; k < cell.strand_count; k++)
        {
            int s = cell.strands[k];
            if (not strand_is_live(game.web, s)) continue;

            collider strand_col = segment_collider(
                game.web.nodes[game.web.strands[s].node_a].pos,
                game.web.nodes[game.web.strands[s].node_b].pos);

            game.comparisons++;
            if (not overlaps(bug_col, strand_col)) continue;

            if (bug.kind == WASP)
            {
                // Wasps cut through rather than sticking.
                game.web.strands[s].tension += 30.0;
            }
            else
            {
                bug.stuck        = true;
                bug.stuck_timer  = STICK_TIME;
                bug.stuck_strand = s;
                bug.velocity     = vector_to(0, 0);
            }
            break;
        }
    }
}