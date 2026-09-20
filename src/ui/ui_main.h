#pragma once
#include <windows.h>
#include <vector>
#include "disassembler.h"
#include "editor_document.h"
#include "debugger.h"

struct UIContext {
    char filePathBuf[MAX_PATH] = "";
    std::vector<DisasmLine> lines;
    int selected_index = -1;
    int scrollToLine = -1;

    bool split_ax = false;
    bool split_bx = false;
    bool split_cx = false;
    bool split_dx = false;
    bool show_decimal = false;

    bool open_assemble_popup = false;
    bool open_insert_popup = false;
    char edit_instruction_buf[128] = "";
    char insert_instruction_buf[128] = "";
    std::string assemble_error = "";
};

void RenderUI(HWND hwnd, bool& done, UIContext& ui, EditorDocument& doc, Debugger& dbg);