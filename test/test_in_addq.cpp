#include "tests_functions.hpp"

#include "m68k.hpp"
#include "memory.hpp"
#include "helpers.hpp"
#include <instructions/addq.hpp>

using namespace M68K;

int main(int, char**){
    TEST_NAME("Instruction ADDQ");

    {
        TEST_LABEL("addq #7, D0");
        auto instruction = INSTRUCTION::Addq::create(0x5E40); // addq #7, D0
        CPUState state = CPUState();
        
        state.registers.set(REG_D0, SIZE_WORD, 100);

        instruction.get()->execute(state);
        uint32_t return_data = state.registers.get(REG_D0, DataSize::SIZE_LONG);
        TEST_TRUE(return_data == 107);

        bool flag_extend = state.registers.get(SR_FLAG_EXTEND);
        bool flag_negative = state.registers.get(SR_FLAG_NEGATIVE);
        bool flag_zero = state.registers.get(SR_FLAG_ZERO);
        bool flag_overflow = state.registers.get(SR_FLAG_OVERFLOW);
        bool flag_carry = state.registers.get(SR_FLAG_CARRY);

        TEST_TRUE(flag_extend == false);
        TEST_TRUE(flag_negative == false);
        TEST_TRUE(flag_zero == false);
        TEST_TRUE(flag_overflow == false);
        TEST_TRUE(flag_carry == false);
    }
}
