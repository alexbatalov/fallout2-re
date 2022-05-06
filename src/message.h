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
bool message_make_path(char* dest, const char* path);
bool message_find(MessageList* msg, int num, int* out_index);
bool message_add(MessageList* msg, MessageListItem* new_entry);
bool message_parse_number(int* out_num, const char* str);
int message_load_field(File* file, char* str);
char* getmsg(MessageList* msg, MessageListItem* entry, int num);
bool messageListFilterBadwords(MessageList* messageList);

#endif /* MESSAGE_H */
