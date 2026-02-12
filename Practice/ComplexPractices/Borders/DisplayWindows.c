#include <ncurses.h>

int main (void)
{
    // Initialization
    int key, x, y, startY, startX, width, height, endX, endY;

    initscr();
    start_color();
    use_default_colors();
    curs_set(0);
    keypad(stdscr, TRUE);
    cbreak();
    noecho();
    clear();
    refresh();

    x = 0;
    y = 1;

    width = COLS / 2;
    height = LINES - 3;
    startY = 1;
    startX = 1;
    endX = startX + width;
    endY = startY + height;

    // Colors
    // Red on clear
    init_pair(1, COLOR_RED, -1);
    // Green on clear
    init_pair(2, COLOR_GREEN, -1);
    // Yellow on clear
    init_pair(3, COLOR_YELLOW, -1);
    
    WINDOW *win = newwin(height + 3, width + 3, startY - 1, startX - 1);

    // Movement
    x = startX;
    y = startY;
    while (key != 'q')
    {
        clear();
        wclear(win);

        color_set(3, NULL);
        
        // Draw bounds
        border(0, 0, 0, 0, 0, 0, 0, 0);
        box(win, 0, 0);

        // Position
        move(LINES-3, COLS-8);
        printw("x = ");
        color_set(2, NULL);
        printw("%d", x);
        color_set(0, NULL);
        move(LINES-2, COLS-8);
        printw("y = ");
        color_set(2, NULL);
        printw("%d", y);
        color_set(0, NULL);

        // Draw Window with raven
        mvwprintw(win, y - startY + 1, x - startX + 1, "*");
        wbkgd(win, COLOR_PAIR(1));
        mvwprintw(win, startY - 1, startX + 1, "File Explor");
        wbkgd(win, COLOR_PAIR(0));

        // Draw everything
        refresh();
        wrefresh(win);

        key = getch();
        if (key == KEY_LEFT)
        {
            x--;
            if (x < startX) x=startX;
        }
        if (key == KEY_RIGHT)
        {
            x++;
            if (x > endX) x=endX;
        }
        if (key == KEY_UP)
        {
            y--;
            if (y < startY) y=startY;
        }
        if (key == KEY_DOWN)
        {
            y++;
            if (y > endY) y=endY;
        }
    }

    endwin();

    return 0;
}
