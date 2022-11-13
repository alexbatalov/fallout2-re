#ifndef FALLOUT_INT_EXPORT_H_
#define FALLOUT_INT_EXPORT_H_

#include "interpreter.h"

int exportGetVariable(const char* identifier, opcode_t* typePtr, int* valuePtr);
int exportStoreStringVariable(const char* identifier, const char* value);
int exportStoreVariable(Program* program, const char* identifier, opcode_t opcode, int data);
int exportStoreVariableByTag(const char* identifier, opcode_t type, int value);
int exportFetchVariable(Program* program, const char* name, opcode_t* opcodePtr, int* dataPtr);
int exportExportVariable(Program* program, const char* identifier);
void initExport();
void exportClose();
Program* exportFindProcedure(const char* identifier, int* addressPtr, int* argumentCountPtr);
int exportExportProcedure(Program* program, const char* identifier, int address, int argumentCount);
void exportClearAllVariables();

#endif /* FALLOUT_INT_EXPORT_H_ */
