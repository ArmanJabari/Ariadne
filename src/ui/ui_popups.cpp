#include "ui_popups.h"
#include "assembler.h"
#include "imgui/imgui.h"
#include <algorithm>

void RenderPopups(bool& open_assemble_popup, bool& open_insert_popup, char* edit_instruction_buf, size_t edit_buf_size, char* insert_instruction_buf, size_t insert_buf_size, std::string& assemble_error, int& selected_index, int& scrollToLine, std::vector<DisasmLine>& lines, EditorDocument& doc, Debugger& dbg) {
    if (open_assemble_popup) {
        ImGui::OpenPopup("Edit Line");
        open_assemble_popup = false;
    }

    if (ImGui::BeginPopupModal("Edit Line", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (selected_index >= 0 && selected_index < static_cast<int>(lines.size())) {
            uint64_t cur_addr = lines[selected_index].address;

            if (ImGui::IsWindowAppearing()) {
                ImGui::SetKeyboardFocusHere();
            }
            bool enter_pressed = ImGui::InputText("##edit_inst", edit_instruction_buf, edit_buf_size, ImGuiInputTextFlags_EnterReturnsTrue);

            if (!assemble_error.empty()) {
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", assemble_error.c_str());
            }

            auto ApplyAssemble = [&]() {
                std::string trimmed_cmd = TrimString(edit_instruction_buf);
                std::vector<uint8_t> new_bytes;

                if (trimmed_cmd.empty()) {
                    new_bytes = { 0x90 };
                } else if (!AssembleInstruction(trimmed_cmd, cur_addr, new_bytes) || new_bytes.empty()) {
                    assemble_error = "Invalid instruction or hex bytes!";
                    return;
                }

                size_t buf_offset = static_cast<size_t>(cur_addr - 0x0100);
                if (buf_offset <= doc.binary_buffer.size()) {
                    doc.PushUndo();

                    size_t del_len = std::min(static_cast<size_t>(lines[selected_index].size), doc.binary_buffer.size() - buf_offset);
                    doc.binary_buffer.erase(doc.binary_buffer.begin() + buf_offset, doc.binary_buffer.begin() + buf_offset + del_len);
                    doc.binary_buffer.insert(doc.binary_buffer.begin() + buf_offset, new_bytes.begin(), new_bytes.end());
                    doc.AdjustJumpOffsets(lines, cur_addr, selected_index, del_len, new_bytes.size(), false);
                    lines = DisassembleBuffer(doc.binary_buffer);
                    scrollToLine = selected_index;
                    dbg.dbg_active = false;
                    dbg.dbg_history.clear();
                }
                ImGui::CloseCurrentPopup();
            };

            if (enter_pressed || ImGui::Button("Replace")) {
                ApplyAssemble();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                ImGui::CloseCurrentPopup();
            }
        } else {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (open_insert_popup) {
        ImGui::OpenPopup("Insert Line");
        open_insert_popup = false;
    }

    if (ImGui::BeginPopupModal("Insert Line", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (selected_index >= 0 && selected_index < static_cast<int>(lines.size())) {
            uint64_t insert_addr = lines[selected_index].address + lines[selected_index].size;

            if (ImGui::IsWindowAppearing()) {
                ImGui::SetKeyboardFocusHere();
            }
            bool enter_pressed = ImGui::InputText("##insert_inst", insert_instruction_buf, insert_buf_size, ImGuiInputTextFlags_EnterReturnsTrue);

            if (!assemble_error.empty()) {
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", assemble_error.c_str());
            }

            auto ApplyInsertAfter = [&]() {
                std::string trimmed_cmd = TrimString(insert_instruction_buf);
                std::vector<uint8_t> new_bytes;

                if (trimmed_cmd.empty()) {
                    new_bytes = { 0x90 };
                } else if (!AssembleInstruction(trimmed_cmd, insert_addr, new_bytes) || new_bytes.empty()) {
                    assemble_error = "Invalid instruction or hex bytes!";
                    return;
                }

                size_t buf_offset = static_cast<size_t>(insert_addr - 0x0100);
                if (buf_offset <= doc.binary_buffer.size()) {
                    doc.PushUndo();

                    doc.binary_buffer.insert(doc.binary_buffer.begin() + buf_offset, new_bytes.begin(), new_bytes.end());
                    doc.AdjustJumpOffsets(lines, insert_addr, -1, 0, new_bytes.size(), true);
                    lines = DisassembleBuffer(doc.binary_buffer);
                    selected_index = selected_index + 1;
                    scrollToLine = selected_index;
                    dbg.dbg_active = false;
                    dbg.dbg_history.clear();
                }
                ImGui::CloseCurrentPopup();
            };

            if (enter_pressed || ImGui::Button("Insert")) {
                ApplyInsertAfter();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                ImGui::CloseCurrentPopup();
            }
        } else {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}