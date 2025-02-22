#include "tests_functions.hpp"
#include "m68k.hpp"

using namespace M68K;

int main(int, char**){
    TEST_NAME("Program bubblesort");
    
    {
        TEST_LABEL("bubblesort");
        CPU cpu = CPU();
        load_elf(&cpu, "../../test/binary/bubblesort.elf");

        while(cpu.state.registers.get(REG_PC, SIZE_LONG) != 0x100c4){
            cpu.step();
        }

        uint32_t data_ptr = 0x3000;
        uint32_t last_data = 0;
        for(uint32_t i = 0; i < 30u; i++){
            uint32_t data_addr = data_ptr + (uint32_t)(i * SIZE_LONG);
            uint32_t data = cpu.state.memory.get(data_addr, SIZE_LONG);
            TEST_TRUE(last_data <= data);
            last_data = data;
        }
    }
}
