#pragma once
#include <cstdint>
#include <cstddef>
#include <array>

#include "defines.hpp"

namespace M68K {
    enum RegisterType {
        REG_D0 = 0,
        REG_D1 = 1,
        REG_D2 = 2,
        REG_D3 = 3,
        REG_D4 = 4,
        REG_D5 = 5,
        REG_D6 = 6,
        REG_D7 = 7,
        REG_A0 = 8,
        REG_A1 = 9,
        REG_A2 = 10,
        REG_A3 = 11,
        REG_A4 = 12,
        REG_A5 = 13,
        REG_A6 = 14,
        REG_A7 = 15,
        REG_USP = 15, // User StackPointer = REG_A7
        REG_SSP = 16,  // Supervisor StackPointer
        REG_PC = 17,
        REG_SR = 18,
        REGS_COUNT = 19,
    };

    enum StatusRegisterFlag {
        SR_FLAG_CARRY = (1 << 0),
        SR_FLAG_OVERFLOW = (1 << 1),
        SR_FLAG_ZERO = (1 << 2),
        SR_FLAG_NEGATIVE = (1 << 3),
        SR_FLAG_EXTEND = (1 << 4),
        SR_FLAG_SUPERVISOR = (1 << 13),
        SR_FLAG_TRACE = (1 << 15)
    };


    class Registers {
    public:
        M68K_MSVC_PUSH_DISABLE_WARNING(4201)
        union {
            std::array<uint32_t, REGS_COUNT> reg_buffer = {};

            struct {
                uint32_t d0, d1, d2, d3, d4, d5, d6, d7;
                uint32_t a0, a1, a2, a3, a4, a5, a6;
                union { uint32_t a7; uint32_t usp; }; // user StackPointer
                uint32_t ssp;  // supervisor StackPointer
                uint32_t pc;
                union { // Status register
                    uint32_t srr;
                    struct SRF {
                        // lower byte
                        uint8_t carry : 1;
                        uint8_t overflow : 1;
                        uint8_t zero : 1;
                        uint8_t negative : 1;
                        uint8_t extend : 1;
                        uint8_t : 3;

                        // upper byte
                        uint8_t interrupt_mask : 3;
                        uint8_t : 1;
                        uint8_t master_switch : 1;
                        uint8_t supervisor : 1;
                        uint8_t trace : 2;
                    } sr;
                };
            };
            struct {
                uint32_t d[8];
                uint32_t a[8];
            };
        };
        M68K_MSVC_POP_WARNING()

    public:
        Registers() = default;

        uint32_t get(RegisterType reg_ind, DataSize size = DataSize::SIZE_LONG);
        void set(RegisterType reg_ind, DataSize size, uint32_t data);

        bool get(StatusRegisterFlag flag);
        void set(StatusRegisterFlag flag, bool value);

        uint32_t& stack_ptr() {
            return sr.supervisor ? ssp : usp;
        }

        template <typename T>
        void set_(RegisterType reg_ind, T val) {
            DataSize sz = (DataSize)(sizeof(T));
            set(reg_ind, sz, (uint32_t)val);
        }
    };
}
