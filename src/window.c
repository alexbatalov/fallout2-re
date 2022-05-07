#include "window.h"

#include "core.h"
#include "draw.h"
#include "interpreter_lib.h"
#include "memory_manager.h"
#include "mouse_manager.h"
#include "movie.h"
#include "text_font.h"
#include "widget.h"
#include "window_manager.h"

#include <stdio.h>

// 0x51DCAC
int dword_51DCAC = 250;

// 0x51DCB0
int dword_51DCB0 = 1;

// 0x51DCB4
int dword_51DCB4 = -1;

// 051DCB8
int dword_51DCB8 = -1;

// 0x51DCBC
INITVIDEOFN off_51DCBC[12] = {
    _init_mode_320_200,
    _init_mode_640_480,
    _init_mode_640_480_16,
    _init_mode_320_400,
    _init_mode_640_480_16,
    _init_mode_640_400,
    _init_mode_640_480_16,
    _init_mode_800_600,
    _init_mode_640_480_16,
    _init_mode_1024_768,
    _init_mode_640_480_16,
    _init_mode_1280_1024,
};

// 0x51DD1C
Size stru_51DD1C[12] = {
    { 320, 200 },
    { 640, 480 },
    { 640, 240 },
    { 320, 400 },
    { 640, 200 },
    { 640, 400 },
    { 800, 300 },
    { 800, 600 },
    { 1024, 384 },
    { 1024, 768 },
    { 1280, 512 },
    { 1280, 1024 },
};

// 0x51DD7C
int dword_51DD7C = 0;

// 0x51DD80
int dword_51DD80 = -1;

// 0x51DD84
int dword_51DD84 = 1;

// 0x66E770
int dword_66E770[16];

// 0x66E7B0
char byte_66E7B0[64 * 256];

// 0x6727B0
STRUCT_6727B0 stru_6727B0[16];

// NOTE: This value is never set.
//
// 0x672D78
void (*off_672D78)(int, STRUCT_6727B0*);

// 0x672D7C
int dword_672D7C;

// 0x672D88
int dword_672D88;

// Highlight color (maybe r).
//
// 0x672D8C
int dword_672D8C;

// 0x672D90
int gWidgetFont;

// Text color (maybe g).
//
// 0x672DA0
int dword_672DA0;

// text color (maybe b).
//
// 0x672DA4
int dword_672DA4;

// 0x672DA8
int gWidgetTextFlags;

// Text color (maybe r)
//
// 0x672DAC
int dword_672DAC;

// highlight color (maybe g)
//
// 0x672DB0
int dword_672DB0;

// Highlight color (maybe b).
//
// 0x672DB4
int dword_672DB4;

// 0x4B7680
bool sub_4B7680()
{
    STRUCT_6727B0* ptr = &(stru_6727B0[dword_51DCB8]);
    if (ptr->window == -1) {
        return false;
    }

    windowRefresh(ptr->window);

    return true;
}

// 0x4B81C4
bool _selectWindowID(int index)
{
    if (index < 0 || index >= 16) {
        return false;
    }

    STRUCT_6727B0* ptr = &(stru_6727B0[index]);
    if (ptr->window == -1) {
        return false;
    }

    dword_51DCB8 = index;

    if (off_672D78 != NULL) {
        off_672D78(index, ptr);
    }

    return true;
}

// 0x4B8414
void _windowPrintBuf(int win, char* string, int stringLength, int width, int maxY, int x, int y, int flags, int textAlignment)
{
    if (y + fontGetLineHeight() > maxY) {
        return;
    }

    if (stringLength > 255) {
        stringLength = 255;
    }

    char* stringCopy = internal_malloc_safe(stringLength + 1, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1078
    strncpy(stringCopy, string, stringLength);
    stringCopy[stringLength] = '\0';

    int stringWidth = fontGetStringWidth(stringCopy);
    int stringHeight = fontGetLineHeight();
    if (stringWidth == 0 || stringHeight == 0) {
        internal_free_safe(stringCopy, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1085
        return;
    }

    if ((flags & FONT_SHADOW) != 0) {
        stringWidth++;
        stringHeight++;
    }

    unsigned char* backgroundBuffer = internal_calloc_safe(stringWidth, stringHeight, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1093
    unsigned char* backgroundBufferPtr = backgroundBuffer;
    fontDrawText(backgroundBuffer, stringCopy, stringWidth, stringWidth, flags);

    switch (textAlignment) {
    case TEXT_ALIGNMENT_LEFT:
        if (stringWidth < width) {
            width = stringWidth;
        }
        break;
    case TEXT_ALIGNMENT_RIGHT:
        if (stringWidth <= width) {
            x += (width - stringWidth);
            width = stringWidth;
        } else {
            backgroundBufferPtr = backgroundBuffer + stringWidth - width;
        }
        break;
    case TEXT_ALIGNMENT_CENTER:
        if (stringWidth <= width) {
            x += (width - stringWidth) / 2;
            width = stringWidth;
        } else {
            backgroundBufferPtr = backgroundBuffer + (stringWidth - width) / 2;
        }
        break;
    }

    if (stringHeight + y > windowGetHeight(win)) {
        stringHeight = windowGetHeight(win) - y;
    }

    if ((flags & 0x2000000) != 0) {
        blitBufferToBufferTrans(backgroundBufferPtr, width, stringHeight, stringWidth, windowGetBuffer(win) + windowGetWidth(win) * y + x, windowGetWidth(win));
    } else {
        blitBufferToBuffer(backgroundBufferPtr, width, stringHeight, stringWidth, windowGetBuffer(win) + windowGetWidth(win) * y + x, windowGetWidth(win));
    }

    internal_free_safe(backgroundBuffer, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1130
    internal_free_safe(stringCopy, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1131
}

// 0x4B8638
char** _windowWordWrap(char* string, int maxLength, int a3, int* substringListLengthPtr)
{
    if (string == NULL) {
        *substringListLengthPtr = 0;
        return NULL;
    }

    char** substringList = NULL;
    int substringListLength = 0;
    
    char* start = string;
    char* pch = string;
    int v1 = a3;
    while (*pch != '\0') {
        v1 += fontGetCharacterWidth(*pch & 0xFF);
        if (*pch != '\n' && v1 <= maxLength) {
            v1 += fontGetLetterSpacing();
            pch++;
        } else {
            while (v1 > maxLength) {
                v1 -= fontGetCharacterWidth(*pch);
                pch--;
            }

            if (*pch != '\n') {
                while (pch != start && *pch != ' ') {
                    pch--;
                }
            }

            if (substringList != NULL) {
                substringList = internal_realloc_safe(substringList, sizeof(*substringList) * (substringListLength + 1), __FILE__, __LINE__); // "..\int\WINDOW.C", 1166
            } else {
                substringList = internal_malloc_safe(sizeof(*substringList), __FILE__, __LINE__); // "..\int\WINDOW.C", 1167
            }

            char* substring = internal_malloc_safe(pch - start + 1, __FILE__, __LINE__); // "..\int\WINDOW.C", 1169
            strncpy(substring, start, pch - start);
            substring[pch - start] = '\0';

            substringList[substringListLength] = substring;

            while (*pch == ' ') {
                pch++;
            }

            v1 = 0;
            start = pch;
            substringListLength++;
        }
    }

    if (start != pch) {
        if (substringList != NULL) {
            substringList = internal_realloc_safe(substringList, sizeof(*substringList) * (substringListLength + 1), __FILE__, __LINE__); // "..\int\WINDOW.C", 1184
        } else {
            substringList = internal_malloc_safe(sizeof(*substringList), __FILE__, __LINE__); // "..\int\WINDOW.C", 1185
        }

        char* substring = internal_malloc_safe(pch - start + 1, __FILE__, __LINE__); // "..\int\WINDOW.C", 1169
        strncpy(substring, start, pch - start);
        substring[pch - start] = '\0';

        substringList[substringListLength] = substring;
        substringListLength++;
    }

    *substringListLengthPtr = substringListLength;

    return substringList;
}

// 0x4B880C
void _windowFreeWordList(char** substringList, int substringListLength)
{
    if (substringList == NULL) {
        return;
    }

    for (int index = 0; index < substringListLength; index++) {
        internal_free_safe(substringList[index], __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1200
    }

    internal_free_safe(substringList, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1201
}

// Renders multiline string in the specified bounding box.
//
// 0x4B8854
void _windowWrapLineWithSpacing(int win, char* string, int width, int height, int x, int y, int flags, int textAlignment, int a9)
{
    if (string == NULL) {
        return;
    }

    int substringListLength;
    char** substringList = _windowWordWrap(string, width, 0, &substringListLength);

    for (int index = 0; index < substringListLength; index++) {
        int v1 = y + index * (a9 + fontGetLineHeight());
        _windowPrintBuf(win, substringList[index], strlen(substringList[index]), width, height + y, x, v1, flags, textAlignment);
    }

    _windowFreeWordList(substringList, substringListLength);
}

// Renders multiline string in the specified bounding box.
//
// 0x4B88FC
void _windowWrapLine(int win, char* string, int width, int height, int x, int y, int flags, int textAlignment)
{
    _windowWrapLineWithSpacing(win, string, width, height, x, y, flags, textAlignment, 0);
}

// 0x4B9048
int _windowGetXres()
{
    return dword_672D7C;
}

// 0x4B9050
int _windowGetYres()
{
    return dword_672D88;
}

// 0x4B9058
void _removeProgramReferences_3(Program* program)
{
    for (int index = 0; index < 16; index++) {
        STRUCT_6727B0* ptr = &(stru_6727B0[index]);
        if (ptr->window != -1) {
            for (int index = 0; index < ptr->buttonsLength; index++) {
                ManagedButton* managedButton = &(ptr->buttons[index]);
                if (program == managedButton->program) {
                    managedButton->program = NULL;
                    managedButton->field_5C = 0;
                    managedButton->field_60 = 0;
                    managedButton->field_54 = 0;
                    managedButton->field_58 = 0;
                }
            }

            for (int index = 0; index < ptr->regionsLength; index++) {
                Region* region = ptr->regions[index];
                if (region != NULL) {
                    if (program == region->program) {
                        region->program = NULL;
                        region->field_4C = 0;
                        region->field_48 = 0;
                        region->field_54 = 0;
                        region->field_50 = 0;
                    }
                }
            }
        }
    }
}

// 0x4B9190
void _initWindow(int resolution, int a2)
{
    char err[MAX_PATH];
    int rc;
    int i, j;

    _interpretRegisterProgramDeleteCallback(_removeProgramReferences_3);

    dword_672DAC = 0;
    dword_672DA0 = 0;
    dword_672DA4 = 0;
    dword_672D8C = 0;
    dword_672DB0 = 0;
    gWidgetTextFlags = 0x2010000;

    dword_672D88 = stru_51DD1C[resolution].height; // screen height
    dword_672DB4 = 0;
    dword_672D7C = stru_51DD1C[resolution].width; // screen width

    for (int i = 0; i < 16; i++) {
        stru_6727B0[i].window = -1;
    }

    rc = windowManagerInit(off_51DCBC[resolution], directDrawFree, a2);
    if (rc != WINDOW_MANAGER_OK) {
        switch (rc) {
        case WINDOW_MANAGER_ERR_INITIALIZING_VIDEO_MODE:
            sprintf(err, "Error initializing video mode %dx%d\n", dword_672D7C, dword_672D88);
            showMesageBox(err);
            exit(1);
            break;
        case WINDOW_MANAGER_ERR_NO_MEMORY:
            sprintf(err, "Not enough memory to initialize video mode\n");
            showMesageBox(err);
            exit(1);
            break;
        case WINDOW_MANAGER_ERR_INITIALIZING_TEXT_FONTS:
            sprintf(err, "Couldn't find/load text fonts\n");
            showMesageBox(err);
            exit(1);
            break;
        case WINDOW_MANAGER_ERR_WINDOW_SYSTEM_ALREADY_INITIALIZED:
            sprintf(err, "Attempt to initialize window system twice\n");
            showMesageBox(err);
            exit(1);
            break;
        case WINDOW_MANAGER_ERR_WINDOW_SYSTEM_NOT_INITIALIZED:
            sprintf(err, "Window system not initialized\n");
            showMesageBox(err);
            exit(1);
            break;
        case WINDOW_MANAGER_ERR_CURRENT_WINDOWS_TOO_BIG:
            sprintf(err, "Current windows are too big for new resolution\n");
            showMesageBox(err);
            exit(1);
            break;
        case WINDOW_MANAGER_ERR_INITIALIZING_DEFAULT_DATABASE:
            sprintf(err, "Error initializing default database.\n");
            showMesageBox(err);
            exit(1);
            break;
        case WINDOW_MANAGER_ERR_8:
            exit(1);
            break;
        case WINDOW_MANAGER_ERR_ALREADY_RUNNING:
            sprintf(err, "Program already running.\n");
            showMesageBox(err);
            exit(1);
            break;
        case WINDOW_MANAGER_ERR_TITLE_NOT_SET:
            sprintf(err, "Program title not set.\n");
            showMesageBox(err);
            exit(1);
            break;
        case WINDOW_MANAGER_ERR_INITIALIZING_INPUT:
            sprintf(err, "Failure initializing input devices.\n");
            showMesageBox(err);
            exit(1);
            break;
        default:
            sprintf(err, "Unknown error code %d\n", rc);
            showMesageBox(err);
            exit(1);
            break;
        }
    }

    gWidgetFont = 100;
    fontSetCurrent(100);

    _initMousemgr();

    _mousemgrSetNameMangler(_interpretMangleName);

    for (i = 0; i < 64; i++) {
        for (j = 0; j < 256; j++) {
            byte_66E7B0[(i << 8) + j] = ((i * j) >> 9);
        }
    }
}

// 0x4B947C
void _windowClose()
{
    // TODO: Incomplete, but required for graceful exit.

    for (int index = 0; index < 16; index++) {
        STRUCT_6727B0* ptr = &(stru_6727B0[index]);
        if (ptr->window != -1) {
            // _deleteWindow(ptr);
        }
    }

    dbExit();
    windowManagerExit();
}

// Deletes button with the specified name or all buttons if it's NULL.
//
// 0x4B9548
bool _windowDeleteButton(const char* buttonName)
{
    if (dword_51DCB8 != -1) {
        return false;
    }

    STRUCT_6727B0* ptr = &(stru_6727B0[dword_51DCB8]);
    if (ptr->buttonsLength == 0) {
        return false;
    }

    if (buttonName == NULL) {
        for (int index = 0; index < ptr->buttonsLength; index++) {
            ManagedButton* managedButton = &(ptr->buttons[index]);
            buttonDestroy(managedButton->btn);

            if (managedButton->field_48 != NULL) {
                internal_free_safe(managedButton->field_48, __FILE__, __LINE__); // "..\int\WINDOW.C", 1648
                managedButton->field_48 = NULL;
            }

            if (managedButton->field_4C != NULL) {
                internal_free_safe(managedButton->field_4C, __FILE__, __LINE__); // "..\int\WINDOW.C", 1649
                managedButton->field_4C = NULL;
            }

            if (managedButton->field_40 != NULL) {
                internal_free_safe(managedButton->field_40, __FILE__, __LINE__); // "..\int\WINDOW.C", 1650
                managedButton->field_40 = NULL;
            }

            if (managedButton->field_44 != NULL) {
                internal_free_safe(managedButton->field_44, __FILE__, __LINE__); // "..\int\WINDOW.C", 1651
                managedButton->field_44 = NULL;
            }

            if (managedButton->field_50 != NULL) {
                internal_free_safe(managedButton->field_44, __FILE__, __LINE__); // "..\int\WINDOW.C", 1652
                managedButton->field_50 = NULL;
            }
        }

        internal_free_safe(ptr->buttons, __FILE__, __LINE__); // "..\int\WINDOW.C", 1654
        ptr->buttons = NULL;
        ptr->buttonsLength = 0;

        return true;
    }

    for (int index = 0; index < ptr->buttonsLength; index++) {
        ManagedButton* managedButton = &(ptr->buttons[index]);
        if (stricmp(managedButton->name, buttonName) == 0) {
            buttonDestroy(managedButton->btn);

            if (managedButton->field_48 != NULL) {
                internal_free_safe(managedButton->field_48, __FILE__, __LINE__); // "..\int\WINDOW.C", 1665
                managedButton->field_48 = NULL;
            }

            if (managedButton->field_4C != NULL) {
                internal_free_safe(managedButton->field_4C, __FILE__, __LINE__); // "..\int\WINDOW.C", 1666
                managedButton->field_4C = NULL;
            }

            if (managedButton->field_40 != NULL) {
                internal_free_safe(managedButton->field_40, __FILE__, __LINE__); // "..\int\WINDOW.C", 1667
                managedButton->field_40 = NULL;
            }

            if (managedButton->field_44 != NULL) {
                internal_free_safe(managedButton->field_44, __FILE__, __LINE__); // "..\int\WINDOW.C", 1668
                managedButton->field_44 = NULL;
            }

            // FIXME: Probably leaking field_50. It's freed when deleting all
            // buttons, but not the specific button.

            if (index != ptr->buttonsLength - 1) {
                // Move remaining buttons up. The last item is not reclaimed.
                memcpy(ptr->buttons + index, ptr->buttons + index + 1, sizeof(*(ptr->buttons)) * (ptr->buttonsLength - index - 1));
            }

            ptr->buttonsLength--;
            if (ptr->buttonsLength == 0) {
                internal_free_safe(ptr->buttons, __FILE__, __LINE__); // "..\int\WINDOW.C", 1672
                ptr->buttons = NULL;
            }

            return true;
        }
    }

    return false;
}

// 0x4B9928
bool _windowSetButtonFlag(const char* buttonName, int value)
{
    if (dword_51DCB8 != -1) {
        return false;
    }

    STRUCT_6727B0* ptr = &(stru_6727B0[dword_51DCB8]);
    if (ptr->buttons == NULL) {
        return false;
    }

    for (int index = 0; index < ptr->buttonsLength; index++) {
        ManagedButton* managedButton = &(ptr->buttons[index]);
        if (stricmp(managedButton->name, buttonName) == 0) {
            managedButton->flags |= value;
            return true;
        }
    }

    return false;
}

// 0x4BA11C
bool _windowAddButtonProc(const char* buttonName, Program* program, int a3, int a4, int a5, int a6)
{
    if (dword_51DCB8 != -1) {
        return false;
    }

    STRUCT_6727B0* ptr = &(stru_6727B0[dword_51DCB8]);
    if (ptr->buttons == NULL) {
        return false;
    }

    for (int index = 0; index < ptr->buttonsLength; index++) {
        ManagedButton* managedButton = &(ptr->buttons[index]);
        if (stricmp(managedButton->name, buttonName) == 0) {
            managedButton->field_5C = a3;
            managedButton->field_60 = a4;
            managedButton->field_54 = a5;
            managedButton->field_58 = a6;
            managedButton->program = program;
            return true;
        }
    }

    return false;
}

// 0x4BA1B4
bool _windowAddButtonRightProc(const char* buttonName, Program* program, int a3, int a4)
{
    if (dword_51DCB8 != -1) {
        return false;
    }

    STRUCT_6727B0* ptr = &(stru_6727B0[dword_51DCB8]);
    if (ptr->buttons == NULL) {
        return false;
    }

    for (int index = 0; index < ptr->buttonsLength; index++) {
        ManagedButton* managedButton = &(ptr->buttons[index]);
        if (stricmp(managedButton->name, buttonName) == 0) {
            managedButton->field_68 = a4;
            managedButton->field_64 = a3;
            managedButton->program = program;
            return true;
        }
    }

    return false;
}

// TODO: There is a value returned, not sure which one - could be either
// currentRegionIndex or points array. For now it can be safely ignored since
// the only caller of this function is opAddRegion, which ignores the returned
// value.
//
// 0x4BA844
void _windowEndRegion()
{
    STRUCT_6727B0* ptr = &(stru_6727B0[dword_51DCB8]);
    Region* region = ptr->regions[ptr->currentRegionIndex];
    _windowAddRegionPoint(region->points->x, region->points->y, false);
    _regionSetBound(region);
}

// 0x4BA988
bool _windowCheckRegionExists(const char* regionName)
{
    if (dword_51DCB8 == -1) {
        return false;
    }

    STRUCT_6727B0* ptr = &(stru_6727B0[dword_51DCB8]);
    if (ptr->window == -1) {
        return false;
    }

    for (int index = 0; index < ptr->regionsLength; index++) {
        Region* region = ptr->regions[index];
        if (region != NULL) {
            if (stricmp(regionGetName(region), regionName) == 0) {
                return true;
            }
        }
    }

    return false;
}

// 0x4BA9FC
bool _windowStartRegion(int initialCapacity)
{
    if (dword_51DCB8 == -1) {
        return false;
    }

    int newRegionIndex;
    STRUCT_6727B0* ptr = &(stru_6727B0[dword_51DCB8]);
    if (ptr->regions == NULL) {
        ptr->regions = internal_malloc_safe(sizeof(&(ptr->regions)), __FILE__, __LINE__); // "..\int\WINDOW.C", 2167
        ptr->regionsLength = 1;
        newRegionIndex = 0;
    } else {
        newRegionIndex = 0;
        for (int index = 0; index < ptr->regionsLength; index++) {
            if (ptr->regions[index] == NULL) {
                break;
            }
            newRegionIndex++;
        }

        if (newRegionIndex == ptr->regionsLength) {
            ptr->regions = internal_realloc_safe(ptr->regions, sizeof(&(ptr->regions)) * (ptr->regionsLength + 1), __FILE__, __LINE__); // "..\int\WINDOW.C", 2178
            ptr->regionsLength++;
        }
    }

    Region* newRegion;
    if (initialCapacity != 0) {
        newRegion = regionCreate(initialCapacity + 1);
    } else {
        newRegion = NULL;
    }

    ptr->regions[newRegionIndex] = newRegion;
    ptr->currentRegionIndex = newRegionIndex;

    return true;
}

// 0x4BAB68
bool _windowAddRegionPoint(int x, int y, bool a3)
{
    if (dword_51DCB8 == -1) {
        return false;
    }

    STRUCT_6727B0* ptr = &(stru_6727B0[dword_51DCB8]);
    Region* region = ptr->regions[ptr->currentRegionIndex];
    if (region == NULL) {
        region = ptr->regions[ptr->currentRegionIndex] = regionCreate(1);
    }

    if (a3) {
        x = (int)(x * ptr->field_54);
        y = (int)(y * ptr->field_58);
    }

    regionAddPoint(region, x, y);

    return true;
}

// 0x4BADC0
bool _windowAddRegionProc(const char* regionName, Program* program, int a3, int a4, int a5, int a6)
{
    if (dword_51DCB8 == -1) {
        return false;
    }

    STRUCT_6727B0* ptr = &(stru_6727B0[dword_51DCB8]);
    for (int index = 0; index < ptr->regionsLength; index++) {
        Region* region = ptr->regions[index];
        if (region != NULL) {
            if (stricmp(region->name, regionName) == 0) {
                region->field_50 = a3;
                region->field_54 = a4;
                region->field_48 = a5;
                region->field_4C = a6;
                region->program = program;
                return true;
            }
        }
    }

    return false;
}

// 0x4BAE8C
bool _windowAddRegionRightProc(const char* regionName, Program* program, int a3, int a4)
{
    if (dword_51DCB8 == -1) {
        return false;
    }

    STRUCT_6727B0* ptr = &(stru_6727B0[dword_51DCB8]);
    for (int index = 0; index < ptr->regionsLength; index++) {
        Region* region = ptr->regions[index];
        if (region != NULL) {
            if (stricmp(region->name, regionName) == 0) {
                region->field_58 = a3;
                region->field_5C = a4;
                region->program = program;
                return true;
            }
        }
    }

    return false;
}

// 0x4BAF2C
bool _windowSetRegionFlag(const char* regionName, int value)
{
    if (dword_51DCB8 != -1) {
        STRUCT_6727B0* ptr = &(stru_6727B0[dword_51DCB8]);
        for (int index = 0; index < ptr->regionsLength; index++) {
            Region* region = ptr->regions[index];
            if (region != NULL) {
                if (stricmp(region->name, regionName) == 0) {
                    regionAddFlag(region, value);
                    return true;
                }
            }
        }
    }

    return false;
}

// 0x4BAFA8
bool _windowAddRegionName(const char* regionName)
{
    if (dword_51DCB8 == -1) {
        return false;
    }

    STRUCT_6727B0* ptr = &(stru_6727B0[dword_51DCB8]);
    Region* region = ptr->regions[ptr->currentRegionIndex];
    if (region == NULL) {
        return false;
    }

    for (int index = 0; index < ptr->regionsLength; index++) {
        if (index != ptr->currentRegionIndex) {
            Region* other = ptr->regions[index];
            if (other != NULL) {
                if (stricmp(regionGetName(other), regionName) == 0) {
                    regionDelete(other);
                    ptr->regions[index] = NULL;
                    break;
                }
            }
        }
    }

    regionSetName(region, regionName);

    return true;
}

// Delete region with the specified name or all regions if it's NULL.
//
// 0x4BB0A8
bool _windowDeleteRegion(const char* regionName)
{
    if (dword_51DCB8 == -1) {
        return false;
    }

    STRUCT_6727B0* ptr = &(stru_6727B0[dword_51DCB8]);
    if (ptr->window == -1) {
        return false;
    }

    if (regionName != NULL) {
        for (int index = 0; index < ptr->regionsLength; index++) {
            Region* region = ptr->regions[index];
            if (region != NULL) {
                if (stricmp(regionGetName(region), regionName) == 0) {
                    regionDelete(region);
                    ptr->regions[index] = NULL;
                    ptr->field_38++;
                    return true;
                }
            }
        }
        return false;
    }

    ptr->field_38++;

    if (ptr->regions != NULL) {
        for (int index = 0; index < ptr->regionsLength; index++) {
            Region* region = ptr->regions[index];
            if (region != NULL) {
                regionDelete(region);
            }
        }

        internal_free_safe(ptr->regions, __FILE__, __LINE__); // "..\int\WINDOW.C", 2353

        ptr->regions = NULL;
        ptr->regionsLength = 0;
    }

    return true;
}

// 0x4BB220
void _updateWindows()
{
    _movieUpdate();
    // TODO: Incomplete.
    // _mousemgrUpdate();
    // _checkAllRegions();
    _update_widgets();
}

// 0x4BB234
int _windowMoviePlaying()
{
    return _moviePlaying();
}

// 0x4BB23C
bool _windowSetMovieFlags(int flags)
{
    if (movieSetFlags(flags) != 0) {
        return false;
    }
    
    return true;
}

// 0x4BB24C
bool _windowPlayMovie(char* filePath)
{
    if (_movieRun(stru_6727B0[dword_51DCB8].window, filePath) != 0) {
        return false;
    }

    return true;
}

// 0x4BB280
bool _windowPlayMovieRect(char* filePath, int a2, int a3, int a4, int a5)
{
    if (_movieRunRect(stru_6727B0[dword_51DCB8].window, filePath, a2, a3, a4, a5) != 0) {
        return false;
    }

    return true;
}

// 0x4BB2C4
void _windowStopMovie()
{
    _movieStop();
}
