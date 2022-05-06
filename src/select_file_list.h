#ifndef SELECT_FILE_LIST_H
#define SELECT_FILE_LIST_H

int compare(const void* a1, const void* a2);
char** getFileList(const char* pattern, int* fileNameListLengthPtr);
void freeFileList(char** fileList);

#endif /* SELECT_FILE_LIST_H */
