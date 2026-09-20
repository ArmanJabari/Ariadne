#include "jump_arrows.h"
#include "emulator.h"
#include "theme.h"
#include <unordered_map>
#include <algorithm>
#include <cmath>

void ComputeJumpArrows(const std::vector<DisasmLine>& lines, std::vector<JumpArrowInfo>& jump_arrows, int& total_lanes) {
    jump_arrows.clear();
    std::unordered_map<uint64_t, int> addr_map;
    addr_map.reserve(lines.size());
    for (int idx = 0; idx < static_cast<int>(lines.size()); idx++) {
        addr_map[lines[idx].address] = idx;
    }

    for (int src_idx = 0; src_idx < static_cast<int>(lines.size()); src_idx++) {
        if (!lines[src_idx].is_data && IsJumpInstruction(lines[src_idx].mnemonic)) {
            uint64_t target_addr = ParseJumpTargetAddress(lines[src_idx].op_str);
            auto it = addr_map.find(target_addr);
            if (it != addr_map.end()) {
                int dst_idx = it->second;
                if (src_idx != dst_idx) {
                    jump_arrows.push_back({ src_idx, dst_idx, std::abs(dst_idx - src_idx), 0 });
                }
            }
        }
    }

    std::sort(jump_arrows.begin(), jump_arrows.end(), [](const JumpArrowInfo& a, const JumpArrowInfo& b) {
        return a.dist < b.dist;
    });

    std::vector<std::vector<std::pair<int, int>>> lane_occupancy;
    for (auto& j : jump_arrows) {
        int start_idx = std::min(j.src, j.dst);
        int end_idx = std::max(j.src, j.dst);
        int assigned_lane = -1;

        for (size_t l = 0; l < lane_occupancy.size(); l++) {
            bool collides = false;
            for (const auto& span : lane_occupancy[l]) {
                if (!(end_idx < span.first || start_idx > span.second)) {
                    collides = true;
                    break;
                }
            }
            if (!collides) {
                assigned_lane = static_cast<int>(l);
                break;
            }
        }

        if (assigned_lane == -1) {
            assigned_lane = static_cast<int>(lane_occupancy.size());
            lane_occupancy.push_back({});
        }

        j.lane = assigned_lane;
        lane_occupancy[assigned_lane].push_back({ start_idx, end_idx });
    }
    total_lanes = static_cast<int>(lane_occupancy.size());
}

void RenderJumpArrows(const std::vector<DisasmLine>& lines, const std::vector<JumpArrowInfo>& jump_arrows, const std::vector<float>& line_y, float gutter_base_x, float total_gutter_width, float lane_spacing, int& selected_index, int& scrollToLine, std::vector<int>& nav_history) {
    if (lines.empty() || jump_arrows.empty()) return;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    float gutter_right = std::floor(gutter_base_x + total_gutter_width - 4.0f);
    ImVec2 mouse_pos = ImGui::GetMousePos();

    auto IsPointNearSegment = [](ImVec2 p, ImVec2 a, ImVec2 b, float threshold) -> bool {
        float l2 = (b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y);
        if (l2 == 0.0f) return std::hypot(p.x - a.x, p.y - a.y) <= threshold;
        float t = std::max(0.0f, std::min(1.0f, ((p.x - a.x) * (b.x - a.x) + (p.y - a.y) * (b.y - a.y)) / l2));
        ImVec2 proj = ImVec2(a.x + t * (b.x - a.x), a.y + t * (b.y - a.y));
        return std::hypot(p.x - proj.x, p.y - proj.y) <= threshold;
    };

    int hovered_arrow_idx = -1;
    for (int idx = 0; idx < static_cast<int>(jump_arrows.size()); idx++) {
        const auto& j = jump_arrows[idx];
        float src_y = std::floor(line_y[j.src]);
        float dst_y = std::floor(line_y[j.dst]);
        float lane_x = std::floor((gutter_right - 8.0f) - (j.lane * lane_spacing));

        ImVec2 p0 = ImVec2(gutter_right - 2.0f, src_y);
        ImVec2 p1 = ImVec2(lane_x, src_y);
        ImVec2 p2 = ImVec2(lane_x, dst_y);
        ImVec2 p3 = ImVec2(gutter_right, dst_y);

        bool hit_geom = IsPointNearSegment(mouse_pos, p0, p1, 3.5f) ||
                        IsPointNearSegment(mouse_pos, p1, p2, 3.5f) ||
                        IsPointNearSegment(mouse_pos, p2, p3, 3.5f);

        if (hit_geom) {
            hovered_arrow_idx = idx;
            break;
        }
    }

    if (hovered_arrow_idx != -1 && ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered()) {
        const auto& hj = jump_arrows[hovered_arrow_idx];
        if (selected_index == hj.src) {
            nav_history.push_back(selected_index);
            selected_index = hj.dst;
            scrollToLine = hj.dst;
        } else {
            nav_history.push_back(selected_index);
            selected_index = hj.src;
            scrollToLine = hj.src;
        }
    }

    auto DrawSingleArrow = [&](int idx) {
        const auto& j = jump_arrows[idx];
        float src_y = std::floor(line_y[j.src]);
        float dst_y = std::floor(line_y[j.dst]);
        float lane_x = std::floor((gutter_right - 8.0f) - (j.lane * lane_spacing));

        bool is_active = (selected_index == j.src || selected_index == j.dst);
        bool is_hovered = (hovered_arrow_idx == idx);
        bool is_upward = (j.dst < j.src);

        ImU32 arrow_color;
        float thickness = 1.2f;

        if (is_hovered || is_active) {
            arrow_color = g_theme.arrow_active_col;
            thickness = is_hovered ? 2.4f : 1.8f;
        } else {
            if (is_upward) {
                arrow_color = g_theme.arrow_up_col;
            } else {
                arrow_color = g_theme.arrow_down_col;
            }
            thickness = 1.2f;
        }

        draw_list->AddCircleFilled(ImVec2(gutter_right - 2.0f, src_y), (is_hovered || is_active) ? 3.0f : 2.0f, arrow_color);

        ImVec2 path[4] = {
            ImVec2(gutter_right - 2.0f, src_y),
            ImVec2(lane_x, src_y),
            ImVec2(lane_x, dst_y),
            ImVec2(gutter_right, dst_y)
        };
        draw_list->AddPolyline(path, 4, arrow_color, 0, thickness);

        float wing_size = is_hovered ? 4.5f : (is_active ? 4.0f : 3.5f);
        ImVec2 wings[3] = {
            ImVec2(gutter_right - (wing_size + 1.5f), dst_y - wing_size),
            ImVec2(gutter_right, dst_y),
            ImVec2(gutter_right - (wing_size + 1.5f), dst_y + wing_size)
        };
        draw_list->AddPolyline(wings, 3, arrow_color, 0, thickness);
    };

    for (int idx = 0; idx < static_cast<int>(jump_arrows.size()); idx++) {
        bool is_active = (selected_index == jump_arrows[idx].src || selected_index == jump_arrows[idx].dst);
        bool is_hovered = (hovered_arrow_idx == idx);
        if (!is_active && !is_hovered) {
            DrawSingleArrow(idx);
        }
    }

    for (int idx = 0; idx < static_cast<int>(jump_arrows.size()); idx++) {
        bool is_active = (selected_index == jump_arrows[idx].src || selected_index == jump_arrows[idx].dst);
        bool is_hovered = (hovered_arrow_idx == idx);
        if (is_active && !is_hovered) {
            DrawSingleArrow(idx);
        }
    }

    if (hovered_arrow_idx != -1) {
        DrawSingleArrow(hovered_arrow_idx);
    }
}