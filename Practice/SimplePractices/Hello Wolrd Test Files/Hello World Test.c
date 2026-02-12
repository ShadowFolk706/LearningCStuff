/* Termusic.c */
#include <ncurses.h>

int main(void)
{
    initscr();             /* Start curses mode - initializes the screen */
    noecho();
    cbreak();
    curs_set(FALSE);
    printw("Hello World !!!"); /* Print a message to the internal buffer */
    refresh();             /* Print the buffer to the real screen */
    getch();               /* Wait for user input */
    endwin();              /* End curses mode and restore the terminal */

    return 0;
}
