#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Instruction_Memory.h"
#include "Registers.h"

void loadInstructions(Instruction_Memory *i_mem, const char *trace);
void parseRType(char *opr, Instruction *instr);
void parseIType(char *opr, Instruction *instr, unsigned opcode);
void parseSBType(char *opr, Instruction *instr);
int regIndex(char *reg);
