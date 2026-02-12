#include <ncurses.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    DIR *dir;
    struct dirent *entry;
    char **files = NULL;
    char *cfile = NULL;
    int file_count = 0;
    int key, y, startY, startX, width, height, endX, endY, i;

    dir = opendir("/Users/hpapez27/Desktop/Musikk/TermusicFiles");
    if (dir == NULL)
    {
        fprintf(stderr, "No such directory.\n");
        return 1;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0)
            continue;

        files = realloc(files, sizeof(char *) * (file_count + 1));
        files[file_count] = strdup(entry->d_name);
        file_count++;
    }
    closedir(dir);

    initscr();
    start_color();
    use_default_colors();
    curs_set(0);
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    clear();

    y = 1;

    width = COLS / 2;
    height = LINES - 3;
    startY = 1;
    startX = 1;
    endX = startX + width;
    endY = startY + height;

    init_color(COLOR_CYAN, 1000, 1000, 1000);
    init_color(COLOR_BLACK, 263, 271, 271);

    // Colors
    // Red on clear
    init_pair(1, COLOR_RED, -1);
    // Green on clear
    init_pair(2, COLOR_GREEN, -1);
    // Yellow on clear
    init_pair(3, COLOR_YELLOW, -1);
    // Highlighted
    init_pair(4, COLOR_CYAN, COLOR_BLACK);

    WINDOW *win = newwin(height + 3, width + 3, startY - 1, startX - 1);

    y = startY;
    while (key != 'q')
    {
        clear();
        wclear(win);

        color_set(3, NULL);

        border(0, 0, 0, 0, 0, 0, 0, 0);
        box(win, 0, 0);

        // Draw Window with raven
        wbkgd(win, COLOR_PAIR(1));
        mvwprintw(win, startY - 1, startX + 1, "File Explor");
        mvwprintw(win, endY + 1, startX + 1, "Use arrows to navegate, return to select, and q to exit");
        wbkgd(win, COLOR_PAIR(0));
        for (i = 0; i < file_count; i++)
        {
            if (i == (y - 1))
            {
                wbkgd(win, COLOR_PAIR(4));
            }
            if (i != (y - 1))
            {
                wbkgd(win, COLOR_PAIR(2));
            }
            mvwprintw(win, i + 2, 2, "%s", files[i]);
        }
        

        wbkgd(win, COLOR_PAIR(3));
        mvwprintw(stdscr, 15, COLS / 2 + 3, "File: %s", cfile);
        wbkgd(win, COLOR_PAIR(0));

        refresh();
        wrefresh(win);

        key = getch();
        if (key == KEY_UP)
        {
            y--;
            if (y < startY) y = startY;
        }
        if (key == KEY_DOWN)
        {
            y++;
            if (y > startY + file_count - 1) y = startY + file_count - 1;
        }
        if (key == KEY_ENTER || key == '\n' || key == '\r')
        {
            cfile = files[y - 1];
        }
    }

    for (i = 0; i < file_count; i++)
    {
        free(files[i]);
    }
    free(files);

    endwin();

    return 0;
}
