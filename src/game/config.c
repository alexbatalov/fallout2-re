#include "game/config.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "plib/gnw/memory.h"

#define CONFIG_FILE_MAX_LINE_LENGTH 256

// The initial number of sections (or key-value) pairs in the config.
#define CONFIG_INITIAL_CAPACITY 10

static bool config_parse_line(Config* config, char* string);
static bool config_split_line(char* string, char* key, char* value);
static bool config_add_section(Config* config, const char* sectionKey);
static bool config_strip_white_space(char* string);

// 0x42BD90
bool config_init(Config* config)
{
    if (config == NULL) {
        return false;
    }

    if (assoc_init(config, CONFIG_INITIAL_CAPACITY, sizeof(ConfigSection), NULL) != 0) {
        return false;
    }

    return true;
}

// 0x42BDBC
void config_exit(Config* config)
{
    if (config == NULL) {
        return;
    }

    for (int sectionIndex = 0; sectionIndex < config->size; sectionIndex++) {
        assoc_pair* sectionEntry = &(config->list[sectionIndex]);

        ConfigSection* section = (ConfigSection*)sectionEntry->data;
        for (int keyValueIndex = 0; keyValueIndex < section->size; keyValueIndex++) {
            assoc_pair* keyValueEntry = &(section->list[keyValueIndex]);

            char** value = (char**)keyValueEntry->data;
            internal_free(*value);
            *value = NULL;
        }

        assoc_free(section);
    }

    assoc_free(config);
}

// Parses command line argments and adds them into the config.
//
// The expected format of [argv] elements are "[section]key=value", otherwise
// the element is silently ignored.
//
// NOTE: This function trims whitespace in key-value pair, but not in section.
// I don't know if this is intentional or it's bug.
//
// 0x42BE38
bool config_cmd_line_parse(Config* config, int argc, char** argv)
{
    if (config == NULL) {
        return false;
    }

    for (int arg = 0; arg < argc; arg++) {
        char* pch;
        char* string = argv[arg];

        // Find opening bracket.
        pch = strchr(string, '[');
        if (pch == NULL) {
            continue;
        }

        char* sectionKey = pch + 1;

        // Find closing bracket.
        pch = strchr(sectionKey, ']');
        if (pch == NULL) {
            continue;
        }

        *pch = '\0';

        char key[260];
        char value[260];
        if (config_split_line(pch + 1, key, value)) {
            if (!config_set_string(config, sectionKey, key, value)) {
                *pch = ']';
                return false;
            }
        }

        *pch = ']';
    }

    return true;
}

// 0x42BF48
bool config_get_string(Config* config, const char* sectionKey, const char* key, char** valuePtr)
{
    if (config == NULL || sectionKey == NULL || key == NULL || valuePtr == NULL) {
        return false;
    }

    int sectionIndex = assoc_search(config, sectionKey);
    if (sectionIndex == -1) {
        return false;
    }

    assoc_pair* sectionEntry = &(config->list[sectionIndex]);
    ConfigSection* section = (ConfigSection*)sectionEntry->data;

    int index = assoc_search(section, key);
    if (index == -1) {
        return false;
    }

    assoc_pair* keyValueEntry = &(section->list[index]);
    *valuePtr = *(char**)keyValueEntry->data;

    return true;
}

// 0x42BF90
bool config_set_string(Config* config, const char* sectionKey, const char* key, const char* value)
{
    if (config == NULL || sectionKey == NULL || key == NULL || value == NULL) {
        return false;
    }

    int sectionIndex = assoc_search(config, sectionKey);
    if (sectionIndex == -1) {
        // FIXME: Looks like a bug, this function never returns -1, which will
        // eventually lead to crash.
        if (config_add_section(config, sectionKey) == -1) {
            return false;
        }
        sectionIndex = assoc_search(config, sectionKey);
    }

    assoc_pair* sectionEntry = &(config->list[sectionIndex]);
    ConfigSection* section = (ConfigSection*)sectionEntry->data;

    int index = assoc_search(section, key);
    if (index != -1) {
        assoc_pair* keyValueEntry = &(section->list[index]);

        char** existingValue = (char**)keyValueEntry->data;
        internal_free(*existingValue);
        *existingValue = NULL;

        assoc_delete(section, key);
    }

    char* valueCopy = internal_strdup(value);
    if (valueCopy == NULL) {
        return false;
    }

    if (assoc_insert(section, key, &valueCopy) == -1) {
        internal_free(valueCopy);
        return false;
    }

    return true;
}

// 0x42C05C
bool config_get_value(Config* config, const char* sectionKey, const char* key, int* valuePtr)
{
    if (valuePtr == NULL) {
        return false;
    }

    char* stringValue;
    if (!config_get_string(config, sectionKey, key, &stringValue)) {
        return false;
    }

    *valuePtr = atoi(stringValue);

    return true;
}

// 0x42C090
bool config_get_values(Config* config, const char* sectionKey, const char* key, int* arr, int count)
{
    if (arr == NULL || count < 2) {
        return false;
    }

    char* string;
    if (!config_get_string(config, sectionKey, key, &string)) {
        return false;
    }

    char temp[CONFIG_FILE_MAX_LINE_LENGTH];
    string = strncpy(temp, string, CONFIG_FILE_MAX_LINE_LENGTH - 1);

    while (1) {
        char* pch = strchr(string, ',');
        if (pch == NULL) {
            break;
        }

        count--;
        if (count == 0) {
            break;
        }

        *pch = '\0';
        *arr++ = atoi(string);
        string = pch + 1;
    }

    if (count <= 1) {
        *arr = atoi(string);
        return true;
    }

    return false;
}

// 0x42C160
bool config_set_value(Config* config, const char* sectionKey, const char* key, int value)
{
    char stringValue[20];
    itoa(value, stringValue, 10);

    return config_set_string(config, sectionKey, key, stringValue);
}

// Reads .INI file into config.
//
// 0x42C280
bool config_load(Config* config, const char* filePath, bool isDb)
{
    if (config == NULL || filePath == NULL) {
        return false;
    }

    char string[CONFIG_FILE_MAX_LINE_LENGTH];

    if (isDb) {
        File* stream = fileOpen(filePath, "rb");
        if (stream != NULL) {
            while (fileReadString(string, sizeof(string), stream) != NULL) {
                config_parse_line(config, string);
            }
            fileClose(stream);
        }
    } else {
        FILE* stream = fopen(filePath, "rt");
        if (stream != NULL) {
            while (fgets(string, sizeof(string), stream) != NULL) {
                config_parse_line(config, string);
            }

            fclose(stream);
        }

        // FIXME: This function returns `true` even if the file was not actually
        // read. I'm pretty sure it's bug.
    }

    return true;
}

// Writes config into .INI file.
//
// 0x42C324
bool config_save(Config* config, const char* filePath, bool isDb)
{
    if (config == NULL || filePath == NULL) {
        return false;
    }

    if (isDb) {
        File* stream = fileOpen(filePath, "wt");
        if (stream == NULL) {
            return false;
        }

        for (int sectionIndex = 0; sectionIndex < config->size; sectionIndex++) {
            assoc_pair* sectionEntry = &(config->list[sectionIndex]);
            filePrintFormatted(stream, "[%s]\n", sectionEntry->name);

            ConfigSection* section = (ConfigSection*)sectionEntry->data;
            for (int index = 0; index < section->size; index++) {
                assoc_pair* keyValueEntry = &(section->list[index]);
                filePrintFormatted(stream, "%s=%s\n", keyValueEntry->name, *(char**)keyValueEntry->data);
            }

            filePrintFormatted(stream, "\n");
        }

        fileClose(stream);
    } else {
        FILE* stream = fopen(filePath, "wt");
        if (stream == NULL) {
            return false;
        }

        for (int sectionIndex = 0; sectionIndex < config->size; sectionIndex++) {
            assoc_pair* sectionEntry = &(config->list[sectionIndex]);
            fprintf(stream, "[%s]\n", sectionEntry->name);

            ConfigSection* section = (ConfigSection*)sectionEntry->data;
            for (int index = 0; index < section->size; index++) {
                assoc_pair* keyValueEntry = &(section->list[index]);
                fprintf(stream, "%s=%s\n", keyValueEntry->name, *(char**)keyValueEntry->data);
            }

            fprintf(stream, "\n");
        }

        fclose(stream);
    }

    return true;
}

// Parses a line from .INI file into config.
//
// A line either contains a "[section]" section key or "key=value" pair. In the
// first case section key is not added to config immediately, instead it is
// stored in |section| for later usage. This prevents empty
// sections in the config.
//
// In case of key-value pair it pretty straight forward - it adds key-value
// pair into previously read section key stored in |section|.
//
// Returns `true` when a section was parsed or key-value pair was parsed and
// added to the config, or `false` otherwise.
//
// 0x42C4BC
static bool config_parse_line(Config* config, char* string)
{
    // 0x518224
    static char section[CONFIG_FILE_MAX_LINE_LENGTH] = "unknown";

    char* pch;

    // Find comment marker and truncate the string.
    pch = strchr(string, ';');
    if (pch != NULL) {
        *pch = '\0';
    }

    // Find opening bracket.
    pch = strchr(string, '[');
    if (pch != NULL) {
        char* sectionKey = pch + 1;

        // Find closing bracket.
        pch = strchr(sectionKey, ']');
        if (pch != NULL) {
            *pch = '\0';
            strcpy(section, sectionKey);
            return config_strip_white_space(section);
        }
    }

    char key[260];
    char value[260];
    if (!config_split_line(string, key, value)) {
        return false;
    }

    return config_set_string(config, section, key, value);
}

// Splits "key=value" pair from [string] and copy appropriate parts into [key]
// and [value] respectively.
//
// Both key and value are trimmed.
//
// 0x42C594
static bool config_split_line(char* string, char* key, char* value)
{
    if (string == NULL || key == NULL || value == NULL) {
        return false;
    }

    // Find equals character.
    char* pch = strchr(string, '=');
    if (pch == NULL) {
        return false;
    }

    *pch = '\0';

    strcpy(key, string);
    strcpy(value, pch + 1);

    *pch = '=';

    config_strip_white_space(key);
    config_strip_white_space(value);

    return true;
}

// Ensures the config has a section with specified key.
//
// Return `true` if section exists or it was successfully added, or `false`
// otherwise.
//
// 0x42C638
static bool config_add_section(Config* config, const char* sectionKey)
{
    if (config == NULL || sectionKey == NULL) {
        return false;
    }

    if (assoc_search(config, sectionKey) != -1) {
        // Section already exists, no need to do anything.
        return true;
    }

    ConfigSection section;
    if (assoc_init(&section, CONFIG_INITIAL_CAPACITY, sizeof(char**), NULL) == -1) {
        return false;
    }

    if (assoc_insert(config, sectionKey, &section) == -1) {
        return false;
    }

    return true;
}

// Removes leading and trailing whitespace from the specified string.
//
// 0x42C698
static bool config_strip_white_space(char* string)
{
    if (string == NULL) {
        return false;
    }

    int length = strlen(string);
    if (length == 0) {
        return true;
    }

    // Starting from the end of the string, loop while it's a whitespace and
    // decrement string length.
    char* pch = string + length - 1;
    while (length != 0 && isspace(*pch)) {
        length--;
        pch--;
    }

    // pch now points to the last non-whitespace character.
    pch[1] = '\0';

    // Starting from the beginning of the string loop while it's a whitespace
    // and decrement string length.
    pch = string;
    while (isspace(*pch)) {
        pch++;
        length--;
    }

    // pch now points for to the first non-whitespace character.
    memmove(string, pch, length + 1);

    return true;
}

// 0x42C718
bool config_get_double(Config* config, const char* sectionKey, const char* key, double* valuePtr)
{
    if (valuePtr == NULL) {
        return false;
    }

    char* stringValue;
    if (!config_get_string(config, sectionKey, key, &stringValue)) {
        return false;
    }

    *valuePtr = strtod(stringValue, NULL);

    return true;
}

// 0x42C74C
bool config_set_double(Config* config, const char* sectionKey, const char* key, double value)
{
    char stringValue[32];
    sprintf(stringValue, "%.6f", value);

    return config_set_string(config, sectionKey, key, stringValue);
}

// NOTE: Boolean-typed variant of [config_get_value].
bool configGetBool(Config* config, const char* sectionKey, const char* key, bool* valuePtr)
{
    if (valuePtr == NULL) {
        return false;
    }

    int integerValue;
    if (!config_get_value(config, sectionKey, key, &integerValue)) {
        return false;
    }

    *valuePtr = integerValue != 0;

    return true;
}

// NOTE: Boolean-typed variant of [configGetInt].
bool configSetBool(Config* config, const char* sectionKey, const char* key, bool value)
{
    return config_set_value(config, sectionKey, key, value ? 1 : 0);
}
