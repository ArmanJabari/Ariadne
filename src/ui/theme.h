#pragma once

#include <windows.h>
#include "imgui.h"

enum ThemeMode {
    THEME_DARK,
    THEME_LIGHT
};

struct AppTheme {
    ThemeMode mode;

    ImVec4 col_header;
    ImVec4 col_offset;
    ImVec4 col_bytes;

    ImVec4 col_mnemonic_mov;
    ImVec4 col_mnemonic_ctrl;
    ImVec4 col_mnemonic_math;
    ImVec4 col_mnemonic_stack;
    ImVec4 col_mnemonic_logic;
    ImVec4 col_mnemonic_other;

    ImVec4 col_op_reg;
    ImVec4 col_op_num;
    ImVec4 col_op_mem;
    ImVec4 col_op_punct;

    ImVec4 col_data_mnem;
    ImVec4 col_data_op;
    ImVec4 col_highlight;

    ImU32  bp_row_col;
    ImU32  bp_marker_col;
    ImColor arrow_active_col;
    ImColor arrow_up_col;
    ImColor arrow_down_col;
};

extern AppTheme g_theme;

void UpdateTitleBarTheme(HWND hwnd, bool dark);
void ApplyThemeDark(HWND hwnd);
void ApplyThemeLight(HWND hwnd);
void SaveThemePreference();
void LoadAndApplyTheme(HWND hwnd);