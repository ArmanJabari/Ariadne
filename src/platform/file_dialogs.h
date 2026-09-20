#pragma once

#include <windows.h>
#include <cstddef>

bool OpenFileDialog(HWND hwnd, char* outPath, size_t maxPathLen);
bool SaveFileDialog(HWND hwnd, char* outPath, size_t maxPathLen);