#include "instructions/clr.hpp"
#include "helpers.hpp"
#include <stdexcept>

using namespace M68K;
using namespace INSTRUCTION;

Clr::Clr(uint16_t opcode) : Instruction(opcode){
    uint16_t ea_mode_part = (opcode >> 3) & 0x7;
    uint16_t ea_reg_part = (opcode >> 0) & 0x7;
    uint16_t data_size_part = (opcode >> 6) & 0x3;

    this->dest_mode = getAddressingMode(ea_mode_part, ea_reg_part);
    this->dest_reg = getRegisterType(ea_mode_part, ea_reg_part);

    if(!IS_MEMORY_ALTERABLE(this->dest_mode)){
        this->is_valid = false;
    }

    switch(data_size_part){
        case 0x00:{ // 0b00
            this->data_size = SIZE_BYTE;
            break;
        }
        case 0x01:{ // 0b01
            this->data_size = SIZE_WORD;
            break;
        }
        case 0x02:{ // 0b10
            this->data_size = SIZE_LONG;
            break;
        }
        
        default:{
            this->is_valid = false;
        }
    }
}

void Clr::execute(CPUState& cpu_state){
    uint32_t pc = cpu_state.registers.get(REG_PC, SIZE_LONG);
    pc += SIZE_WORD;
    cpu_state.registers.set(REG_PC, SIZE_LONG, pc);

    cpu_state.setData(this->dest_mode, this->dest_reg, this->data_size, 0);

    cpu_state.registers.set(SR_FLAG_NEGATIVE, false);
    cpu_state.registers.set(SR_FLAG_ZERO, true);
    cpu_state.registers.set(SR_FLAG_OVERFLOW, false);
    cpu_state.registers.set(SR_FLAG_CARRY, false);
}

std::string Clr::disassembly(CPUState& cpu_state){
    uint32_t pc = cpu_state.registers.get(REG_PC, SIZE_LONG);
    pc += SIZE_WORD;
    cpu_state.registers.set(REG_PC, SIZE_LONG, pc);

    std::ostringstream output;
    output << "clr" << DISASSEMBLER::sizeSuffix(this->data_size)
           << " " << DISASSEMBLER::effectiveAddress(this->dest_mode, this->dest_reg, this->data_size, cpu_state);
               
    return output.str();
}

std::unique_ptr<INSTRUCTION::Instruction> Clr::create(uint16_t opcode){
    return std::make_unique<Clr>(opcode);
}