#include "debugger.h"
#include "emulator.h"
#include <algorithm>

void Debugger::Reset(const std::vector<uint8_t>& binary_buffer, std::vector<DisasmLine>& lines, int& selected_index, int& scrollToLine) {
    if (lines.empty()) return;
    dbg_state = CPUState();
    dbg_state.ip = 0x0100;
    dbg_state.sp = 0xFFFE;
    dbg_state.cs = 0x1000;
    dbg_state.ds = 0x1000;
    dbg_state.es = 0x1000;
    dbg_state.ss = 0x1000;
    dbg_state.flag_if = true;
    for (size_t b = 0; b < binary_buffer.size() && (0x0100 + b) < 0x10000; b++) {
        dbg_state.memory[0x0100 + b] = binary_buffer[b];
    }
    prev_dbg_state = dbg_state;
    selected_index = 0;
    scrollToLine = 0;
    dbg_active = true;
    nav_history.clear();
    dbg_history.clear();
    lines[0].state = dbg_state;
}

void Debugger::ExecuteStep(bool step_over, const std::vector<uint8_t>& binary_buffer, std::vector<DisasmLine>& lines, int& selected_index, int& scrollToLine) {
    if (lines.empty()) return;

    if (!dbg_active) {
        dbg_state = CPUState();
        dbg_state.ip = 0x0100;
        dbg_state.sp = 0xFFFE;
        dbg_state.cs = 0x1000;
        dbg_state.ds = 0x1000;
        dbg_state.es = 0x1000;
        dbg_state.ss = 0x1000;
        dbg_state.flag_if = true;
        for (size_t b = 0; b < binary_buffer.size() && (0x0100 + b) < 0x10000; b++) {
            dbg_state.memory[0x0100 + b] = binary_buffer[b];
        }
        if (selected_index > 0 && selected_index < static_cast<int>(lines.size())) {
            dbg_state = lines[selected_index - 1].state;
            dbg_state.ip = static_cast<uint16_t>(lines[selected_index].address);
        }
        prev_dbg_state = dbg_state;
        dbg_active = true;
    }

    if (selected_index < 0) selected_index = 0;
    if (selected_index >= static_cast<int>(lines.size())) return;

    dbg_history.push_back({ selected_index, dbg_state, prev_dbg_state, nav_history });

    const auto& cur_line = lines[selected_index];
    prev_dbg_state = dbg_state;

    if (cur_line.is_data) {
        selected_index++;
        if (selected_index < static_cast<int>(lines.size())) {
            dbg_state.ip = static_cast<uint16_t>(lines[selected_index].address);
        }
        scrollToLine = selected_index;
        return;
    }

    std::string m = cur_line.mnemonic;
    std::transform(m.begin(), m.end(), m.begin(), ::tolower);

    if (m == "loop" || m == "loope" || m == "loopz" || m == "loopne" || m == "loopnz") {
        dbg_state.cx--;
        bool take_branch = false;
        if (m == "loop") {
            take_branch = (dbg_state.cx != 0);
        } else if (m == "loope" || m == "loopz") {
            take_branch = (dbg_state.cx != 0 && dbg_state.flag_zf);
        } else if (m == "loopne" || m == "loopnz") {
            take_branch = (dbg_state.cx != 0 && !dbg_state.flag_zf);
        }

        if (step_over) {
            while (dbg_state.cx > 0) {
                dbg_state.cx--;
                if (m == "loope" || m == "loopz") {
                    if (!dbg_state.flag_zf) break;
                } else if (m == "loopne" || m == "loopnz") {
                    if (dbg_state.flag_zf) break;
                }
            }
            take_branch = false;
        }

        if (take_branch) {
            uint64_t target_addr = ParseJumpTargetAddress(cur_line.op_str);
            int target_idx = -1;
            for (int idx = 0; idx < static_cast<int>(lines.size()); idx++) {
                if (lines[idx].address == target_addr) {
                    target_idx = idx;
                    break;
                }
            }
            if (target_idx != -1) {
                selected_index = target_idx;
            } else {
                selected_index++;
            }
        } else {
            selected_index++;
        }
    } else if (m == "jcxz") {
        bool take_branch = (dbg_state.cx == 0);
        if (take_branch) {
            uint64_t target_addr = ParseJumpTargetAddress(cur_line.op_str);
            int target_idx = -1;
            for (int idx = 0; idx < static_cast<int>(lines.size()); idx++) {
                if (lines[idx].address == target_addr) {
                    target_idx = idx;
                    break;
                }
            }
            if (target_idx != -1) {
                selected_index = target_idx;
            } else {
                selected_index++;
            }
        } else {
            selected_index++;
        }
    } else if (IsJumpInstruction(m) && m != "call") {
        bool take_branch = false;
        if (m == "jmp") take_branch = true;
        else if (m == "je" || m == "jz") take_branch = dbg_state.flag_zf;
        else if (m == "jne" || m == "jnz") take_branch = !dbg_state.flag_zf;
        else if (m == "js") take_branch = dbg_state.flag_sf;
        else if (m == "jns") take_branch = !dbg_state.flag_sf;
        else if (m == "jo") take_branch = dbg_state.flag_of;
        else if (m == "jno") take_branch = !dbg_state.flag_of;
        else if (m == "jb" || m == "jc" || m == "jnae") take_branch = dbg_state.flag_cf;
        else if (m == "jae" || m == "jnb" || m == "jnc") take_branch = !dbg_state.flag_cf;
        else if (m == "jbe" || m == "jna") take_branch = (dbg_state.flag_cf || dbg_state.flag_zf);
        else if (m == "ja" || m == "jnbe") take_branch = (!dbg_state.flag_cf && !dbg_state.flag_zf);
        else if (m == "jl" || m == "jnge") take_branch = (dbg_state.flag_sf != dbg_state.flag_of);
        else if (m == "jge" || m == "jnl") take_branch = (dbg_state.flag_sf == dbg_state.flag_of);
        else if (m == "jle" || m == "jng") take_branch = (dbg_state.flag_zf || (dbg_state.flag_sf != dbg_state.flag_of));
        else if (m == "jg" || m == "jnle") take_branch = (!dbg_state.flag_zf && (dbg_state.flag_sf == dbg_state.flag_of));
        else if (m == "jp" || m == "jpe") take_branch = dbg_state.flag_pf;
        else if (m == "jnp" || m == "jpo") take_branch = !dbg_state.flag_pf;

        if (take_branch) {
            uint64_t target_addr = ParseJumpTargetAddress(cur_line.op_str);
            int target_idx = -1;
            for (int idx = 0; idx < static_cast<int>(lines.size()); idx++) {
                if (lines[idx].address == target_addr) {
                    target_idx = idx;
                    break;
                }
            }
            if (target_idx != -1) {
                selected_index = target_idx;
            } else {
                selected_index++;
            }
        } else {
            selected_index++;
        }
    } else if (m == "call") {
        if (step_over) {
            selected_index++;
        } else {
            uint16_t ret_addr = static_cast<uint16_t>(cur_line.address + cur_line.size);
            dbg_state.push(ret_addr);
            nav_history.push_back(selected_index);
            uint64_t target_addr = ParseJumpTargetAddress(cur_line.op_str);
            int target_idx = -1;
            for (int idx = 0; idx < static_cast<int>(lines.size()); idx++) {
                if (lines[idx].address == target_addr) {
                    target_idx = idx;
                    break;
                }
            }
            if (target_idx != -1) {
                selected_index = target_idx;
            } else {
                selected_index++;
            }
        }
    } else if (m == "ret" || m == "retn" || m == "retf") {
        uint16_t ret_addr = dbg_state.pop();
        int target_idx = -1;
        for (int idx = 0; idx < static_cast<int>(lines.size()); idx++) {
            if (lines[idx].address == ret_addr) {
                target_idx = idx;
                break;
            }
        }
        if (target_idx != -1) {
            selected_index = target_idx;
        } else if (!nav_history.empty()) {
            selected_index = nav_history.back() + 1;
            nav_history.pop_back();
        } else {
            selected_index++;
        }
    } else {
        SimulateInstruction(cur_line, dbg_state);
        selected_index++;
    }

    if (selected_index >= static_cast<int>(lines.size())) {
        selected_index = static_cast<int>(lines.size()) - 1;
    }
    if (selected_index >= 0) {
        dbg_state.ip = static_cast<uint16_t>(lines[selected_index].address);
        lines[selected_index].state = dbg_state;
    }
    scrollToLine = selected_index;
}

void Debugger::StepBack(std::vector<DisasmLine>& lines, int& selected_index, int& scrollToLine) {
    if (!dbg_history.empty()) {
        DbgSnapshot snap = dbg_history.back();
        dbg_history.pop_back();
        selected_index = snap.index;
        dbg_state = snap.state;
        prev_dbg_state = snap.prev_state;
        nav_history = snap.call_history;
        if (selected_index >= 0 && selected_index < static_cast<int>(lines.size())) {
            lines[selected_index].state = dbg_state;
        }
        scrollToLine = selected_index;
    }
}

void Debugger::Run(const std::vector<uint8_t>& binary_buffer, std::vector<DisasmLine>& lines, int& selected_index, int& scrollToLine) {
    if (!lines.empty()) {
        int max_steps = 100000;
        while (max_steps-- > 0) {
            ExecuteStep(false, binary_buffer, lines, selected_index, scrollToLine);
            if (selected_index < 0 || selected_index >= static_cast<int>(lines.size())) break;
            uint64_t cur_addr = lines[selected_index].address;
            if (breakpoints.find(cur_addr) != breakpoints.end()) {
                break;
            }
            const auto& cur_ln = lines[selected_index];
            if (cur_ln.mnemonic == "int" && (cur_ln.op_str == "0x21" || cur_ln.op_str == "33")) {
                if ((dbg_state.ax >> 8) == 0x4C) break;
            }
        }
    }
}

void Debugger::ToggleBreakpoint(uint64_t addr) {
    auto it = breakpoints.find(addr);
    if (it != breakpoints.end()) {
        breakpoints.erase(it);
    } else {
        breakpoints.insert(addr);
    }
}