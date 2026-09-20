#pragma once
#include <vector>
#include "imgui/imgui.h"
#include "disassembler.h"

struct JumpArrowInfo {
    int src;
    int dst;
    int dist;
    int lane;
};

void ComputeJumpArrows(const std::vector<DisasmLine>& lines, std::vector<JumpArrowInfo>& jump_arrows, int& total_lanes);
void RenderJumpArrows(const std::vector<DisasmLine>& lines, const std::vector<JumpArrowInfo>& jump_arrows, const std::vector<float>& line_y, float gutter_base_x, float total_gutter_width, float lane_spacing, int& selected_index, int& scrollToLine, std::vector<int>& nav_history);