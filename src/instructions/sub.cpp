#include "instructions/sub.hpp"
#include "helpers.hpp"
#include <stdexcept>

using namespace M68K;
using namespace INSTRUCTION;

Sub::Sub(uint16_t opcode) : Instruction(opcode){
    uint16_t data_reg_part = (opcode >> 9) & 0x7;
    uint16_t ea_mode_part = (opcode >> 3) & 0x7;
    uint16_t ea_reg_part = (opcode >> 0) & 0x7;
    uint16_t data_size_part = (opcode >> 6) & 0x3;

    this->op_mode = (opcode >> 8) & 0x1;

    if(this->op_mode){
        this->dest_mode = getAddressingMode(ea_mode_part, ea_reg_part);
        this->dest_reg = getRegisterType(ea_mode_part, ea_reg_part);

        this->src_mode = ADDR_MODE_DIRECT_DATA;
        this->src_reg = getRegisterType(0, data_reg_part);

        if(!IS_MEMORY_ALTERABLE(this->dest_mode)){
            this->is_valid = false;
        }
    }else{
        this->dest_mode = ADDR_MODE_DIRECT_DATA;
        this->dest_reg = getRegisterType(0, data_reg_part);

        this->src_mode = getAddressingMode(ea_mode_part, ea_reg_part);
        this->src_reg = getRegisterType(ea_mode_part, ea_reg_part);
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

void Sub::execute(CPUState& cpu_state){
    uint32_t pc = cpu_state.registers.get(REG_PC, SIZE_LONG);
    pc += SIZE_WORD;
    cpu_state.registers.set(REG_PC, SIZE_LONG, pc);

    uint32_t src_data = cpu_state.getData(this->src_mode, this->src_reg, this->data_size);
    uint32_t dest_data = cpu_state.getDataSilent(this->dest_mode, this->dest_reg, this->data_size);
    uint64_t result = dest_data - src_data;

    cpu_state.setData(this->dest_mode, this->dest_reg, this->data_size, (uint32_t)result);

    cpu_state.registers.set(SR_FLAG_EXTEND, IS_CARRY(result, this->data_size));
    cpu_state.registers.set(SR_FLAG_NEGATIVE, IS_NEGATIVE(result, this->data_size));
    cpu_state.registers.set(SR_FLAG_ZERO, IS_ZERO(result, this->data_size));
    cpu_state.registers.set(SR_FLAG_OVERFLOW, IS_OVERFLOW(src_data, dest_data, this->data_size));
    cpu_state.registers.set(SR_FLAG_CARRY, IS_CARRY(result, this->data_size));
}

std::string Sub::disassembly(CPUState& cpu_state){
    uint32_t pc = cpu_state.registers.get(REG_PC, SIZE_LONG);
    pc += SIZE_WORD;
    cpu_state.registers.set(REG_PC, SIZE_LONG, pc);

    std::ostringstream output;
    output << "sub" << DISASSEMBLER::sizeSuffix(this->data_size)
           << " " << DISASSEMBLER::effectiveAddress(this->src_mode, this->src_reg, this->data_size, cpu_state)
           << ", " << DISASSEMBLER::effectiveAddress(this->dest_mode, this->dest_reg, this->data_size, cpu_state);
               
    return output.str();
}

std::unique_ptr<INSTRUCTION::Instruction> Sub::create(uint16_t opcode){
    return std::make_unique<Sub>(opcode);
}
