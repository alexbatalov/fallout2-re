#include "plib/db/db.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "plib/xfile/xfile.h"

static int db_list_compare(const void* p1, const void* p2);

// Generic file progress report handler.
//
// 0x51DEEC
static FileReadProgressHandler* read_callback = NULL;

// Bytes read so far while tracking progress.
//
// Once this value reaches [read_threshold] the handler is called
// and this value resets to zero.
//
// 0x51DEF0
static int read_counter = 0;

// The number of bytes to read between calls to progress handler.
//
// 0x673040
static int read_threshold;

// 0x673044
static FileList* db_file_lists;

// Opens file database.
//
// Returns -1 if [filePath1] was specified, but could not be opened by the
// underlying xbase implementation. Result of opening [filePath2] is ignored.
// Returns 0 on success.
//
// NOTE: There are two unknown parameters passed via edx and ecx. The [a2] is
// always 0 at the calling sites, and [a4] is always 1. Both parameters are not
// used, so it's impossible to figure out their meaning.
//
// 0x4C5D30
int db_init(const char* filePath1, int a2, const char* filePath2, int a4)
{
    if (filePath1 != NULL) {
        if (!xaddpath(filePath1)) {
            return -1;
        }
    }

    if (filePath2 != NULL) {
        xaddpath(filePath2);
    }

    return 0;
}

// 0x4C5D54
int db_select(int dbHandle)
{
    return 0;
}

// NOTE: Uncollapsed 0x4C5D54.
int db_current()
{
    return 0;
}

// 0x4C5D58
int db_total()
{
    return 0;
}

// 0x4C5D60
void db_exit()
{
    xsetpath(NULL);
}

// TODO: sizePtr should be long*.
//
// 0x4C5D68
int db_dir_entry(const char* filePath, int* sizePtr)
{
    assert(filePath); // "filename", "db.c", 108
    assert(sizePtr); // "de", "db.c", 109

    File* stream = xfopen(filePath, "rb");
    if (stream == NULL) {
        return -1;
    }

    *sizePtr = xfilelength(stream);

    xfclose(stream);

    return 0;
}

// 0x4C5DD4
int db_read_to_buf(const char* filePath, void* ptr)
{
    assert(filePath); // "filename", "db.c", 141
    assert(ptr); // "buf", "db.c", 142

    File* stream = xfopen(filePath, "rb");
    if (stream == NULL) {
        return -1;
    }

    long size = xfilelength(stream);
    if (read_callback != NULL) {
        unsigned char* byteBuffer = (unsigned char*)ptr;

        long remainingSize = size;
        long chunkSize = read_threshold - read_counter;

        while (remainingSize >= chunkSize) {
            size_t bytesRead = xfread(byteBuffer, sizeof(*byteBuffer), chunkSize, stream);
            byteBuffer += bytesRead;
            remainingSize -= bytesRead;

            read_counter = 0;
            read_callback();

            chunkSize = read_threshold;
        }

        if (remainingSize != 0) {
            read_counter += xfread(byteBuffer, sizeof(*byteBuffer), remainingSize, stream);
        }
    } else {
        xfread(ptr, 1, size, stream);
    }

    xfclose(stream);

    return 0;
}

// 0x4C5EB4
int db_fclose(File* stream)
{
    return xfclose(stream);
}

// 0x4C5EC8
File* db_fopen(const char* filename, const char* mode)
{
    return xfopen(filename, mode);
}

// 0x4C5ED0
int db_fprintf(File* stream, const char* format, ...)
{
    assert(format); // "format", "db.c", 224

    va_list args;
    va_start(args, format);

    int rc = xvfprintf(stream, format, args);

    va_end(args);

    return rc;
}

// 0x4C5F24
int db_fgetc(File* stream)
{
    if (read_callback != NULL) {
        int ch = xfgetc(stream);

        read_counter++;
        if (read_counter >= read_threshold) {
            read_callback();
            read_counter = 0;
        }

        return ch;
    }

    return xfgetc(stream);
}

// 0x4C5F70
char* db_fgets(char* string, size_t size, File* stream)
{
    if (read_callback != NULL) {
        if (xfgets(string, size, stream) == NULL) {
            return NULL;
        }

        read_counter += strlen(string);
        while (read_counter >= read_threshold) {
            read_callback();
            read_counter -= read_threshold;
        }

        return string;
    }

    return xfgets(string, size, stream);
}

// 0x4C5FEC
int db_fputs(const char* string, File* stream)
{
    return xfputs(string, stream);
}

// 0x4C5FFC
size_t db_fread(void* ptr, size_t size, size_t count, File* stream)
{
    if (read_callback != NULL) {
        unsigned char* byteBuffer = (unsigned char*)ptr;

        size_t totalBytesRead = 0;
        long remainingSize = size * count;
        long chunkSize = read_threshold - read_counter;

        while (remainingSize >= chunkSize) {
            size_t bytesRead = xfread(byteBuffer, sizeof(*byteBuffer), chunkSize, stream);
            byteBuffer += bytesRead;
            totalBytesRead += bytesRead;
            remainingSize -= bytesRead;

            read_counter = 0;
            read_callback();

            chunkSize = read_threshold;
        }

        if (remainingSize != 0) {
            size_t bytesRead = xfread(byteBuffer, sizeof(*byteBuffer), remainingSize, stream);
            read_counter += bytesRead;
            totalBytesRead += bytesRead;
        }

        return totalBytesRead / size;
    }

    return xfread(ptr, size, count, stream);
}

// 0x4C60B8
size_t db_fwrite(const void* buf, size_t size, size_t count, File* stream)
{
    return xfwrite(buf, size, count, stream);
}

// 0x4C60C0
int db_fseek(File* stream, long offset, int origin)
{
    return xfseek(stream, offset, origin);
}

// 0x4C60C8
long db_ftell(File* stream)
{
    return xftell(stream);
}

// 0x4C60D0
void db_rewind(File* stream)
{
    xrewind(stream);
}

// 0x4C60D8
int db_feof(File* stream)
{
    return xfeof(stream);
}

// NOTE: Not sure about signness.
//
// 0x4C60E0
int db_freadByte(File* stream, unsigned char* valuePtr)
{
    int value = db_fgetc(stream);
    if (value == -1) {
        return -1;
    }

    *valuePtr = value & 0xFF;

    return 0;
}

// NOTE: Not sure about signness.
//
// 0x4C60F4
int db_freadShort(File* stream, unsigned short* valuePtr)
{
    unsigned char high;
    // NOTE: Uninline.
    if (db_freadByte(stream, &high) == -1) {
        return -1;
    }

    unsigned char low;
    // NOTE: Uninline.
    if (db_freadByte(stream, &low) == -1) {
        return -1;
    }

    *valuePtr = (high << 8) | low;

    return 0;
}

// 0x4C614C
int fileReadInt32(File* stream, int* valuePtr)
{
    int value;

    if (xfread(&value, 4, 1, stream) == -1) {
        return -1;
    }

    *valuePtr = ((value >> 24) & 0xFF) | ((value >> 8) & 0xFF00) | ((value << 8) & 0xFF0000) | ((value << 24) & 0xFF000000);

    return 0;
}

// NOTE: Uncollapsed 0x4C614C. The opposite of [db_fwriteLong]. It can be either
// signed vs. unsigned variant, as well as int vs. long. It's provided here to
// identify places where data was written with [db_fwriteLong].
int _db_freadInt(File* stream, int* valuePtr)
{
    return fileReadInt32(stream, valuePtr);
}

// NOTE: Probably uncollapsed 0x4C614C.
int fileReadUInt32(File* stream, unsigned int* valuePtr)
{
    return _db_freadInt(stream, (int*)valuePtr);
}

// NOTE: Uncollapsed 0x4C614C. The opposite of [db_fwriteFloat].
int db_freadFloat(File* stream, float* valuePtr)
{
    return fileReadInt32(stream, (int*)valuePtr);
}

int fileReadBool(File* stream, bool* valuePtr)
{
    int value;
    if (fileReadInt32(stream, &value) == -1) {
        return -1;
    }

    *valuePtr = (value != 0);

    return 0;
}

// NOTE: Not sure about signness.
//
// 0x4C61AC
int db_fwriteByte(File* stream, unsigned char value)
{
    return xfputc(value, stream);
};

// 0x4C61C8
int db_fwriteShort(File* stream, unsigned short value)
{
    // NOTE: Uninline.
    if (db_fwriteByte(stream, (value >> 8) & 0xFF) == -1) {
        return -1;
    }

    // NOTE: Uninline.
    if (db_fwriteByte(stream, value & 0xFF) == -1) {
        return -1;
    }

    return 0;
}

// 0x4C6214
int db_fwriteInt(File* stream, int value)
{
    // NOTE: Uninline.
    return db_fwriteLong(stream, value);
}

// 0x4C6244
int db_fwriteLong(File* stream, unsigned long value)
{
    if (db_fwriteShort(stream, (value >> 16) & 0xFFFF) == -1) {
        return -1;
    }

    if (db_fwriteShort(stream, value & 0xFFFF) == -1) {
        return -1;
    }

    return 0;
}

// 0x4C62C4
int db_fwriteFloat(File* stream, float value)
{
    // NOTE: Uninline.
    return db_fwriteLong(stream, *(unsigned long*)&value);
}

int fileWriteBool(File* stream, bool value)
{
    return db_fwriteLong(stream, value ? 1 : 0);
}

// 0x4C62FC
int db_freadByteCount(File* stream, unsigned char* arr, int count)
{
    for (int index = 0; index < count; index++) {
        unsigned char ch;
        // NOTE: Uninline.
        if (db_freadByte(stream, &ch) == -1) {
            return -1;
        }

        arr[index] = ch;
    }

    return 0;
}

// 0x4C6330
int db_freadShortCount(File* stream, unsigned short* arr, int count)
{
    for (int index = 0; index < count; index++) {
        short value;
        // NOTE: Uninline.
        if (db_freadShort(stream, &value) == -1) {
            return -1;
        }

        arr[index] = value;
    }

    return 0;
}

// NOTE: Not sure about signed/unsigned int/long.
//
// 0x4C63BC
int fileReadInt32List(File* stream, int* arr, int count)
{
    if (count == 0) {
        return 0;
    }

    if (db_fread(arr, sizeof(*arr) * count, 1, stream) < 1) {
        return -1;
    }

    for (int index = 0; index < count; index++) {
        int value = arr[index];
        arr[index] = ((value >> 24) & 0xFF) | ((value >> 8) & 0xFF00) | ((value << 8) & 0xFF0000) | ((value << 24) & 0xFF000000);
    }

    return 0;
}

// NOTE: Uncollapsed 0x4C63BC.
int db_freadLongCount(File* stream, unsigned long* arr, int count)
{
    return fileReadInt32List(stream, arr, count);
}

// NOTE: Probably uncollapsed 0x4C63BC.
int fileReadUInt32List(File* stream, unsigned int* arr, int count)
{
    return fileReadInt32List(stream, (int*)arr, count);
}

// 0x4C6464
int db_fwriteByteCount(File* stream, unsigned char* arr, int count)
{
    for (int index = 0; index < count; index++) {
        // NOTE: Uninline.
        if (db_fwriteByte(stream, arr[index]) == -1) {
            return -1;
        }
    }

    return 0;
}

// 0x4C6490
int db_fwriteShortCount(File* stream, unsigned short* arr, int count)
{
    for (int index = 0; index < count; index++) {
        // NOTE: Uninline.
        if (db_fwriteShort(stream, arr[index]) == -1) {
            return -1;
        }
    }

    return 0;
}

// NOTE: Can be either signed/unsigned + int/long variant.
//
// 0x4C64F8
int fileWriteInt32List(File* stream, int* arr, int count)
{
    for (int index = 0; index < count; index++) {
        // NOTE: Uninline.
        if (db_fwriteLong(stream, arr[index]) == -1) {
            return -1;
        }
    }

    return 0;
}

// 0x4C6550
int db_fwriteLongCount(File* stream, unsigned long* arr, int count)
{
    for (int index = 0; index < count; index++) {
        int value = arr[index];

        // NOTE: Uninline.
        if (db_fwriteShort(stream, (value >> 16) & 0xFFFF) == -1) {
            return -1;
        }

        // NOTE: Uninline.
        if (db_fwriteShort(stream, value & 0xFFFF) == -1) {
            return -1;
        }
    }

    return 0;
}

// NOTE: Probably uncollapsed 0x4C64F8 or 0x4C6550.
int fileWriteUInt32List(File* stream, unsigned int* arr, int count)
{
    return fileWriteInt32List(stream, (int*)arr, count);
}

// 0x4C6628
int db_get_file_list(const char* pattern, char*** fileNameListPtr, int a3, int a4)
{
    FileList* fileList = (FileList*)malloc(sizeof(*fileList));
    if (fileList == NULL) {
        return 0;
    }

    memset(fileList, 0, sizeof(*fileList));

    XList* xlist = &(fileList->xlist);
    if (!xbuild_filelist(pattern, xlist)) {
        free(fileList);
        return 0;
    }

    int length = 0;
    if (xlist->fileNamesLength != 0) {
        qsort(xlist->fileNames, xlist->fileNamesLength, sizeof(*xlist->fileNames), db_list_compare);

        int fileNamesLength = xlist->fileNamesLength;
        for (int index = 0; index < fileNamesLength - 1; index++) {
            if (stricmp(xlist->fileNames[index], xlist->fileNames[index + 1]) == 0) {
                char* temp = xlist->fileNames[index + 1];
                memmove(&(xlist->fileNames[index + 1]), &(xlist->fileNames[index + 2]), sizeof(*xlist->fileNames) * (xlist->fileNamesLength - index - 1));
                xlist->fileNames[xlist->fileNamesLength - 1] = temp;

                fileNamesLength--;
                index--;
            }
        }

        bool isWildcard = *pattern == '*';

        for (int index = 0; index < fileNamesLength; index += 1) {
            const char* name = xlist->fileNames[index];
            char dir[_MAX_DIR];
            char fileName[_MAX_FNAME];
            char extension[_MAX_EXT];
            _splitpath(name, NULL, dir, fileName, extension);

            if (!isWildcard || *dir == '\0' || strchr(dir, '\\') == NULL) {
                // FIXME: There is a buffer overlow bug in this implementation.
                // `fileNames` entries are dynamically allocated strings
                // themselves produced by `strdup` in `xlistenumfunc`.
                // In some circumstances we can end up placing long file name
                // in a short buffer (if that shorter buffer is alphabetically
                // preceding current file name).
                //
                // It can be easily spotted by creating `a.txt` in the game
                // directory and then trying print character data from character
                // editor. Because of compiler differencies original game will
                // crash immediately, and RE can crash anytime after closing
                // file dialog.
                sprintf(xlist->fileNames[length], "%s%s", fileName, extension);
                length++;
            }
        }
    }

    fileList->next = db_file_lists;
    db_file_lists = fileList;

    *fileNameListPtr = xlist->fileNames;

    return length;
}

// 0x4C6868
void db_free_file_list(char*** fileNameListPtr, int a2)
{
    if (db_file_lists == NULL) {
        return;
    }

    FileList* currentFileList = db_file_lists;
    FileList* previousFileList = db_file_lists;
    while (*fileNameListPtr != currentFileList->xlist.fileNames) {
        previousFileList = currentFileList;
        currentFileList = currentFileList->next;
        if (currentFileList == NULL) {
            return;
        }
    }

    if (previousFileList == db_file_lists) {
        db_file_lists = currentFileList->next;
    } else {
        previousFileList->next = currentFileList->next;
    }

    xfree_filelist(&(currentFileList->xlist));

    free(currentFileList);
}

// NOTE: This function does nothing. It was probably used to set memory procs
// for building file name list.
//
// 0x4C68B8
void db_register_mem(MallocProc* mallocProc, StrdupProc* strdupProc, FreeProc* freeProc)
{
}

// TODO: Return type should be long.
//
// 0x4C68BC
int db_filelength(File* stream)
{
    return xfilelength(stream);
}

// 0x4C68C4
void db_register_callback(FileReadProgressHandler* handler, int size)
{
    if (handler != NULL && size != 0) {
        read_callback = handler;
        read_threshold = size;
    } else {
        read_callback = NULL;
        read_threshold = 0;
    }
}

// NOTE: This function is called when fallout2.cfg has "hashing" enabled, but
// it does nothing. It's impossible to guess it's name.
//
// 0x4C68E4
void db_enable_hash_table()
{
}

// 0x4C68E8
static int db_list_compare(const void* p1, const void* p2)
{
    return stricmp(*(const char**)p1, *(const char**)p2);
}
