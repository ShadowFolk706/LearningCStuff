#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include <dirent.h>
#include <stdio.h>
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
} MiniaudioPlayer;

const char* get_filename(const char* filepath) {
    const char* filename = strrchr(filepath, '/');
    if (filename == NULL) {
        return filepath;
    }
    return filename + 1;
}

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    MiniaudioPlayer* player = (MiniaudioPlayer*)pDevice->pUserData;

    if (!player->current.is_active) {
        // Silence if nothing playing
        memset(pOutput, 0, frameCount * ma_get_bytes_per_frame(pDevice->playback.format, pDevice->playback.channels));
        return;
    }
    
    // Read from current decoder
    ma_uint64 framesRead = 0;
    ma_decoder_read_pcm_frames(&player->current.decoder, pOutput, frameCount, &framesRead);
    
    if (framesRead < frameCount) {
        // Fill rest with silence
        size_t bytesPerFrame = ma_get_bytes_per_frame(pDevice->playback.format, pDevice->playback.channels);
        memset((unsigned char*)pOutput + (framesRead * bytesPerFrame), 0, 
               (frameCount - framesRead) * bytesPerFrame);
        
        // Auto-advance to next track if enabled
        if (player->auto_advance && player->next.is_active) {
            ma_decoder_uninit(&player->current.decoder);
            player->current = player->next;
            player->next.is_active = MA_FALSE;
            player->current_index++;
            
            printf("\rNow playing: %s\n", get_filename(player->current.filepath));

            // Pre-load the next track
            if (player->current_index + 1 < player->playlist_count) {
                strcpy(player->next.filepath, player->playlist[player->current_index + 1]);
                if (ma_decoder_init_file(player->next.filepath, NULL, &player->next.decoder) == MA_SUCCESS) {
                    player->next.is_active = MA_TRUE;
                    printf("Preloaded: %s", get_filename(player->next.filepath));
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

    if (ma_device_init(NULL, &config, &player->device) != MA_SUCCESS) {
        printf("\rFailed to initialize playback device.\n");
        return -1;
    }

    if (ma_device_start(&player->device) != MA_SUCCESS) {
        printf("\rFailed to start playback device.\n");
        ma_device_uninit(&player->device);
        return -1;
    }
    
    return 0;
}

int player_play_file(MiniaudioPlayer* player, const char* filepath)
{
    if (player->current.is_active)
    {
        ma_decoder_uninit(&player->current.decoder);
    }

    strcpy(player->current.filepath, filepath);
    if (ma_decoder_init_file(filepath, NULL, &player->current.decoder) != MA_SUCCESS)
    {
        printf("\rFailed to load file: %s\n", filepath);
        player->current.is_active = MA_FALSE;
        return -1;
    }

    player->current.is_active = MA_TRUE;
    player->auto_advance = MA_FALSE;
    printf("\rNow playing: %s\n", get_filename(filepath));

    return 0;
}

int player_play_playlist(MiniaudioPlayer* player, char** files, int count)
{
    if (count == 0) return -1;

    player->playlist = files;
    player->playlist_count = count;
    player->current_index = 0;
    player->auto_advance = MA_TRUE;

    if (player_play_file(player, files[0]) != 0) return -1;

    player->auto_advance = MA_TRUE;

    if (count > 1)
    {
        strcpy(player->next.filepath, files[1]);
        if (ma_decoder_init_file(files[1], NULL, &player->next.decoder) == MA_SUCCESS)
        {
            player->next.is_active = MA_TRUE;
            printf("\rPreloaded: %s", get_filename(files[1]));
            fflush(stdout);
        }
    }

    return 0;
}

int player_stop(MiniaudioPlayer* player)
{
    if (player->current.is_active) {
        ma_decoder_uninit(&player->current.decoder);
        player->current.is_active = MA_FALSE;
    }
    if (player->next.is_active) {
        ma_decoder_uninit(&player->next.decoder);
        player->next.is_active = MA_FALSE;
    }

    return 0;
}

void player_cleanup(MiniaudioPlayer* player)
{
    player_stop(player);
    ma_device_uninit(&player->device);
}

int compare_strings(const void* a, const void* b) {
    return strcmp(*(const char**)a, *(const char**)b);
}

int is_audio_file(const char* filename) {
    const char* ext = strrchr(filename, '.');
    if (ext == NULL) return 0;
    
    // Check for common audio extensions
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

int main()
{
    MiniaudioPlayer player;
    DIR *dir;
    struct dirent *entry;
    char **files = NULL;
    int file_count = 0;
    int i;

    const char* music_dir = "/Users/hpapez27/Desktop/Musikk/TermusicFiles/Pink_Floyd_-_The_Dark_Side_of_the_Moon";

    dir = opendir(music_dir);
    if (dir == NULL)
    {
        printf("No such directory.");
        return -1;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        if ((strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0))
            continue;

        if (!is_audio_file(entry->d_name))
            continue;

        files = realloc(files, sizeof(char *) * (file_count + 1));
        
        // Allocate space for full path
        size_t path_len = strlen(music_dir) + strlen(entry->d_name) + 2;
        files[file_count] = malloc(path_len);
        snprintf(files[file_count], path_len, "%s/%s", music_dir, entry->d_name);
        
        file_count++;
    }
    closedir(dir);

    qsort(files, file_count, sizeof(char*), compare_strings);

    if (player_init(&player) != 0)
    {
        return 1;
    }

    printf("Press Enter to quit.\n");
    player_play_playlist(&player, files, file_count);

    getchar();
    player_cleanup(&player);
    for (i = 0; i < file_count; i++)
    {
        free(files[i]);
    }
    free(files);
    return 0;
}
