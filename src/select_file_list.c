#include "select_file_list.h"

#include "db.h"

#include <stdlib.h>
#include <string.h>

// 0x4AA250
int compare(const void* a1, const void* a2)
{
    const char* v1 = *(const char**)a1;
    const char* v2 = *(const char**)a2;
    return strcmp(v1, v2);
}

// 0x4AA2A4
char** getFileList(const char* pattern, int* fileNameListLengthPtr)
{
    char** fileNameList;
    int fileNameListLength = fileNameListInit(pattern, &fileNameList, 0, 0);
    *fileNameListLengthPtr = fileNameListLength;
    if (fileNameListLength == 0) {
        return NULL;
    }

    qsort(fileNameList, fileNameListLength, sizeof(*fileNameList), compare);

    return fileNameList;
}

// 0x4AA2DC
void freeFileList(char** fileList)
{
    fileNameListFree(&fileList, 0);
}
