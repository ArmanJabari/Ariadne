#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "cpu_state.h"

struct DisasmLine {
    uint64_t address;
    uint32_t size;
    std::string bytes;
    std::string mnemonic;
    std::string op_str;
    bool is_data;
    CPUState state;
};

std::vector<DisasmLine> DisassembleBuffer(const std::vector<uint8_t>& buffer);