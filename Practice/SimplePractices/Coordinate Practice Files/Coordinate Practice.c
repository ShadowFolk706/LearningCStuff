#include <ncurses.h>

int main (void)
{
    int x, y;

    initscr();
    clear();

    getyx(stdscr, y, x);
    printw( "x = %d"
            "\ny = %d",
            x, y);
    refresh();

    y = 6;
    x = 9;

    move(y, x);

    printw("Jeg er her nå!!!");
    
    getyx(stdscr, y, x);
    printw( "\nx = %d"
            "\ny = %d",
            x, y);
    refresh();

    getch();
    endwin();

    return 0;
}
