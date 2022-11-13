#include "int/export.h"

#include <ctype.h>
#include <string.h>

#include "interpreter_lib.h"
#include "memory_manager.h"

typedef struct ExternalVariable {
    char name[32];
    char* programName;
    opcode_t type;
    union {
        int value;
        char* stringValue;
    };
} ExternalVariable;

typedef struct ExternalProcedure {
    char name[32];
    Program* program;
    int argumentCount;
    int address;
} ExternalProcedure;

static unsigned int hashName(const char* identifier);
static ExternalProcedure* findProc(const char* identifier);
static ExternalProcedure* findEmptyProc(const char* identifier);
static ExternalVariable* findVar(const char* identifier);
static ExternalVariable* findEmptyVar(const char* identifier);
static void removeProgramReferences(Program* program);

// 0x570C00
static ExternalProcedure procHashTable[1013];

// 0x57BA1C
static ExternalVariable varHashTable[1013];

// NOTE: Inlined.
//
// 0x440F10
static unsigned int hashName(const char* identifier)
{
    unsigned int v1 = 0;
    const char* pch = identifier;
    while (*pch != '\0') {
        int ch = *pch & 0xFF;
        v1 += (tolower(ch) & 0xFF) + (v1 * 8) + (v1 >> 29);
        pch++;
    }

    v1 = v1 % 1013;
    return v1;
}

// 0x440F58
static ExternalProcedure* findProc(const char* identifier)
{
    // NOTE: Uninline.
    unsigned int v1 = hashName(identifier);
    unsigned int v2 = v1;

    ExternalProcedure* externalProcedure = &(procHashTable[v1]);
    if (externalProcedure->program != NULL) {
        if (stricmp(externalProcedure->name, identifier) == 0) {
            return externalProcedure;
        }
    }

    do {
        v1 += 7;
        if (v1 >= 1013) {
            v1 -= 1013;
        }

        externalProcedure = &(procHashTable[v1]);
        if (externalProcedure->program != NULL) {
            if (stricmp(externalProcedure->name, identifier) == 0) {
                return externalProcedure;
            }
        }
    } while (v1 != v2);

    return NULL;
}

// 0x441018
static ExternalProcedure* findEmptyProc(const char* identifier)
{
    // NOTE: Uninline.
    unsigned int v1 = hashName(identifier);
    unsigned int a2 = v1;

    ExternalProcedure* externalProcedure = &(procHashTable[v1]);
    if (externalProcedure->name[0] == '\0') {
        return externalProcedure;
    }

    do {
        v1 += 7;
        if (v1 >= 1013) {
            v1 -= 1013;
        }

        externalProcedure = &(procHashTable[v1]);
        if (externalProcedure->name[0] == '\0') {
            return externalProcedure;
        }
    } while (v1 != a2);

    return NULL;
}

// 0x4410AC
static ExternalVariable* findVar(const char* identifier)
{
    // NOTE: Uninline.
    unsigned int v1 = hashName(identifier);
    unsigned int v2 = v1;

    ExternalVariable* exportedVariable = &(varHashTable[v1]);
    if (stricmp(exportedVariable->name, identifier) == 0) {
        return exportedVariable;
    }

    do {
        exportedVariable = &(varHashTable[v1]);
        if (exportedVariable->name[0] == '\0') {
            break;
        }

        v1 += 7;
        if (v1 >= 1013) {
            v1 -= 1013;
        }

        exportedVariable = &(varHashTable[v1]);
        if (stricmp(exportedVariable->name, identifier) == 0) {
            return exportedVariable;
        }
    } while (v1 != v2);

    return NULL;
}

// NOTE: Unused.
//
// 0x441164
int exportGetVariable(const char* identifier, opcode_t* typePtr, int* valuePtr)
{
    ExternalVariable* variable;

    variable = findVar(identifier);
    if (variable != NULL) {
        *typePtr = variable->type;
        *valuePtr = variable->value;
        return 1;
    }

    *typePtr = 0;
    *valuePtr = 0;

    return 0;
}

// 0x44118C
static ExternalVariable* findEmptyVar(const char* identifier)
{
    // NOTE: Uninline.
    unsigned int v1 = hashName(identifier);
    unsigned int v2 = v1;

    ExternalVariable* exportedVariable = &(varHashTable[v1]);
    if (exportedVariable->name[0] == '\0') {
        return exportedVariable;
    }

    do {
        v1 += 7;
        if (v1 >= 1013) {
            v1 -= 1013;
        }

        exportedVariable = &(varHashTable[v1]);
        if (exportedVariable->name[0] == '\0') {
            return exportedVariable;
        }
    } while (v1 != v2);

    return NULL;
}

// NOTE: Unused.
//
// 0x441220
int exportStoreStringVariable(const char* identifier, const char* value)
{
    ExternalVariable* variable;

    variable = findVar(identifier);
    if (variable != NULL) {
        if ((variable->type & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
            internal_free_safe(variable->stringValue, __FILE__, __LINE__); // "..\int\EXPORT.C", 155
        }

        variable->type = VALUE_TYPE_DYNAMIC_STRING;
        variable->stringValue = strdup_safe(value, __FILE__, __LINE__); // "..\int\EXPORT.C", 159

        return 0;
    }

    return 1;
}

// 0x44127C
int exportStoreVariable(Program* program, const char* name, opcode_t opcode, int data)
{
    ExternalVariable* exportedVariable = findVar(name);
    if (exportedVariable == NULL) {
        return 1;
    }

    if ((exportedVariable->type & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        internal_free_safe(exportedVariable->stringValue, __FILE__, __LINE__); // "..\\int\\EXPORT.C", 169
    }

    if ((opcode & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        if (program != NULL) {
            const char* stringValue = programGetString(program, opcode, data);
            exportedVariable->type = VALUE_TYPE_DYNAMIC_STRING;

            exportedVariable->stringValue = (char*)internal_malloc_safe(strlen(stringValue) + 1, __FILE__, __LINE__); // "..\\int\\EXPORT.C", 175
            strcpy(exportedVariable->stringValue, stringValue);
        }
    } else {
        exportedVariable->value = data;
        exportedVariable->type = opcode;
    }

    return 0;
}

// NOTE: Unused.
//
// 0x441330
int exportStoreVariableByTag(const char* identifier, opcode_t type, int value)
{
    ExternalVariable* variable;

    variable = findVar(identifier);
    if (variable != NULL) {
        if ((variable->type & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
            internal_free_safe(variable->stringValue, __FILE__, __LINE__); // "..\int\EXPORT.C", 191
        }

        if ((type & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
            variable->type = VALUE_TYPE_DYNAMIC_STRING;
            variable->stringValue = (char*)internal_malloc_safe(strlen((char*)value) + 1, __FILE__, __LINE__); // "..\int\EXPORT.C", 196
            strcpy(variable->stringValue, (char*)value);
        } else {
            variable->value = value;
            variable->type = type;
        }

        return 0;
    }

    return 1;
}

// 0x4413D4
int exportFetchVariable(Program* program, const char* name, opcode_t* opcodePtr, int* dataPtr)
{
    ExternalVariable* exportedVariable = findVar(name);
    if (exportedVariable == NULL) {
        return 1;
    }

    *opcodePtr = exportedVariable->type;

    if ((exportedVariable->type & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        *dataPtr = programPushString(program, exportedVariable->stringValue);
    } else {
        *dataPtr = exportedVariable->value;
    }

    return 0;
}

// 0x4414B8
int exportExportVariable(Program* program, const char* identifier)
{
    const char* programName = program->name;
    ExternalVariable* exportedVariable = findVar(identifier);

    if (exportedVariable != NULL) {
        if (stricmp(exportedVariable->programName, programName) != 0) {
            return 1;
        }

        if ((exportedVariable->type & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
            internal_free_safe(exportedVariable->stringValue, __FILE__, __LINE__); // "..\\int\\EXPORT.C", 234
        }
    } else {
        exportedVariable = findEmptyVar(identifier);
        if (exportedVariable == NULL) {
            return 1;
        }

        strncpy(exportedVariable->name, identifier, 31);

        exportedVariable->programName = (char*)internal_malloc_safe(strlen(programName) + 1, __FILE__, __LINE__); // // "..\\int\\EXPORT.C", 243
        strcpy(exportedVariable->programName, programName);
    }

    exportedVariable->type = VALUE_TYPE_INT;
    exportedVariable->value = 0;

    return 0;
}

// 0x4414FC
static void removeProgramReferences(Program* program)
{
    for (int index = 0; index < 1013; index++) {
        ExternalProcedure* externalProcedure = &(procHashTable[index]);
        if (externalProcedure->program == program) {
            externalProcedure->name[0] = '\0';
            externalProcedure->program = NULL;
        }
    }
}

// 0x44152C
void initExport()
{
    intLibRegisterProgramDeleteCallback(removeProgramReferences);
}

// 0x441538
void exportClose()
{
    for (int index = 0; index < 1013; index++) {
        ExternalVariable* exportedVariable = &(varHashTable[index]);

        if (exportedVariable->name[0] != '\0') {
            internal_free_safe(exportedVariable->programName, __FILE__, __LINE__); // ..\\int\\EXPORT.C, 274
        }

        if (exportedVariable->type == VALUE_TYPE_DYNAMIC_STRING) {
            internal_free_safe(exportedVariable->stringValue, __FILE__, __LINE__); // ..\\int\\EXPORT.C, 276
        }
    }
}

// 0x44158C
Program* exportFindProcedure(const char* identifier, int* addressPtr, int* argumentCountPtr)
{
    ExternalProcedure* externalProcedure = findProc(identifier);
    if (externalProcedure == NULL) {
        return NULL;
    }

    if (externalProcedure->program == NULL) {
        return NULL;
    }

    *addressPtr = externalProcedure->address;
    *argumentCountPtr = externalProcedure->argumentCount;

    return externalProcedure->program;
}

// 0x4415B0
int exportExportProcedure(Program* program, const char* identifier, int address, int argumentCount)
{
    ExternalProcedure* externalProcedure = findProc(identifier);
    if (externalProcedure != NULL) {
        if (program != externalProcedure->program) {
            return 1;
        }
    } else {
        externalProcedure = findEmptyProc(identifier);
        if (externalProcedure == NULL) {
            return 1;
        }

        strncpy(externalProcedure->name, identifier, 31);
    }

    externalProcedure->argumentCount = argumentCount;
    externalProcedure->address = address;
    externalProcedure->program = program;

    return 0;
}

// 0x441824
void exportClearAllVariables()
{
    for (int index = 0; index < 1013; index++) {
        ExternalVariable* exportedVariable = &(varHashTable[index]);
        if (exportedVariable->name[0] != '\0') {
            if ((exportedVariable->type & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
                if (exportedVariable->stringValue != NULL) {
                    internal_free_safe(exportedVariable->stringValue, __FILE__, __LINE__); // "..\\int\\EXPORT.C", 387
                }
            }

            if (exportedVariable->programName != NULL) {
                internal_free_safe(exportedVariable->programName, __FILE__, __LINE__); // "..\\int\\EXPORT.C", 393
                exportedVariable->programName = NULL;
            }

            exportedVariable->name[0] = '\0';
            exportedVariable->type = 0;
        }
    }
}
