#pragma once
#include "cpu_state.hpp"
#include "instruction_decoder.hpp"

namespace M68K {
class CPU {
private:
public:
    CPU() = default;

    SimpleMemory memory;
    CPUState state = CPUState(&memory);
    InstructionDecoder instruction_decoder = InstructionDecoder();

    void step();
};

extern bool load_elf(CPU* cpu, const std::string& filename);

};  // namespace M68K
