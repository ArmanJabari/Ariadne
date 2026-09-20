#include "file_dialogs.h"
#include <commdlg.h>
#include <cstring>

bool OpenFileDialog(HWND hwnd, char* outPath, size_t maxPathLen) {
    OPENFILENAMEA ofn;
    char szFile[MAX_PATH] = "";

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Supported Files (*.com;*.bin)\0*.com;*.bin\0DOS COM Executable (*.com)\0*.com\0Raw Binary (*.bin)\0*.bin\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = nullptr;
    ofn.lpstrTitle = "Select a file";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn)) {
        strncpy(outPath, szFile, maxPathLen - 1);
        outPath[maxPathLen - 1] = '\0';
        return true;
    }
    return false;
}

bool SaveFileDialog(HWND hwnd, char* outPath, size_t maxPathLen) {
    OPENFILENAMEA ofn;
    char szFile[MAX_PATH] = "";

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "DOS COM Executable (*.com)\0*.com\0Raw Binary (*.bin)\0*.bin\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = nullptr;
    ofn.lpstrTitle = "Save File";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

    if (GetSaveFileNameA(&ofn)) {
        strncpy(outPath, szFile, maxPathLen - 1);
        outPath[maxPathLen - 1] = '\0';
        return true;
    }
    return false;
}