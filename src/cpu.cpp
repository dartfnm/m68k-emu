#include "cpu.hpp"
#include "elfio/elfio.hpp"

#include <iostream>


using namespace ELFIO;


namespace M68K {
void CPU::step(){
    uint32_t pc = (uint32_t)this->state.registers.get(REG_PC);
    uint16_t opcode = (uint16_t)this->state.memory.get(pc, SIZE_WORD);

    INSTRUCTION::Instruction* instruction;
    instruction = this->instruction_decoder.Decode(opcode);
    //std::cout << typeid(*instruction).name() << std::endl;
    instruction->execute(this->state);
}


bool M68K::load_elf(CPU* cpu, const std::string& file_name){
    ELFIO::elfio elf_reader;
    if(!elf_reader.load(file_name)){
        return false;
    }

    if(
        (elf_reader.get_class() != ELFIO::ELFCLASS32) ||
        (elf_reader.get_encoding() != ELFIO::ELFDATA2MSB) ||
        (elf_reader.get_type() != ELFIO::ET_EXEC) ||
        (elf_reader.get_machine() != ELFIO::EM_68K)
    ){
        return false;
    }

    uint32_t entry_address = (uint32_t)elf_reader.get_entry();

    for (const auto& segment : elf_reader.sections) {
        std::string name = segment->get_name();
        std::cout << name;
        for (size_t i = name.size(); i < 16; ++i)
            std::cout << ' ';
        std::cout << "\t"
                   << segment->get_address() << "\t"
                   << segment->get_size() << "\t"
                   << segment->get_offset() << "\t"
                   << segment->get_addr_align() << "\t"
                   << segment->get_link() << "\t"
                   << segment->get_flags() << "\t"
                   << segment->get_info() << "\t"
                   << segment->get_type() << std::endl;

        Elf_Xword f = segment->get_flags();
        if(segment->get_type() == ELFIO::PT_LOAD && (f & SHF_ALLOC) != 0){
            uint32_t base_address = (uint32_t)segment->get_address();
            uint32_t size = (uint32_t)segment->get_size();
            auto data = segment->get_data();
            for(uint32_t i = 0; i < size; i++){
                uint32_t address = base_address + i;
                cpu->state.memory.set(address, SIZE_BYTE, data[i]); // little slow, but good enough
            }
        }
    }

    cpu->state.registers.set(REG_USP, SIZE_LONG, MEMORY_SIZE);
    cpu->state.registers.set(REG_PC, SIZE_LONG, entry_address);
    return true;
}

};  // namespace M68K
