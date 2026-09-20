#pragma once
#include <vector>
#include <unordered_set>
#include <cstdint>
#include "cpu_state.h"
#include "disassembler.h"

struct DbgSnapshot {
    int index;
    CPUState state;
    CPUState prev_state;
    std::vector<int> call_history;
};

class Debugger {
public:
    CPUState dbg_state;
    CPUState prev_dbg_state;
    bool dbg_active = false;
    std::vector<DbgSnapshot> dbg_history;
    std::unordered_set<uint64_t> breakpoints;
    std::vector<int> nav_history;

    void Reset(const std::vector<uint8_t>& binary_buffer, std::vector<DisasmLine>& lines, int& selected_index, int& scrollToLine);
    void ExecuteStep(bool step_over, const std::vector<uint8_t>& binary_buffer, std::vector<DisasmLine>& lines, int& selected_index, int& scrollToLine);
    void StepBack(std::vector<DisasmLine>& lines, int& selected_index, int& scrollToLine);
    void Run(const std::vector<uint8_t>& binary_buffer, std::vector<DisasmLine>& lines, int& selected_index, int& scrollToLine);
    void ToggleBreakpoint(uint64_t addr);
};