#ifndef FALLOUT_GAME_GZ_H_
#define FALLOUT_GAME_GZ_H_

int gzRealUncompressCopyReal_file(const char* existingFilePath, const char* newFilePath);
int gzcompress_file(const char* existingFilePath, const char* newFilePath);
int gzdecompress_file(const char* existingFilePath, const char* newFilePath);

#endif /* FALLOUT_GAME_GZ_H_ */
