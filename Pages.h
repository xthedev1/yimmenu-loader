#pragma once
#include <iostream>
#include <winsock.h>
#include <vector>
#include <thread>
#include <bitset>
#include <cmath>
#include <memory>
#include <TlHelp32.h>
#include <urlmon.h>
#include <Shlwapi.h>
#include <shellapi.h>
#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "ws2_32.lib")
#include "Includes.h"
#include "Bytes.h"
#include "font_awesome_5.h"

#ifndef ICON_FA_HOME
#define ICON_FA_HOME "\xef\x80\x95"
#endif

#ifndef ICON_FA_TIMES
#define ICON_FA_TIMES "\xef\x80\x8d"
#endif

#ifndef ImGuiToastType_Error
#define ImGuiToastType_Error 0
#endif

#ifndef ImGuiToastType_Success
#define ImGuiToastType_Success 1
#endif

#ifndef ImGuiToastType_Warning
#define ImGuiToastType_Warning 2
#endif

#ifndef ImGuiToastType_Info
#define ImGuiToastType_Info 3
#endif

#define maxx(a, b) (((a) > (b)) ? (a) : (b))

namespace Pages {
    namespace Main {
        static float ButtonWidth = 40.0f;
        static float ButtonHeight = 35.0f;

        struct Notification {
            ImGuiToastType type;
            int duration;
            std::string text;
            float alpha;
            float timer;
            bool is_active;
        };

        static std::vector<Notification> notifications;

        void InsertAnimatedNotification(ImGuiToastType type, int duration, const char* text) {
            Notification notification;
            notification.type = type;
            notification.duration = duration;
            notification.text = text;
            notification.alpha = 0.0f;
            notification.timer = 0.0f;
            notification.is_active = true;
            notifications.push_back(notification);
        }

        void UpdateNotifications(float delta_time) {
            for (auto& notification : notifications) {
                if (!notification.is_active) continue;
                if (notification.timer < 0.5f) {
                    notification.alpha += delta_time * 2.0f;
                    if (notification.alpha > 1.0f) notification.alpha = 1.0f;
                }
                else if (notification.timer > (notification.duration / 1000.0f - 0.5f)) {
                    notification.alpha -= delta_time * 2.0f;
                    if (notification.alpha < 0.0f) {
                        notification.alpha = 0.0f;
                        notification.is_active = false;
                    }
                }
                notification.timer += delta_time;
                if (notification.timer > notification.duration / 1000.0f) {
                    notification.is_active = false;
                }
            }
            notifications.erase(
                std::remove_if(notifications.begin(), notifications.end(),
                    [](const Notification& n) { return !n.is_active; }),
                notifications.end());
        }

        void RenderNotifications() {
            bool has_active_notification = false;
            for (const auto& notification : notifications) {
                if (notification.is_active) {
                    has_active_notification = true;
                    break;
                }
            }
            if (!has_active_notification) return;
            ImGuiIO& io = ImGui::GetIO();
            ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 10.0f, 10.0f), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
            ImGui::SetNextWindowSize(ImVec2(300.0f, 0.0f));
            ImGui::Begin("Notifications", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
            for (const auto& notification : notifications) {
                if (!notification.is_active) continue;
                ImVec4 color;
                switch (notification.type) {
                    case ImGuiToastType_Error:   color = ImVec4(1.0f, 0.2f, 0.2f, notification.alpha); break;
                    case ImGuiToastType_Success: color = ImVec4(0.2f, 1.0f, 0.2f, notification.alpha); break;
                    case ImGuiToastType_Warning: color = ImVec4(1.0f, 1.0f, 0.2f, notification.alpha); break;
                    default:                     color = ImVec4(0.2f, 0.8f, 1.0f, notification.alpha); break;
                }
                ImGui::PushStyleColor(ImGuiCol_Text, color);
                ImGui::Text("%s", notification.text.c_str());
                ImGui::PopStyleColor();
                ImGui::Spacing();
            }
            ImGui::End();
        }

        void LoadingIndicatorCircle(const char* label, float indicator_radius,
            const ImVec4& main_color, const ImVec4& backdrop_color,
            int circle_count, float speed) {
            ImGuiWindow* window = ImGui::GetCurrentWindow();
            if (window->SkipItems) return;
            ImGuiContext& g = *GImGui;
            const ImGuiID id = window->GetID(label);
            const ImVec2 pos = window->DC.CursorPos;
            const float circle_radius = indicator_radius / 10.0f;
            const ImRect bb(pos, ImVec2(pos.x + indicator_radius * 2.0f, pos.y + indicator_radius * 2.0f));
            ImGui::ItemSize(bb, ImGui::GetStyle().FramePadding.y);
            if (!ImGui::ItemAdd(bb, id)) return;
            const float t = static_cast<float>(g.Time);
            const float degree_offset = 2.0f * IM_PI / circle_count;
            for (int i = 0; i < circle_count; ++i) {
                const float x = indicator_radius * static_cast<float>(std::sin(degree_offset * i));
                const float y = indicator_radius * static_cast<float>(std::cos(degree_offset * i));
                const float growth = maxx(0.0f, static_cast<float>(std::sin(t * speed - i * degree_offset)));
                ImVec4 color;
                color.x = main_color.x * growth + backdrop_color.x * (1.0f - growth);
                color.y = main_color.y * growth + backdrop_color.y * (1.0f - growth);
                color.z = main_color.z * growth + backdrop_color.z * (1.0f - growth);
                color.w = 1.0f;
                window->DrawList->AddCircleFilled(ImVec2(pos.x + indicator_radius + x, pos.y + indicator_radius - y),
                    circle_radius + growth * circle_radius, ImGui::GetColorU32(color));
            }
        }

        DWORD WINAPI WaitThread(LPVOID lpParam) {
            HANDLE process_handle = reinterpret_cast<HANDLE>(lpParam);
            DWORD wait_result = WaitForSingleObject(process_handle, INFINITE);
            return (wait_result == WAIT_OBJECT_0) ? 0 : 1;
        }

        int RunProgram(LPSTR ProgramName) {
            STARTUPINFO si;
            PROCESS_INFORMATION pi;
            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            ZeroMemory(&pi, sizeof(pi));
            if (!CreateProcess(NULL, ProgramName, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi)) {
                DWORD error = GetLastError();
                LPSTR messageBuffer = nullptr;
                size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
                std::string message(messageBuffer, size);
                LocalFree(messageBuffer);
                std::string errorText = "Creating The Process has failed with error " + std::to_string(error) + ": " + message;
                InsertAnimatedNotification(ImGuiToastType_Error, 1000, errorText.c_str());
                return static_cast<int>(error);
            }
            std::unique_ptr<void, decltype(&CloseHandle)> process_handle(pi.hProcess, CloseHandle);
            std::unique_ptr<void, decltype(&CloseHandle)> thread_handle(pi.hThread, CloseHandle);
            std::thread wait_thread(WaitThread, pi.hProcess);
            wait_thread.detach();
            return 10;
        }

        static bool IsProcessRunning(const char* processName) {
            bool exists = false;
            HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (snap != INVALID_HANDLE_VALUE) {
                PROCESSENTRY32 proc{};
                proc.dwSize = sizeof(proc);
                if (Process32First(snap, &proc)) {
                    do {
                        if (_stricmp(proc.szExeFile, processName) == 0) {
                            exists = true;
                            break;
                        }
                    } while (Process32Next(snap, &proc));
                }
                CloseHandle(snap);
            }
            return exists;
        }

        static DWORD GetProcId(const char* pn) {
            DWORD procId = 0;
            HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (hSnap != INVALID_HANDLE_VALUE) {
                PROCESSENTRY32 pe;
                pe.dwSize = sizeof(pe);
                if (Process32First(hSnap, &pe)) {
                    do {
                        if (!_stricmp(pe.szExeFile, pn)) {
                            procId = pe.th32ProcessID;
                            break;
                        }
                    } while (Process32Next(hSnap, &pe));
                }
                CloseHandle(hSnap);
            }
            return procId;
        }

        static int InjectDLL(DWORD procID, const char* dllPath) {
            HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procID);
            if (!hProc) {
                std::cerr << "Failed to open process: " << GetLastError() << std::endl;
                return -1;
            }
            void* loc = VirtualAllocEx(hProc, nullptr, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            if (!loc) {
                CloseHandle(hProc);
                return -1;
            }
            if (!WriteProcessMemory(hProc, loc, dllPath, strlen(dllPath) + 1, nullptr)) {
                VirtualFreeEx(hProc, loc, 0, MEM_RELEASE);
                CloseHandle(hProc);
                return -1;
            }
            HANDLE hThread = CreateRemoteThread(hProc, nullptr, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, loc, 0, nullptr);
            if (!hThread) {
                VirtualFreeEx(hProc, loc, 0, MEM_RELEASE);
                CloseHandle(hProc);
                return -1;
            }
            WaitForSingleObject(hThread, INFINITE);
            CloseHandle(hThread);
            VirtualFreeEx(hProc, loc, 0, MEM_RELEASE);
            CloseHandle(hProc);
            return 0;
        }

        struct AnimatedLine {
            ImVec2 start;
            ImVec2 end;
            float alpha;
            float time;
            float velocity_x;
            float velocity_y;
        };

        static std::vector<AnimatedLine> animated_lines;
        static const int MAX_LINES = 12;
        static const float LINE_LIFETIME = 1.8f;

        static void InitAnimatedLines() {
            static bool initialized = false;
            if (!initialized) {
                ImGuiIO& io = ImGui::GetIO();
                animated_lines.clear();
                for (int i = 0; i < MAX_LINES; ++i) {
                    AnimatedLine line;
                    float x = static_cast<float>(rand()) / RAND_MAX * io.DisplaySize.x;
                    float y = static_cast<float>(rand()) / RAND_MAX * io.DisplaySize.y;
                    float length = 12.0f + static_cast<float>(rand()) / RAND_MAX * 18.0f;
                    float angle = static_cast<float>(rand()) / RAND_MAX * 2.0f * IM_PI;
                    line.start = ImVec2(x, y);
                    line.end = ImVec2(x + length * std::cos(angle), y + length * std::sin(angle));
                    line.alpha = 0.35f;
                    line.time = static_cast<float>(rand()) / RAND_MAX * LINE_LIFETIME;
                    line.velocity_x = -12.0f + static_cast<float>(rand()) / RAND_MAX * 24.0f;
                    line.velocity_y = 8.0f + static_cast<float>(rand()) / RAND_MAX * 16.0f;
                    animated_lines.push_back(line);
                }
                initialized = true;
            }
        }

        static void UpdateAnimatedLines(float delta_time) {
            ImGuiIO& io = ImGui::GetIO();
            for (auto& line : animated_lines) {
                line.start.x += line.velocity_x * delta_time;
                line.start.y += line.velocity_y * delta_time;
                line.end.x += line.velocity_x * delta_time;
                line.end.y += line.velocity_y * delta_time;
                line.time += delta_time;
                line.alpha = 0.35f * (1.0f - line.time / LINE_LIFETIME);
                if (line.start.y > io.DisplaySize.y || line.time > LINE_LIFETIME) {
                    float x = static_cast<float>(rand()) / RAND_MAX * io.DisplaySize.x;
                    float y = -50.0f;
                    float length = 12.0f + static_cast<float>(rand()) / RAND_MAX * 18.0f;
                    float angle = static_cast<float>(rand()) / RAND_MAX * 2.0f * IM_PI;
                    line.start = ImVec2(x, y);
                    line.end = ImVec2(x + length * std::cos(angle), y + length * std::sin(angle));
                    line.alpha = 0.35f;
                    line.time = 0.0f;
                    line.velocity_x = -12.0f + static_cast<float>(rand()) / RAND_MAX * 24.0f;
                    line.velocity_y = 8.0f + static_cast<float>(rand()) / RAND_MAX * 16.0f;
                }
                if (line.start.x < 0) {
                    line.start.x += io.DisplaySize.x;
                    line.end.x += io.DisplaySize.x;
                }
                if (line.start.x > io.DisplaySize.x) {
                    line.start.x -= io.DisplaySize.x;
                    line.end.x -= io.DisplaySize.x;
                }
            }
        }

        static void DrawAnimatedLines() {
            ImGuiWindow* window = ImGui::GetCurrentWindow();
            for (const auto& line : animated_lines) {
                window->DrawList->AddLine(
                    line.start,
                    line.end,
                    IM_COL32(0, 122, 204, static_cast<int>(line.alpha * 255)),
                    1.0f
                );
            }
        }

        static void Tabs() {
            if (Helper::Tabs == 0) {
                static bool is_closing = false;
                static float close_alpha = 1.0f;
                static float close_timer = 0.0f;

                if (is_closing) {
                    close_timer += ImGui::GetIO().DeltaTime;
                    close_alpha = 1.0f - (close_timer / 0.5f);
                    if (close_alpha <= 0.0f) {
                        exit(EXIT_SUCCESS);
                    }
                }

                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, close_alpha);

                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 9.0f);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 12.0f);
                ImGui::PushFont(Helper::fonts.Boldfont);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                ImGui::Text("Dashboard");
                ImGui::PopStyleColor();
                ImGui::PopFont();
                ImGui::SameLine();
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - 30.0f);
                ImGui::PushFont(Helper::fonts.IconFont);
                ImGui::Text(ICON_FA_TIMES);
                if (ImGui::IsItemClicked() && !is_closing) {
                    is_closing = true;
                }
                ImGui::PopFont();
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 12.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 12.0f));
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(ImColor(0, 0, 0, 0)));
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15.0f);
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
                InitAnimatedLines();
                UpdateAnimatedLines(ImGui::GetIO().DeltaTime);
                DrawAnimatedLines();
                static float slide_offset = -300.0f;
                static bool is_sliding = true;
                if (is_sliding) {
                    slide_offset += 600.0f * ImGui::GetIO().DeltaTime;
                    if (slide_offset >= 0.0f) {
                        slide_offset = 0.0f;
                        is_sliding = false;
                    }
                }
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + slide_offset);
                ImGui::BeginChild("##GTA5", ImVec2(ImGui::GetContentRegionAvail().x - 3.0f, 140.0f), true);
                ImGui::Spacing(); ImGui::Spacing();
                ImGui::PushFont(Helper::fonts.Boldfont);
                ImGui::Text("Gta 5 - YimMenu");
                ImGui::PopFont();
                ImGui::Spacing();
                ImGui::SameLine();
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 270.0f);
                static float pulse = 0.0f;
                pulse = (sin(ImGui::GetTime() * 5) + 1) / 2;
                ImVec4 button_color = ImLerp(ImVec4(0.2f, 0.4f, 0.6f, 1.0f), ImVec4(0.4f, 0.6f, 0.8f, 1.0f), pulse);
                ImGui::PushStyleColor(ImGuiCol_Button, button_color);
                static bool downloading = false;
                const char* btnLabel = downloading ? "Downloading DLL..." : "Inject";
                if (downloading) {
                    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
                }
                bool injectPressed = ImGui::Button(btnLabel, ImVec2(110.0f, 35.0f));
                if (downloading) {
                    ImGui::PopStyleVar();
                    ImGui::PopItemFlag();
                }
                ImGui::PopStyleColor();
                if (injectPressed && !downloading) {
                    DWORD pid = GetProcId("GTA5.exe");
                    if (!pid) {
                        InsertAnimatedNotification(ImGuiToastType_Warning, 3000, "GTA5.exe not found.");
                    }
                    else {
                        downloading = true;
                        InsertAnimatedNotification(ImGuiToastType_Success, 2000, "Downloading DLL...");
                        const char* url = "https://raw.githubusercontent.com/xthedev1/yimmenu-server/main/yim.dll";
                        char tempPath[MAX_PATH] = { 0 };
                        GetTempPathA(MAX_PATH, tempPath);
                        std::string dllPath = std::string(tempPath) + "yim.dll";
                        HRESULT hr = URLDownloadToFileA(NULL, url, dllPath.c_str(), 0, NULL);
                        if (hr != S_OK) {
                            std::string msg = "Failed to download DLL. HRESULT: " + std::to_string(hr);
                            InsertAnimatedNotification(ImGuiToastType_Error, 3000, msg.c_str());
                        }
                        else {
                            int injRes = InjectDLL(pid, dllPath.c_str());
                            if (injRes == 0) {
                                InsertAnimatedNotification(ImGuiToastType_Success, 3000, "Injected successfully!");
                            }
                            else {
                                InsertAnimatedNotification(ImGuiToastType_Error, 3000, "Injection failed.");
                            }
                            Sleep(1000);
                            DeleteFileA(dllPath.c_str());
                        }
                        downloading = false;
                    }
                }
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::PushFont(Helper::fonts.mainFont);
                ImGui::Spacing(); ImGui::Spacing();
                ImGui::TextColored(ImColor(0, 122, 204, 255), "YimMenu loader by xthedev1");
                if (ImGui::IsItemClicked()) ShellExecuteA(NULL, "open", "https://github.com/xthedev1", NULL, NULL, SW_SHOWNORMAL);
                ImGui::PopFont();
                ImGui::EndChild();
                ImGui::PopStyleVar(2);
                ImGui::PopStyleColor();
                UpdateNotifications(ImGui::GetIO().DeltaTime);
                RenderNotifications();
                ImGui::PopStyleVar();
            }
        }

        void RenderMainPage() {
            Helper::ShowLoading = false;
            ImGui::PushFont(Helper::fonts.mainFont);
            Tabs();
            ImGui::PopFont();
        }
    }
}