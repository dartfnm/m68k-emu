#pragma once
#include "memory.hpp"
#include "registers.hpp"

namespace M68K {
class CPUState {
public:
    IMemory* memoryPtr = nullptr;
    IMemory& memory = *memoryPtr;
    Registers registers = Registers();

public:
    CPUState(IMemory* in_memory = nullptr) : memoryPtr(in_memory) {
    }
    void operator=(const CPUState& rh) {
        memoryPtr = rh.memoryPtr, registers = rh.registers;
    }

    uint32_t stackPop(DataSize size);
    void stackPush(DataSize size, uint32_t data);
    uint32_t getControlAddress(AddressingMode mode, RegisterType reg, DataSize size);
    uint32_t getData(AddressingMode mode, RegisterType reg, DataSize size);
    uint32_t getDataSilent(AddressingMode mode, RegisterType reg, DataSize size);
    void setData(AddressingMode mode, RegisterType reg, DataSize size, uint32_t data);
    bool checkCondition(Condition cond);

    void debugPrint();
};
}  // namespace M68K
