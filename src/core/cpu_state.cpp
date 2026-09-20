#include "cpu_state.h"

void CPUState::push(uint16_t value) {
    sp -= 2;
    memory[sp] = static_cast<uint8_t>(value & 0xFF);
    memory[sp + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
}

uint16_t CPUState::pop() {
    uint16_t val = memory[sp] | (memory[sp + 1] << 8);
    sp += 2;
    return val;
}

uint16_t CPUState::read_stack_word(uint16_t addr) const {
    return memory[addr] | (memory[addr + 1] << 8);
}