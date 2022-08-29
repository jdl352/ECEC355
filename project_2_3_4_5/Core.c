#include "Core.h"

Core *initCore(Instruction_Memory *i_mem)
{
    Core *core = (Core *)malloc(sizeof(Core));
    core->clk = 0;
    core->PC = 0;
    core->instr_mem = i_mem;
    core->tick = tickFunc;
    core->stall = false;

    uint64_t arr[2] = {40};
    int64_t vals[2] = {48};
    for (size_t i = 0; i < sizeof(core->instr_mem)/sizeof(core->instr_mem[0]); i++)
    {
        core->data_mem[i] = 0;
    }
    for (size_t i = 0; i < sizeof(arr) / sizeof(arr[0]); i++)
    {
        core->data_mem[(8 * i)] = (arr[i] >> 56) % (2 << 8);
        core->data_mem[(8 * i) + 1] = (arr[i] >> 48) % (2 << 8);
        core->data_mem[(8 * i) + 2] = (arr[i] >> 40) % (2 << 8);
        core->data_mem[(8 * i) + 3] = (arr[i] >> 32) % (2 << 8);
        core->data_mem[(8 * i) + 4] = (arr[i] >> 24) % (2 << 8);
        core->data_mem[(8 * i) + 5] = (arr[i] >> 16) % (2 << 8);
        core->data_mem[(8 * i) + 6] = (arr[i] >> 8) % (2 << 8);
        core->data_mem[(8 * i) + 7] = arr[i] % (2 << 8);
    }
    for (size_t i = 0; i < sizeof(core->reg_file)/sizeof(core->reg_file[0]); i++)
    {
        core->reg_file[i] = 0;
        core->scoreboard[i] = 1;
    }
    core->reg_file[2] = 10;
    core->reg_file[3] = -15;
    core->reg_file[4] = 20;
    core->reg_file[5] = 30;
    core->reg_file[6] = -35;

    return core;
}

// FIXME, implement this function
bool tickFunc(Core *core)
{

    Signal zero;

    // WB stage

    if (core->write_back != 0)
    {
        ControlSignals wb_controls;
        Signal wb_opcode = core->write_back % (1 << 7);
        ControlUnit(wb_opcode, &wb_controls);
        if (wb_controls.RegWrite)
        {
            Signal wb_rd = (core->write_back >> 7) % (1 << 5);
            core->reg_file[wb_rd] = core->wb_result;
            core->scoreboard[wb_rd] = true;
        }
        core->write_back = 0; 
    }

    // MEM stage
    if (core->memory != 0)
    {
        ControlSignals mem_controls;
        Signal mem_opcode = core->memory % (1 << 7);
        Signal mem_funct3 = (core->memory >> 12) % (1 << 3);
        Signal mem_rs1 = (core->memory >> 15) % (1 << 5);
        Signal mem_rs2 = (core->memory >> 20) % (1 << 5);
        Signal mem_funct7 = (core->memory >> 25) % (1 << 7);
        Signal input_0 = core->reg_file[mem_rs1];
        ControlUnit(mem_opcode, &mem_controls);
        Signal mem_alu_control = ALUControlUnit(mem_controls.ALUOp, mem_funct7, mem_funct3);

        if (mem_opcode == 3 && mem_funct3 == 3) // ld
        {
            core->mem_result = 0;
            Signal imm = mem_rs2 | (mem_funct7 << 5);
            Signal addr;
            ALU(input_0, imm, mem_alu_control, &addr, &zero);
            core->mem_result += (uint64_t)(core->data_mem[addr]) << 56;
            core->mem_result += (uint64_t)(core->data_mem[addr + 1]) << 48;
            core->mem_result += (uint64_t)(core->data_mem[addr + 2]) << 40;
            core->mem_result += (uint64_t)(core->data_mem[addr + 3]) << 32;
            core->mem_result += (uint64_t)(core->data_mem[addr + 4]) << 24;
            core->mem_result += (uint64_t)(core->data_mem[addr + 5]) << 16;
            core->mem_result += (uint64_t)(core->data_mem[addr + 6]) << 8;
            core->mem_result += (uint64_t)(core->data_mem[addr + 7]);
        }

        core->write_back = core->memory;
        core->memory = 0;
        core->wb_result = core->mem_result;
    }

    // EX stage

    if (core->execute != 0)
    {

        ControlSignals ex_controls;
        Signal ex_opcode = core->execute % (1 << 7);
        Signal ex_rd = (core->execute >> 7) % (1 << 5);
        Signal ex_funct3 = (core->execute >> 12) % (1 << 3);
        Signal ex_rs1 = (core->execute >> 15) % (1 << 5);
        Signal ex_rs2 = (core->execute >> 20) % (1 << 5);
        Signal ex_funct7 = (core->execute >> 25) % (1 << 7);
        ControlUnit(ex_opcode, &ex_controls);
        Signal ex_alu_control = ALUControlUnit(ex_controls.ALUOp, ex_funct7, ex_funct3);

        Signal input_0 = core->reg_file[ex_rs1];
        Signal input_1 = core->reg_file[ex_rs2];

        Signal zero;

        if (ex_opcode == 51) // add and sub
        {
            ALU(input_0, input_1, ex_alu_control, &(core->ex_result), &zero);
        }
        else if (ex_opcode == 19) // addi and slli
        {
            Signal imm = ex_rs2 | (ex_funct7 << 5);
            if (ex_funct3 == 0) // addi
            {
                ALU(input_0, imm, ex_alu_control, &(core->ex_result), &zero);
            }
            else if (ex_funct3 == 1) // slli
            {
                core->ex_result = input_0 << imm;
            }
        }
        else if (ex_opcode == 99) // bne
        {
            ALU(input_0, input_1, ex_alu_control, &(core->ex_result), &zero);
        }

        core->memory = core->execute;
        core->execute = 0;
        core->mem_result = core->ex_result;
    }

    // DE stage

    if (core->decode != 0)
    {
        Signal de_rd = (core->decode >> 7) % (1 << 5);
        Signal de_rs1 = (core->decode >> 15) % (1 << 5);
        Signal de_rs2 = (core->decode >> 20) % (1 << 5);
        Signal de_opcode = core->decode % (1 << 7);
        if (!core->stall)
        {
            if (core->reg_file[de_rs1] == false || 
                core->reg_file[de_rs2] == false ||
                core->reg_file[de_rd] == false)
            {
                core->stall = true;
            }
            else
            {
                core->scoreboard[de_rd] = false;
                core->execute = core->decode;
                core->decode = 0;
            }
        }
        else
        {
            if (core->reg_file[de_rs1] == true && 
                core->reg_file[de_rs2] == true &&
                core->reg_file[de_rd] == true)
                {
                    core->stall = false;
                }
        }
    }

    if (core->fetch == 0)
    {
        core->fetch = core->instr_mem->instructions[core->PC / 4].instruction;
        core->PC += 4;
        if (core->decode == 0)
        {
            core->decode = core->fetch;
            core->fetch = 0;
        }
    }
    else
    {
        if (core->decode == 0)
        {
            core->decode = core->fetch;
            core->fetch = 0;
        }
    }

    ++core->clk;

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
        signals->ALUSrc = 0;
        signals->MemtoReg = 0;
        signals->RegWrite = 1;
        signals->MemRead = 0;
        signals->MemWrite = 0;
        signals->Branch = 0;
        signals->ALUOp = 0;
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

bool reg_available(Core* core, Signal input)
{
    Signal rd = (input >> 7) % (1 << 5);

    if (core->scoreboard[rd] == true)
    {
        return true;
    }
    else
    {
        return false;
    }
}
