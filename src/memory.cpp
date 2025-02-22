#include "memory.hpp"
#include "helpers.hpp"
#include <stdexcept>
#include <cstdlib>


namespace M68K {

uint32_t IMemory::read_real_mem(void* mem_addr, DataSize size) {
    uint8_t* addr = (uint8_t*)(mem_addr);
    uint32_t output_data = 0;
    switch (size) {
        case DataSize::SIZE_BYTE:
            output_data = addr[0];
            break;
        case DataSize::SIZE_WORD:
            output_data = addr[0];
            output_data = (output_data << 8) | addr[1];
            break;
        case DataSize::SIZE_LONG:
            output_data = addr[0];
            output_data = (output_data << 8) | addr[1];
            output_data = (output_data << 8) | addr[2];
            output_data = (output_data << 8) | addr[3];
            break;
        default:
            throw std::length_error("Invalid memory request size. " + std::to_string(size));
    }
    return output_data;
}


void IMemory::write_real_mem(void* mem_addr, DataSize size, uint32_t data) {
    uint8_t* addr = (uint8_t*)(mem_addr);
    switch (size) {
        case DataSize::SIZE_BYTE:
            addr[0] = (uint8_t)MASK_8(data);
            break;
        case DataSize::SIZE_WORD:
            addr[0] = (uint8_t)MASK_8(data >> 8);
            addr[1] = (uint8_t)MASK_8(data);
            break;
        case DataSize::SIZE_LONG:
            addr[0] = (uint8_t)MASK_8(data >> 24);
            addr[1] = (uint8_t)MASK_8(data >> 16);
            addr[2] = (uint8_t)MASK_8(data >> 8);
            addr[3] = (uint8_t)MASK_8(data);
            break;
        default:
            throw std::length_error("Invalid memory request size. " + std::to_string(size));
    }
    return;
}



 BaseMemory::BaseMemory(void* _baseAddr, uint32_t _size) : baseAddr((uint8_t*)_baseAddr), memSize(_size) {
    if (baseAddr || memSize) {
        baseAddr[memSize - 1] = 0;
        assert(!baseAddr[memSize - 1] && "memory test fail");
    }
}


uint32_t BaseMemory::get(std::size_t address, DataSize size){
    address = MASK_ADDR(address);
    if((size != DataSize::SIZE_BYTE) && (address % 2 != 0)){ // only even is valid in word and long mode
        throw std::length_error("Memory address must be even."); //TODO: Throw special exception
    }
    if(address + size > this->memSize){ //memory overflow
        throw std::out_of_range(
            "Memory address out of range. " +
            std::to_string(address) + ">" + std::to_string(memSize)
        ); //TODO: Throw special exception
    }
    uint8_t* addr = &baseAddr[address];
    return read_real_mem(addr, size);
}


void BaseMemory::set(std::size_t address, DataSize size, uint32_t data){
    address = MASK_ADDR(address);
    if((size != DataSize::SIZE_BYTE) && (address % 2 != 0)){ // only even is valid in word and long mode
        throw std::length_error("Memory address must be even."); //TODO: Throw special exception
    }
    
    if(address + size > this->memSize){ // memory out of range.
        throw std::out_of_range(
            "Memory address out of range. " +
            std::to_string(address) + ">" + std::to_string(memSize)
        ); //TODO: Throw special exception
    }

    uint8_t* addr = &baseAddr[address];
    write_real_mem(addr, size, data);
}


}; // namespace M68K
