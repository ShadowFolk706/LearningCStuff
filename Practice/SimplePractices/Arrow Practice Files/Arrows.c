#include <ncurses.h>

int main (void)
{
    int key, x, y;
    
    initscr();
    keypad(stdscr, TRUE);
    noecho();

    x = 0;
    y = 5;
    
    while (key != 'q')
    {
        clear();

        move(0, 0);
        printw("Press LEFT, RIGHT, UP, or DOWN arrow keys to move. Exit by pressing q.");

        move(y, x);
        printw("*");
        refresh();

        key = getch();
        if (key == KEY_LEFT)
        {
            x--;
            if (x < 0) x=0;
        }
        if (key == KEY_RIGHT)
        {
            x++;
            if (x > 40) x=40;
        }
        if (key == KEY_UP)
        {
            y--;
            if (y < 1) y=1;
        }
        if (key == KEY_DOWN)
        {
            y++;
            if (y > 20) y=20;
        }
    }

    endwin();

    return 0;
}
