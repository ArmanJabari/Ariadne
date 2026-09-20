#include "editor_document.h"
#include "emulator.h"
#include "assembler.h"
#include <fstream>
#include <algorithm>

bool EditorDocument::LoadFromFile(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return false;

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    binary_buffer.resize(size);
    if (!file.read(reinterpret_cast<char*>(binary_buffer.data()), size)) {
        binary_buffer.clear();
        return false;
    }
    file.close();
    return true;
}

bool EditorDocument::SaveToFile(const std::string& filepath) const {
    std::ofstream outFile(filepath, std::ios::binary);
    if (!outFile.is_open()) return false;
    outFile.write(reinterpret_cast<const char*>(binary_buffer.data()), binary_buffer.size());
    outFile.close();
    return true;
}

void EditorDocument::Clear() {
    binary_buffer.clear();
    undo_stack.clear();
    redo_stack.clear();
}

bool EditorDocument::CanUndo() const {
    return !undo_stack.empty();
}

bool EditorDocument::CanRedo() const {
    return !redo_stack.empty();
}

void EditorDocument::Undo() {
    if (!undo_stack.empty()) {
        redo_stack.push_back(binary_buffer);
        binary_buffer = undo_stack.back();
        undo_stack.pop_back();
    }
}

void EditorDocument::Redo() {
    if (!redo_stack.empty()) {
        undo_stack.push_back(binary_buffer);
        binary_buffer = redo_stack.back();
        redo_stack.pop_back();
    }
}

void EditorDocument::PushUndo() {
    undo_stack.push_back(binary_buffer);
    redo_stack.clear();
}

void EditorDocument::AdjustJumpOffsets(const std::vector<DisasmLine>& old_lines, uint64_t cur_addr, int mod_idx, size_t old_len, size_t new_len, bool is_insert) {
    if (old_lines.empty()) return;

    int delta = static_cast<int>(new_len) - static_cast<int>(old_len);
    if (delta == 0) return;

    struct JumpFixup {
        uint64_t new_jump_addr;
        uint64_t new_target_addr;
    };
    std::vector<JumpFixup> fixups;

    struct MemFixup {
        uint64_t new_insn_addr;
        uint32_t old_target;
        uint32_t new_target;
        uint32_t insn_size;
    };
    std::vector<MemFixup> mem_fixups;

    for (int i = 0; i < static_cast<int>(old_lines.size()); i++) {
        if (!is_insert && i == mod_idx) continue;

        const auto& ln = old_lines[i];
        if (ln.is_data) continue;

        if (IsJumpInstruction(ln.mnemonic)) {
            uint64_t old_target = ParseJumpTargetAddress(ln.op_str);
            if (old_target == 0) continue;

            uint64_t new_jump_addr = ln.address;
            if (is_insert) {
                if (ln.address >= cur_addr) {
                    new_jump_addr = ln.address + delta;
                }
            } else {
                if (ln.address > cur_addr) {
                    new_jump_addr = ln.address + delta;
                }
            }

            uint64_t new_target_addr = old_target;
            if (is_insert) {
                if (old_target >= cur_addr) {
                    new_target_addr = old_target + delta;
                }
            } else {
                if (old_target > cur_addr) {
                    if (old_target >= cur_addr + old_len) {
                        new_target_addr = old_target + delta;
                    } else {
                        new_target_addr = cur_addr;
                    }
                } else if (old_target == cur_addr) {
                    new_target_addr = cur_addr;
                }
            }

            fixups.push_back({ new_jump_addr, new_target_addr });
        } else {
            uint32_t mem_target = 0;
            bool is_b = false, is_w = false;
            if (ParseDirectMemory(ln.op_str, mem_target, is_b, is_w) && mem_target >= 0x0100) {
                uint64_t new_insn_addr = ln.address;
                if (is_insert) {
                    if (ln.address >= cur_addr) {
                        new_insn_addr = ln.address + delta;
                    }
                } else {
                    if (ln.address > cur_addr) {
                        new_insn_addr = ln.address + delta;
                    }
                }

                uint32_t new_target = mem_target;
                if (is_insert) {
                    if (mem_target >= cur_addr) {
                        new_target = mem_target + delta;
                    }
                } else {
                    if (mem_target > cur_addr) {
                        if (mem_target >= cur_addr + old_len) {
                            new_target = mem_target + delta;
                        } else {
                            new_target = static_cast<uint32_t>(cur_addr);
                        }
                    } else if (mem_target == cur_addr) {
                        new_target = static_cast<uint32_t>(cur_addr);
                    }
                }

                if (new_target != mem_target) {
                    mem_fixups.push_back({ new_insn_addr, mem_target, new_target, ln.size });
                }
            }
        }
    }

    for (const auto& f : fixups) {
        if (f.new_jump_addr < 0x0100) continue;
        size_t idx = static_cast<size_t>(f.new_jump_addr - 0x0100);
        if (idx >= binary_buffer.size()) continue;

        uint8_t op = binary_buffer[idx];

        bool is_short_jcc = (op >= 0x70 && op <= 0x7F);
        bool is_loop_or_jcxz = (op >= 0xE0 && op <= 0xE3);
        bool is_jmp_short = (op == 0xEB);

        if (is_short_jcc || is_loop_or_jcxz || is_jmp_short) {
            int64_t rel = static_cast<int64_t>(f.new_target_addr) - static_cast<int64_t>(f.new_jump_addr + 2);
            if (rel >= -128 && rel <= 127 && idx + 1 < binary_buffer.size()) {
                binary_buffer[idx + 1] = static_cast<uint8_t>(rel & 0xFF);
            }
        } else if (op == 0xE9 || op == 0xE8) {
            int64_t rel = static_cast<int64_t>(f.new_target_addr) - static_cast<int64_t>(f.new_jump_addr + 3);
            if (rel >= -32768 && rel <= 32767 && idx + 2 < binary_buffer.size()) {
                binary_buffer[idx + 1] = static_cast<uint8_t>(rel & 0xFF);
                binary_buffer[idx + 2] = static_cast<uint8_t>((rel >> 8) & 0xFF);
            }
        } else if (op == 0x0F && idx + 3 < binary_buffer.size()) {
            uint8_t op2 = binary_buffer[idx + 1];
            if (op2 >= 0x80 && op2 <= 0x8F) {
                int64_t rel = static_cast<int64_t>(f.new_target_addr) - static_cast<int64_t>(f.new_jump_addr + 4);
                if (rel >= -32768 && rel <= 32767) {
                    binary_buffer[idx + 2] = static_cast<uint8_t>(rel & 0xFF);
                    binary_buffer[idx + 3] = static_cast<uint8_t>((rel >> 8) & 0xFF);
                }
            }
        }
    }

    for (const auto& mf : mem_fixups) {
        if (mf.new_insn_addr < 0x0100) continue;
        size_t idx = static_cast<size_t>(mf.new_insn_addr - 0x0100);
        if (idx >= binary_buffer.size()) continue;

        uint8_t old_lo = static_cast<uint8_t>(mf.old_target & 0xFF);
        uint8_t old_hi = static_cast<uint8_t>((mf.old_target >> 8) & 0xFF);
        uint8_t new_lo = static_cast<uint8_t>(mf.new_target & 0xFF);
        uint8_t new_hi = static_cast<uint8_t>((mf.new_target >> 8) & 0xFF);

        size_t max_scan = std::min(static_cast<size_t>(mf.insn_size), binary_buffer.size() - idx);
        for (size_t offset = 0; offset + 1 < max_scan; offset++) {
            if (binary_buffer[idx + offset] == old_lo && binary_buffer[idx + offset + 1] == old_hi) {
                binary_buffer[idx + offset] = new_lo;
                binary_buffer[idx + offset + 1] = new_hi;
                break;
            }
        }
    }
}