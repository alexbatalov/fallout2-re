#ifndef MESSAGE_H
#define MESSAGE_H

#include "db.h"

#include <stdbool.h>

#define BADWORD_LENGTH_MAX 80

#define MESSAGE_LIST_ITEM_TEXT_FILTERED 0x01

#define MESSAGE_LIST_ITEM_FIELD_MAX_SIZE 1024

typedef struct MessageListItem {
    int num;
    int flags;
    char* audio;
    char* text;
} MessageListItem;

typedef struct MessageList {
    int entries_num;
    MessageListItem* entries;
} MessageList;

extern char byte_50B79C[];
extern const char* gBadwordsReplacements;

extern char** gBadwords;
extern int gBadwordsCount;
extern int* gBadwordsLengths;
extern char* off_5195A4;

extern char byte_63207C[MESSAGE_LIST_ITEM_FIELD_MAX_SIZE];

int badwordsInit();
void badwordsExit();
bool messageListInit(MessageList* msg);
bool messageListFree(MessageList* msg);
bool messageListLoad(MessageList* msg, const char* path);
bool messageListGetItem(MessageList* msg, MessageListItem* entry);
bool sub_484CB8(char* dest, const char* path);
bool sub_484D10(MessageList* msg, int num, int* out_index);
bool sub_484D68(MessageList* msg, MessageListItem* new_entry);
bool sub_484F60(int* out_num, const char* str);
int sub_484FB4(File* file, char* str);
char* getmsg(MessageList* msg, MessageListItem* entry, int num);
bool messageListFilterBadwords(MessageList* messageList);

#endif /* MESSAGE_H */
