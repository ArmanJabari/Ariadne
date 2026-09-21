#include "ui_main.h"
#include "theme.h"
#include "jump_arrows.h"
#include "ui_popups.h"
#include "file_dialogs.h"
#include "emulator.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

static uint16_t ExtractTargetAddress(const std::string& op_str, bool& has_addr, bool& is_bracket) {
    has_addr = false;
    is_bracket = false;
    size_t open_b = op_str.find('[');
    size_t close_b = op_str.find(']');
    if (open_b != std::string::npos && close_b != std::string::npos && close_b > open_b) {
        std::string inner = op_str.substr(open_b + 1, close_b - open_b - 1);
        size_t hex_pos = inner.find("0x");
        if (hex_pos == std::string::npos) hex_pos = inner.find("0X");
        if (hex_pos != std::string::npos) {
            try {
                unsigned long val = std::stoul(inner.substr(hex_pos), nullptr, 16);
                if (val >= 0x0100 && val <= 0xFFFF) {
                    has_addr = true;
                    is_bracket = true;
                    return static_cast<uint16_t>(val);
                }
            } catch (...) {}
        }
    }
    size_t hex_pos = op_str.find("0x");
    if (hex_pos == std::string::npos) hex_pos = op_str.find("0X");
    if (hex_pos != std::string::npos) {
        try {
            unsigned long val = std::stoul(op_str.substr(hex_pos), nullptr, 16);
            if (val >= 0x0100 && val <= 0xFFFF) {
                has_addr = true;
                is_bracket = false;
                return static_cast<uint16_t>(val);
            }
        } catch (...) {}
    }
    return 0;
}

static std::string ResolveAnnotation(uint16_t addr, bool is_bracket, const std::vector<uint8_t>& buf) {
    if (addr < 0x0100) return "";
    size_t offset = static_cast<size_t>(addr - 0x0100);
    if (offset >= buf.size()) return "";

    bool is_string = false;
    size_t str_len = 0;
    for (size_t i = offset; i < buf.size() && (i - offset) < 128; ++i) {
        uint8_t b = buf[i];
        if (b == '$' || b == 0x00) {
            if (str_len >= 1) {
                is_string = true;
            }
            break;
        }
        if (b >= 32 && b <= 126) {
            str_len++;
        } else {
            break;
        }
    }

    if (is_string) {
        std::string res = "; \"";
        for (size_t i = offset; i < buf.size() && (i - offset) < 128; ++i) {
            uint8_t b = buf[i];
            if (b == '$') {
                res += '$';
                break;
            }
            if (b == 0x00) {
                break;
            }
            res += static_cast<char>(b);
        }
        res += "\"";
        return res;
    }

    if (!is_bracket) {
        return "";
    }

    if (offset + 1 < buf.size()) {
        uint16_t val = static_cast<uint16_t>(buf[offset]) | (static_cast<uint16_t>(buf[offset + 1]) << 8);
        return "; = " + std::to_string(val);
    } else {
        uint8_t val = buf[offset];
        return "; = " + std::to_string(static_cast<int>(val));
    }
}

void RenderUI(HWND hwnd, bool& done, UIContext& ui, EditorDocument& doc, Debugger& dbg) {
    ImGuiIO& io = ImGui::GetIO();

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::Begin("Ariadne", nullptr, 
        ImGuiWindowFlags_NoTitleBar | 
        ImGuiWindowFlags_NoResize | 
        ImGuiWindowFlags_NoMove | 
        ImGuiWindowFlags_NoScrollbar | 
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_MenuBar);

    auto SyncToSelectedLine = [&](int target_idx) {
        if (target_idx < 0 || target_idx >= static_cast<int>(ui.lines.size())) return;
        dbg.Reset(doc.binary_buffer, ui.lines, ui.selected_index, ui.scrollToLine);
        for (int k = 0; k <= target_idx; ++k) {
            dbg.ExecuteStep(false, doc.binary_buffer, ui.lines, ui.selected_index, ui.scrollToLine);
        }
        ui.selected_index = target_idx;
        ui.scrollToLine = target_idx;
    };

    auto TriggerOpenFile = [&]() {
        if (OpenFileDialog(hwnd, ui.filePathBuf, sizeof(ui.filePathBuf))) {
            if (doc.LoadFromFile(ui.filePathBuf)) {
                ui.lines = DisassembleBuffer(doc.binary_buffer);
                dbg.nav_history.clear();
                dbg.breakpoints.clear();
                doc.undo_stack.clear();
                doc.redo_stack.clear();
                ui.selected_index = ui.lines.empty() ? -1 : 0;
                ui.scrollToLine = ui.selected_index;
                dbg.dbg_active = false;
                dbg.dbg_history.clear();
                if (!ui.lines.empty()) {
                    SyncToSelectedLine(0);
                }
            }
        }
    };

    auto TriggerSaveFile = [&]() {
        if (!doc.binary_buffer.empty()) {
            char savePath[MAX_PATH] = "";
            if (SaveFileDialog(hwnd, savePath, sizeof(savePath))) {
                doc.SaveToFile(savePath);
            }
        }
    };

    auto TriggerReload = [&]() {
        if (ui.filePathBuf[0] != '\0') {
            if (doc.LoadFromFile(ui.filePathBuf)) {
                ui.lines = DisassembleBuffer(doc.binary_buffer);
                dbg.nav_history.clear();
                dbg.breakpoints.clear();
                doc.undo_stack.clear();
                doc.redo_stack.clear();
                ui.selected_index = ui.lines.empty() ? -1 : 0;
                ui.scrollToLine = ui.selected_index;
                dbg.dbg_active = false;
                dbg.dbg_history.clear();
                if (!ui.lines.empty()) {
                    SyncToSelectedLine(0);
                }
            }
        }
    };

    auto TriggerUndo = [&]() {
        if (doc.CanUndo()) {
            doc.Undo();
            ui.lines = DisassembleBuffer(doc.binary_buffer);
            if (ui.selected_index >= static_cast<int>(ui.lines.size())) {
                ui.selected_index = ui.lines.empty() ? -1 : static_cast<int>(ui.lines.size()) - 1;
            }
            ui.scrollToLine = ui.selected_index;
            dbg.dbg_active = false;
            dbg.dbg_history.clear();
            if (ui.selected_index >= 0) {
                SyncToSelectedLine(ui.selected_index);
            }
        }
    };

    auto TriggerRedo = [&]() {
        if (doc.CanRedo()) {
            doc.Redo();
            ui.lines = DisassembleBuffer(doc.binary_buffer);
            if (ui.selected_index >= static_cast<int>(ui.lines.size())) {
                ui.selected_index = ui.lines.empty() ? -1 : static_cast<int>(ui.lines.size()) - 1;
            }
            ui.scrollToLine = ui.selected_index;
            dbg.dbg_active = false;
            dbg.dbg_history.clear();
            if (ui.selected_index >= 0) {
                SyncToSelectedLine(ui.selected_index);
            }
        }
    };

    auto TriggerEdit = [&]() {
        if (ui.selected_index >= 0 && ui.selected_index < static_cast<int>(ui.lines.size())) {
            ui.open_assemble_popup = true;
            std::string cur_text = ui.lines[ui.selected_index].mnemonic;
            if (!ui.lines[ui.selected_index].op_str.empty()) {
                cur_text += " " + ui.lines[ui.selected_index].op_str;
            }
            strncpy(ui.edit_instruction_buf, cur_text.c_str(), sizeof(ui.edit_instruction_buf) - 1);
            ui.edit_instruction_buf[sizeof(ui.edit_instruction_buf) - 1] = '\0';
            ui.assemble_error = "";
        }
    };

    auto TriggerInsert = [&]() {
        if (ui.selected_index >= 0 && ui.selected_index < static_cast<int>(ui.lines.size())) {
            ui.open_insert_popup = true;
            ui.insert_instruction_buf[0] = '\0';
            ui.assemble_error = "";
        }
    };

    auto TriggerDelete = [&]() {
        if (ui.selected_index >= 0 && ui.selected_index < static_cast<int>(ui.lines.size())) {
            uint64_t cur_addr = ui.lines[ui.selected_index].address;
            size_t buf_offset = static_cast<size_t>(cur_addr - 0x0100);
            if (buf_offset < doc.binary_buffer.size()) {
                doc.PushUndo();

                size_t del_len = std::min(static_cast<size_t>(ui.lines[ui.selected_index].size), doc.binary_buffer.size() - buf_offset);
                doc.binary_buffer.erase(doc.binary_buffer.begin() + buf_offset, doc.binary_buffer.begin() + buf_offset + del_len);
                doc.AdjustJumpOffsets(ui.lines, cur_addr, ui.selected_index, del_len, 0, false);
                ui.lines = DisassembleBuffer(doc.binary_buffer);
                if (ui.selected_index >= static_cast<int>(ui.lines.size())) {
                    ui.selected_index = ui.lines.empty() ? -1 : static_cast<int>(ui.lines.size()) - 1;
                }
                ui.scrollToLine = ui.selected_index;
                dbg.dbg_active = false;
                dbg.dbg_history.clear();
                if (ui.selected_index >= 0) {
                    SyncToSelectedLine(ui.selected_index);
                }
            }
        }
    };

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open File", "Ctrl+O")) {
                TriggerOpenFile();
            }
            if (ImGui::MenuItem("Save File", "Ctrl+S", false, !doc.binary_buffer.empty())) {
                TriggerSaveFile();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Reload", "F5", false, ui.filePathBuf[0] != '\0')) {
                TriggerReload();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                done = true;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Debugger")) {
            if (ImGui::MenuItem("Run", "F9", false, !ui.lines.empty())) {
                dbg.Run(doc.binary_buffer, ui.lines, ui.selected_index, ui.scrollToLine);
            }
            if (ImGui::MenuItem("Step Into", "F7", false, !ui.lines.empty())) {
                dbg.ExecuteStep(false, doc.binary_buffer, ui.lines, ui.selected_index, ui.scrollToLine);
            }
            if (ImGui::MenuItem("Step Over", "F8", false, !ui.lines.empty())) {
                dbg.ExecuteStep(true, doc.binary_buffer, ui.lines, ui.selected_index, ui.scrollToLine);
            }
            if (ImGui::MenuItem("Step Back", "Alt+F7", false, !dbg.dbg_history.empty())) {
                dbg.StepBack(ui.lines, ui.selected_index, ui.scrollToLine);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Restart", "Ctrl+F2", false, !ui.lines.empty())) {
                dbg.Reset(doc.binary_buffer, ui.lines, ui.selected_index, ui.scrollToLine);
                if (!ui.lines.empty()) {
                    SyncToSelectedLine(0);
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Toggle Breakpoint", "F2", false, ui.selected_index >= 0)) {
                if (ui.selected_index >= 0 && ui.selected_index < static_cast<int>(ui.lines.size())) {
                    dbg.ToggleBreakpoint(ui.lines[ui.selected_index].address);
                }
            }
            if (ImGui::MenuItem("Clear All Breakpoints", nullptr, false, !dbg.breakpoints.empty())) {
                dbg.breakpoints.clear();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Patching")) {
            if (ImGui::MenuItem("Edit Line", "Space", false, ui.selected_index >= 0)) {
                TriggerEdit();
            }
            if (ImGui::MenuItem("Insert Line", "Shift+Space", false, ui.selected_index >= 0)) {
                TriggerInsert();
            }
            if (ImGui::MenuItem("Delete Line", "Ctrl+D", false, ui.selected_index >= 0)) {
                TriggerDelete();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Undo", "Ctrl+Z", false, doc.CanUndo())) {
                TriggerUndo();
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Y", false, doc.CanRedo())) {
                TriggerRedo();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Theme")) {
            if (ImGui::MenuItem("Dark", nullptr, g_theme.mode == THEME_DARK)) {
                ApplyThemeDark();
            }
            if (ImGui::MenuItem("Light", nullptr, g_theme.mode == THEME_LIGHT)) {
                ApplyThemeLight();
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }

    bool popup_active = ImGui::IsPopupOpen("Edit Line") || ImGui::IsPopupOpen("Insert Line");

    if (!ImGui::IsAnyItemActive() && !popup_active) {
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_O, false)) {
            TriggerOpenFile();
        }
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false)) {
            TriggerSaveFile();
        }
        if (ImGui::IsKeyPressed(ImGuiKey_F5, false)) {
            TriggerReload();
        }
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false)) {
            TriggerUndo();
        }
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y, false)) {
            TriggerRedo();
        }
        if ((io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D, false)) || ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
            TriggerDelete();
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Space, false)) {
            if (io.KeyShift) {
                TriggerInsert();
            } else {
                TriggerEdit();
            }
        }
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
            if (ui.selected_index > 0) {
                SyncToSelectedLine(ui.selected_index - 1);
            } else if (ui.selected_index == -1 && !ui.lines.empty()) {
                SyncToSelectedLine(0);
            }
        }
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
            if (!ui.lines.empty() && ui.selected_index < static_cast<int>(ui.lines.size()) - 1) {
                SyncToSelectedLine(ui.selected_index + 1);
            }
        }
        if (ImGui::IsKeyPressed(ImGuiKey_F2, false) && !io.KeyCtrl) {
            if (ui.selected_index >= 0 && ui.selected_index < static_cast<int>(ui.lines.size())) {
                dbg.ToggleBreakpoint(ui.lines[ui.selected_index].address);
            }
        }
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_F2, false)) {
            dbg.Reset(doc.binary_buffer, ui.lines, ui.selected_index, ui.scrollToLine);
            if (!ui.lines.empty()) {
                SyncToSelectedLine(0);
            }
        }
        if (io.KeyAlt && ImGui::IsKeyPressed(ImGuiKey_F7, true)) {
            dbg.StepBack(ui.lines, ui.selected_index, ui.scrollToLine);
        } else if (!io.KeyAlt && ImGui::IsKeyPressed(ImGuiKey_F7, true)) {
            dbg.ExecuteStep(false, doc.binary_buffer, ui.lines, ui.selected_index, ui.scrollToLine);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_F8, true)) {
            dbg.ExecuteStep(true, doc.binary_buffer, ui.lines, ui.selected_index, ui.scrollToLine);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_F9, false)) {
            dbg.Run(doc.binary_buffer, ui.lines, ui.selected_index, ui.scrollToLine);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false)) {
            if (ui.selected_index >= 0 && ui.selected_index < static_cast<int>(ui.lines.size())) {
                const auto& cur_line = ui.lines[ui.selected_index];
                if (!cur_line.is_data && IsJumpInstruction(cur_line.mnemonic)) {
                    uint64_t target_addr = ParseJumpTargetAddress(cur_line.op_str);
                    for (int idx = 0; idx < static_cast<int>(ui.lines.size()); idx++) {
                        if (ui.lines[idx].address == target_addr) {
                            dbg.nav_history.push_back(ui.selected_index);
                            SyncToSelectedLine(idx);
                            break;
                        }
                    }
                }
            }
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
            if (!dbg.nav_history.empty()) {
                int prev_idx = dbg.nav_history.back();
                dbg.nav_history.pop_back();
                SyncToSelectedLine(prev_idx);
            } else if (ui.selected_index >= 0 && ui.selected_index < static_cast<int>(ui.lines.size())) {
                uint64_t my_addr = ui.lines[ui.selected_index].address;
                for (int idx = 0; idx < static_cast<int>(ui.lines.size()); idx++) {
                    if (!ui.lines[idx].is_data && IsJumpInstruction(ui.lines[idx].mnemonic)) {
                        if (ParseJumpTargetAddress(ui.lines[idx].op_str) == my_addr) {
                            SyncToSelectedLine(idx);
                            break;
                        }
                    }
                }
            }
        }
    } else {
        if (!ImGui::IsAnyItemActive() && !popup_active) {
            if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_O, false)) {
                TriggerOpenFile();
            }
        }
    }

    float right_panel_width = 280.0f;
    float remaining_width = ImGui::GetContentRegionAvail().x - right_panel_width - 16.0f;
    float col_offset_width = 230.0f;
    float col_asm_width = remaining_width - col_offset_width;
    float total_avail_y = ImGui::GetContentRegionAvail().y;

    static float sync_scroll_y = 0.0f;

    ImGui::BeginChild("OffsetOpcodeRegion", ImVec2(col_offset_width, total_avail_y), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::TextColored(g_theme.col_header, "Offset");
    ImGui::SameLine(90.0f);
    ImGui::TextColored(g_theme.col_header, "Opcode Bytes");
    ImGui::Separator();

    ImGui::BeginChild("OffsetOpcodeScrollArea", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar);

    if (ui.scrollToLine == -1) {
        ImGui::SetScrollY(sync_scroll_y);
    }

    for (int i = 0; i < static_cast<int>(ui.lines.size()); i++) {
        const auto& line = ui.lines[i];
        bool is_selected = (ui.selected_index == i);
        bool has_bp = (dbg.breakpoints.find(line.address) != dbg.breakpoints.end());

        if (ui.scrollToLine == i) {
            ImGui::SetScrollHereY(0.5f);
            sync_scroll_y = ImGui::GetScrollY();
        }

        ImVec2 row_pos = ImGui::GetCursorScreenPos();
        float row_h = ImGui::GetTextLineHeightWithSpacing();
        float avail_w = ImGui::GetWindowWidth();

        if (has_bp && !is_selected) {
            ImGui::GetWindowDrawList()->AddRectFilled(row_pos, ImVec2(row_pos.x + avail_w, row_pos.y + row_h), g_theme.bp_row_col);
        }

        if (has_bp) {
            if (g_theme.mode == THEME_DARK) {
                ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.70f, 0.15f, 0.20f, 0.80f));
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.78f, 0.20f, 0.25f, 0.85f));
                ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.85f, 0.25f, 0.30f, 0.90f));
            } else {
                ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.92f, 0.55f, 0.60f, 0.80f));
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.95f, 0.62f, 0.66f, 0.85f));
                ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.96f, 0.70f, 0.73f, 0.90f));
            }
        }

        char selectable_id[64];
        snprintf(selectable_id, sizeof(selectable_id), "##line_%d", i);

        if (ImGui::Selectable(selectable_id, is_selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap)) {
            SyncToSelectedLine(i);
        }

        if (has_bp) {
            ImGui::PopStyleColor(3);
        }

        ImGui::SameLine(0.0f, 0.0f);
        ImGui::TextColored(g_theme.col_offset, "0x%04X:", (unsigned int)line.address);
        ImGui::SameLine(90.0f);
        ImGui::TextColored(g_theme.col_bytes, "%s", line.bytes.c_str());
    }

    ImGui::EndChild();
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("AssemblyCodeRegion", ImVec2(col_asm_width, total_avail_y), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::TextColored(g_theme.col_header, "Assembly Code");
    ImGui::Separator();

    ImGui::BeginChild("AssemblyCodeScrollArea", ImVec2(0, 0), false);

    if (ui.scrollToLine == -1) {
        if (ImGui::GetScrollY() != sync_scroll_y) {
            sync_scroll_y = ImGui::GetScrollY();
        }
    }

    std::vector<JumpArrowInfo> jump_arrows;
    int total_lanes = 0;
    ComputeJumpArrows(ui.lines, jump_arrows, total_lanes);

    const float lane_spacing = 7.0f;
    const float sel_indicator_width = 16.0f;
    float total_gutter_width = sel_indicator_width + (total_lanes > 0 ? (total_lanes * lane_spacing + 14.0f) : 18.0f);
    if (total_gutter_width < 34.0f) total_gutter_width = 34.0f;

    std::vector<float> line_y(ui.lines.size(), 0.0f);
    float gutter_base_x = 0.0f;

    for (int i = 0; i < static_cast<int>(ui.lines.size()); i++) {
        const auto& line = ui.lines[i];
        bool is_selected = (ui.selected_index == i);
        bool has_bp = (dbg.breakpoints.find(line.address) != dbg.breakpoints.end());

        if (ui.scrollToLine == i) {
            ImGui::SetScrollHereY(0.5f);
            sync_scroll_y = ImGui::GetScrollY();
        }

        ImVec2 cursor_screen_pos = ImGui::GetCursorScreenPos();
        float line_height = ImGui::GetTextLineHeight();
        line_y[i] = std::floor(cursor_screen_pos.y + (line_height * 0.5f));
        gutter_base_x = cursor_screen_pos.x;

        if (is_selected) {
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            float center_y = line_y[i];
            float start_x = cursor_screen_pos.x + 3.0f;

            if (has_bp) {
                ImVec2 r_min = ImVec2(start_x, center_y - 4.5f);
                ImVec2 r_max = ImVec2(start_x + 9.0f, center_y + 4.5f);
                draw_list->AddRectFilled(r_min, r_max, g_theme.bp_marker_col);
            } else {
                ImVec2 p1 = ImVec2(start_x, center_y - 4.5f);
                ImVec2 p2 = ImVec2(start_x + 6.5f, center_y);
                ImVec2 p3 = ImVec2(start_x, center_y + 4.5f);
                draw_list->AddTriangleFilled(p1, p2, p3, g_theme.arrow_active_col);
            }
        }

        ImGui::Dummy(ImVec2(total_gutter_width, 0.0f));
        ImGui::SameLine(0.0f, 0.0f);

        if (line.is_data) {
            ImGui::TextColored(g_theme.col_data_mnem, "%s", line.mnemonic.c_str());
            ImGui::SameLine();
            ImGui::TextColored(g_theme.col_data_op, "%s", line.op_str.c_str());
        } else {
            ImVec4 mnemonic_color = g_theme.col_mnemonic_other;
            if (line.mnemonic == "mov" || line.mnemonic == "lea" || line.mnemonic == "xchg" || line.mnemonic == "lodsb" || line.mnemonic == "stosb" || line.mnemonic == "movsb" || line.mnemonic == "movsw") {
                mnemonic_color = g_theme.col_mnemonic_mov;
            } else if (line.mnemonic == "call" || line.mnemonic == "ret" || line.mnemonic == "retn" || line.mnemonic == "retf" ||
                       line.mnemonic == "jmp" || line.mnemonic[0] == 'j' || line.mnemonic.find("loop") == 0 ||
                       line.mnemonic == "int" || line.mnemonic == "into" || line.mnemonic == "iret") {
                mnemonic_color = g_theme.col_mnemonic_ctrl;
            } else if (line.mnemonic == "add" || line.mnemonic == "sub" || line.mnemonic == "inc" || line.mnemonic == "dec" ||
                       line.mnemonic == "mul" || line.mnemonic == "imul" || line.mnemonic == "div" || line.mnemonic == "idiv" ||
                       line.mnemonic == "cmp" || line.mnemonic == "neg" || line.mnemonic == "cbw" || line.mnemonic == "cwd") {
                mnemonic_color = g_theme.col_mnemonic_math;
            } else if (line.mnemonic == "push" || line.mnemonic == "pop" || line.mnemonic == "pushf" || line.mnemonic == "popf" || line.mnemonic == "pusha" || line.mnemonic == "popa") {
                mnemonic_color = g_theme.col_mnemonic_stack;
            } else if (line.mnemonic == "xor" || line.mnemonic == "and" || line.mnemonic == "or" || line.mnemonic == "not" ||
                       line.mnemonic == "test" || line.mnemonic == "shl" || line.mnemonic == "shr" || line.mnemonic == "sal" || line.mnemonic == "sar" || line.mnemonic == "rol" || line.mnemonic == "ror") {
                mnemonic_color = g_theme.col_mnemonic_logic;
            }

            ImGui::TextColored(mnemonic_color, "%s", line.mnemonic.c_str());
            if (!line.op_str.empty()) {
                ImGui::SameLine();
                const std::string& ops = line.op_str;
                size_t p = 0;
                while (p < ops.size()) {
                    if (ops[p] == ' ') {
                        ImGui::Text(" ");
                        ImGui::SameLine(0.0f, 0.0f);
                        p++;
                    } else if (ops[p] == ',' || ops[p] == '+' || ops[p] == '-' || ops[p] == '*' || ops[p] == ':') {
                        char ch[2] = { ops[p], '\0' };
                        ImGui::TextColored(g_theme.col_op_punct, "%s", ch);
                        if (p + 1 < ops.size()) ImGui::SameLine(0.0f, 0.0f);
                        p++;
                    } else if (ops[p] == '[' || ops[p] == ']') {
                        char ch[2] = { ops[p], '\0' };
                        ImGui::TextColored(g_theme.col_op_mem, "%s", ch);
                        if (p + 1 < ops.size()) ImGui::SameLine(0.0f, 0.0f);
                        p++;
                    } else {
                        size_t start = p;
                        while (p < ops.size() && ops[p] != ' ' && ops[p] != ',' && ops[p] != '+' && ops[p] != '-' && ops[p] != '*' && ops[p] != ':' && ops[p] != '[' && ops[p] != ']') {
                            p++;
                        }
                        std::string token = ops.substr(start, p - start);
                        ImVec4 tcol = g_theme.col_op_punct;

                        if ((token[0] >= '0' && token[0] <= '9') || (token.size() > 2 && token[0] == '0' && (token[1] == 'x' || token[1] == 'X'))) {
                            tcol = g_theme.col_op_num;
                        } else if (token == "ax" || token == "bx" || token == "cx" || token == "dx" ||
                                   token == "ah" || token == "al" || token == "bh" || token == "bl" ||
                                   token == "ch" || token == "cl" || token == "dh" || token == "dl" ||
                                   token == "si" || token == "di" || token == "bp" || token == "sp" ||
                                   token == "cs" || token == "ds" || token == "es" || token == "ss" || token == "ip") {
                            tcol = g_theme.col_op_reg;
                        } else if (token == "byte" || token == "word" || token == "dword" || token == "ptr" || token == "offset" || token == "short") {
                            tcol = g_theme.col_mnemonic_other;
                        }

                        ImGui::TextColored(tcol, "%s", token.c_str());
                        if (p < ops.size()) {
                            ImGui::SameLine(0.0f, 0.0f);
                        }
                    }
                }
            }

            bool has_addr = false;
            bool is_bracket = false;
            uint16_t target_addr = ExtractTargetAddress(line.op_str, has_addr, is_bracket);
            if (has_addr && !IsJumpInstruction(line.mnemonic)) {
                std::string annot = ResolveAnnotation(target_addr, is_bracket, doc.binary_buffer);
                if (!annot.empty()) {
                    float annot_x = total_gutter_width + 280.0f;
                    if (ImGui::GetCursorPosX() < annot_x) {
                        ImGui::SameLine(annot_x);
                    } else {
                        ImGui::SameLine(0.0f, 15.0f);
                    }
                    ImGui::TextColored(ImVec4(0.55f, 0.68f, 0.35f, 0.85f), "%s", annot.c_str());
                }
            }
        }
    }

    RenderJumpArrows(ui.lines, jump_arrows, line_y, gutter_base_x, total_gutter_width, lane_spacing, ui.selected_index, ui.scrollToLine, dbg.nav_history);

    sync_scroll_y = ImGui::GetScrollY();

    if (ui.scrollToLine != -1) {
        ui.scrollToLine = -1;
    }

    ImGui::EndChild();
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginGroup();

    CPUState default_initial_state;

    CPUState display_state = dbg.dbg_active 
                             ? dbg.dbg_state 
                             : ((ui.selected_index >= 0 && ui.selected_index < static_cast<int>(ui.lines.size())) 
                                ? ui.lines[ui.selected_index].state 
                                : default_initial_state);

    CPUState prev_state = dbg.dbg_active 
                          ? dbg.prev_dbg_state 
                          : ((ui.selected_index > 0 && ui.selected_index < static_cast<int>(ui.lines.size())) 
                             ? ui.lines[ui.selected_index - 1].state 
                             : (ui.selected_index == 0 ? default_initial_state : display_state));

    ImGui::BeginChild("CpuRegistersSection", ImVec2(right_panel_width, 0.0f), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar);
    ImGui::TextColored(g_theme.col_header, "CPU Registers");
    ImGui::Separator();

    const float second_col_x = 75.0f;

    auto RenderGpRegister = [&](const char* reg_16, const char* reg_h, const char* reg_l, uint16_t current_val, uint16_t prev_val, bool& is_split) {
        ImVec2 start_pos = ImGui::GetCursorPos();
        uint8_t cur_h = static_cast<uint8_t>((current_val >> 8) & 0xFF);
        uint8_t prev_h = static_cast<uint8_t>((prev_val >> 8) & 0xFF);
        uint8_t cur_l = static_cast<uint8_t>(current_val & 0xFF);
        uint8_t prev_l = static_cast<uint8_t>(prev_val & 0xFF);

        bool changed_h = (ui.selected_index >= 0 && cur_h != prev_h);
        bool changed_l = (ui.selected_index >= 0 && cur_l != prev_l);
        bool changed_16 = (ui.selected_index >= 0 && current_val != prev_val);

        if (!is_split) {
            if (changed_16) ImGui::PushStyleColor(ImGuiCol_Text, g_theme.col_highlight);
            if (ui.show_decimal) {
                ImGui::Text("%s: %-5u", reg_16, current_val);
            } else {
                ImGui::Text("%s: 0x%04X", reg_16, current_val);
            }
            if (changed_16) ImGui::PopStyleColor();
        } else {
            if (changed_h) ImGui::PushStyleColor(ImGuiCol_Text, g_theme.col_highlight);
            if (ui.show_decimal) {
                ImGui::Text("%s: %-3u", reg_h, cur_h);
            } else {
                ImGui::Text("%s: 0x%02X", reg_h, cur_h);
            }
            if (changed_h) ImGui::PopStyleColor();

            ImGui::SameLine(second_col_x);

            if (changed_l) ImGui::PushStyleColor(ImGuiCol_Text, g_theme.col_highlight);
            if (ui.show_decimal) {
                ImGui::Text("%s: %-3u", reg_l, cur_l);
            } else {
                ImGui::Text("%s: 0x%02X", reg_l, cur_l);
            }
            if (changed_l) ImGui::PopStyleColor();
        }

        ImVec2 end_pos = ImGui::GetCursorPos();
        ImGui::SetCursorPos(start_pos);
        char btn_id[32];
        snprintf(btn_id, sizeof(btn_id), "##inv_btn_%s", reg_16);
        if (ImGui::InvisibleButton(btn_id, ImVec2(150.0f, ImGui::GetTextLineHeight()))) {
        }
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
            is_split = !is_split;
        }
        ImGui::SetCursorPos(end_pos);
    };

    auto RenderPointerRegister = [&](const char* name, uint16_t current_val, uint16_t prev_val) {
        bool is_changed = (ui.selected_index >= 0 && current_val != prev_val);
        if (is_changed) {
            ImGui::PushStyleColor(ImGuiCol_Text, g_theme.col_highlight);
        }
        ImGui::Text("%s: 0x%04X", name, current_val);
        if (is_changed) {
            ImGui::PopStyleColor();
        }
    };

    ImVec2 line1_pos = ImGui::GetCursorPos();
    RenderGpRegister("AX", "AH", "AL", display_state.ax, prev_state.ax, ui.split_ax);
    ImVec2 after_ax_pos = ImGui::GetCursorPos();

    ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() - 40.0f, line1_pos.y));
    if (ImGui::SmallButton(ui.show_decimal ? "HEX##btn" : "DEC##btn")) {
        ui.show_decimal = !ui.show_decimal;
    }
    ImGui::SetCursorPos(after_ax_pos);

    RenderGpRegister("BX", "BH", "BL", display_state.bx, prev_state.bx, ui.split_bx);
    RenderGpRegister("CX", "CH", "CL", display_state.cx, prev_state.cx, ui.split_cx);
    RenderGpRegister("DX", "DH", "DL", display_state.dx, prev_state.dx, ui.split_dx);
    ImGui::Separator();

    RenderPointerRegister("SI", display_state.si, prev_state.si);
    RenderPointerRegister("DI", display_state.di, prev_state.di);
    RenderPointerRegister("BP", display_state.bp, prev_state.bp);
    RenderPointerRegister("SP", display_state.sp, prev_state.sp);
    ImGui::Text("IP: 0x%04X", display_state.ip);
    ImGui::Separator();

    ImGui::Text("CS: 0x%04X", display_state.cs);
    ImGui::Text("DS: 0x%04X", display_state.ds);
    ImGui::Text("ES: 0x%04X", display_state.es);
    ImGui::Text("SS: 0x%04X", display_state.ss);
    float box1_actual_height = ImGui::GetWindowSize().y;
    ImGui::EndChild();

    ImGui::Spacing();

    ImGui::BeginChild("FlagsRegisterSection", ImVec2(right_panel_width, 0.0f), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar);
    ImGui::TextColored(g_theme.col_header, "Flags Register");
    ImGui::Separator();

    auto RenderFlag = [&](const char* name, bool current_val, bool prev_val) {
        bool is_changed = (ui.selected_index >= 0 && current_val != prev_val);
        if (is_changed) {
            ImGui::PushStyleColor(ImGuiCol_Text, g_theme.col_highlight);
        }
        ImGui::Text("%s %d", name, current_val ? 1 : 0);
        if (is_changed) {
            ImGui::PopStyleColor();
        }
    };

    RenderFlag("CF (Carry):    ", display_state.flag_cf, prev_state.flag_cf);
    RenderFlag("ZF (Zero):     ", display_state.flag_zf, prev_state.flag_zf);
    RenderFlag("SF (Sign):     ", display_state.flag_sf, prev_state.flag_sf);
    RenderFlag("OF (Overflow): ", display_state.flag_of, prev_state.flag_of);
    RenderFlag("PF (Parity):   ", display_state.flag_pf, prev_state.flag_pf);
    RenderFlag("AF (Auxiliary):", display_state.flag_af, prev_state.flag_af);
    RenderFlag("IF (Interrupt):", display_state.flag_if, prev_state.flag_if);
    RenderFlag("DF (Direction):", display_state.flag_df, prev_state.flag_df);
    float box2_actual_height = ImGui::GetWindowSize().y;
    ImGui::EndChild();

    ImGui::Spacing();

    float spacing = ImGui::GetStyle().ItemSpacing.y;
    const float stack_bottom_margin = 8.0f;
    float box3_height = total_avail_y - box1_actual_height - box2_actual_height - (2.0f * spacing) - stack_bottom_margin;

    ImGui::BeginChild("StackSection", ImVec2(right_panel_width, box3_height), true);
    ImGui::TextColored(g_theme.col_header, "Stack");
    ImGui::Separator();

    ImGui::Text("Address     Value");
    ImGui::Separator();

    for (uint32_t addr = display_state.sp; addr <= 0xFFFE; addr += 2) {
        uint16_t stack_addr = static_cast<uint16_t>(addr);
        uint16_t stack_val = display_state.read_stack_word(stack_addr);
        ImGui::Text("0x%04X:    0x%04X", stack_addr, stack_val);
    }
    ImGui::EndChild();

    ImGui::EndGroup();

    RenderPopups(ui.open_assemble_popup, ui.open_insert_popup, ui.edit_instruction_buf, sizeof(ui.edit_instruction_buf), ui.insert_instruction_buf, sizeof(ui.insert_instruction_buf), ui.assemble_error, ui.selected_index, ui.scrollToLine, ui.lines, doc, dbg);

    ImGui::End();
}
