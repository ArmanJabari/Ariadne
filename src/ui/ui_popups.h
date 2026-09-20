#pragma once
#include <vector>
#include <string>
#include "disassembler.h"
#include "editor_document.h"
#include "debugger.h"

void RenderPopups(bool& open_assemble_popup, bool& open_insert_popup, char* edit_instruction_buf, size_t edit_buf_size, char* insert_instruction_buf, size_t insert_buf_size, std::string& assemble_error, int& selected_index, int& scrollToLine, std::vector<DisasmLine>& lines, EditorDocument& doc, Debugger& dbg);