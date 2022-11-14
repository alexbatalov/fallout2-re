#ifndef FALLOUT_GAME_MESSAGE_H_
#define FALLOUT_GAME_MESSAGE_H_

#include <stdbool.h>

// TODO: Convert to enum.
#define MESSAGE_LIST_ITEM_TEXT_FILTERED 0x01

// TODO: Probably should be private.
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

int init_message();
void exit_message();
bool message_init(MessageList* msg);
bool message_exit(MessageList* msg);
bool message_load(MessageList* msg, const char* path);
bool message_search(MessageList* msg, MessageListItem* entry);
bool message_make_path(char* dest, const char* path);
char* getmsg(MessageList* msg, MessageListItem* entry, int num);
bool message_filter(MessageList* messageList);

#endif /* FALLOUT_GAME_MESSAGE_H_ */
