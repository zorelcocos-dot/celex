#include "renderer.h"
#include "imgui/IconsFontAwesome6.h"
#include "../rbx/globals/globals.h"
#include <map>
#include <string>
ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
bool g_SwapChainOccluded = false;
UINT g_ResizeWidth = 0, g_ResizeHeight = 0;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
bool IsGameOnTop(const std::string& expectedTitle) {
    HWND hwnd = GetForegroundWindow();
    if (!hwnd) return false;
    char windowTitle[256];
    int length = GetWindowTextA(hwnd, windowTitle, sizeof(windowTitle));
    if (length == 0) return false;
    return expectedTitle == std::string(windowTitle);
}
void SetTransparency(HWND hwnd, bool boolean) {
    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    if (boolean) {
        exStyle |= WS_EX_TRANSPARENT;
        SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);
    } else {
        exStyle &= ~WS_EX_TRANSPARENT;
        SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);
    }
}
void RenderKeybindList(ImDrawList* drawList) {
    if (!Options::Misc::KeybindList) return;
    ImGuiIO& io = ImGui::GetIO();
    std::vector<std::pair<std::string, std::string>> activeBinds;
    // Check Aimbot
    if (Options::Aimbot::Aimbot && Options::Aimbot::AimbotKey != 0) {
        bool isActive = false;
        if (Options::Aimbot::ToggleType == 1) // Toggle
            isActive = Options::Aimbot::Toggled;
        else // Hold
            isActive = (GetAsyncKeyState(Options::Aimbot::AimbotKey) & 0x8000) != 0;
        if (isActive) activeBinds.push_back({"Aimbot", Options::Aimbot::ToggleType == 1 ? "[Toggled]" : "[Hold]"});
    }
    // Check Triggerbot
    if (Options::Triggerbot::Enabled && Options::Triggerbot::TriggerbotKey != 0) {
        bool isActive = false;
        if (Options::Triggerbot::ToggleType == 1) // Toggle
            isActive = Options::Triggerbot::Toggled;
        else // Hold
            isActive = (GetAsyncKeyState(Options::Triggerbot::TriggerbotKey) & 0x8000) != 0;
        if (isActive) activeBinds.push_back({"Triggerbot", Options::Triggerbot::ToggleType == 1 ? "[Toggled]" : "[Hold]"});
    }
    // Check Fly
    if (Options::Fly::Enabled && Options::Fly::FlyKey != 0) {
        bool isActive = false;
        if (Options::Fly::ToggleType == 1) // Toggle
            isActive = Options::Fly::Toggled;
        else // Hold
            isActive = (GetAsyncKeyState(Options::Fly::FlyKey) & 0x8000) != 0;
        if (isActive) activeBinds.push_back({"Fly", Options::Fly::ToggleType == 1 ? "[Toggled]" : "[Hold]"});
    }
    // Check WalkSpeed
    if (Options::WalkSpeed::Enabled && Options::WalkSpeed::WalkSpeedKey != 0) {
        bool isActive = false;
        if (Options::WalkSpeed::ToggleType == 1) // Toggle
            isActive = Options::WalkSpeed::Toggled;
        else // Hold
            isActive = (GetAsyncKeyState(Options::WalkSpeed::WalkSpeedKey) & 0x8000) != 0;
        if (isActive) activeBinds.push_back({"WalkSpeed", Options::WalkSpeed::ToggleType == 1 ? "[Toggled]" : "[Hold]"});
    }
    // Check Desync
    if (Options::Desync::Enabled && Options::Desync::Key != 0) {
        bool isActive = false;
        if (Options::Desync::ToggleType == 1) // Toggle
            isActive = Options::Desync::Toggled;
        else // Hold
            isActive = (GetAsyncKeyState(Options::Desync::Key) & 0x8000) != 0;
        if (isActive) activeBinds.push_back({"Desync", Options::Desync::ToggleType == 1 ? "[Toggled]" : "[Hold]"});
    }
    if (activeBinds.empty()) return;
    // Calculate dimensions - much smaller and compact
    float paddingX = 12.0f;
    float paddingY = 8.0f;
    float lineHeight = 18.0f;
    float titleHeight = 24.0f;
    float minWidth = 160.0f;
    float nameWidths = 0.0f;
    float statusWidths = 0.0f;

    for (const auto& bind : activeBinds) {
        float nW = ImGui::CalcTextSize(bind.first.c_str()).x;
        float sW = ImGui::CalcTextSize(bind.second.c_str()).x;
        if (nW > nameWidths) nameWidths = nW;
        if (sW > statusWidths) statusWidths = sW;
    }

    float boxWidth = (std::max)(minWidth, nameWidths + statusWidths + paddingX * 3.0f);
    float boxHeight = titleHeight + (activeBinds.size() * lineHeight) + paddingY;

    // Use custom position from sliders
    ImVec2 pos = ImVec2(Options::Misc::KeybindListX, Options::Misc::KeybindListY);

    // Draw background
    drawList->AddRectFilled(pos, ImVec2(pos.x + boxWidth, pos.y + boxHeight), IM_COL32(15, 15, 15, 240), 6.0f);
    drawList->AddRect(pos, ImVec2(pos.x + boxWidth, pos.y + boxHeight), IM_COL32(40, 40, 40, 255), 6.0f);

    // Draw title - centered
    const char* title = "Keybinds";
    float titleWidth = ImGui::CalcTextSize(title).x;
    float titleX = pos.x + (boxWidth - titleWidth) / 2.0f;
    drawList->AddText(ImVec2(titleX, pos.y + 4), IM_COL32(255, 255, 255, 255), title);

    drawList->AddLine(ImVec2(pos.x, pos.y + titleHeight), ImVec2(pos.x + boxWidth, pos.y + titleHeight), IM_COL32(40, 40, 40, 255));

    // Draw active binds
    float yOffset = pos.y + titleHeight + 4;
    for (const auto& bind : activeBinds) {
        // Draw name on left
        drawList->AddText(ImVec2(pos.x + paddingX, yOffset), IM_COL32(200, 200, 200, 255), bind.first.c_str());

        // Draw status aligned to right
        float statusW = ImGui::CalcTextSize(bind.second.c_str()).x;
        drawList->AddText(ImVec2(pos.x + boxWidth - paddingX - statusW, yOffset),
                         IM_COL32(main_color.x * 255, main_color.y * 255, main_color.z * 255, 255),
                         bind.second.c_str());
        yOffset += lineHeight;
    }
}
void ShowImgui() {
    ImGui_ImplWin32_EnableDpiAwareness();
    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));
    size_t width = (size_t)GetSystemMetrics(SM_CXSCREEN);
    size_t height = (size_t)GetSystemMetrics(SM_CYSCREEN);
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        wc.lpszClassName,
        L"redstoneprojrjr&kam546",
        WS_POPUP,
        0, 0, (int)width + 1, (int)height + 1,
        nullptr, nullptr, wc.hInstance, nullptr);
    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 255, LWA_ALPHA);
    MARGINS Margin = { -1 };
    DwmExtendFrameIntoClientArea(hwnd, &Margin);
    // Apply streamproof if enabled (WDA_EXCLUDEFROMCAPTURE = 0x00000011)
    if (Options::Misc::StreamProof) {
        SetWindowDisplayAffinity(hwnd, 0x00000011);
    }
    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    }
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    // Apply dark style base
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    // Fix purple accent colors globally
    style.Colors[ImGuiCol_SliderGrab]        = ImVec4(0.49f, 0.23f, 0.93f, 1.0f);
    style.Colors[ImGuiCol_SliderGrabActive]  = ImVec4(0.60f, 0.30f, 1.00f, 1.0f);
    style.Colors[ImGuiCol_CheckMark]         = ImVec4(0.49f, 0.23f, 0.93f, 1.0f);
    style.Colors[ImGuiCol_FrameBg]           = ImVec4(0.08f, 0.08f, 0.12f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered]    = ImVec4(0.12f, 0.08f, 0.20f, 1.0f);
    style.Colors[ImGuiCol_FrameBgActive]     = ImVec4(0.15f, 0.08f, 0.28f, 1.0f);
    style.Colors[ImGuiCol_Button]            = ImVec4(0.15f, 0.08f, 0.28f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered]     = ImVec4(0.25f, 0.12f, 0.45f, 1.0f);
    style.Colors[ImGuiCol_PopupBg]           = ImVec4(0.08f, 0.08f, 0.12f, 1.0f);
    style.Colors[ImGuiCol_Header]            = ImVec4(0.20f, 0.10f, 0.38f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered]     = ImVec4(0.28f, 0.15f, 0.50f, 1.0f);
    style.Colors[ImGuiCol_WindowBg]          = ImVec4(0.05f, 0.05f, 0.07f, 1.0f);
    style.FrameRounding    = 4.0f;
    style.GrabRounding     = 4.0f;
    style.PopupRounding    = 6.0f;
    style.ItemSpacing      = ImVec2(8, 4);
    // Load Inter Regular as main font
    ImFontConfig fontCfg;
    fontCfg.MergeMode  = false;
    fontCfg.PixelSnapH = true;
    fontCfg.OversampleH = 3;
    fontCfg.OversampleV = 1;
    std::string interPath = Globals::executablePath + "\\Inter-Regular.ttf";
    ImFont* font = io.Fonts->AddFontFromFileTTF(interPath.c_str(), 16.0f, &fontCfg);
    if (!font)
        font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 16.0f, &fontCfg);
        
    // Merge FontAwesome 6 icons into the main font atlas
    ImFontConfig iconsCfg;
    iconsCfg.MergeMode   = true;
    iconsCfg.PixelSnapH  = true;
    iconsCfg.GlyphOffset = ImVec2(0, 1);
    static const ImWchar iconsRanges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    std::string faPath = Globals::executablePath + "\\fa-solid-900.ttf";
    io.Fonts->AddFontFromFileTTF(faPath.c_str(), 15.0f, &iconsCfg, iconsRanges);

    ImFontConfig titleCfg = fontCfg;
    ImFont* titleFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeuib.ttf", 26.0f, &titleCfg);
    if (!titleFont) titleFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 26.0f, &titleCfg);
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
    ImGui_ImplDX11_CreateDeviceObjects();
    ImVec4 clear_color = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    bool done = false;
    bool menu_open = true;
    while (!done) {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;
        if (g_SwapChainOccluded && g_pSwapChain->Present(0, 0) == DXGI_STATUS_OCCLUDED) {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        if (GetAsyncKeyState(Options::Misc::MenuKey) & 1) {
            menu_open = !menu_open;
            SetTransparency(hwnd, !menu_open);
        }
        // Fade animation using exact deltas for smooth visual transitions
        static float menuAlpha = 0.0f;
        static float backgroundAlpha = 0.0f;
        float fadeSpeed = 4.0f * ImGui::GetIO().DeltaTime;

        if (menu_open) {
            menuAlpha = std::min(1.0f, menuAlpha + fadeSpeed);
            backgroundAlpha = std::min(0.7f, backgroundAlpha + fadeSpeed);
        } else {
            menuAlpha = std::max(0.0f, menuAlpha - fadeSpeed);
            backgroundAlpha = std::max(0.0f, backgroundAlpha - fadeSpeed);
        }
        // Dynamic streamproof toggle
        static bool lastStreamProofState = Options::Misc::StreamProof;
        if (lastStreamProofState != Options::Misc::StreamProof) {
            if (Options::Misc::StreamProof) {
                SetWindowDisplayAffinity(hwnd, 0x00000011); // WDA_EXCLUDEFROMCAPTURE
            } else {
                SetWindowDisplayAffinity(hwnd, 0x00000000); // WDA_NONE
            }
            lastStreamProofState = Options::Misc::StreamProof;
        }
        // Update main_color from options
        main_color = ImVec4(Options::Misc::MenuAccentColor[0], Options::Misc::MenuAccentColor[1], Options::Misc::MenuAccentColor[2], 1.0f);
        if (menu_open || menuAlpha > 0.0f) {
            if (backgroundAlpha > 0.0f) {
                ImGui::GetBackgroundDrawList()->AddRectFilled(
                    ImVec2(0, 0), ImVec2(io.DisplaySize.x, io.DisplaySize.y),
                    IM_COL32(0, 0, 0, static_cast<int>(backgroundAlpha * 120))
                );
            }

            ImGui::SetNextWindowSize(ImVec2(800, 560), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowBgAlpha(menuAlpha);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, menuAlpha);

            // Apple styling overrides
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.08f, 0.09f, 0.94f));
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.2f, 0.2f, 0.22f, 0.5f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

            ImGui::Begin("AppleUI", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings);

            ImVec2 p = ImGui::GetWindowPos();
            ImVec2 s = ImGui::GetWindowSize();
            auto draw = ImGui::GetWindowDrawList();

            // Custom Dragging Logic for the whole top bar
            static bool is_dragging = false;
            static ImVec2 drag_offset;
            ImVec2 mouse_pos = ImGui::GetIO().MousePos;

            // Top bar area for dragging
            ImVec2 top_min = p;
            ImVec2 top_max = ImVec2(p.x + s.x, p.y + 60.0f);
            if (ImGui::IsMouseHoveringRect(top_min, top_max) && ImGui::IsMouseClicked(0)) {
                is_dragging = true;
                drag_offset = ImVec2(mouse_pos.x - p.x, mouse_pos.y - p.y);
            }
            if (is_dragging && ImGui::IsMouseDown(0)) {
                ImGui::SetWindowPos(ImVec2(mouse_pos.x - drag_offset.x, mouse_pos.y - drag_offset.y));
            }
            if (!ImGui::IsMouseDown(0)) {
                is_dragging = false;
            }

            // Apple UI Components
            ImColor accentCol(0.0f, 0.478f, 1.0f, 1.0f); // Apple Blue
            ImColor textDim(150, 150, 155, 255);
            ImColor white(255, 255, 255, 255);
            ImColor panelBg(25, 25, 28, 180);

            // Sidebar
            float sidebarW = 180.0f;
            draw->AddRectFilled(p, ImVec2(p.x + sidebarW, p.y + s.y), ImColor(18, 18, 20, 240), 12.0f, ImDrawFlags_RoundCornersLeft);
            draw->AddLine(ImVec2(p.x + sidebarW, p.y), ImVec2(p.x + sidebarW, p.y + s.y), ImColor(40, 40, 45, 255));

            // Title
            ImGui::PushFont(titleFont);
            draw->AddText(ImVec2(p.x + 20, p.y + 20), white, "Star");
            ImGui::PopFont();

            // Sidebar Navigation
            static int mainTab = 0;
            const char* sbn[] = {"Aimbot", "Visuals", "Misc", "Triggerbot"};
            const char* sbicons[] = {ICON_FA_CROSSHAIRS, ICON_FA_EYE, ICON_FA_SLIDERS, ICON_FA_GUN};

            for(int i = 0; i < 4; i++) {
                float iy = p.y + 80.0f + i * 45.0f;
                float ix = p.x + 10.0f;
                float iw = sidebarW - 20.0f;
                float ih = 36.0f;

                bool act = (mainTab == i);
                bool hov = ImGui::IsMouseHoveringRect(ImVec2(ix, iy), ImVec2(ix + iw, iy + ih));

                if (act) {
                    draw->AddRectFilled(ImVec2(ix, iy), ImVec2(ix + iw, iy + ih), accentCol, 8.0f);
                } else if (hov) {
                    draw->AddRectFilled(ImVec2(ix, iy), ImVec2(ix + iw, iy + ih), ImColor(40, 40, 45, 150), 8.0f);
                }

                float oy = iy + 10;
                draw->AddText(ImVec2(ix + 15, oy), act ? white : textDim, sbicons[i]);
                draw->AddText(ImVec2(ix + 40, oy), act ? white : textDim, sbn[i]);

                if(hov && ImGui::IsMouseClicked(0)) { mainTab = i; }
            }

            // Helper Widgets
            auto AppleToggle = [&](const char* title, const char* desc, bool* val) {
                ImGui::PushID(title);
                ImGui::BeginGroup();
                ImVec2 pos = ImGui::GetCursorScreenPos();
                float width = ImGui::GetContentRegionAvail().x;
                float height = desc[0] != '\0' ? 40.0f : 24.0f;

                ImGui::InvisibleButton("##btn", ImVec2(width, height));
                if (ImGui::IsItemClicked()) *val = !*val;

                // Text
                ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + 2));
                ImGui::TextColored(ImVec4(1,1,1,1), "%s", title);
                if (desc[0] != '\0') {
                    ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + 20));
                    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.62f, 1.0f), "%s", desc);
                }
                
                // Toggle Switch
                float toggleW = 40.0f;
                float toggleH = 22.0f;
                ImVec2 tPos = ImVec2(pos.x + width - toggleW, pos.y + (height - toggleH) / 2.0f);

                // Animated toggle
                static std::map<ImGuiID, float> anims;
                ImGuiID id = ImGui::GetID("##toggle");
                float target = *val ? 1.0f : 0.0f;
                if (anims.find(id) == anims.end()) anims[id] = target;
                anims[id] = ImLerp(anims[id], target, ImGui::GetIO().DeltaTime * 12.0f);
                float t = anims[id];
                
                ImU32 bgCol = ImColor::HSV(0.0f, 0.0f, 0.2f + t * 0.1f);
                ImU32 activeCol = ImGui::ColorConvertFloat4ToU32(ImVec4(0.2f, 0.8f, 0.35f, 1.0f)); // Apple Green
                ImU32 col = ImU32((bgCol & 0xFF000000) |
                    ((int)(((activeCol & 0xFF0000) >> 16) * t + ((bgCol & 0xFF0000) >> 16) * (1-t)) << 16) |
                    ((int)(((activeCol & 0xFF00) >> 8) * t + ((bgCol & 0xFF00) >> 8) * (1-t)) << 8) |
                    ((int)((activeCol & 0xFF) * t + (bgCol & 0xFF) * (1-t))));
                
                ImGui::GetWindowDrawList()->AddRectFilled(tPos, ImVec2(tPos.x + toggleW, tPos.y + toggleH), col, toggleH / 2.0f);

                float circle_r = toggleH / 2.0f - 2.0f;
                float circle_x = tPos.x + 2.0f + circle_r + (toggleW - toggleH) * t;
                ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(circle_x, tPos.y + toggleH / 2.0f), circle_r, IM_COL32(255, 255, 255, 255));

                ImGui::EndGroup();
                ImGui::PopID();
                ImGui::Dummy(ImVec2(0, 4));
            };

            auto AppleSlider = [&](const char* label, float* v, float v_min, float v_max, const char* format) {
                ImGui::PushID(label);
                ImGui::BeginGroup();
                ImVec2 pos = ImGui::GetCursorScreenPos();
                float width = ImGui::GetContentRegionAvail().x;

                ImGui::TextColored(ImVec4(1,1,1,1), "%s", label);

                char valBuf[32]; snprintf(valBuf, 32, format, *v);
                float valW = ImGui::CalcTextSize(valBuf).x;
                ImGui::SetCursorScreenPos(ImVec2(pos.x + width - valW, pos.y));
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.62f, 1.0f), "%s", valBuf);

                ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + 24));

                float sliderH = 6.0f;
                ImVec2 sPos = ImGui::GetCursorScreenPos();
                ImGui::InvisibleButton("##sl", ImVec2(width, 16.0f));
                bool active = ImGui::IsItemActive();
                bool hovered = ImGui::IsItemHovered();

                if (active) {
                    float t = ImClamp((ImGui::GetIO().MousePos.x - sPos.x) / width, 0.0f, 1.0f);
                    *v = v_min + t * (v_max - v_min);
                }

                float t = (*v - v_min) / (v_max - v_min);
                t = ImClamp(t, 0.0f, 1.0f);

                auto dl = ImGui::GetWindowDrawList();
                dl->AddRectFilled(ImVec2(sPos.x, sPos.y + 5), ImVec2(sPos.x + width, sPos.y + 5 + sliderH), ImColor(45, 45, 50, 255), sliderH/2);
                dl->AddRectFilled(ImVec2(sPos.x, sPos.y + 5), ImVec2(sPos.x + width * t, sPos.y + 5 + sliderH), accentCol, sliderH/2);
                dl->AddCircleFilled(ImVec2(sPos.x + width * t, sPos.y + 5 + sliderH/2), 8.0f, IM_COL32(255,255,255,255));

                ImGui::EndGroup();
                ImGui::PopID();
                ImGui::Dummy(ImVec2(0, 4));
            };

            auto AppleCombo = [&](const char* label, int* current_item, const char* const items[], int items_count) {
                ImGui::PushID(label);
                ImGui::BeginGroup();
                ImVec2 pos = ImGui::GetCursorScreenPos();
                float width = ImGui::GetContentRegionAvail().x;

                ImGui::TextColored(ImVec4(1,1,1,1), "%s", label);
                ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + 24));

                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.16f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.2f, 0.2f, 0.22f, 1.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
                ImGui::SetNextItemWidth(width);
                ImGui::Combo("##cb", current_item, items, items_count);
                ImGui::PopStyleVar();
                ImGui::PopStyleColor(2);

                ImGui::EndGroup();
                ImGui::PopID();
                ImGui::Dummy(ImVec2(0, 4));
            };

            auto AppleBind = [&](const char* label, int* key) {
                ImGui::PushID(label);
                ImGui::BeginGroup();
                ImVec2 pos = ImGui::GetCursorScreenPos();
                float width = ImGui::GetContentRegionAvail().x;

                ImGui::TextColored(ImVec4(1,1,1,1), "%s", label);

                auto GetKeyName = [](int k) -> std::string {
                    if (k == 0) return "None";
                    if (k == VK_LBUTTON) return "LMB";
                    if (k == VK_RBUTTON) return "RMB";
                    if (k == VK_MBUTTON) return "MMB";
                    if (k == VK_XBUTTON1) return "MB4";
                    if (k == VK_XBUTTON2) return "MB5";
                    if (k == VK_SHIFT || k == VK_LSHIFT || k == VK_RSHIFT) return "Shift";
                    if (k == VK_CONTROL || k == VK_LCONTROL || k == VK_RCONTROL) return "Ctrl";
                    if (k == VK_MENU || k == VK_LMENU || k == VK_RMENU) return "Alt";
                    char name[128];
                    UINT scanCode = MapVirtualKeyA(k, MAPVK_VK_TO_VSC);
                    if (GetKeyNameTextA(scanCode << 16, name, 128) > 0) return std::string(name);
                    return "Key";
                };

                static std::string waiting_for = "";
                bool is_waiting = (waiting_for == label);
                std::string k_str = is_waiting ? "..." : GetKeyName(key ? *key : 0);

                float btnW = ImGui::CalcTextSize(k_str.c_str()).x + 24.0f;
                if (btnW < 60.0f) btnW = 60.0f;

                ImGui::SetCursorScreenPos(ImVec2(pos.x + width - btnW, pos.y - 4));

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.16f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.22f, 1.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
                if (ImGui::Button(k_str.c_str(), ImVec2(btnW, 26.0f))) {
                    waiting_for = label;
                }
                ImGui::PopStyleVar();
                ImGui::PopStyleColor(2);

                if (is_waiting) {
                    for (int k = 1; k < 256; k++) {
                        if (GetAsyncKeyState(k) & 0x8000) {
                            if (k != VK_LBUTTON && k != VK_ESCAPE && k != VK_RETURN) {
                                if (key) *key = k;
                                waiting_for = ""; break;
                            } else if (k == VK_ESCAPE) {
                                if (key) *key = 0;
                                waiting_for = ""; break;
                            }
                        }
                    }
                }
                
                ImGui::EndGroup();
                ImGui::PopID();
                ImGui::Dummy(ImVec2(0, 4));
            };

            auto BeginApplePanel = [&](const char* title, float height = 0) {
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.11f, 0.11f, 0.12f, 1.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 16));
                ImGui::BeginChild(title, ImVec2(0, height), true, ImGuiWindowFlags_AlwaysUseWindowPadding);
                ImGui::TextColored(ImVec4(1,1,1,1), "%s", title);
                ImGui::Dummy(ImVec2(0, 8));
            };
            auto EndApplePanel = [&]() {
                ImGui::EndChild();
                ImGui::PopStyleVar(2);
                ImGui::PopStyleColor();
            };

            // Main Content Area
            ImGui::SetCursorPos(ImVec2(sidebarW + 20, 20));
            ImGui::BeginChild("Content", ImVec2(s.x - sidebarW - 40, s.y - 40), false);

            // Header
            ImGui::PushFont(titleFont);
            draw->AddText(ImGui::GetCursorScreenPos(), white, sbn[mainTab]);
            ImGui::PopFont();
            ImGui::Dummy(ImVec2(0, 40));

            if (mainTab == 0) { // Aimbot
                ImGui::BeginGroup();
                ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.48f);

                BeginApplePanel("Main settings", 420);
                AppleToggle("Enabled", "Master switch for aimbot", &Options::Aimbot::Aimbot);
                AppleBind("Aimbot Key", &Options::Aimbot::AimbotKey);
                const char* aimTypes[] = {"Camera", "Mouse"};
                AppleCombo("Aiming Type", &Options::Aimbot::AimingType, aimTypes, 2);
                const char* toggleTypes[] = {"Hold", "Toggle"};
                AppleCombo("Activation Mode", &Options::Aimbot::ToggleType, toggleTypes, 2);

                AppleToggle("Prediction", "Predict target movement", &Options::Aimbot::Prediction);
                if (Options::Aimbot::Prediction) {
                    AppleSlider("Pred X", &Options::Aimbot::PredictionX, 0.0f, 10.0f, "%.2f");
                    AppleSlider("Pred Y", &Options::Aimbot::PredictionY, 0.0f, 10.0f, "%.2f");
                }
                EndApplePanel();
                ImGui::PopItemWidth();
                ImGui::EndGroup();

                ImGui::SameLine(0, 20);

                ImGui::BeginGroup();
                BeginApplePanel("Modifiers", 420);
                AppleSlider("Smoothing", &Options::Aimbot::Smoothness, 0.0f, 1.0f, "%.2f");
                AppleSlider("FOV", &Options::Aimbot::FOV, 10.0f, 360.0f, "%.0f");
                AppleSlider("Max Distance", &Options::Aimbot::Range, 10.0f, 10000.0f, "%.0f");

                AppleToggle("Shake", "Simulate recoil/shake", &Options::Aimbot::Shake);
                AppleToggle("Sticky Aim", "Keep aiming at same target", &Options::Aimbot::StickyAim);
                AppleToggle("Teamcheck", "Ignore teammates", &Options::Aimbot::TeamCheck);
                AppleToggle("Knockcheck", "Ignore downed players", &Options::Aimbot::DownedCheck);
                EndApplePanel();
                ImGui::EndGroup();

            } else if (mainTab == 1) { // Visuals
                ImGui::BeginGroup();
                ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.48f);

                BeginApplePanel("ESP Targets", 280);
                AppleToggle("Names", "Show player names", &Options::ESP::Name);
                AppleToggle("Distance", "Show distance to player", &Options::ESP::Distance);
                AppleToggle("Health", "Show player health", &Options::ESP::Health);
                AppleToggle("Tracers", "Draw lines to players", &Options::ESP::Tracers);
                AppleToggle("Teamcheck", "Hide teammates", &Options::ESP::TeamCheck);
                EndApplePanel();
                ImGui::PopItemWidth();
                ImGui::EndGroup();

                ImGui::SameLine(0, 20);

                ImGui::BeginGroup();
                BeginApplePanel("ESP Settings", 280);
                const char* boxTypes[] = {"None", "2D Box", "3D Box"};
                AppleCombo("Box Type", &Options::ESP::BoxType, boxTypes, 3);
                AppleSlider("Box Thickness", &Options::ESP::BoxThickness, 1.0f, 10.0f, "%.1f");
                AppleSlider("Tracer Thickness", &Options::ESP::TracerThickness, 1.0f, 10.0f, "%.1f");
                EndApplePanel();
                ImGui::EndGroup();

            } else if (mainTab == 2) { // Misc
                ImGui::BeginGroup();
                ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.48f);

                BeginApplePanel("Utility", 240);
                AppleToggle("Show FOV", "Render aimbot FOV circle", &Options::Aimbot::ShowFOV);
                AppleToggle("Crosshair", "Custom crosshair", &Options::Crosshair::Enabled);
                AppleToggle("Stream Proof", "Hide overlay from capture", &Options::Misc::StreamProof);
                AppleToggle("Keybind List", "Show active keybinds", &Options::Misc::KeybindList);
                AppleBind("Menu Key", &Options::Misc::MenuKey);
                EndApplePanel();
                ImGui::PopItemWidth();
                ImGui::EndGroup();

                ImGui::SameLine(0, 20);

                ImGui::BeginGroup();
                BeginApplePanel("Desync", 240);
                AppleToggle("Enable Desync", "Desynchronize player state", &Options::Desync::Enabled);
                AppleBind("Desync Key", &Options::Desync::Key);
                const char* desyncModes[] = {"Hold", "Toggle"};
                AppleCombo("Mode", &Options::Desync::ToggleType, desyncModes, 2);
                AppleToggle("Show Server Pos", "Visualize real position", &Options::Desync::Visualizer);
                EndApplePanel();
                ImGui::EndGroup();

            } else if (mainTab == 3) { // Triggerbot
                ImGui::BeginGroup();
                ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.48f);

                BeginApplePanel("Triggerbot", 240);
                AppleToggle("Enabled", "Auto-fire when aiming at target", &Options::Triggerbot::Enabled);
                AppleBind("Trigger Key", &Options::Triggerbot::TriggerbotKey);
                const char* tbToggle[] = {"Hold", "Toggle"};
                AppleCombo("Activation Mode", &Options::Triggerbot::ToggleType, tbToggle, 2);

                float delayF = (float)Options::Triggerbot::Delay;
                AppleSlider("Delay (ms)", &delayF, 0.0f, 500.0f, "%.0f");
                Options::Triggerbot::Delay = (int)delayF;

                EndApplePanel();
                ImGui::PopItemWidth();
                ImGui::EndGroup();
            }

            ImGui::EndChild(); // Content

            ImGui::End();
            ImGui::PopStyleVar(4);
            ImGui::PopStyleColor(2);
        }
        if (IsGameOnTop("Roblox"))
        {
            if (!menu_open)
            {
                RenderESP(ImGui::GetBackgroundDrawList());
                RunAimbot(ImGui::GetBackgroundDrawList());
                RunTriggerbot();
                RunMacro();
            }
            
            // Render advanced FOV visualization even when menu is open
            RenderAdvancedFOV(ImGui::GetBackgroundDrawList());
            RenderAimbotFOV(ImGui::GetBackgroundDrawList());
            
            // Render crosshair even when menu is open
            RenderCrosshair(ImGui::GetBackgroundDrawList());
            
            // Render Desync Visualizer
            if (Options::Desync::Enabled && Options::Desync::Visualizer && Options::Desync::Active) {
                if (Options::Desync::SpawnPos.x != 0.0f || Options::Desync::SpawnPos.y != 0.0f || Options::Desync::SpawnPos.z != 0.0f) {
                    auto visualizer2D = WorldToScreen(Options::Desync::SpawnPos);
                    if (visualizer2D.x != -1 && visualizer2D.y != -1) {
                        ImColor visColor = IM_COL32(
                            static_cast<int>(Options::Desync::VisualizerColor[0] * 255.f),
                            static_cast<int>(Options::Desync::VisualizerColor[1] * 255.f),
                            static_cast<int>(Options::Desync::VisualizerColor[2] * 255.f),
                            255);
                        
                        // Draw a dot for the server-sided position
                        ImGui::GetBackgroundDrawList()->AddCircleFilled(ImVec2(visualizer2D.x, visualizer2D.y), 4.0f, visColor);
                        // Optional outline
                        ImGui::GetBackgroundDrawList()->AddCircle(ImVec2(visualizer2D.x, visualizer2D.y), 5.0f, IM_COL32(0,0,0,255), 0, 1.5f);
                        
                        // Draw "Server Pos" text above it
                        ImFont* font = ImGui::GetFont();
                        std::string text = "Server Pos";
                        ImVec2 textSize = font->CalcTextSizeA(14.0f, FLT_MAX, 0.f, text.c_str());
                        ImGui::GetBackgroundDrawList()->AddText(font, 14.0f, ImVec2(visualizer2D.x - textSize.x/2, visualizer2D.y - 20.0f), visColor, text.c_str());
                    }
                }
            }
            
            // Render keybind list
            RenderKeybindList(ImGui::GetBackgroundDrawList());
            std::string str = "Star | " + std::to_string(static_cast<int>(io.Framerate)) + " FPS";
            ImVec2 textSize = ImGui::CalcTextSize(str.c_str());
            ImVec2 pos = ImVec2(io.DisplaySize.x - textSize.x - 10.0f, 10.0f);
            ImDrawList* drawList = ImGui::GetBackgroundDrawList();
            drawList->AddText(pos, IM_COL32(255, 255, 255, 255), str.c_str());
        }
        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        HRESULT hr = g_pSwapChain->Present(0, 0);
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
}
bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 4;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK) return false;
    CreateRenderTarget();
    return true;
}
void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}
void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}
void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;
    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED) return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam);
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
