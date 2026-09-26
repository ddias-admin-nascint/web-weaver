#include "web.h"
#include <cmath>

/**
 * Attach a strand index to a node's edge list.
 * @param node the node to attach to
 * @param strand_id the index of the strand in the graph's strand array
 */
static void attach_edge(web_node &node, int strand_id)
{
    if (node.edge_count < MAX_EDGES_PER_NODE)
    {
        node.edges[node.edge_count] = strand_id;
        node.edge_count++;
    }
}

/**
 * Remove a strand index from a node's edge list by copying the last entry
 * over it and decrementing the count. Order within an edge list carries no
 * meaning, so compaction is safe here.
 * @param node the node to detach from
 * @param strand_id the strand index to remove
 */
static void detach_edge(web_node &node, int strand_id)
{
    for (int i = 0; i < node.edge_count; i++)
    {
        if (node.edges[i] == strand_id)
        {
            node.edges[i] = node.edges[node.edge_count - 1];
            node.edge_count--;
            return;
        }
    }
}

/**
 * Add a strand between two nodes and register it with both endpoints.
 * @returns the new strand's index, or -1 if the graph is full
 */
int add_strand(web_graph &web, int node_a, int node_b, double integrity)
{
    if (web.strand_count >= MAX_STRANDS) return -1;

    int id = web.strand_count;
    web.strands[id].node_a    = node_a;
    web.strands[id].node_b    = node_b;
    web.strands[id].tension   = 0.0;
    web.strands[id].integrity = integrity;
    web.strands[id].active    = true;
    web.strand_count++;

    attach_edge(web.nodes[node_a], id);
    attach_edge(web.nodes[node_b], id);
    return id;
}

/**
 * Deactivate a strand and detach it from both endpoint nodes. The strand
 * keeps its slot so that indices held elsewhere stay valid; only the active
 * flag and the edge lists change.
 */
void remove_strand(web_graph &web, int strand_id)
{
    if (strand_id < 0 || strand_id >= web.strand_count) return;
    if (not web.strands[strand_id].active) return;

    web.strands[strand_id].active  = false;
    web.strands[strand_id].tension = 0.0;
    detach_edge(web.nodes[web.strands[strand_id].node_a], strand_id);
    detach_edge(web.nodes[web.strands[strand_id].node_b], strand_id);
}

/**
 * Build the starting web: a radial structure of concentric rings joined by
 * spokes, with anchor nodes fixed near the screen corners.
 */
void build_web(web_graph &web)
{
    web.node_count   = 0;
    web.strand_count = 0;

    double cx = SCREEN_WIDTH / 2.0;
    // Centred between the HUD bar and the bottom edge, sized so the anchor
    // ring stays fully on screen.
    double cy = 406.0;
    double ring_radius[WEB_RINGS] = { 90.0, 160.0, 225.0 };

    // Ring and spoke nodes. Index = ring * WEB_SPOKES + spoke.
    for (int ring = 0; ring < WEB_RINGS; ring++)
    {
        for (int spoke = 0; spoke < WEB_SPOKES; spoke++)
        {
            double angle = spoke * (360.0 / WEB_SPOKES) * M_PI / 180.0;
            int id = web.node_count;
            web.nodes[id].pos        = point_at(cx + cos(angle) * ring_radius[ring],
                                                cy + sin(angle) * ring_radius[ring]);
            web.nodes[id].edge_count = 0;
            web.nodes[id].is_anchor  = false;
            web.nodes[id].reachable  = true;
            web.node_count++;
        }
    }

    // Eight anchor nodes out towards the screen edge. More anchors means a
    // single failure cannot detach the whole web.
    int first_anchor = web.node_count;
    double anchor_radius = 330.0;
    for (int i = 0; i < WEB_SPOKES; i++)
    {
        double angle = (i * (360.0 / WEB_SPOKES)) * M_PI / 180.0;
        int id = web.node_count;
        web.nodes[id].pos        = point_at(cx + cos(angle) * anchor_radius,
                                            cy + sin(angle) * anchor_radius);
        web.nodes[id].edge_count = 0;
        web.nodes[id].is_anchor  = true;
        web.nodes[id].reachable  = true;
        web.node_count++;
    }

    // Circumferential strands around each ring.
    for (int ring = 0; ring < WEB_RINGS; ring++)
    {
        for (int spoke = 0; spoke < WEB_SPOKES; spoke++)
        {
            int a = ring * WEB_SPOKES + spoke;
            int b = ring * WEB_SPOKES + ((spoke + 1) % WEB_SPOKES);
            add_strand(web, a, b, 100.0 + ring * 20.0);
        }
    }

    // Radial strands joining each ring to the next.
    for (int ring = 0; ring < WEB_RINGS - 1; ring++)
    {
        for (int spoke = 0; spoke < WEB_SPOKES; spoke++)
        {
            int a = ring * WEB_SPOKES + spoke;
            int b = (ring + 1) * WEB_SPOKES + spoke;
            add_strand(web, a, b, 120.0);
        }
    }

    // Anchor strands from the outer ring to the anchors. These are the
    // structural strands, so they are the strongest in the web.
    for (int i = 0; i < WEB_SPOKES; i++)
    {
        int outer = (WEB_RINGS - 1) * WEB_SPOKES + i;
        add_strand(web, outer, first_anchor + i, 300.0);
    }

    rebuild_reachability(web);
}

/**
 * Recalculate which nodes are still connected to an anchor, using a
 * breadth-first search outward from every anchor node.
 *
 * The search has to be re-run whenever a strand is added or removed,
 * because connectivity cannot be worked out in advance once the graph
 * changes during play.
 */
void rebuild_reachability(web_graph &web)
{
    int queue[MAX_NODES];
    int head = 0;
    int tail = 0;

    // 1. Clear the reachable flag on every node.
    for (int i = 0; i < web.node_count; i++)
    {
        web.nodes[i].reachable = false;
    }

    // 2. Push every anchor node and mark it reachable.
    for (int i = 0; i < web.node_count; i++)
    {
        if (web.nodes[i].is_anchor)
        {
            web.nodes[i].reachable = true;
            queue[tail] = i;
            tail++;
        }
    }

    // 3. Pop a node, walk its edge list, and push any endpoint not yet seen.
    while (head < tail)
    {
        int current = queue[head];
        head++;

        for (int e = 0; e < web.nodes[current].edge_count; e++)
        {
            int strand_id = web.nodes[current].edges[e];
            if (not web.strands[strand_id].active) continue;

            // The node at the other end of this strand.
            int other = web.strands[strand_id].node_a;
            if (other == current) other = web.strands[strand_id].node_b;

            if (not web.nodes[other].reachable)
            {
                web.nodes[other].reachable = true;
                queue[tail] = other;
                tail++;
            }
        }
    }

    // 4. Any node still unmarked is severed from every anchor.
}

/**
 * @returns the strand as a line between its two endpoint node positions
 */
line strand_line(const web_graph &web, int strand_id)
{
    return line_from(web.nodes[web.strands[strand_id].node_a].pos,
                     web.nodes[web.strands[strand_id].node_b].pos);
}

/**
 * A strand is live when it is active and both of its endpoints are still
 * connected to an anchor. Slack strands are drawn greyed out and catch
 * nothing.
 */
bool strand_is_live(const web_graph &web, int strand_id)
{
    if (not web.strands[strand_id].active) return false;
    return web.nodes[web.strands[strand_id].node_a].reachable
        and web.nodes[web.strands[strand_id].node_b].reachable;
}

/**
 * @returns how many strands are currently active
 */
int active_strand_count(const web_graph &web)
{
    int count = 0;
    for (int i = 0; i < web.strand_count; i++)
    {
        if (web.strands[i].active) count++;
    }
    return count;
}

/**
 * @returns how many non-anchor nodes have been cut off from every anchor
 */
int severed_node_count(const web_graph &web)
{
    int count = 0;
    for (int i = 0; i < web.node_count; i++)
    {
        if (not web.nodes[i].reachable) count++;
    }
    return count;
}

/**
 * Advance strand tension. Tension bleeds away on its own, and any strand
 * pushed past its integrity snaps, which triggers a fresh connectivity
 * search because the shape of the graph has changed.
 */
void update_web(game_data &game, double dt)
{
    bool graph_changed = false;

    for (int i = 0; i < game.web.strand_count; i++)
    {
        if (not game.web.strands[i].active) continue;

        game.web.strands[i].tension -= 14.0 * dt;
        if (game.web.strands[i].tension < 0.0)
        {
            game.web.strands[i].tension = 0.0;
        }

        if (game.web.strands[i].tension >= game.web.strands[i].integrity)
        {
            remove_strand(game.web, i);
            graph_changed = true;
        }
    }

    if (graph_changed)
    {
        rebuild_reachability(game.web);
    }
}

/**
 * Spend a repair charge to restore the broken strand nearest the spider.
 * Restoring an edge changes the graph, so reachability is recalculated.
 * @returns true if a strand was repaired
 */
bool repair_nearest(game_data &game)
{
    if (game.repairs <= 0) return false;

    int    best     = -1;
    double best_gap = 220.0;   // spider must be reasonably close

    for (int i = 0; i < game.web.strand_count; i++)
    {
        if (game.web.strands[i].active) continue;

        point_2d a = game.web.nodes[game.web.strands[i].node_a].pos;
        point_2d b = game.web.nodes[game.web.strands[i].node_b].pos;
        point_2d mid = point_at((a.x + b.x) / 2.0, (a.y + b.y) / 2.0);

        double gap = point_point_distance(game.player.ent.pos, mid);
        if (gap < best_gap)
        {
            best_gap = gap;
            best     = i;
        }
    }

    if (best < 0) return false;

    game.web.strands[best].active  = true;
    game.web.strands[best].tension = 0.0;
    attach_edge(game.web.nodes[game.web.strands[best].node_a], best);
    attach_edge(game.web.nodes[game.web.strands[best].node_b], best);
    game.repairs--;

    rebuild_reachability(game.web);
    return true;
}