#include <windows.h>
#include <d3d9.h>
#include "d3d_device.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx9.h"
#include "theme.h"
#include "ui_main.h"
#include "editor_document.h"
#include "debugger.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED) {
            g_d3dpp.BackBufferWidth = LOWORD(lParam);
            g_d3dpp.BackBufferHeight = HIWORD(lParam);
            ResetDevice();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    HICON hIconBig = (HICON)LoadImageA(hInstance, MAKEINTRESOURCEA(101), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
    HICON hIconSmall = (HICON)LoadImageA(hInstance, MAKEINTRESOURCEA(102), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

    WNDCLASSEXA wc = {
        sizeof(wc),
        CS_CLASSDC,
        WndProc,
        0L,
        0L,
        hInstance,
        hIconBig,
        LoadCursorA(nullptr, IDC_ARROW),
        nullptr,
        nullptr,
        "AriadneClass",
        hIconSmall
    };
    RegisterClassExA(&wc);

    HWND hwnd = CreateWindowA(
        wc.lpszClassName,
        "Ariadne",
        WS_OVERLAPPEDWINDOW,
        100, 100, 1280, 720,
        nullptr,
        nullptr,
        wc.hInstance,
        nullptr
    );

    SendMessageA(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIconBig);
    SendMessageA(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIconSmall);

    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        UnregisterClassA(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;

    ApplyThemeDark();
    ImGui::GetStyle().Colors[ImGuiCol_NavHighlight] = ImVec4(0, 0, 0, 0);

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(g_pd3dDevice);

    UIContext ui;
    EditorDocument doc;
    Debugger dbg;

    bool done = false;
    while (!done) {
        MSG msg;
        while (PeekMessageA(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        RenderUI(hwnd, done, ui, doc, dbg);

        ImGui::EndFrame();

        g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

        ImVec4 clear_color = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
        D3DCOLOR clear_col = D3DCOLOR_RGBA(
            (int)(clear_color.x * 255.0f),
            (int)(clear_color.y * 255.0f),
            (int)(clear_color.z * 255.0f),
            255
        );
        g_pd3dDevice->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col, 1.0f, 0);

        if (g_pd3dDevice->BeginScene() >= 0) {
            ImGui::Render();
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            g_pd3dDevice->EndScene();
        }

        HRESULT result = g_pd3dDevice->Present(nullptr, nullptr, nullptr, nullptr);
        if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET) {
            ResetDevice();
        }
    }

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    DestroyWindow(hwnd);
    UnregisterClassA(wc.lpszClassName, wc.hInstance);

    return 0;
}