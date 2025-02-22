#pragma once
#include <cstdint>
#include <vector>

#include "defines.hpp"

namespace M68K {

class IMemory {
public:
    virtual uint32_t get(std::size_t address, M68K::DataSize size) = 0;
    virtual void set(std::size_t address, M68K::DataSize size, uint32_t data) = 0;

    template <typename T>
    void set_(size_t address, T val) {
        DataSize sz = (DataSize)(sizeof(T));
        set(address, sz, (uint32_t)val);
    }

    static uint32_t read_real_mem(void* mem_addr, DataSize size);
    static void write_real_mem(void* mem_addr, DataSize size, uint32_t data);

    virtual ~IMemory() = default;

}; // class IMemory
//////////////////////////////////////////////////////////////////////////



class BaseMemory : public IMemory {
public:
    uint8_t* baseAddr = nullptr;
    uint32_t memSize = 0;
public:
    BaseMemory(void* _baseAddr, uint32_t _size);
    virtual uint32_t get(std::size_t address, DataSize size) override;
    virtual void set(std::size_t address, DataSize size, uint32_t data) override;
};  // class BaseMemory
//////////////////////////////////////////////////////////////////////////



class SimpleMemory final : public BaseMemory {
public:
    SimpleMemory() : BaseMemory(new uint8_t[MEMORY_SIZE], MEMORY_SIZE) {}
    virtual ~SimpleMemory() {
        delete[] BaseMemory::baseAddr;
    }
}; // class SimpleMemory
//////////////////////////////////////////////////////////////////////////


}  // namespace M68K
