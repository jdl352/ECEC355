#include <stdio.h>

#include "Core.h"
#include "Parser.h"

int main(int argc, const char *argv[])
{	
    if (argc != 2)
    {
        printf("Usage: %s %s\n", argv[0], "<trace-file>");

        return 0;
    }

    /* Task One */
    // TODO, (1) parse and translate all the assembly instructions into binary format;
    // (2) store the translated binary instructions into instruction memory.
    Instruction_Memory instr_mem;
    instr_mem.last = NULL;
    loadInstructions(&instr_mem, argv[1]);

    /* Task Two */
    // TODO, implement Core.{h,c}
    Core *core = initCore(&instr_mem);

    /* Task Three - Simulation */
    while (core->tick(core));

    printf("Simulation is finished.\n");
    printf("Final PC: %ld\n", core->PC);
    for (size_t i = 0; i < 32; i++)
    {
        printf("Reg %ld: %ld\n", i, core->reg_file[i]);
    }

    free(core);    
}
