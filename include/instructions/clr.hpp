#pragma once
#include "instruction.hpp"
#include <memory>

namespace M68K{
    namespace INSTRUCTION{
        class Clr : public Instruction{
        private:
            AddressingMode dest_mode = ADDR_MODE_UNKNOWN;
            RegisterType dest_reg = REG_D0;

            DataSize data_size;
        public:
            Clr(uint16_t opcode);
            void execute(CPUState& cpu_state) override;
            std::string disassembly(CPUState& cpu_state) override;

            static std::unique_ptr<INSTRUCTION::Instruction> create(uint16_t opcode);
        };
    }
}
