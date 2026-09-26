// ---------------------------------------------------------------------------
// Web Weaver - SIT102 Custom Project
// Stage 0: an empty window that closes cleanly.
// ---------------------------------------------------------------------------

#include "game_types.h"

int main()
{
    open_window("Web Weaver", SCREEN_WIDTH, SCREEN_HEIGHT);

    while (not quit_requested())
    {
        process_events();

        if (key_typed(ESCAPE_KEY)) break;

        clear_screen(rgba_color(28, 43, 31, 255));
        draw_text("Web Weaver - stage 0", color_white(), 20, 20);

        refresh_screen(60);
    }

    close_all_windows();
    return 0;
}