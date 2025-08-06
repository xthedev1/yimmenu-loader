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
#include "imgui/imgui_internal.h"

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
                std::string msg = "Failed to open process: " + std::to_string(GetLastError());
                InsertAnimatedNotification(ImGuiToastType_Error, 3000, msg.c_str());
                return -1;
            }
            void* loc = VirtualAllocEx(hProc, nullptr, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            if (!loc) {
                InsertAnimatedNotification(ImGuiToastType_Error, 3000, "Failed to allocate memory in target process.");
                CloseHandle(hProc);
                return -1;
            }
            if (!WriteProcessMemory(hProc, loc, dllPath, strlen(dllPath) + 1, nullptr)) {
                InsertAnimatedNotification(ImGuiToastType_Error, 3000, "Failed to write DLL path to target process memory.");
                VirtualFreeEx(hProc, loc, 0, MEM_RELEASE);
                CloseHandle(hProc);
                return -1;
            }
            HANDLE hThread = CreateRemoteThread(hProc, nullptr, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, loc, 0, nullptr);
            if (!hThread) {
                InsertAnimatedNotification(ImGuiToastType_Error, 3000, "Failed to create remote thread in target process.");
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

        struct AnimatedDot {
            ImVec2 pos;
            float alpha;
            float time;
            float velocity_x;
            float velocity_y;
            float size;
        };
        static std::vector<AnimatedDot> animated_dots;
        static const int MAX_DOTS = 48;
        static const float DOT_LIFETIME = 2.5f;

        static void InitAnimatedDots() {
            static bool initialized = false;
            if (!initialized) {
                ImGuiIO& io = ImGui::GetIO();
                animated_dots.clear();
                for (int i = 0; i < MAX_DOTS; ++i) {
                    AnimatedDot dot;
                    dot.pos = ImVec2(
                        static_cast<float>(rand()) / RAND_MAX * io.DisplaySize.x,
                        static_cast<float>(rand()) / RAND_MAX * io.DisplaySize.y
                    );
                    dot.alpha = 0.7f;
                    dot.time = static_cast<float>(rand()) / RAND_MAX * DOT_LIFETIME;
                    float angle = static_cast<float>(rand()) / RAND_MAX * 2.0f * IM_PI;
                    float speed = 8.0f + static_cast<float>(rand()) / RAND_MAX * 12.0f;
                    dot.velocity_x = std::cos(angle) * speed;
                    dot.velocity_y = std::sin(angle) * speed;
                    dot.size = 1.5f + static_cast<float>(rand()) / RAND_MAX * 1.5f;
                    animated_dots.push_back(dot);
                }
                initialized = true;
            }
        }

        static void UpdateAnimatedDots(float delta_time) {
            ImGuiIO& io = ImGui::GetIO();
            for (auto& dot : animated_dots) {
                dot.pos.x += dot.velocity_x * delta_time;
                dot.pos.y += dot.velocity_y * delta_time;
                dot.time += delta_time;
                float twinkle = 0.5f + 0.5f * sinf(dot.time * 6.0f + dot.pos.x);
                dot.alpha = 0.5f * (1.0f - dot.time / DOT_LIFETIME) * twinkle;
                // Respawn if out of bounds or expired
                if (dot.time > DOT_LIFETIME ||
                    dot.pos.x < 0 || dot.pos.x > io.DisplaySize.x ||
                    dot.pos.y < 0 || dot.pos.y > io.DisplaySize.y) {
                    dot.pos = ImVec2(
                        static_cast<float>(rand()) / RAND_MAX * io.DisplaySize.x,
                        static_cast<float>(rand()) / RAND_MAX * io.DisplaySize.y
                    );
                    dot.alpha = 0.7f;
                    dot.time = 0.0f;
                    float angle = static_cast<float>(rand()) / RAND_MAX * 2.0f * IM_PI;
                    float speed = 8.0f + static_cast<float>(rand()) / RAND_MAX * 12.0f;
                    dot.velocity_x = std::cos(angle) * speed;
                    dot.velocity_y = std::sin(angle) * speed;
                    dot.size = 1.5f + static_cast<float>(rand()) / RAND_MAX * 1.5f;
                }
            }
        }

        static void DrawAnimatedDotsForeground() {
            ImDrawList* draw_list = ImGui::GetForegroundDrawList();
            for (const auto& dot : animated_dots) {
                // Glow: draw a faint larger dot
                draw_list->AddCircleFilled(dot.pos, dot.size * 2.2f, IM_COL32(180, 220, 255, static_cast<int>(dot.alpha * 60)), 8);
                // Main dot
                draw_list->AddCircleFilled(dot.pos, dot.size, IM_COL32(200, 240, 255, static_cast<int>(dot.alpha * 255)), 8);
            }
        }

        static bool injecting = false; // Add this flag to track injection state
        static bool show_inject_loading = false; // Overlay flag
        static std::thread inject_thread; // Thread handle
        static std::string inject_status_msg = "";
        static bool show_inject_success_overlay = false;
        static float inject_success_timer = 0.0f;
        static constexpr float inject_success_duration = 5.0f;

        static void InjectDLLThread(DWORD pid, std::string dllPath) {
            if (GetFileAttributesA(dllPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
                InsertAnimatedNotification(ImGuiToastType_Error, 3000, "DLL file not found after download.");
                injecting = false;
                show_inject_loading = false;
                return;
            }
            int injRes = InjectDLL(pid, dllPath.c_str());
            if (injRes == 0) {
                InsertAnimatedNotification(ImGuiToastType_Success, 3000, "Injected successfully!");
                inject_status_msg = "Injected successfully!";
                show_inject_success_overlay = true;
                inject_success_timer = 0.0f;
            } else {
                InsertAnimatedNotification(ImGuiToastType_Error, 3000, "Injection failed. See log for details.");
                inject_status_msg = "Injection failed.";
            }
            Sleep(1000);
            if (!DeleteFileA(dllPath.c_str())) {
                InsertAnimatedNotification(ImGuiToastType_Warning, 3000, "Warning: Could not delete temporary DLL file.");
            }
            injecting = false;
            show_inject_loading = false;
        }
        // --- Infinity Loader with Rotation ---
        static int rotation_start_index;
        static void ImRotateStart() {
            rotation_start_index = ImGui::GetWindowDrawList()->VtxBuffer.Size;
        }
        static ImVec2 ImRotationCenter() {
            ImVec2 l{FLT_MAX, FLT_MAX}, u{-FLT_MAX, -FLT_MAX};
            const auto& buf = ImGui::GetWindowDrawList()->VtxBuffer;
            for (int i = rotation_start_index; i < buf.Size; i++)
                l = ImMin(l, buf[i].pos), u = ImMax(u, buf[i].pos);
            return {(l.x + u.x) / 2, (l.y + u.y) / 2};
        }
        static ImVec2 MyImRotate(const ImVec2& v, float s, float c) {
            return ImVec2(v.x * c - v.y * s, v.x * s + v.y * c);
        }
        static void ImRotateEnd(float rad, ImVec2 center) {
            float s = ImSin(rad), c = ImCos(rad);
            center = MyImRotate(center, s, c) - center;
            auto& buf = ImGui::GetWindowDrawList()->VtxBuffer;
            for (int i = rotation_start_index; i < buf.Size; i++)
                buf[i].pos = MyImRotate(buf[i].pos, s, c) - center;
        }
        static void LoadingIndicatorInfinity(const char* label, float size, const ImVec4& color, float speed = 2.0f) {
            ImGuiWindow* window = ImGui::GetCurrentWindow();
            if (window->SkipItems) return;
            ImGuiContext& g = *GImGui;
            const ImGuiID id = window->GetID(label);
            const ImVec2 pos = window->DC.CursorPos;
            const ImRect bb(pos, ImVec2(pos.x + size, pos.y + size));
            ImGui::ItemSize(bb, ImGui::GetStyle().FramePadding.y);
            if (!ImGui::ItemAdd(bb, id)) return;
            float t = static_cast<float>(g.Time) * speed;
            ImDrawList* draw_list = window->DrawList;
            ImVec2 center = ImVec2(pos.x + size * 0.5f, pos.y + size * 0.5f);
            float a = size * 0.32f;
            int segments = 100;
            float thickness = 4.0f;
            ImRotateStart();
            for (int i = 0; i < segments; ++i) {
                float p = (float)i / (float)(segments - 1) * 2.0f * IM_PI;
                float x = a * cosf(p) / (1.0f + sinf(p) * sinf(p));
                float y = a * sinf(2.0f * p) / 2.0f;
                ImVec2 pt1 = ImVec2(center.x + x, center.y + y);
                float p2 = (float)(i + 1) / (float)(segments - 1) * 2.0f * IM_PI;
                float x2 = a * cosf(p2) / (1.0f + sinf(p2) * sinf(p2));
                float y2 = a * sinf(2.0f * p2) / 2.0f;
                ImVec2 pt2 = ImVec2(center.x + x2, center.y + y2);
                float alpha = 0.3f + 0.7f * (float)i / (float)segments;
                ImU32 col = ImGui::GetColorU32(ImVec4(color.x, color.y, color.z, color.w * alpha));
                draw_list->AddLine(pt1, pt2, col, thickness);
            }
            ImRotateEnd(t, center);
        }
        static void RenderInjectLoadingOverlay() {
            if (!show_inject_loading) return;
            ImGuiIO& io = ImGui::GetIO();
            ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
            ImGui::SetNextWindowSize(io.DisplaySize);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0.45f));
            ImGui::Begin("##inject_loading_overlay", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            ImGui::SetCursorPos(ImVec2(io.DisplaySize.x / 2 - 40, io.DisplaySize.y / 2 - 40));
            LoadingIndicatorInfinity("##inject_infinity", 80.0f, ImVec4(0.2f, 0.6f, 1.0f, 1.0f), 2.5f);
            ImGui::End();
            ImGui::PopStyleColor();
            ImGui::PopStyleVar(2);
        }

        static void RenderInjectSuccessOverlay() {
            if (!show_inject_success_overlay) return;
            ImGuiIO& io = ImGui::GetIO();
            ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
            ImGui::SetNextWindowSize(io.DisplaySize);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0.85f));
            ImGui::Begin("##inject_success_overlay", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            ImGui::SetCursorPos(ImVec2(io.DisplaySize.x / 2 - 220, io.DisplaySize.y / 2 - 60));
            ImGui::PushFont(Helper::fonts.Boldfont);
            ImGui::TextColored(ImColor(0, 255, 128, 255), "DLL injected!");
            ImGui::PopFont();
            ImGui::Spacing();
            ImGui::SetCursorPosX(io.DisplaySize.x / 2 - 180);
            ImGui::PushFont(Helper::fonts.mainFont);
            ImGui::Text("Closing loader in %.1f seconds...", inject_success_duration - inject_success_timer);
            ImGui::PopFont();
            ImGui::End();
            ImGui::PopStyleColor();
            ImGui::PopStyleVar(2);
        }

        // --- Enhanced Close Animation State ---
        #include <atomic>
        static std::atomic<bool> is_closing(false);
        static float close_alpha = 1.0f;
        static float close_timer = 0.0f;
        static float close_scale = 1.0f;
        static float close_blur = 0.0f;
        static void ResetCloseAnim() {
            is_closing = false;
            close_alpha = 1.0f;
            close_timer = 0.0f;
            close_scale = 1.0f;
            close_blur = 0.0f;
        }

        static void Tabs() {
            // --- Sparkle Lines Background ---
            InitAnimatedDots();
            UpdateAnimatedDots(ImGui::GetIO().DeltaTime);
            DrawAnimatedDotsForeground();
            if (Helper::Tabs == 0) {
                // --- Enhanced Close Animation ---
                if (is_closing) {
                    close_timer += ImGui::GetIO().DeltaTime;
                    float t = close_timer / 0.7f; // slower, smoother
                    close_alpha = 1.0f - t;
                    close_scale = 1.0f - 0.1f * t;
                    close_blur = t * 8.0f;
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
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 8.0f));
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 0.7f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.2f, 0.2f, 0.9f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.0f, 0.0f, 1.0f));
                if (ImGui::Button(ICON_FA_TIMES, ImVec2(28, 28)) && !is_closing) {
                    is_closing = true;
                }
                ImGui::PopStyleColor(3);
                ImGui::PopStyleVar(2);
                ImGui::PopFont();
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 12.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 12.0f));
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(ImColor(0, 0, 0, 0)));
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15.0f);
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
                InitAnimatedDots();
                UpdateAnimatedDots(ImGui::GetIO().DeltaTime);
                DrawAnimatedDotsForeground();
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
                const char* btnLabel = downloading ? "Downloading DLL..." : (injecting ? "Injecting..." : "Inject");
                if (downloading || injecting) {
                    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
                }
                bool injectPressed = ImGui::Button(btnLabel, ImVec2(110.0f, 35.0f));
                if (downloading || injecting) {
                    ImGui::PopStyleVar();
                    ImGui::PopItemFlag();
                }
                ImGui::PopStyleColor();
                if (injectPressed && !downloading && !injecting) {
                    DWORD pid = GetProcId("GTA5.exe");
                    if (!pid) {
                        InsertAnimatedNotification(ImGuiToastType_Warning, 3000, "GTA5.exe not found.");
                    } else {
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
                        } else {
                            if (GetFileAttributesA(dllPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
                                InsertAnimatedNotification(ImGuiToastType_Error, 3000, "Downloaded DLL file not found.");
                            } else {
                                injecting = true;
                                show_inject_loading = true;
                                inject_status_msg = "";
                                inject_thread = std::thread(InjectDLLThread, pid, dllPath);
                                inject_thread.detach();
                            }
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
                RenderInjectLoadingOverlay();
                if (show_inject_success_overlay) {
                    inject_success_timer += ImGui::GetIO().DeltaTime;
                    RenderInjectSuccessOverlay();
                    if (inject_success_timer >= inject_success_duration) {
                        exit(EXIT_SUCCESS);
                    }
                }
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