#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include <stdio.h>
#include <ncurses.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

typedef struct
{
    ma_decoder decoder;
    ma_bool32 is_active;
    char filepath[512];
} AudioTrack;

typedef struct
{
    AudioTrack current;
    AudioTrack next;
    ma_device device;
    char** playlist;
    int playlist_count;
    int current_index;
    ma_bool32 auto_advance;
    ma_bool32 is_paused;
} MiniaudioPlayer;

const char* get_filename(const char* filepath)
{
    const char* filename = strrchr(filepath, '/');
    if (filename == NULL) {
        return filepath;
    }
    return filename + 1;
}

const char* remove_extension(const char* filename)
{
    if (strcmp(filename, ".") == 0)
    {
        return ".";
    }
    static char buffer[256];
    const char* dot = strrchr(filename, '.');
    
    if (dot == NULL)
    {
        strncpy(buffer, filename, sizeof(buffer) - 4);
        buffer[sizeof(buffer) - 1] = '\0';
        return buffer + 3;
    }
    
    size_t len = dot - filename;
    if (len >= sizeof(buffer))
        len = sizeof(buffer) - 4;
    
    strncpy(buffer, filename, len);
    buffer[len] = '\0';
    
    return buffer + 3;
}

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    MiniaudioPlayer* player = (MiniaudioPlayer*)pDevice->pUserData;

    if (!player->current.is_active || player->is_paused)
    {
        memset(pOutput, 0, frameCount * ma_get_bytes_per_frame(pDevice->playback.format, pDevice->playback.channels));
        return;
    }
    
    ma_uint64 framesRead = 0;
    ma_decoder_read_pcm_frames(&player->current.decoder, pOutput, frameCount, &framesRead);
    
    if (framesRead < frameCount)
    {
        size_t bytesPerFrame = ma_get_bytes_per_frame(pDevice->playback.format, pDevice->playback.channels);
        memset((unsigned char*)pOutput + (framesRead * bytesPerFrame), 0, 
               (frameCount - framesRead) * bytesPerFrame);
        
        if (player->auto_advance && player->next.is_active)
        {
            ma_decoder_uninit(&player->current.decoder);
            player->current = player->next;
            player->next.is_active = MA_FALSE;
            player->current_index++;

            if (player->current_index + 1 < player->playlist_count)
            {
                strcpy(player->next.filepath, player->playlist[player->current_index + 1]);
                if (ma_decoder_init_file(player->next.filepath, NULL, &player->next.decoder) == MA_SUCCESS)
                {
                    player->next.is_active = MA_TRUE;
                    fflush(stdout);
                }
            }
        } else {
            player->current.is_active = MA_FALSE;
        }
    }
    
    (void)pInput;
}

int player_init(MiniaudioPlayer* player)
{
    memset(player, 0, sizeof(MiniaudioPlayer));

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.playback.channels = 2;
    config.sampleRate = 48000;
    config.dataCallback = data_callback;
    config.pUserData = player;

    if (ma_device_init(NULL, &config, &player->device) != MA_SUCCESS)
    {
        printf("\rFailed to initialize playback device.\n");
        return -1;
    }

    if (ma_device_start(&player->device) != MA_SUCCESS)
    {
        printf("\rFailed to start playback device.\n");
        ma_device_uninit(&player->device);
        return -1;
    }
    
    return 0;
}

void player_toggle_pause(MiniaudioPlayer* player)
{
    player->is_paused = !player->is_paused;
}

int player_skip_next(MiniaudioPlayer* player)
{
    if (!player->auto_advance || player->current_index + 1 >= player->playlist_count)
        return -1;

    ma_device_stop(&player->device);
    
    if (player->current.is_active)
    {
        ma_decoder_uninit(&player->current.decoder);
        player->current.is_active = MA_FALSE;
    }
    
    if (player->next.is_active)
    {
        player->current = player->next;
        player->next.is_active = MA_FALSE;
        player->current_index++;
    }
    else
    {
        player->current_index++;
        strcpy(player->current.filepath, player->playlist[player->current_index]);
        if (ma_decoder_init_file(player->playlist[player->current_index], NULL, &player->current.decoder) != MA_SUCCESS)
        {
            player->current.is_active = MA_FALSE;
            ma_device_start(&player->device);
            return -1;
        }
    }
    
    player->current.is_active = MA_TRUE;
    
    if (player->current_index + 1 < player->playlist_count)
    {
        strcpy(player->next.filepath, player->playlist[player->current_index + 1]);
        if (ma_decoder_init_file(player->next.filepath, NULL, &player->next.decoder) == MA_SUCCESS)
        {
            player->next.is_active = MA_TRUE;
        }
    }
    
    ma_device_start(&player->device);
    return 0;
}

int player_skip_previous(MiniaudioPlayer* player)
{
    if (!player->auto_advance || player->current_index <= 0)
        return -1;

    ma_device_stop(&player->device);
    
    if (player->current.is_active)
    {
        ma_decoder_uninit(&player->current.decoder);
        player->current.is_active = MA_FALSE;
    }
    
    if (player->next.is_active)
    {
        ma_decoder_uninit(&player->next.decoder);
        player->next.is_active = MA_FALSE;
    }
    
    player->current_index--;
    
    strcpy(player->current.filepath, player->playlist[player->current_index]);
    if (ma_decoder_init_file(player->playlist[player->current_index], NULL, &player->current.decoder) != MA_SUCCESS)
    {
        player->current.is_active = MA_FALSE;
        ma_device_start(&player->device);
        return -1;
    }
    player->current.is_active = MA_TRUE;
    
    if (player->current_index + 1 < player->playlist_count)
    {
        strcpy(player->next.filepath, player->playlist[player->current_index + 1]);
        if (ma_decoder_init_file(player->next.filepath, NULL, &player->next.decoder) == MA_SUCCESS)
        {
            player->next.is_active = MA_TRUE;
        }
    }
    
    ma_device_start(&player->device);
    return 0;
}

int player_play_file(MiniaudioPlayer* player, const char* filepath)
{
    ma_device_stop(&player->device);
    
    if (player->current.is_active)
    {
        ma_decoder_uninit(&player->current.decoder);
        player->current.is_active = MA_FALSE;
    }
    
    if (player->next.is_active)
    {
        ma_decoder_uninit(&player->next.decoder);
        player->next.is_active = MA_FALSE;
    }

    strcpy(player->current.filepath, filepath);
    if (ma_decoder_init_file(filepath, NULL, &player->current.decoder) != MA_SUCCESS)
    {
        printf("\rFailed to load file: %s\n", filepath);
        player->current.is_active = MA_FALSE;
        ma_device_start(&player->device);
        return -1;
    }

    player->current.is_active = MA_TRUE;
    player->auto_advance = MA_FALSE;
    player->is_paused = MA_FALSE;
    
    ma_device_start(&player->device);

    return 0;
}

int player_play_playlist(MiniaudioPlayer* player, char** files, int count)
{
    if (count == 0) return -1;
    
    ma_device_stop(&player->device);
    
    if (player->current.is_active)
    {
        ma_decoder_uninit(&player->current.decoder);
        player->current.is_active = MA_FALSE;
    }
    
    if (player->next.is_active)
    {
        ma_decoder_uninit(&player->next.decoder);
        player->next.is_active = MA_FALSE;
    }

    if (player->playlist != NULL)
    {
        for (int i = 0; i < player->playlist_count; i++)
        {
            free(player->playlist[i]);
        }
        free(player->playlist);
        player->playlist = NULL;
    }

    player->playlist = malloc(sizeof(char*) * count);
    for (int i = 0; i < count; i++)
    {
        player->playlist[i] = strdup(files[i]);
    }

    player->playlist_count = count;
    player->current_index = 0;
    player->auto_advance = MA_TRUE;
    player->is_paused = MA_FALSE;

    strcpy(player->current.filepath, player->playlist[0]);
    if (ma_decoder_init_file(player->playlist[0], NULL, &player->current.decoder) != MA_SUCCESS)
    {
        printf("\rFailed to load file: %s\n", player->playlist[0]);
        player->current.is_active = MA_FALSE;
        ma_device_start(&player->device);
        return -1;
    }
    player->current.is_active = MA_TRUE;

    if (count > 1)
    {
        strcpy(player->next.filepath, player->playlist[1]);
        if (ma_decoder_init_file(player->playlist[1], NULL, &player->next.decoder) == MA_SUCCESS)
        {
            player->next.is_active = MA_TRUE;
        }
    }
    
    ma_device_start(&player->device);

    return 0;
}

int player_stop(MiniaudioPlayer* player)
{
    ma_device_stop(&player->device);
    
    if (player->current.is_active)
    {
        ma_decoder_uninit(&player->current.decoder);
        player->current.is_active = MA_FALSE;
    }
    if (player->next.is_active)
    {
        ma_decoder_uninit(&player->next.decoder);
        player->next.is_active = MA_FALSE;
    }
    
    ma_device_start(&player->device);

    return 0;
}

void player_cleanup(MiniaudioPlayer* player)
{
    player_stop(player);
    
    if (player->playlist != NULL)
    {
        for (int i = 0; i < player->playlist_count; i++)
        {
            free(player->playlist[i]);
        }
        free(player->playlist);
        player->playlist = NULL;
    }
    
    ma_device_uninit(&player->device);
}

int compare_strings(const void* a, const void* b)
{
    return strcmp(*(const char**)a, *(const char**)b);
}

int is_audio_file(const char* filename)
{
    const char* ext = strrchr(filename, '.');
    if (ext == NULL) return 0;
    
    return (strcmp(ext, ".mp3") == 0 ||
            strcmp(ext, ".MP3") == 0 ||
            strcmp(ext, ".wav") == 0 ||
            strcmp(ext, ".WAV") == 0 ||
            strcmp(ext, ".flac") == 0 ||
            strcmp(ext, ".FLAC") == 0 ||
            strcmp(ext, ".ogg") == 0 ||
            strcmp(ext, ".OGG") == 0 ||
            strcmp(ext, ".m4a") == 0 ||
            strcmp(ext, ".M4A") == 0);
}




















































































// -----------------------------------------------------------------------------------------------------------------------
int main()
{
    MiniaudioPlayer player;
    DIR *dir;
    FILE *log;
    char *logFilepath = "/Users/hpapez27/Termusic/Practice/ComplexPractices/PSFSP/log.txt";
    struct dirent *entry;
    char **files = NULL;
    char **filesToBePlayed = NULL;
    char *cfile = NULL;
    char *songName = NULL;
    char cfileFilePath[512];
    int file_count = 0;
    int key, y, startY, startX, width, height, endX, endY, i;

    const char* music_dir = "/Users/hpapez27/Desktop/Musikk/TermusicFiles/Pink_Floyd_-_The_Dark_Side_of_the_Moon";
    dir = opendir(music_dir);
    if (dir == NULL)
    {
        fprintf(stderr, "No such directory.");
        return 1;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, "..") == 0)
            continue;

        if (strcmp(entry->d_name, ".DS_Store") == 0)
            continue;

        files = realloc(files, sizeof(char *) * (file_count + 1));
        
        size_t path_len = strlen(music_dir) + strlen(entry->d_name) + 2;
        files[file_count] = malloc(path_len);
        snprintf(files[file_count], path_len, "%s", entry->d_name);
        
        file_count++;
    }
    closedir(dir);

    qsort(files, file_count, sizeof(char*), compare_strings);

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

    if (player_init(&player) != 0)
    {
        return 1;
    }

    init_pair(1, COLOR_RED, -1);
    init_pair(2, COLOR_GREEN, -1);
    init_pair(3, COLOR_YELLOW, -1);
    init_pair(4, COLOR_CYAN, COLOR_BLACK);

    WINDOW *win = newwin(height + 3, width + 3, startY - 1, startX - 1);
    
    y = startY;
    while (key != 'q')
    {
        log = fopen(logFilepath, "a");
        fprintf(log, "Log Initialized\n");

        clear();
        wclear(win);
        color_set(3, NULL);

        border(0, 0, 0, 0, 0, 0, 0, 0);
        box(win, 0, 0);

        wbkgd(win, COLOR_PAIR(1));
        mvwprintw(win, startY - 1, startX + 1, "File Explor");
        mvwprintw(win, endY + 1, startX + 1, "UP/DOWN navegate, return select, space pause, ,/. skip, q exit");
        wbkgd(win, COLOR_PAIR(0));
        for (i = 0; i < file_count; i++)
        {
            if (i == (y - startY))
            {
                wattron(win, COLOR_PAIR(4));
                mvwprintw(win, i + 2, 2, "%s", files[i]);
                mvwhline(win, i + 2, 2 + strlen(files[i]), ' ', width - 2 - strlen(files[i]));
                wattroff(win, COLOR_PAIR(4));
            }
            if (i != (y - 1))
            {
                wbkgd(win, COLOR_PAIR(2));
                mvwprintw(win, i + 2, 2, "%s", files[i]);
                wbkgd(win, COLOR_PAIR(0));
            }
        }

        wbkgd(win, COLOR_PAIR(3));
        if (cfile != NULL)
        {
            mvwprintw(stdscr, LINES / 2 - 2, COLS / 2 + 3, "File: %s", remove_extension(cfile));
            mvwprintw(stdscr, LINES / 2 - 1, COLS / 2 + 3, "Status: %s", player.is_paused ? "||" : "|>");
        }
        else
        {
            mvwprintw(stdscr, LINES / 2 - 2, COLS / 2 + 3, "File: None");
            mvwprintw(stdscr, LINES / 2 - 1, COLS / 2 + 3, "Status: %s", player.is_paused ? "||" : "|>");
        }
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
        if (key == ' ')
        {
            player_toggle_pause(&player);
        }
        if (key == '.')
        {
            player_skip_next(&player);
        }
        if (key == ',')
        {
            player_skip_previous(&player);
        }
        if (key == KEY_ENTER || key == '\n' || key == '\r')
        {
            cfile = files[y - 1];
            mvwprintw(stdscr, 15, COLS / 2 + 3, cfile);
            snprintf(cfileFilePath, sizeof(cfileFilePath), "%s/%s", music_dir, cfile);
            mvwprintw(win, LINES / 2 + 4, startX, cfile);
            if (strcmp(cfile, ".") == 0)
            {
                mvwprintw(stdscr, LINES / 2 - 3, COLS / 2 + 4, "Dot Detected.");
                fprintf(log, "Dot Detected.\n");
                
                char **fullPaths = malloc(sizeof(char*) * (file_count - 1));
                int playlistCount = 0;
                
                for (int i = 1; i < file_count; ++i)
                {
                    if (is_audio_file(files[i]))
                    {
                        size_t path_len = strlen(music_dir) + strlen(files[i]) + 2;
                        fullPaths[playlistCount] = malloc(path_len);
                        snprintf(fullPaths[playlistCount], path_len, "%s/%s", music_dir, files[i]);
                        playlistCount++;
                    }
                }
                
                if (playlistCount > 0)
                {
                    if (player_play_playlist(&player, fullPaths, playlistCount) == -1)
                    {
                        fprintf(log, "ERROR: Failed to play playlist\n");
                    }
                }
                else
                {
                    fprintf(log, "ERROR: No audio files found\n");
                }
                
                for (int i = 0; i < playlistCount; i++)
                {
                    free(fullPaths[i]);
                }
                free(fullPaths);
            }
            else
            {
                player_play_file(&player, cfileFilePath);
            }
        }
        fclose(log);
    }

    player_cleanup(&player);
    for (i = 0; i < file_count; i++)
    {
        free(files[i]);
    }
    free(files);

    endwin();

    return 0;
}
