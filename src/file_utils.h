#ifndef FILE_UTILS_H
#define FILE_UTILS_H

int fileCopyDecompressed(const char* existingFilePath, const char* newFilePath);
int fileCopyCompressed(const char* existingFilePath, const char* newFilePath);
int gzdecompress_file(const char* existingFilePath, const char* newFilePath);

#endif /* FILE_UTILS_H */
