#include "theme.h"

AppTheme g_theme;

void ApplyThemeDark() {
    g_theme.mode = THEME_DARK;
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_Text]                  = ImVec4(0.85f, 0.87f, 0.91f, 1.00f);
    colors[ImGuiCol_TextDisabled]          = ImVec4(0.45f, 0.48f, 0.55f, 1.00f);
    colors[ImGuiCol_WindowBg]              = ImVec4(0.11f, 0.12f, 0.16f, 1.00f);
    colors[ImGuiCol_ChildBg]               = ImVec4(0.13f, 0.14f, 0.19f, 1.00f);
    colors[ImGuiCol_PopupBg]               = ImVec4(0.14f, 0.15f, 0.20f, 0.98f);
    colors[ImGuiCol_Border]                = ImVec4(0.20f, 0.22f, 0.29f, 1.00f);
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]               = ImVec4(0.16f, 0.17f, 0.23f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.22f, 0.24f, 0.32f, 1.00f);
    colors[ImGuiCol_FrameBgActive]         = ImVec4(0.26f, 0.28f, 0.38f, 1.00f);
    colors[ImGuiCol_TitleBg]               = ImVec4(0.10f, 0.11f, 0.14f, 1.00f);
    colors[ImGuiCol_TitleBgActive]         = ImVec4(0.13f, 0.14f, 0.19f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.10f, 0.11f, 0.14f, 1.00f);
    colors[ImGuiCol_MenuBarBg]             = ImVec4(0.14f, 0.15f, 0.20f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.11f, 0.12f, 0.16f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.24f, 0.26f, 0.35f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.30f, 0.33f, 0.44f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.36f, 0.40f, 0.53f, 1.00f);
    colors[ImGuiCol_CheckMark]             = ImVec4(0.45f, 0.65f, 0.95f, 1.00f);
    colors[ImGuiCol_SliderGrab]            = ImVec4(0.40f, 0.55f, 0.85f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.50f, 0.68f, 0.98f, 1.00f);
    colors[ImGuiCol_Button]                = ImVec4(0.18f, 0.20f, 0.27f, 1.00f);
    colors[ImGuiCol_ButtonHovered]         = ImVec4(0.25f, 0.28f, 0.38f, 1.00f);
    colors[ImGuiCol_ButtonActive]          = ImVec4(0.30f, 0.34f, 0.46f, 1.00f);
    colors[ImGuiCol_Header]                = ImVec4(0.20f, 0.23f, 0.32f, 1.00f);
    colors[ImGuiCol_HeaderHovered]         = ImVec4(0.26f, 0.30f, 0.41f, 1.00f);
    colors[ImGuiCol_HeaderActive]          = ImVec4(0.31f, 0.36f, 0.49f, 1.00f);
    colors[ImGuiCol_Separator]             = ImVec4(0.20f, 0.22f, 0.29f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.32f, 0.36f, 0.48f, 1.00f);
    colors[ImGuiCol_SeparatorActive]       = ImVec4(0.42f, 0.47f, 0.62f, 1.00f);
    colors[ImGuiCol_ResizeGrip]            = ImVec4(0.24f, 0.26f, 0.35f, 1.00f);
    colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.35f, 0.39f, 0.52f, 1.00f);
    colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.45f, 0.50f, 0.66f, 1.00f);
    colors[ImGuiCol_Tab]                   = ImVec4(0.14f, 0.16f, 0.22f, 1.00f);
    colors[ImGuiCol_TabHovered]            = ImVec4(0.24f, 0.27f, 0.37f, 1.00f);
    colors[ImGuiCol_TabActive]             = ImVec4(0.19f, 0.22f, 0.30f, 1.00f);
    colors[ImGuiCol_TabUnfocused]          = ImVec4(0.12f, 0.13f, 0.18f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.16f, 0.18f, 0.24f, 1.00f);

    style.WindowRounding    = 0.0f;
    style.ChildRounding     = 0.0f;
    style.FrameRounding     = 2.0f;
    style.PopupRounding     = 2.0f;
    style.ScrollbarRounding = 2.0f;
    style.GrabRounding      = 2.0f;
    style.TabRounding       = 2.0f;

    g_theme.col_header          = ImVec4(0.45f, 0.65f, 0.95f, 1.00f);
    g_theme.col_offset          = ImVec4(0.45f, 0.58f, 0.78f, 1.00f);
    g_theme.col_bytes           = ImVec4(0.55f, 0.60f, 0.70f, 1.00f);

    g_theme.col_mnemonic_mov    = ImVec4(0.35f, 0.75f, 0.85f, 1.00f);
    g_theme.col_mnemonic_ctrl   = ImVec4(0.78f, 0.55f, 0.92f, 1.00f);
    g_theme.col_mnemonic_math   = ImVec4(0.35f, 0.82f, 0.65f, 1.00f);
    g_theme.col_mnemonic_stack  = ImVec4(0.92f, 0.72f, 0.40f, 1.00f);
    g_theme.col_mnemonic_logic  = ImVec4(0.85f, 0.45f, 0.65f, 1.00f);
    g_theme.col_mnemonic_other  = ImVec4(0.85f, 0.87f, 0.91f, 1.00f);

    g_theme.col_op_reg          = ImVec4(0.40f, 0.75f, 0.95f, 1.00f);
    g_theme.col_op_num          = ImVec4(0.88f, 0.65f, 0.42f, 1.00f);
    g_theme.col_op_mem          = ImVec4(0.85f, 0.55f, 0.75f, 1.00f);
    g_theme.col_op_punct        = ImVec4(0.55f, 0.58f, 0.65f, 1.00f);

    g_theme.col_data_mnem       = ImVec4(0.45f, 0.52f, 0.65f, 1.00f);
    g_theme.col_data_op         = ImVec4(0.70f, 0.82f, 0.55f, 1.00f);
    g_theme.col_highlight       = ImVec4(0.95f, 0.78f, 0.35f, 1.00f);

    g_theme.bp_row_col          = IM_COL32(85, 25, 32, 130);
    g_theme.bp_marker_col       = IM_COL32(235, 65, 75, 255);
    g_theme.arrow_active_col    = ImColor(0.95f, 0.78f, 0.35f, 1.00f);
    g_theme.arrow_up_col        = ImColor(0.85f, 0.45f, 0.45f, 0.85f);
    g_theme.arrow_down_col      = ImColor(0.45f, 0.75f, 0.85f, 0.85f);
}

void ApplyThemeLight() {
    g_theme.mode = THEME_LIGHT;
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_Text]                  = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);
    colors[ImGuiCol_TextDisabled]          = ImVec4(0.55f, 0.56f, 0.58f, 1.00f);
    colors[ImGuiCol_WindowBg]              = ImVec4(0.92f, 0.90f, 0.86f, 1.00f);
    colors[ImGuiCol_ChildBg]               = ImVec4(0.96f, 0.94f, 0.91f, 1.00f);
    colors[ImGuiCol_PopupBg]               = ImVec4(0.97f, 0.95f, 0.92f, 0.98f);
    colors[ImGuiCol_Border]                = ImVec4(0.80f, 0.77f, 0.72f, 1.00f);
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]               = ImVec4(0.88f, 0.86f, 0.82f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.83f, 0.80f, 0.75f, 1.00f);
    colors[ImGuiCol_FrameBgActive]         = ImVec4(0.78f, 0.75f, 0.70f, 1.00f);
    colors[ImGuiCol_TitleBg]               = ImVec4(0.87f, 0.85f, 0.80f, 1.00f);
    colors[ImGuiCol_TitleBgActive]         = ImVec4(0.84f, 0.81f, 0.76f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.87f, 0.85f, 0.80f, 1.00f);
    colors[ImGuiCol_MenuBarBg]             = ImVec4(0.89f, 0.87f, 0.83f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.92f, 0.90f, 0.86f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.76f, 0.73f, 0.68f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.68f, 0.64f, 0.58f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.58f, 0.54f, 0.48f, 1.00f);
    colors[ImGuiCol_CheckMark]             = ImVec4(0.20f, 0.45f, 0.72f, 1.00f);
    colors[ImGuiCol_SliderGrab]            = ImVec4(0.28f, 0.50f, 0.75f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.20f, 0.42f, 0.68f, 1.00f);
    colors[ImGuiCol_Button]                = ImVec4(0.86f, 0.83f, 0.78f, 1.00f);
    colors[ImGuiCol_ButtonHovered]         = ImVec4(0.80f, 0.76f, 0.70f, 1.00f);
    colors[ImGuiCol_ButtonActive]          = ImVec4(0.74f, 0.70f, 0.64f, 1.00f);
    colors[ImGuiCol_Header]                = ImVec4(0.83f, 0.79f, 0.73f, 1.00f);
    colors[ImGuiCol_HeaderHovered]         = ImVec4(0.77f, 0.73f, 0.67f, 1.00f);
    colors[ImGuiCol_HeaderActive]          = ImVec4(0.71f, 0.67f, 0.60f, 1.00f);
    colors[ImGuiCol_Separator]             = ImVec4(0.80f, 0.77f, 0.72f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.68f, 0.65f, 0.60f, 1.00f);
    colors[ImGuiCol_SeparatorActive]       = ImVec4(0.55f, 0.52f, 0.47f, 1.00f);
    colors[ImGuiCol_ResizeGrip]            = ImVec4(0.76f, 0.73f, 0.68f, 1.00f);
    colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.68f, 0.64f, 0.58f, 1.00f);
    colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.58f, 0.54f, 0.48f, 1.00f);
    colors[ImGuiCol_Tab]                   = ImVec4(0.87f, 0.84f, 0.79f, 1.00f);
    colors[ImGuiCol_TabHovered]            = ImVec4(0.80f, 0.76f, 0.70f, 1.00f);
    colors[ImGuiCol_TabActive]             = ImVec4(0.96f, 0.94f, 0.91f, 1.00f);
    colors[ImGuiCol_TabUnfocused]          = ImVec4(0.89f, 0.86f, 0.82f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.93f, 0.91f, 0.88f, 1.00f);

    style.WindowRounding    = 0.0f;
    style.ChildRounding     = 0.0f;
    style.FrameRounding     = 2.0f;
    style.PopupRounding     = 2.0f;
    style.ScrollbarRounding = 2.0f;
    style.GrabRounding      = 2.0f;
    style.TabRounding       = 2.0f;

    g_theme.col_header          = ImVec4(0.18f, 0.38f, 0.62f, 1.00f);
    g_theme.col_offset          = ImVec4(0.38f, 0.44f, 0.54f, 1.00f);
    g_theme.col_bytes           = ImVec4(0.48f, 0.50f, 0.52f, 1.00f);

    g_theme.col_mnemonic_mov    = ImVec4(0.12f, 0.45f, 0.58f, 1.00f);
    g_theme.col_mnemonic_ctrl   = ImVec4(0.50f, 0.22f, 0.60f, 1.00f);
    g_theme.col_mnemonic_math   = ImVec4(0.15f, 0.52f, 0.36f, 1.00f);
    g_theme.col_mnemonic_stack  = ImVec4(0.68f, 0.40f, 0.15f, 1.00f);
    g_theme.col_mnemonic_logic  = ImVec4(0.65f, 0.22f, 0.42f, 1.00f);
    g_theme.col_mnemonic_other  = ImVec4(0.24f, 0.27f, 0.30f, 1.00f);

    g_theme.col_op_reg          = ImVec4(0.18f, 0.38f, 0.65f, 1.00f);
    g_theme.col_op_num          = ImVec4(0.66f, 0.34f, 0.10f, 1.00f);
    g_theme.col_op_mem          = ImVec4(0.55f, 0.24f, 0.50f, 1.00f);
    g_theme.col_op_punct        = ImVec4(0.45f, 0.47f, 0.50f, 1.00f);

    g_theme.col_data_mnem       = ImVec4(0.45f, 0.47f, 0.50f, 1.00f);
    g_theme.col_data_op         = ImVec4(0.30f, 0.48f, 0.22f, 1.00f);
    g_theme.col_highlight       = ImVec4(0.78f, 0.42f, 0.05f, 1.00f);

    g_theme.bp_row_col          = IM_COL32(230, 185, 180, 160);
    g_theme.bp_marker_col       = IM_COL32(195, 45, 45, 255);
    g_theme.arrow_active_col    = ImColor(0.78f, 0.42f, 0.05f, 1.00f);
    g_theme.arrow_up_col        = ImColor(0.70f, 0.28f, 0.28f, 0.85f);
    g_theme.arrow_down_col      = ImColor(0.22f, 0.45f, 0.65f, 0.85f);
}