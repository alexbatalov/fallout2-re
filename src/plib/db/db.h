#ifndef FALLOUT_PLIB_DB_DB_H_
#define FALLOUT_PLIB_DB_DB_H_

#include <stdbool.h>
#include <stddef.h>

#include "memory_defs.h"
#include "plib/xfile/xfile.h"

typedef XFile File;
typedef void FileReadProgressHandler();
typedef char* StrdupProc(const char* string);

typedef struct FileList {
    XList xlist;
    struct FileList* next;
} FileList;

int db_init(const char* filePath1, int a2, const char* filePath2, int a4);
int db_select(int dbHandle);
int db_current();
int db_total();
void db_exit();
int db_dir_entry(const char* filePath, int* sizePtr);
int db_read_to_buf(const char* filePath, void* ptr);
int db_fclose(File* stream);
File* db_fopen(const char* filename, const char* mode);
int db_fprintf(File* stream, const char* format, ...);
int db_fgetc(File* stream);
char* db_fgets(char* str, size_t size, File* stream);
int db_fputs(const char* s, File* stream);
size_t db_fread(void* buf, size_t size, size_t count, File* stream);
size_t db_fwrite(const void* buf, size_t size, size_t count, File* stream);
int db_fseek(File* stream, long offset, int origin);
long db_ftell(File* stream);
void db_rewind(File* stream);
int db_feof(File* stream);
int db_freadByte(File* stream, unsigned char* valuePtr);
int db_freadShort(File* stream, unsigned short* valuePtr);
int db_freadInt(File* stream, int* valuePtr);
int db_freadLong(File* stream, unsigned long* valuePtr);
int db_freadFloat(File* stream, float* valuePtr);
int fileReadBool(File* stream, bool* valuePtr);
int db_fwriteByte(File* stream, unsigned char value);
int db_fwriteShort(File* stream, unsigned short value);
int db_fwriteInt(File* stream, int value);
int db_fwriteLong(File* stream, unsigned long value);
int db_fwriteFloat(File* stream, float value);
int fileWriteBool(File* stream, bool value);
int db_freadByteCount(File* stream, unsigned char* arr, int count);
int db_freadShortCount(File* stream, unsigned short* arr, int count);
int db_freadIntCount(File* stream, int* arr, int count);
int db_freadLongCount(File* stream, unsigned long* arr, int count);
int db_fwriteByteCount(File* stream, unsigned char* arr, int count);
int db_fwriteShortCount(File* stream, unsigned short* arr, int count);
int db_fwriteIntCount(File* stream, int* arr, int count);
int db_fwriteLongCount(File* stream, unsigned long* arr, int count);
int db_get_file_list(const char* pattern, char*** fileNames, int a3, int a4);
void db_free_file_list(char*** fileNames, int a2);
void db_register_mem(MallocProc* mallocProc, StrdupProc* strdupProc, FreeProc* freeProc);
int db_filelength(File* stream);
void db_register_callback(FileReadProgressHandler* handler, int size);
void db_enable_hash_table();

#endif /* FALLOUT_PLIB_DB_DB_H_ */
