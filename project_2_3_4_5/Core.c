#include "Core.h"

Core *initCore(Instruction_Memory *i_mem)
{
    Core *core = (Core *)malloc(sizeof(Core));
    core->clk = 0;
    core->PC = 0;
    core->instr_mem = i_mem;
    core->tick = tickFunc;

    uint64_t arr[4] = {16, 128, 8, 4};

    for (size_t i = 0; i < sizeof(arr) / sizeof(arr[0]); i++)
    {
        core->data_mem[(64 * i)] = (arr[i] >> 56) % (2 << 8);
        core->data_mem[(64 * i) + 8] = (arr[i] >> 48) % (2 << 8);
        core->data_mem[(64 * i) + 16] = (arr[i] >> 40) % (2 << 8);
        core->data_mem[(64 * i) + 24] = (arr[i] >> 32) % (2 << 8);
        core->data_mem[(64 * i) + 32] = (arr[i] >> 24) % (2 << 8);
        core->data_mem[(64 * i) + 40] = (arr[i] >> 16) % (2 << 8);
        core->data_mem[(64 * i) + 48] = (arr[i] >> 8) % (2 << 8);
        core->data_mem[(64 * i) + 56] = arr[i] % (2 << 8);
    }

    for (size_t i = 0; i < sizeof(arr) / sizeof(arr[0]); i++)
    {
        printf("%d ", core->data_mem[(64 * i)]);
        printf("%d ", core->data_mem[(64 * i) + 8]);
        printf("%d ", core->data_mem[(64 * i) + 16]);
        printf("%d ", core->data_mem[(64 * i) + 24]);
        printf("%d ", core->data_mem[(64 * i) + 32]);
        printf("%d ", core->data_mem[(64 * i) + 40]);
        printf("%d ", core->data_mem[(64 * i) + 48]);
        printf("%d\n", core->data_mem[(64 * i) + 56]);
    }

    core->reg_file[25] = 4;
    core->reg_file[10] = 4;
    core->reg_file[22] = 1;

    return core;
}

// FIXME, implement this function
bool tickFunc(Core *core)
{
    // Steps may include
    // (Step 1) Reading instruction from instruction memory
    unsigned instruction = core->instr_mem->instructions[core->PC / 4].instruction;

    // Generate control signals and read input regs
    ControlSignals controls;
    Signal opcode = instruction % (1 << 7);
    Signal rd = (instruction >> 7) % (1 << 5);
    Signal funct3 = (instruction >> 12) % (1 << 3);
    Signal rs1 = (instruction >> 15) % (1 << 5);
    Signal rs2 = (instruction >> 20) % (1 << 5);
    Signal funct7 = (instruction >> 25) % (1 << 7);
    ControlUnit(opcode, &controls);
    Signal alu_control = ALUControlUnit(controls.ALUOp, funct7, funct3);

    Register input1 = core->reg_file[rs1];
    Register input2 = core->reg_file[rs2];

    printf("Value at rs1: %ld\n", input1);
    printf("Value at rs2: %ld\n", input2);

    // Operate on data

    if (opcode == 51) // add
    {

    }

    // Write to destination

    // (Step N) Increment PC. FIXME, is it correct to always increment PC by 4?!
    // if branch, add imm to program counter
    // sign extend, shift left one bit, add to PC to compute branch address
    core->PC += 4;

    ++core->clk;
    // Are we reaching the final instruction?
    if (core->PC > core->instr_mem->last->addr)
    {
        return false;
    }
    return true;
}

// FIXME (1). Control Unit. Refer to Figure 4.18.
// input is the opcode
void ControlUnit(Signal input,
                 ControlSignals *signals)
{
    // For R-type
    if (input == 51)
    {
        signals->ALUSrc = 0;
        signals->MemtoReg = 0;
        signals->RegWrite = 1;
        signals->MemRead = 0;
        signals->MemWrite = 0;
        signals->Branch = 0;
        signals->ALUOp = 2;
    }
    else if (input == 3) // ld
    {
        signals->ALUSrc = 1;
        signals->MemtoReg = 1;
        signals->RegWrite = 1;
        signals->MemRead = 1;
        signals->MemWrite = 0;
        signals->Branch = 0;
        signals->ALUOp = 0;
    }
    else if (input == 3) // sd
    {
        signals->ALUSrc = 1;
        signals->MemtoReg = 0; // don't care
        signals->RegWrite = 0;
        signals->MemRead = 0;
        signals->MemWrite = 1;
        signals->Branch = 0;
        signals->ALUOp = 0;
    }
    else if (input == 99) // beq, extending to bne as well
    {
        signals->ALUSrc = 0;
        signals->MemtoReg = 0;
        signals->RegWrite = 0;
        signals->MemRead = 0;
        signals->MemWrite = 0;
        signals->Branch = 1;
        signals->ALUOp = 1;
    }
    else if (input == 19) // addi or slli
    {

    }
}

// FIXME (2). ALU Control Unit. Refer to Figure 4.12.
Signal ALUControlUnit(Signal ALUOp,
                      Signal Funct7,
                      Signal Funct3)
{
    // For add
    if (ALUOp == 2)
    {
        if (Funct7 == 0 && Funct3 == 0)
        {
            return 2;
        }
        else if (Funct7 == 32 && Funct3 == 0)
        { // R-type sub
            return 6;
        }
        else if (Funct7 == 0 && Funct3 == 7)
        {
            return 0;
        }
        else if (Funct7 == 0 && Funct3 == 6)
        {
            return 1;
        }
    }
    else if (ALUOp == 1) // Beq. repurposed for bne here: subtract and see if not equal
    {
        return 6;
    }
    else if (ALUOp == 0) // ld and sd use add to get address
    {
        return 2;
    }

    // AND = 0, OR = 1, add = 2, subtract = 6
}

// FIXME (3). Imme. Generator
Signal ImmeGen(Signal input)
{
}

// FIXME (4). ALU
void ALU(Signal input_0,
         Signal input_1,
         Signal ALU_ctrl_signal,
         Signal *ALU_result,
         Signal *zero)
{
    // For addition
    if (ALU_ctrl_signal == 2)
    {
        *ALU_result = (input_0 + input_1);
        if (*ALU_result == 0)
        {
            *zero = 1;
        }
        else
        {
            *zero = 0;
        }
    }
    else if (ALU_ctrl_signal == 6) // subtraction
    {
        *ALU_result = (input_0 - input_1);
        if (*ALU_result == 0)
        {
            *zero = 1;
        }
        else
        {
            *zero = 0;
        }
    }
}

// (4). MUX
Signal MUX(Signal sel,
           Signal input_0,
           Signal input_1)
{
    if (sel == 0)
    {
        return input_0;
    }
    else
    {
        return input_1;
    }
}

// (5). Add
Signal Add(Signal input_0,
           Signal input_1)
{
    return (input_0 + input_1);
}

// (6). ShiftLeft1
Signal ShiftLeft1(Signal input)
{
    return input << 1;
}
