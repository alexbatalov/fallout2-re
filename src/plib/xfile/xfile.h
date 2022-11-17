#ifndef FALLOUT_PLIB_XFILE_XFILE_H_
#define FALLOUT_PLIB_XFILE_XFILE_H_

#include <stdbool.h>
#include <stdio.h>

#include <zlib.h>

#include "dfile.h"

typedef enum XFileType {
    XFILE_TYPE_FILE,
    XFILE_TYPE_DFILE,
    XFILE_TYPE_GZFILE,
} XFileType;

// A universal database of files.
typedef struct XBase {
    // The path to directory or .DAT file that this xbase represents.
    char* path;

    // The [DBase] instance that this xbase represents.
    DBase* dbase;

    // A flag used to denote that this xbase represents .DAT file (true), or
    // a directory (false).
    //
    // NOTE: Original type is 1 byte, likely unsigned char.
    bool isDbase;

    // Next [XBase] in linked list.
    struct XBase* next;
} XBase;

typedef struct XFile {
    XFileType type;
    union {
        FILE* file;
        DFile* dfile;
        gzFile gzfile;
    };
} XFile;

typedef struct XList {
    int fileNamesLength;
    char** fileNames;
} XList;

typedef enum XFileEnumerationEntryType {
    XFILE_ENUMERATION_ENTRY_TYPE_FILE,
    XFILE_ENUMERATION_ENTRY_TYPE_DIRECTORY,
    XFILE_ENUMERATION_ENTRY_TYPE_DFILE,
} XFileEnumerationEntryType;

typedef struct XListEnumerationContext {
    char name[FILENAME_MAX];
    unsigned char type;
    XList* xlist;
} XListEnumerationContext;

typedef bool(XListEnumerationHandler)(XListEnumerationContext* context);

int xfclose(XFile* stream);
XFile* xfopen(const char* filename, const char* mode);
int xfprintf(XFile* xfile, const char* format, ...);
int xvfprintf(XFile* stream, const char* format, va_list args);
int xfgetc(XFile* stream);
char* xfgets(char* string, int size, XFile* stream);
int xfputc(int ch, XFile* stream);
int xfputs(const char* s, XFile* stream);
size_t xfread(void* ptr, size_t size, size_t count, XFile* stream);
size_t xfwrite(const void* buf, size_t size, size_t count, XFile* stream);
int xfseek(XFile* stream, long offset, int origin);
long xftell(XFile* stream);
void xrewind(XFile* stream);
int xfeof(XFile* stream);
long xfilelength(XFile* stream);
bool xsetpath(char* paths);
bool xaddpath(const char* path);
bool xenumpath(const char* pattern, XListEnumerationHandler* handler, XList* xlist);
bool xbuild_filelist(const char* pattern, XList* xlist);
void xfree_filelist(XList* xlist);
int xmkdir(const char* path);

#endif /* FALLOUT_PLIB_XFILE_XFILE_H_ */
