#include "select_file_list.h"

#include "db.h"

#include <stdlib.h>
#include <string.h>

// 0x4AA250
int sub_4AA250(const void* a1, const void* a2)
{
    const char* v1 = *(const char**)a1;
    const char* v2 = *(const char**)a2;
    return strcmp(v1, v2);
}

// 0x4AA2A4
char** sub_4AA2A4(const char* pattern, int* fileNameListLengthPtr)
{
    char** fileNameList;
    int fileNameListLength = fileNameListInit(pattern, &fileNameList, 0, 0);
    *fileNameListLengthPtr = fileNameListLength;
    if (fileNameListLength == 0) {
        return NULL;
    }

    qsort(fileNameList, fileNameListLength, sizeof(*fileNameList), sub_4AA250);

    return fileNameList;
}

// 0x4AA2DC
void sub_4AA2DC(char** fileList)
{
    fileNameListFree(&fileList, 0);
}
