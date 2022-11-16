#include "game/cd.h"

#include <stdlib.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// 0x420B10
int sub_420B10(const char* a1)
{
    return GetDriveTypeA(a1) == DRIVE_CDROM;
}

// 0x420B28
int sub_420B28(const char* a1, const char* a2)
{
    UINT oldErrorMode;
    char volumeName[MAX_PATH];
    BOOL success;

    oldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
    success = GetVolumeInformationA(a1, volumeName, MAX_PATH, NULL, NULL, NULL, NULL, 0);
    if (success) {
        success = lstrcmpA(a2, volumeName) == 0;
    }
    SetErrorMode(oldErrorMode);

    return success;
}

// 0x420B8C
int sub_420B8C(const char* a1)
{
    DWORD attributes;

    attributes = GetFileAttributesA(a1);
    if (attributes != INVALID_FILE_ATTRIBUTES) {
        if ((attributes & FILE_ATTRIBUTE_READONLY) != 0) {
            if (!SetFileAttributesA(a1, FILE_ATTRIBUTE_NORMAL)) {
                return 1;
            }
            SetFileAttributesA(a1, attributes);
        }
    }

    return 0;
}

// 0x420BDC
int sub_420BDC(const char* a1, int a2)
{
    DWORD sectorsPerCluster;
    DWORD bytesPerSector;
    DWORD numberOfFreeClusters;
    DWORD totalNumberOfClusters;

    if (GetDiskFreeSpaceA(a1, &sectorsPerCluster, &bytesPerSector, &numberOfFreeClusters, &totalNumberOfClusters)) {
        if (a2 == bytesPerSector) {
            return 1;
        }
    }

    return 0;
}

// 0x420C18
int sub_420C18(const char* a1, const char* a2, int a3)
{
    char drive[_MAX_DRIVE + 1];

    _splitpath(a1, drive, NULL, NULL, NULL);
    lstrcatA(drive, "\\");

    if ((a3 & 0x1) != 0) {
        // NOTE: Uninline.
        if (!sub_420B10(drive)) {
            return 0;
        }
    }

    if ((a3 & 0x02) != 0) {
        if (a2 != NULL) {
            if (!sub_420B28(drive, a2)) {
                return 0;
            }
        }
    }

    if ((a3 & 0x04) != 0) {
        if (!sub_420B8C(a1)) {
            return 0;
        }
    }

    if ((a3 & 0x08) != 0) {
        if (!sub_420BDC(drive, 2048)) {
            return 0;
        }
    }

    return 1;
}
