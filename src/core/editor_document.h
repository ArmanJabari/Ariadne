#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include "disassembler.h"

class EditorDocument {
public:
    std::vector<uint8_t> binary_buffer;
    std::vector<std::vector<uint8_t>> undo_stack;
    std::vector<std::vector<uint8_t>> redo_stack;

    bool LoadFromFile(const std::string& filepath);
    bool SaveToFile(const std::string& filepath) const;
    void Clear();
    bool CanUndo() const;
    bool CanRedo() const;
    void Undo();
    void Redo();
    void PushUndo();
    void AdjustJumpOffsets(const std::vector<DisasmLine>& old_lines, uint64_t cur_addr, int mod_idx, size_t old_len, size_t new_len, bool is_insert);
};