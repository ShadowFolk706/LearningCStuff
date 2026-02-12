#include <ncurses.h>

int main (void)
{
    bool running = TRUE;
    char letter;
    
    initscr();
    clear();
    while(running)
    {
        printw("Press any key");
        refresh();
        
        letter = getch();
        clear();
        printw("You pressed the %c key", letter);
        refresh();

        char letter2;
        letter2 = getch();

        if (letter2=='q')
        {
            clear();
            refresh();
            running = FALSE;
        }
        clear();
    }

    return 0;
}
