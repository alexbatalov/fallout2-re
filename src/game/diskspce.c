#include "game/diskspce.h"

#include <ctype.h>
#include <direct.h>
#include <stdlib.h>

#include "game/gconfig.h"

// 0x431560
int GetFreeDiskSpace(long* diskSpacePtr)
{
    char* path;
    char drive[_MAX_DRIVE];
    int useGetDrive;
    int driveNumber;
    struct diskfree_t df;

    *diskSpacePtr = 0;
    useGetDrive = 1;

    if (config_get_string(&game_config, GAME_CONFIG_SYSTEM_KEY, GAME_CONFIG_PATCHES_KEY, &path) == 1) {
        _splitpath(path, drive, NULL, NULL, NULL);

        if (drive[0] != '\0') {
            useGetDrive = 0;
            driveNumber = toupper(drive[0]) - 64;
        }
    }

    if (useGetDrive) {
        driveNumber = _getdrive();
    }

    if (_getdiskfree(driveNumber, &df) == 0) {
        *diskSpacePtr = ((long)df.bytes_per_sector * (long)df.sectors_per_cluster * (long)df.avail_clusters) / 1024;
        return 0;
    }

    return -1;
}
