#include "Main.h"
#include "IconsData.h"
#include "Pages.h"
#include "tahoma.h"
#include "Bytes.h"
#include "includes.h"

void* AuthInit()
{
    Sleep(1696);
    Helper::ShowLoading = false;
    Helper::ShowMainPage = true;

    return NULL;
}

int APIENTRY WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, ("Control Application"), NULL };
    wc.hbrBackground = (HBRUSH)0;
    RegisterClassEx(&wc);
    main_hwnd = CreateWindowEx(NULL, wc.lpszClassName, ("Control Application"), WS_POPUP, (GetSystemMetrics(SM_CXSCREEN) / 2) - (WINDOW_W / 2), (GetSystemMetrics(SM_CYSCREEN) / 2) - (WINDOW_H / 2), WINDOW_W, WINDOW_H, 0, 0, 0, 0);

    HANDLE mutex_handle = CreateMutexA(NULL, TRUE, ("erg34"));
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        exit(EXIT_FAILURE);
    }

    if (!CreateDeviceD3D(main_hwnd)) {
        CleanupDeviceD3D();
        UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ShowWindow(main_hwnd, SW_SHOW);
    UpdateWindow(main_hwnd);

    HWND Stealth;
    AllocConsole();
    Stealth = FindWindowA(("ConsoleWindowClass"), NULL);
    ShowWindow(Stealth, 0);

    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();

    ImGuiStyle& style = ImGui::GetStyle();

    ImGui_ImplWin32_Init(main_hwnd);
    ImGui_ImplDX9_Init(g_pd3dDevice);

    io.Fonts->AddFontDefault();
    static const ImWchar icons_ranges[] = { 0xf000, 0xf3ff, 0 };
    ImFontConfig icons_config;

    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    icons_config.OversampleH = 3;
    icons_config.OversampleV = 3;

    ImFontConfig CustomFont;
    CustomFont.FontDataOwnedByAtlas = false;

    Helper::fonts.IconFont = io.Fonts->AddFontFromMemoryTTF((void*)tahoma, sizeof(tahoma), 15.f, &CustomFont);
    ImGui::MergeIconsWithLatestFont(16.f, false);

    Helper::fonts.mainFont = io.Fonts->AddFontFromMemoryTTF(const_cast<std::uint8_t*>(Custom), sizeof(Custom), 15.5f, &CustomFont);
    Helper::fonts.Boldfont = io.Fonts->AddFontFromMemoryTTF(PagesFont, sizeof(PagesFont), 19.5f, &CustomFont);
    Helper::fonts.afont = io.Fonts->AddFontFromMemoryTTF(const_cast<std::uint8_t*>(Custom), sizeof(Custom), 18.5f, &CustomFont);
    Helper::fonts.Fontie = io.Fonts->AddFontFromMemoryTTF(const_cast<std::uint8_t*>(Custom), sizeof(Custom), 16.5f, &CustomFont);

    CreateThread(0, 0, (LPTHREAD_START_ROUTINE)AuthInit, 0, 0, 0);

    MSG msg;
    ZeroMemory(&msg, sizeof(msg));

    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }

        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();

        Helper::Style();
        ImGui::NewFrame();

        {
            ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Once);
            ImGui::SetNextWindowSize(ImVec2((float)WINDOW_W, (float)WINDOW_H));

            ImGui::Begin(("Control Application"), &Helper::g_Running, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse);
            {
                static POINT mouse;
                static RECT rc = { 0 };
                GetWindowRect(main_hwnd, &rc);

                if (ImGui::GetWindowPos().x != 0 || ImGui::GetWindowPos().y != 0)
                {
                    MoveWindow(main_hwnd, rc.left + ImGui::GetWindowPos().x, rc.top + ImGui::GetWindowPos().y, WINDOW_W, WINDOW_H, TRUE);
                    ImGui::SetWindowPos(ImVec2(0.f, 0.f));
                }
                GetCursorPos(&mouse);

                if (GetAsyncKeyState(VK_F5)) Helper::AlphaAnimation = 0.0f;
                Helper::AlphaAnimation = ImLerp(Helper::AlphaAnimation, 1.0f, 0.0125f);

                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, Helper::AlphaAnimation);

                if (Helper::ShowLoading == true)
                {
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 340.001f);
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 242.915f);
                    Pages::Main::LoadingIndicatorCircle(("##loading"), 28.340f, ImVec4(ImColor(150, 33, 33, 255)), ImVec4(ImColor(17, 17, 17, 255)), 7, 5.5f);
                }

                if (Helper::ShowMainPage == true)
                {
                    Pages::Main::RenderMainPage();
                }
                ImGui::PopStyleVar();

                ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 6, 6 });
                ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(43.f / 255.f, 43.f / 255.f, 43.f / 255.f, 100.f / 255.f));
                ImGui::PushFont(Helper::fonts.IconFont);
                ImGui::RenderNotifications();
                ImGui::PopFont();
                ImGui::PopStyleVar(2);
                ImGui::PopStyleColor(1);

            }
            ImGui::End();
        }
        ImGui::EndFrame();

        g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
        if (g_pd3dDevice->BeginScene() >= 0)
        {
            ImGui::Render();
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            g_pd3dDevice->EndScene();
        }

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        HRESULT result = g_pd3dDevice->Present(NULL, NULL, NULL, NULL);

        if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET) {
            ResetDevice();
        }
        if (!Helper::g_Running) {
            ReleaseMutex(mutex_handle);
            msg.message = WM_QUIT;
        }
    }

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    DestroyWindow(main_hwnd);
    UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
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
        ::PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}