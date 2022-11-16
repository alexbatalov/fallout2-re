#include "game/message.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "plib/gnw/debug.h"
#include "game/gconfig.h"
#include "plib/gnw/memory.h"
#include "game/roll.h"

#define BADWORD_LENGTH_MAX 80

static bool message_find(MessageList* msg, int num, int* out_index);
static bool message_add(MessageList* msg, MessageListItem* new_entry);
static bool message_parse_number(int* out_num, const char* str);
static int message_load_field(File* file, char* str);

// 0x519598
static char** bad_word = NULL;

// 0x51959C
static int bad_total = 0;

// 0x5195A0
static int* bad_len = NULL;

// Temporary message list item text used during filtering badwords.
//
// 0x63207C
static char bad_copy[MESSAGE_LIST_ITEM_FIELD_MAX_SIZE];

// 0x484770
int init_message()
{
    File* stream = fileOpen("data\\badwords.txt", "rt");
    if (stream == NULL) {
        return -1;
    }

    char word[BADWORD_LENGTH_MAX];

    bad_total = 0;
    while (fileReadString(word, BADWORD_LENGTH_MAX - 1, stream)) {
        bad_total++;
    }

    bad_word = (char**)internal_malloc(sizeof(*bad_word) * bad_total);
    if (bad_word == NULL) {
        fileClose(stream);
        return -1;
    }

    bad_len = (int*)internal_malloc(sizeof(*bad_len) * bad_total);
    if (bad_len == NULL) {
        internal_free(bad_word);
        fileClose(stream);
        return -1;
    }

    fileSeek(stream, 0, SEEK_SET);

    int index = 0;
    for (; index < bad_total; index++) {
        if (!fileReadString(word, BADWORD_LENGTH_MAX - 1, stream)) {
            break;
        }

        int len = strlen(word);
        if (word[len - 1] == '\n') {
            len--;
            word[len] = '\0';
        }

        bad_word[index] = internal_strdup(word);
        if (bad_word[index] == NULL) {
            break;
        }

        strupr(bad_word[index]);

        bad_len[index] = len;
    }

    fileClose(stream);

    if (index != bad_total) {
        for (; index > 0; index--) {
            internal_free(bad_word[index - 1]);
        }

        internal_free(bad_word);
        internal_free(bad_len);

        return -1;
    }

    return 0;
}

// 0x4848F0
void exit_message()
{
    for (int index = 0; index < bad_total; index++) {
        internal_free(bad_word[index]);
    }

    if (bad_total != 0) {
        internal_free(bad_word);
        internal_free(bad_len);
    }

    bad_total = 0;
}

// message_init
// 0x48494C
bool message_init(MessageList* messageList)
{
    if (messageList != NULL) {
        messageList->entries_num = 0;
        messageList->entries = NULL;
    }
    return true;
}

// 0x484964
bool message_exit(MessageList* messageList)
{
    int i;
    MessageListItem* entry;

    if (messageList == NULL) {
        return false;
    }

    for (i = 0; i < messageList->entries_num; i++) {
        entry = &(messageList->entries[i]);

        if (entry->audio != NULL) {
            internal_free(entry->audio);
        }

        if (entry->text != NULL) {
            internal_free(entry->text);
        }
    }

    messageList->entries_num = 0;

    if (messageList->entries != NULL) {
        internal_free(messageList->entries);
        messageList->entries = NULL;
    }

    return true;
}

// message_load
// 0x484AA4
bool message_load(MessageList* messageList, const char* path)
{
    char* language;
    char localized_path[FILENAME_MAX];
    File* file_ptr;
    char num[1024];
    char audio[1024];
    char text[1024];
    int rc;
    bool success;
    MessageListItem entry;

    success = false;

    if (messageList == NULL) {
        return false;
    }

    if (path == NULL) {
        return false;
    }

    if (!config_get_string(&game_config, GAME_CONFIG_SYSTEM_KEY, GAME_CONFIG_LANGUAGE_KEY, &language)) {
        return false;
    }

    sprintf(localized_path, "%s\\%s\\%s", "text", language, path);

    file_ptr = fileOpen(localized_path, "rt");
    if (file_ptr == NULL) {
        return false;
    }

    entry.num = 0;
    entry.audio = audio;
    entry.text = text;

    while (1) {
        rc = message_load_field(file_ptr, num);
        if (rc != 0) {
            break;
        }

        if (message_load_field(file_ptr, audio) != 0) {
            debug_printf("\nError loading audio field.\n", localized_path);
            goto err;
        }

        if (message_load_field(file_ptr, text) != 0) {
            debug_printf("\nError loading text field.\n", localized_path);
            goto err;
        }

        if (!message_parse_number(&(entry.num), num)) {
            debug_printf("\nError parsing number.\n", localized_path);
            goto err;
        }

        if (!message_add(messageList, &entry)) {
            debug_printf("\nError adding message.\n", localized_path);
            goto err;
        }
    }

    if (rc == 1) {
        success = true;
    }

err:

    if (!success) {
        debug_printf("Error loading message file %s at offset %x.", localized_path, fileTell(file_ptr));
    }

    fileClose(file_ptr);

    return success;
}

// 0x484C30
bool message_search(MessageList* msg, MessageListItem* entry)
{
    int index;
    MessageListItem* ptr;

    if (msg == NULL) {
        return false;
    }

    if (entry == NULL) {
        return false;
    }

    if (msg->entries_num == 0) {
        return false;
    }

    if (!message_find(msg, entry->num, &index)) {
        return false;
    }

    ptr = &(msg->entries[index]);
    entry->flags = ptr->flags;
    entry->audio = ptr->audio;
    entry->text = ptr->text;

    return true;
}

// Builds language-aware path in "text" subfolder.
//
// 0x484CB8
bool message_make_path(char* dest, const char* path)
{
    char* language;

    if (dest == NULL) {
        return false;
    }

    if (path == NULL) {
        return false;
    }

    if (!config_get_string(&game_config, GAME_CONFIG_SYSTEM_KEY, GAME_CONFIG_LANGUAGE_KEY, &language)) {
        return false;
    }

    sprintf(dest, "%s\\%s\\%s", "text", language, path);

    return true;
}

// 0x484D10
bool message_find(MessageList* msg, int num, int* out_index)
{
    int r, l, mid;
    int cmp;

    if (msg->entries_num == 0) {
        *out_index = 0;
        return false;
    }

    r = msg->entries_num - 1;
    l = 0;

    do {
        mid = (l + r) / 2;
        cmp = num - msg->entries[mid].num;
        if (cmp == 0) {
            *out_index = mid;
            return true;
        }

        if (cmp > 0) {
            l = l + 1;
        } else {
            r = r - 1;
        }
    } while (r >= l);

    if (cmp < 0) {
        *out_index = mid;
    } else {
        *out_index = mid + 1;
    }

    return false;
}

// 0x484D68
bool message_add(MessageList* msg, MessageListItem* new_entry)
{
    int index;
    MessageListItem* entries;
    MessageListItem* existing_entry;

    if (message_find(msg, new_entry->num, &index)) {
        existing_entry = &(msg->entries[index]);

        if (existing_entry->audio != NULL) {
            internal_free(existing_entry->audio);
        }

        if (existing_entry->text != NULL) {
            internal_free(existing_entry->text);
        }
    } else {
        if (msg->entries != NULL) {
            entries = (MessageListItem*)internal_realloc(msg->entries, sizeof(MessageListItem) * (msg->entries_num + 1));
            if (entries == NULL) {
                return false;
            }

            msg->entries = entries;

            if (index != msg->entries_num) {
                // Move all items below insertion point
                memmove(&(msg->entries[index + 1]), &(msg->entries[index]), sizeof(MessageListItem) * (msg->entries_num - index));
            }
        } else {
            msg->entries = (MessageListItem*)internal_malloc(sizeof(MessageListItem));
            if (msg->entries == NULL) {
                return false;
            }
            msg->entries_num = 0;
            index = 0;
        }

        existing_entry = &(msg->entries[index]);
        existing_entry->flags = 0;
        existing_entry->audio = 0;
        existing_entry->text = 0;
        msg->entries_num++;
    }

    existing_entry->audio = internal_strdup(new_entry->audio);
    if (existing_entry->audio == NULL) {
        return false;
    }

    existing_entry->text = internal_strdup(new_entry->text);
    if (existing_entry->text == NULL) {
        return false;
    }

    existing_entry->num = new_entry->num;

    return true;
}

// 0x484F60
bool message_parse_number(int* out_num, const char* str)
{
    const char* ch;
    bool success;

    ch = str;
    if (*ch == '\0') {
        return false;
    }

    success = true;
    if (*ch == '+' || *ch == '-') {
        ch++;
    }

    while (*ch != '\0') {
        if (!isdigit(*ch)) {
            success = false;
            break;
        }
        ch++;
    }

    *out_num = atoi(str);
    return success;
}

// Read next message file field, the `str` should be at least 1024 bytes long.
//
// Returns:
// 0 - ok
// 1 - eof
// 2 - mismatched delimeters
// 3 - unterminated field
// 4 - limit exceeded (> 1024)
//
// 0x484FB4
int message_load_field(File* file, char* str)
{
    int ch;
    int len;

    len = 0;

    while (1) {
        ch = fileReadChar(file);
        if (ch == -1) {
            return 1;
        }

        if (ch == '}') {
            debug_printf("\nError reading message file - mismatched delimiters.\n");
            return 2;
        }

        if (ch == '{') {
            break;
        }
    }

    while (1) {
        ch = fileReadChar(file);

        if (ch == -1) {
            debug_printf("\nError reading message file - EOF reached.\n");
            return 3;
        }

        if (ch == '}') {
            *(str + len) = '\0';
            return 0;
        }

        if (ch != '\n') {
            *(str + len) = ch;
            len++;

            if (len > 1024) {
                debug_printf("\nError reading message file - text exceeds limit.\n");
                return 4;
            }
        }
    }

    return 0;
}

// 0x48504C
char* getmsg(MessageList* msg, MessageListItem* entry, int num)
{
    // 0x5195A4
    static char message_error_str[] = "Error";

    entry->num = num;

    if (!message_search(msg, entry)) {
        entry->text = message_error_str;
        debug_printf("\n ** String not found @ getmsg(), MESSAGE.C **\n");
    }

    return entry->text;
}

// 0x485078
bool message_filter(MessageList* messageList)
{
    // TODO: Check.
    // 0x50B960
    static const char* replacements = "!@#$%&*@#*!&$%#&%#*%!$&%@*$@&";

    if (messageList == NULL) {
        return false;
    }

    if (messageList->entries_num == 0) {
        return true;
    }

    if (bad_total == 0) {
        return true;
    }

    int languageFilter = 0;
    config_get_value(&game_config, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_LANGUAGE_FILTER_KEY, &languageFilter);
    if (languageFilter != 1) {
        return true;
    }

    int replacementsCount = strlen(replacements);
    int replacementsIndex = roll_random(1, replacementsCount) - 1;

    for (int index = 0; index < messageList->entries_num; index++) {
        MessageListItem* item = &(messageList->entries[index]);
        strcpy(bad_copy, item->text);
        strupr(bad_copy);

        for (int badwordIndex = 0; badwordIndex < bad_total; badwordIndex++) {
            // I don't quite understand the loop below. It has no stop
            // condition besides no matching substring. It also overwrites
            // already masked words on every iteration.
            for (char* p = bad_copy;; p++) {
                const char* substr = strstr(p, bad_word[badwordIndex]);
                if (substr == NULL) {
                    break;
                }

                if (substr == bad_copy || (!isalpha(substr[-1]) && !isalpha(substr[bad_len[badwordIndex]]))) {
                    item->flags |= MESSAGE_LIST_ITEM_TEXT_FILTERED;
                    char* ptr = item->text + (substr - bad_copy);

                    for (int j = 0; j < bad_len[badwordIndex]; j++) {
                        *ptr++ = replacements[replacementsIndex++];
                        if (replacementsIndex == replacementsCount) {
                            replacementsIndex = 0;
                        }
                    }
                }
            }
        }
    }

    return true;
}
