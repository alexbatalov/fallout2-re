#ifndef FALLOUT_GAME_STRPARSE_H_
#define FALLOUT_GAME_STRPARSE_H_

typedef int(StringParserCallback)(char* string, int* valuePtr);

int strParseValue(char** stringPtr, int* valuePtr);
int strParseStrFromList(char** stringPtr, int* valuePtr, const char** list, int count);
int strParseStrFromFunc(char** stringPtr, int* valuePtr, StringParserCallback* callback);
int strParseStrSepVal(char** stringPtr, const char* key, int* valuePtr, const char* delimeter);
int strParseStrAndSepVal(char** stringPtr, char* key, int* valuePtr, const char* delimeter);

#endif /* FALLOUT_GAME_STRPARSE_H_ */
