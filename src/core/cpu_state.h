#pragma once
#include <cstdint>

struct CPUState {
    uint16_t ax = 0x0000;
    uint16_t bx = 0x0000;
    uint16_t cx = 0x0000;
    uint16_t dx = 0x0000;
    uint16_t si = 0x0000;
    uint16_t di = 0x0000;
    uint16_t bp = 0x0000;
    uint16_t sp = 0xFFFE;
    uint16_t ip = 0x0100;
    uint16_t cs = 0x1000;
    uint16_t ds = 0x1000;
    uint16_t es = 0x1000;
    uint16_t ss = 0x1000;

    bool flag_cf = false;
    bool flag_zf = false;
    bool flag_sf = false;
    bool flag_of = false;
    bool flag_pf = false;
    bool flag_af = false;
    bool flag_if = true;
    bool flag_df = false;

    uint8_t memory[0x10000] = { 0 };

    void push(uint16_t value);
    uint16_t pop();
    uint16_t read_stack_word(uint16_t addr) const;
};