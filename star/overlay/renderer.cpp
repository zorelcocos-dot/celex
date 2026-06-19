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
void DrawNode(RobloxInstance& node) {
    const auto& children = node.GetChildren();
    if (children.empty()) {
        ImGui::BulletText(node.Name().c_str());
    } else {
        if (ImGui::TreeNode(node.Name().c_str())) {
            for (auto child : children) {
                DrawNode(child);
            }
            ImGui::TreePop();
        }
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
    float padding = 8.0f;
    float lineHeight = 14.0f;
    float titleHeight = 20.0f;
    float minWidth = 150.0f; // Reduced minimum width
    float maxWidth = minWidth;
    for (const auto& bind : activeBinds) {
        std::string fullText = bind.first + " " + bind.second;
        float textWidth = ImGui::CalcTextSize(fullText.c_str()).x;
        if (textWidth > maxWidth) maxWidth = textWidth;
    }
    float boxWidth = maxWidth + padding * 2;
    float boxHeight = titleHeight + (activeBinds.size() * lineHeight) + padding;
    // Use custom position from sliders
    ImVec2 pos = ImVec2(Options::Misc::KeybindListX, Options::Misc::KeybindListY);
    // Draw background - fully opaque (255 alpha instead of 200)
    drawList->AddRectFilled(pos, ImVec2(pos.x + boxWidth, pos.y + boxHeight), IM_COL32(8, 8, 8, 255), 4.0f);
    drawList->AddRect(pos, ImVec2(pos.x + boxWidth, pos.y + boxHeight), IM_COL32(27, 27, 27, 255), 4.0f);
    // Draw title - centered
    const char* title = "Keybinds";
    float titleWidth = ImGui::CalcTextSize(title).x;
    float titleX = pos.x + (boxWidth - titleWidth) / 2.0f;
    drawList->AddText(ImVec2(titleX, pos.y + 4), IM_COL32(255, 255, 255, 255), title);
    drawList->AddLine(ImVec2(pos.x, pos.y + titleHeight), ImVec2(pos.x + boxWidth, pos.y + titleHeight), IM_COL32(27, 27, 27, 255));
    // Draw active binds - centered
    float yOffset = pos.y + titleHeight + 3;
    for (const auto& bind : activeBinds) {
        std::string fullText = bind.first + " " + bind.second;
        float textWidth = ImGui::CalcTextSize(fullText.c_str()).x;
        float textX = pos.x + (boxWidth - textWidth) / 2.0f;
        // Draw the full text centered
        drawList->AddText(ImVec2(textX, yOffset), IM_COL32(255, 255, 255, 255), bind.first.c_str());
        // Draw status in accent color right after the name
        float nameWidth = ImGui::CalcTextSize(bind.first.c_str()).x;
        drawList->AddText(ImVec2(textX + nameWidth + 5, yOffset), IM_COL32(main_color.x * 255, main_color.y * 255, main_color.z * 255, 255), bind.second.c_str());
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
    int tab = 0;
    int tab2 = 0;
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
        // Fade animation
        static float menuAlpha = 0.0f;
        static float backgroundAlpha = 0.0f;
        float fadeSpeed = 0.08f; // Adjust for faster/slower fade
        if (menu_open) {
            if (menuAlpha < 1.0f) menuAlpha += fadeSpeed;
            if (menuAlpha > 1.0f) menuAlpha = 1.0f;
            if (backgroundAlpha < 0.7f) backgroundAlpha += fadeSpeed;
            if (backgroundAlpha > 0.7f) backgroundAlpha = 0.7f;
        } else {
            if (menuAlpha > 0.0f) menuAlpha -= fadeSpeed;
            if (menuAlpha < 0.0f) menuAlpha = 0.0f;
            if (backgroundAlpha > 0.0f) backgroundAlpha -= fadeSpeed;
            if (backgroundAlpha < 0.0f) backgroundAlpha = 0.0f;
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
            // Draw dark background overlay
            if (backgroundAlpha > 0.0f) {
                ImGui::GetBackgroundDrawList()->AddRectFilled(
                    ImVec2(0, 0),
                    ImVec2(io.DisplaySize.x, io.DisplaySize.y),
                    IM_COL32(0, 0, 0, static_cast<int>(backgroundAlpha * 180))
                );
            }
            auto s = ImVec2{}, p = ImVec2{}, gs = ImVec2{ 1050, 750 };
            ImGui::SetNextWindowSize(gs);
            ImGui::SetNextWindowBgAlpha(menuAlpha);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, menuAlpha);
            ImGui::Begin("##GUI", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove);
            {
                s = ImVec2(ImGui::GetWindowSize().x - ImGui::GetStyle().WindowPadding.x * 2, ImGui::GetWindowSize().y - ImGui::GetStyle().WindowPadding.y * 2);
                p = ImVec2(ImGui::GetWindowPos().x + ImGui::GetStyle().WindowPadding.x, ImGui::GetWindowPos().y + ImGui::GetStyle().WindowPadding.y);
                auto draw = ImGui::GetWindowDrawList();
                // Custom Dragging Logic
                static bool is_dragging = false;
                static ImVec2 drag_offset;
                ImVec2 mouse_pos = ImGui::GetIO().MousePos;
                ImVec2 top_min = p;
                ImVec2 top_max = ImVec2(p.x + s.x, p.y + 50.0f);
                ImVec2 bot_min = ImVec2(p.x, p.y + s.y - 32.0f);
                ImVec2 bot_max = ImVec2(p.x + s.x, p.y + s.y);
                if ((ImGui::IsMouseHoveringRect(top_min, top_max) || ImGui::IsMouseHoveringRect(bot_min, bot_max)) && ImGui::IsMouseClicked(0)) {
                    is_dragging = true;
                    drag_offset = ImVec2(mouse_pos.x - ImGui::GetWindowPos().x, mouse_pos.y - ImGui::GetWindowPos().y);
                }
                if (is_dragging && ImGui::IsMouseDown(0)) {
                    ImGui::SetWindowPos(ImVec2(mouse_pos.x - drag_offset.x, mouse_pos.y - drag_offset.y));
                }
                if (!ImGui::IsMouseDown(0)) {
                    is_dragging = false;
                }
                // ─── celex UI (New Premium) ─────────────────────────────────────────
                ImColor bg(0, 0, 0, 255); 
                ImColor bgSidebar(0, 0, 0, 0); 
                ImColor bgPanel(8, 8, 10, 190);
                ImColor border(24, 24, 28, 255); 
                ImColor accentCol(166, 110, 255, 255); // Soft violet accent (#A66EFF)
                ImColor textDim(90, 90, 100, 255), textBright(200, 200, 210, 255), white(255, 255, 255, 255);
                float W = s.x, H = s.y, headerH = 65.0f, sidebarW = 180.0f, bottomH = 40.0f;
                float contentY = p.y + headerH, contentH = H - headerH - bottomH;
                // Main bg
                draw->AddRectFilled(ImVec2(p.x, p.y), ImVec2(p.x + W, p.y + H), bg, 12.0f);
                
                // Animated background blur/glow (Apple VisionOS style oversized radial gradient)
                static float time = 0.0f;
                time += ImGui::GetIO().DeltaTime;
                
                float cx1 = p.x + W * (0.5f + 0.4f * sinf(time * 0.4f));
                float cy1 = p.y + H * (0.5f + 0.4f * cosf(time * 0.3f));
                float maxRadius1 = W * 0.85f;
                
                float cx2 = p.x + W * (0.5f + 0.4f * cosf(time * 0.5f));
                float cy2 = p.y + H * (0.5f + 0.4f * sinf(time * 0.45f));
                float maxRadius2 = W * 0.75f;

                int steps = 20;
                for (int i = steps; i >= 1; i--) {
                    float fraction = (float)i / (float)steps;
                    float radiusScale = pow(fraction, 0.7f);
                    
                    int a = 1; // 1 out of 255 for extreme low opacity
                    draw->AddCircleFilled(ImVec2(cx1, cy1), maxRadius1 * radiusScale, ImColor(150, 100, 250, a), 64);
                    draw->AddCircleFilled(ImVec2(cx2, cy2), maxRadius2 * radiusScale, ImColor(80, 60, 200, a), 64);
                }

                // Add subtle moving dot grid
                float dotSpacing = 35.0f;
                float dotOffset = fmodf(time * 15.0f, dotSpacing);
                for (float x = p.x - dotSpacing; x < p.x + W + dotSpacing; x += dotSpacing) {
                    for (float y = p.y - dotSpacing; y < p.y + H + dotSpacing; y += dotSpacing) {
                        float dx = x + dotOffset;
                        float dy = y + dotOffset;
                        if (dx >= p.x && dy >= p.y && dx <= p.x + W && dy <= p.y + H) {
                            float distEdgeX = fminf(dx - p.x, p.x + W - dx);
                            float distEdgeY = fminf(dy - p.y, p.y + H - dy);
                            float alphaScale = fmaxf(0.0f, fminf(fminf(distEdgeX, distEdgeY) / 50.0f, 1.0f));
                            if (alphaScale > 0.0f) {
                                draw->AddCircleFilled(ImVec2(dx, dy), 1.0f, ImColor(255, 255, 255, static_cast<int>(12 * alphaScale)));
                            }
                        }
                    }
                }

                // Outer border
                draw->AddRect(ImVec2(p.x, p.y), ImVec2(p.x + W, p.y + H), border, 12.0f, 0, 1.5f);
                // Structural Lines
                // Horizontal line between header and content
                draw->AddLine(ImVec2(p.x, p.y + headerH), ImVec2(p.x + W, p.y + headerH), border, 1.0f);
                // Horizontal line between content and footer
                draw->AddLine(ImVec2(p.x, p.y + H - bottomH), ImVec2(p.x + W, p.y + H - bottomH), border, 1.0f);
                // Vertical line for sidebar (starts below header line, ends above footer line)
                draw->AddLine(ImVec2(p.x + sidebarW, p.y + headerH), ImVec2(p.x + sidebarW, p.y + H - bottomH), border, 1.0f);
                ImGui::PushFont(font);
                // Big celex logo
                { 
                    float lx = p.x + 24, ly = p.y + 24;
                    // Draw "celex" without pixelation by using titleFont
                    draw->AddText(titleFont, 24.0f, ImVec2(lx + 8, ly - 4), accentCol, "celex");
                }
                // Top tabs
                static int mainTab = 0;
                {
                    const char* tdisp[] = {"Aimbot", "Visuals", "Misc"};
                    float tsx = p.x + 210.0f, tw = 75.0f;
                    for(int i = 0; i < 3; i++) {
                        float tx = tsx + i * tw;
                        bool act = (mainTab == i);
                        bool hov = ImGui::IsMouseHoveringRect(ImVec2(tx, p.y + 10), ImVec2(tx + tw, p.y + headerH));
                        ImVec2 tsz = ImGui::CalcTextSize(tdisp[i]);
                        draw->AddText(ImVec2(tx + (tw - tsz.x) * 0.5f, p.y + 28), act ? white : textDim, tdisp[i]);
                        if(act) {
                            draw->AddRectFilled(ImVec2(tx + 8, p.y + headerH - 1), ImVec2(tx + tw - 8, p.y + headerH + 1), accentCol, 1.0f);
                            draw->AddRectFilled(ImVec2(tx + 8, p.y + headerH - 2), ImVec2(tx + tw - 8, p.y + headerH + 2), ImColor(150, 80, 255, 120), 2.0f);
                        }
                        if(hov && ImGui::IsMouseClicked(0)) { mainTab = i; tab2 = 0; }
                    }
                }
                // Sidebar items
                const char* sbn[] = {"Aimbot", "Visuals", "Misc"};
                const char* sbicons[] = {ICON_FA_CROSSHAIRS, ICON_FA_EYE, ICON_FA_SLIDERS};
                for(int i = 0; i < 3; i++) {
                  float iy = contentY + 16.0f + i * 58.0f, ix = p.x + 16.0f, iw = sidebarW - 32.0f, ih = 46.0f;
                  bool act = (mainTab == i);
                  bool hov = ImGui::IsMouseHoveringRect(ImVec2(ix, iy), ImVec2(ix + iw, iy + ih));
                  if(act) {
                      draw->AddRectFilled(ImVec2(ix, iy), ImVec2(ix + iw, iy + ih), ImColor(150, 80, 255, 15), 8.0f);
                      draw->AddRect(ImVec2(ix, iy), ImVec2(ix + iw, iy + ih), ImColor(150, 80, 255, 120), 8.0f, 0, 1.0f);
                      draw->AddRect(ImVec2(ix - 1, iy - 1), ImVec2(ix + iw + 1, iy + ih + 1), ImColor(150, 80, 255, 30), 8.0f, 0, 1.0f);
                  }
                  else if(hov) draw->AddRectFilled(ImVec2(ix, iy), ImVec2(ix + iw, iy + ih), ImColor(30, 25, 45, 150), 8.0f);
                  
                  float ox = ix + 16, oy = iy + 14;
                  if (act) {
                      ImColor iconCol = accentCol;
                      iconCol.Value.w = 0.85f; // 85% opacity
                      draw->AddText(ImVec2(ox, oy), iconCol, sbicons[i]);
                  } else {
                      draw->AddText(ImVec2(ox, oy), textDim, sbicons[i]);
                  }
                  draw->AddText(ImVec2(ox + 26, iy + 15), act ? white : textDim, sbn[i]);
                  if(hov && ImGui::IsMouseClicked(0)) { mainTab = i; tab2 = 0; }
                }
                // Removed Sidebar footer
                // Content area panels
                float contentX = p.x + sidebarW, contentAreaW = W - sidebarW;
                float pp = 16.0f;
                float lpw = (contentAreaW - pp * 3) * 0.46f, rpw = (contentAreaW - pp * 3) * 0.54f;
                float px1 = contentX + pp, px2 = px1 + lpw + pp;
                float py2 = contentY + 16.0f, ph = contentH - 32.0f;
                ImColor headerIconCol = accentCol;
                headerIconCol.Value.w = 0.85f; // 85% opacity
                // Panels rendering moved to scrollable area
                // Bottom bar
                float fy = p.y + H - bottomH + 12.0f;
                draw->AddText(ImVec2(p.x + 16, fy), textDim, ICON_FA_BOX);
                draw->AddText(ImVec2(p.x + 36, fy), textDim, "Build: celex.gg");
                float buildW = ImGui::CalcTextSize("Build: celex.gg").x;
                float lineX = p.x + 36 + buildW + 12;
                draw->AddText(ImVec2(lineX, fy), textDim, "|");
                draw->AddText(ImVec2(lineX + 16, fy), accentCol, "discord.gg/celexrbx");
                // Components Definitions
                auto CelexCheck = [&](float x, float& y, const char* cbid, const char* title, const char* desc, bool* val) {
                  auto draw = ImGui::GetWindowDrawList();
                  ImVec2 cb_pos = ImVec2(x, y);
                  float cb_size = 16.0f;
                  ImGui::SetCursorScreenPos(cb_pos);
                  ImGui::InvisibleButton(cbid, ImVec2(cb_size + 16.0f + ImGui::CalcTextSize(title).x, 20.0f));
                  bool hovered = ImGui::IsItemHovered();
                  if (ImGui::IsItemClicked()) { *val = !*val; }
                  
                  ImColor bgCb;
                  if (*val) bgCb = accentCol;
                  else bgCb = hovered ? ImColor(40, 30, 50, 255) : ImColor(20, 15, 25, 255);
                  
                  if (*val) draw->AddRectFilled(cb_pos, ImVec2(cb_pos.x + cb_size, cb_pos.y + cb_size), ImColor(150, 80, 255, 60), 4.0f);
                  
                  draw->AddRectFilled(cb_pos, ImVec2(cb_pos.x + cb_size, cb_pos.y + cb_size), bgCb, 4.0f);
                  if (*val) {
                      ImVec2 csz = ImGui::CalcTextSize(ICON_FA_CHECK);
                      draw->AddText(ImVec2(cb_pos.x + (cb_size - csz.x)/2.0f, cb_pos.y + (cb_size - csz.y)/2.0f), white, ICON_FA_CHECK);
                  }
                  draw->AddText(ImVec2(x + 32, y + 1), white, title);
                  y += 24.0f;
                  draw->AddText(ImGui::GetFont(), ImGui::GetFontSize() - 1.0f, ImVec2(x + 32, y), textDim, desc);
                  y += 27.0f;
                };
                float riw = rpw - 32.0f;
                auto CelexSlider = [&](float x, float& y, const char* lbl, float* val, float mn, float mx, const char* fmt) {
                  auto draw = ImGui::GetWindowDrawList();
                  char vb[32]; snprintf(vb, 32, fmt, *val);
                  draw->AddText(ImVec2(x, y), textBright, lbl);
                  ImVec2 vsz = ImGui::CalcTextSize(vb);
                  draw->AddText(ImVec2(x + riw - vsz.x, y), textDim, vb);
                  y += 27.0f;
                  ImVec2 s_pos = ImVec2(x, y);
                  float s_w = riw;
                  float s_h = 4.0f;
                  float fraction = (*val - mn) / (mx - mn);
                  if (fraction < 0.0f) fraction = 0.0f;
                  if (fraction > 1.0f) fraction = 1.0f;
                  draw->AddRectFilled(s_pos, ImVec2(s_pos.x + s_w, s_pos.y + s_h), ImColor(30, 25, 40, 255), 2.0f);
                  
                  float fill_w = s_w * fraction;
                  draw->AddRectFilled(ImVec2(s_pos.x, s_pos.y - 1.0f), ImVec2(s_pos.x + fill_w, s_pos.y + s_h + 1.0f), ImColor(150, 80, 255, 80), 2.0f);
                  draw->AddRectFilled(s_pos, ImVec2(s_pos.x + fill_w, s_pos.y + s_h), accentCol, 2.0f);
                  
                  float thumb_x = s_pos.x + fill_w;
                  float thumb_y = s_pos.y + s_h / 2.0f;
                  draw->AddRectFilled(ImVec2(thumb_x - 3.5f, thumb_y - 7.0f), ImVec2(thumb_x + 3.5f, thumb_y + 7.0f), white, 3.5f);
                  draw->AddRectFilled(ImVec2(thumb_x - 5.0f, thumb_y - 9.0f), ImVec2(thumb_x + 5.0f, thumb_y + 9.0f), ImColor(255, 255, 255, 60), 4.0f);
                  ImGui::SetCursorScreenPos(ImVec2(x, y - 6.0f));
                  char sid[64]; snprintf(sid, 64, "##%s", lbl);
                  ImGui::InvisibleButton(sid, ImVec2(s_w, 16.0f));
                  if (ImGui::IsItemActive()) {
                      float mouse_x = ImGui::GetIO().MousePos.x;
                      fraction = (mouse_x - s_pos.x) / s_w;
                      if (fraction < 0.0f) fraction = 0.0f;
                      if (fraction > 1.0f) fraction = 1.0f;
                      *val = mn + fraction * (mx - mn);
                  }
                  y += 24.0f;
                };
                static int openComboHash = 0;
                auto CelexCombo = [&](float x, float& y, const char* lbl, const char* icon, int* val, const char** items, int items_count) {
                  auto draw = ImGui::GetWindowDrawList();
                  draw->AddText(ImVec2(x, y), textDim, lbl); y += 27.0f;
                  ImVec2 boxPos = ImVec2(x, y);
                  float boxH = 36.0f; float boxW = riw;
                  
                  ImGui::SetCursorScreenPos(boxPos);
                  char id[64]; snprintf(id, 64, "##combo_%s", lbl);
                  ImGui::InvisibleButton(id, ImVec2(boxW, boxH));
                  bool hovered = ImGui::IsItemHovered();
                  bool clicked = ImGui::IsItemClicked();
                  
                  ImColor bgCb = hovered ? ImColor(25, 20, 35, 255) : ImColor(18, 15, 25, 255);
                  draw->AddRectFilled(boxPos, ImVec2(boxPos.x + boxW, boxPos.y + boxH), bgCb, 6.0f);
                  draw->AddRect(boxPos, ImVec2(boxPos.x + boxW, boxPos.y + boxH), ImColor(30, 25, 45, 255), 6.0f, 0, 1.0f);
                  
                  if (icon && icon[0] != '\0') {
                      draw->AddText(ImVec2(boxPos.x + 12, boxPos.y + 10), textDim, icon);
                      draw->AddText(ImVec2(boxPos.x + 36, boxPos.y + 10), white, items[*val]);
                  } else {
                      draw->AddText(ImVec2(boxPos.x + 12, boxPos.y + 10), white, items[*val]);
                  }
                  draw->AddText(ImVec2(boxPos.x + boxW - 24, boxPos.y + 10), textDim, ICON_FA_CHEVRON_DOWN);
                  
                  if (clicked) {
                      if (openComboHash == (int)ImGui::GetID(id)) openComboHash = 0;
                      else { openComboHash = (int)ImGui::GetID(id); ImGui::OpenPopup(id); }
                  }
                  
                  ImGui::SetNextWindowPos(ImVec2(boxPos.x, boxPos.y + boxH + 4));
                  ImGui::SetNextWindowSize(ImVec2(boxW, 0));
                  ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(18/255.f, 15/255.f, 25/255.f, 1.0f));
                  ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(30/255.f, 25/255.f, 45/255.f, 1.0f));
                  ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(30/255.f, 25/255.f, 45/255.f, 1.0f));
                  ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(40/255.f, 30/255.f, 60/255.f, 1.0f));
                  ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 6.0f);
                  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
                  
                  if (ImGui::BeginPopup(id)) {
                      for (int n = 0; n < items_count; n++) {
                          bool is_selected = (*val == n);
                          if (ImGui::Selectable(items[n], is_selected)) { *val = n; openComboHash = 0; }
                      }
                      ImGui::EndPopup();
                  } else {
                      if (openComboHash == (int)ImGui::GetID(id)) openComboHash = 0;
                  }
                  ImGui::PopStyleVar(2);
                  ImGui::PopStyleColor(4);
                  
                  y += boxH + 22.0f;
                };
                auto CelexBind = [&](float x, float& y, const char* lbl, int* key) {
                  auto draw = ImGui::GetWindowDrawList();
                  auto GetKeyName = [](int k) -> std::string {
                      if (k == 0) return "[None]";
                      if (k == VK_LBUTTON) return "[M1]";
                      if (k == VK_RBUTTON) return "[M2]";
                      if (k == VK_MBUTTON) return "[M3]";
                      if (k == VK_XBUTTON1) return "[M4]";
                      if (k == VK_XBUTTON2) return "[M5]";
                      if (k == VK_SHIFT || k == VK_LSHIFT || k == VK_RSHIFT) return "[Shift]";
                      if (k == VK_CONTROL || k == VK_LCONTROL || k == VK_RCONTROL) return "[Ctrl]";
                      if (k == VK_MENU || k == VK_LMENU || k == VK_RMENU) return "[Alt]";
                      if (k == VK_INSERT) return "[INS]";
                      if (k == VK_DELETE) return "[DEL]";
                      if (k == VK_HOME) return "[HOME]";
                      if (k == VK_END) return "[END]";
                      if (k == VK_PRIOR) return "[PGUP]";
                      if (k == VK_NEXT) return "[PGDN]";
                      char name[128];
                      UINT scanCode = MapVirtualKeyA(k, MAPVK_VK_TO_VSC);
                      if (GetKeyNameTextA(scanCode << 16, name, 128) > 0) return std::string("[") + name + "]";
                      return "[Key]";
                  };
                  
                  static std::map<std::string, int> bind_modes;
                  if (bind_modes.find(lbl) == bind_modes.end()) bind_modes[lbl] = 0;
                  
                  draw->AddText(ImVec2(x, y), textDim, lbl); y += 27.0f;
                  ImVec2 bpos = ImVec2(x, y);
                  
                  static std::string waiting_for = "";
                  bool is_waiting = (waiting_for == lbl);
                  
                  std::string k_str = is_waiting ? "[...]" : GetKeyName(key ? *key : 0);
                  if (k_str == "[None]") k_str = "BIND";
                  
                  ImVec2 tsz = ImGui::CalcTextSize(k_str.c_str());
                  float bw = (tsz.x + 20.0f > 70.0f) ? (tsz.x + 20.0f) : 70.0f;
                  float bh = 30.0f;
                  
                  ImGui::SetCursorScreenPos(ImVec2(x, y));
                  ImGui::InvisibleButton(lbl, ImVec2(bw, bh));
                  bool hovered = ImGui::IsItemHovered();
                  if (ImGui::IsItemClicked()) { waiting_for = lbl; }
                  
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
                  
                  ImColor borderCol = hovered ? ImColor(180, 110, 255, 255) : accentCol;
                  draw->AddRectFilled(bpos, ImVec2(bpos.x + bw, bpos.y + bh), ImColor(20, 15, 30, 255), 6.0f);
                  draw->AddRectFilled(bpos, ImVec2(bpos.x + bw, bpos.y + bh), ImColor(150, 80, 255, 20), 6.0f); // Glow
                  draw->AddRect(bpos, ImVec2(bpos.x + bw, bpos.y + bh), borderCol, 6.0f, 0, 1.5f);
                  draw->AddText(ImVec2(bpos.x + (bw - tsz.x)/2.0f, bpos.y + (bh - tsz.y)/2.0f), accentCol, k_str.c_str());
                  
                  ImVec2 gpos = ImVec2(bpos.x + bw + 14, bpos.y + 7);
                  draw->AddText(gpos, textDim, ICON_FA_GEAR);
                  ImGui::SetCursorScreenPos(ImVec2(gpos.x - 4, gpos.y - 4));
                  std::string gid = std::string("##gear_") + lbl;
                  ImGui::InvisibleButton(gid.c_str(), ImVec2(24, 24));
                  if (ImGui::IsItemClicked()) { ImGui::OpenPopup(gid.c_str()); }
                  
                  ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(18/255.f, 15/255.f, 25/255.f, 1.0f));
                  ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(30/255.f, 25/255.f, 45/255.f, 1.0f));
                  ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 6.0f);
                  if (ImGui::BeginPopup(gid.c_str())) {
                      ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.6f, 1.0f), "Bind Mode");
                      ImGui::Separator();
                      if (ImGui::Selectable("Hold", bind_modes[lbl] == 0)) bind_modes[lbl] = 0;
                      if (ImGui::Selectable("Toggle", bind_modes[lbl] == 1)) bind_modes[lbl] = 1;
                      if (ImGui::Selectable("Always On", bind_modes[lbl] == 2)) bind_modes[lbl] = 2;
                      ImGui::EndPopup();
                  }
                  ImGui::PopStyleVar();
                  ImGui::PopStyleColor(2);
                  y += bh + 19.0f;
                };
                // --- Left and Right Panel Content ---
                ImGui::SetCursorScreenPos(ImVec2(contentX, contentY));
                ImGui::BeginChild("###TabScroll", ImVec2(contentAreaW, contentH - bottomH + 12.0f), false, ImGuiWindowFlags_NoBackground);
                auto tabDraw = ImGui::GetWindowDrawList();
                py2 = ImGui::GetCursorScreenPos().y + 16.0f;
                
                auto DrawPanelBg = [&](float x, float y, float w, float h, const char* icon, const char* title) {
                    ImColor headerIconCol = accentCol; headerIconCol.Value.w = 0.85f;
                    tabDraw->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), bgPanel, 8.0f);
                    tabDraw->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), border, 8.0f, 0, 1.0f);
                    tabDraw->AddText(ImVec2(x + 16, y + 16), headerIconCol, icon);
                    tabDraw->AddText(ImVec2(x + 36, y + 15), white, title);
                };

                if (mainTab == 0) {
                    float lh = 45.0f + 357.0f + (Options::Aimbot::Prediction ? 228.0f : 0.0f) + 16.0f;
                    float rh = 45.0f + 445.0f + 16.0f;
                    float ph = lh > rh ? lh : rh; // Symmetric height
                    
                    DrawPanelBg(px1, py2, lpw, ph, ICON_FA_CROSSHAIRS, "Main Group");
                    DrawPanelBg(px2, py2, rpw, ph, ICON_FA_SLIDERS, "Modifiers");
                    
                    ImGui::SetCursorScreenPos(ImVec2(px1, py2 + 45.0f));
                    ImGui::BeginChild("###LeftPanel", ImVec2(lpw, ph - 45.0f), false, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                    float lix = ImGui::GetCursorScreenPos().x + 16.0f;
                    float cy = ImGui::GetCursorScreenPos().y + 9.0f;
                    riw = lpw - 32.0f;
                    CelexCheck(lix,cy,"##en",  "Enabled",             "Enable aimbot",          &Options::Aimbot::Aimbot);
                    CelexCheck(lix,cy,"##pr",  "Prediction",          "Predict movement",        &Options::Aimbot::Prediction);
                    if (Options::Aimbot::Prediction) {
                        static const char* pt[]={"Default (Dynamic)","Static","Advanced"}; static int pti=0;
                        CelexCombo(lix, cy, "Prediction Type", ICON_FA_CHART_LINE, &pti, pt, 3);
                        static const char* curves[] = {"Linear", "Ease In", "Ease Out", "Ease In-Out", "Custom"};
                        CelexCombo(lix, cy, "Smoothness Curve", ICON_FA_WAVE_SQUARE, &Options::Aimbot::SmoothnessCurve, curves, 5);
                        CelexSlider(lix, cy, "Pred. X", &Options::Aimbot::PredictionX, 0.0f, 10.0f, "%.2f");
                        CelexSlider(lix, cy, "Pred. Y", &Options::Aimbot::PredictionY, 0.0f, 10.0f, "%.2f");
                    }
                    CelexCheck(lix,cy,"##sh",  "Shake",               "Compensate recoil",       &Options::Aimbot::Shake);
                    CelexCheck(lix,cy,"##sa",  "Sticky Aim",          "Sticky target aim",       &Options::Aimbot::StickyAim);
                    CelexCheck(lix,cy,"##df",  "Disable Outside FOV", "Ignore outside FOV",      &Options::Aimbot::TeamCheck);
                    CelexCheck(lix,cy,"##tc",  "Teamcheck",           "Ignore teammates",        &Options::Aimbot::TeamCheck);
                    CelexCheck(lix,cy,"##kc",  "Knockcheck",          "Ignore knocked players",  &Options::Aimbot::DownedCheck);
                    ImGui::EndChild();

                    ImGui::SetCursorScreenPos(ImVec2(px2, py2 + 45.0f));
                    ImGui::BeginChild("###RightPanel", ImVec2(rpw, ph - 45.0f), false, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                    float rix = ImGui::GetCursorScreenPos().x + 16.0f;
                    float ry = ImGui::GetCursorScreenPos().y + 9.0f;
                    riw = rpw - 32.0f;
                    static const char* am[]={"Camera","Mouse"};
                    CelexCombo(rix, ry, "Aimbot Type", ICON_FA_VIDEO, &Options::Aimbot::AimingType, am, 2);
                    CelexBind(rix, ry, "Bind", &Options::Aimbot::AimbotKey);
                    auto drawL = ImGui::GetWindowDrawList();
                    drawL->AddText(ImVec2(rix,ry),textDim,"Hold to activate"); ry+=30.0f;
                    CelexSlider(rix,ry,"Smoothing",&Options::Aimbot::Smoothness,0.0f,1.0f,"%.0f%%");
                    CelexSlider(rix,ry,"FOV",&Options::Aimbot::FOV,10.0f,360.0f,"%.0f%%");
                    CelexSlider(rix,ry,"Max Distance",&Options::Aimbot::Range,10.0f,10000.0f,"%.0f");
                    static float dz=0,xo=0,yo=0;
                    CelexSlider(rix,ry,"Deadzone",&dz,0.0f,100.0f,"%.0f%%");
                    CelexSlider(rix,ry,"X Offset",&xo,-100.0f,100.0f,"%.0f");
                    CelexSlider(rix,ry,"Y Offset",&yo,-100.0f,100.0f,"%.0f");
                    ImGui::EndChild();
                    
                    ImGui::SetCursorScreenPos(ImVec2(px1, py2 + ph + 16.0f));
                    ImGui::Dummy(ImVec2(10, 10));
                } else if (mainTab == 1) {
                    float ph = 316.0f; // max(316, 226)
                    
                    DrawPanelBg(px1, py2, lpw, ph, ICON_FA_CROSSHAIRS, "Main Group");
                    DrawPanelBg(px2, py2, rpw, ph, ICON_FA_SLIDERS, "Modifiers");
                    
                    ImGui::SetCursorScreenPos(ImVec2(px1, py2 + 45.0f));
                    ImGui::BeginChild("###LeftPanel", ImVec2(lpw, ph - 45.0f), false, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                    float lix = ImGui::GetCursorScreenPos().x + 16.0f;
                    float cy = ImGui::GetCursorScreenPos().y + 9.0f;
                    riw = lpw - 32.0f;
                    CelexCheck(lix,cy,"##en2","Names",   "Show player names",  &Options::ESP::Name);
                    CelexCheck(lix,cy,"##di", "Distance","Show distance",      &Options::ESP::Distance);
                    CelexCheck(lix,cy,"##hl", "Health",  "Show health bar",     &Options::ESP::Health);
                    CelexCheck(lix,cy,"##tr", "Tracers", "Draw tracers",        &Options::ESP::Tracers);
                    CelexCheck(lix,cy,"##tm", "Team Check","Ignore teammates", &Options::ESP::TeamCheck);
                    ImGui::EndChild();

                    ImGui::SetCursorScreenPos(ImVec2(px2, py2 + 45.0f));
                    ImGui::BeginChild("###RightPanel", ImVec2(rpw, ph - 45.0f), false, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                    float rix = ImGui::GetCursorScreenPos().x + 16.0f;
                    float ry = ImGui::GetCursorScreenPos().y + 9.0f;
                    riw = rpw - 32.0f;
                    static const char* bt[]={"None","Normal Box","3D Box"};
                    CelexCombo(rix, ry, "Box Type", "", &Options::ESP::BoxType, bt, 3);
                    CelexSlider(rix,ry,"Box Thickness",&Options::ESP::BoxThickness,1.0f,10.0f,"%.1f");
                    CelexSlider(rix,ry,"Tracer Thickness",&Options::ESP::TracerThickness,1.0f,10.0f,"%.1f");
                    ImGui::EndChild();
                    
                    ImGui::SetCursorScreenPos(ImVec2(px1, py2 + ph + 16.0f));
                    ImGui::Dummy(ImVec2(10, 10));
                } else if (mainTab == 2) {
                    float ph1 = 380.0f; // Row 1
                    float ph2 = 230.0f; // Row 2
                    
                    // Misc Row 1 (Main Settings)
                    DrawPanelBg(px1, py2, lpw, ph1, ICON_FA_CROSSHAIRS, "Main Group");
                    DrawPanelBg(px2, py2, rpw, ph1, ICON_FA_SLIDERS, "Modifiers");
                    
                    ImGui::SetCursorScreenPos(ImVec2(px1, py2 + 45.0f));
                    ImGui::BeginChild("###MiscLeft1", ImVec2(lpw, ph1 - 45.0f), false, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                    float lix = ImGui::GetCursorScreenPos().x + 16.0f;
                    float cy = ImGui::GetCursorScreenPos().y + 9.0f;
                    riw = lpw - 32.0f;
                    CelexCheck(lix,cy,"##sf","Show FOV",   "Render FOV circle",  &Options::Aimbot::ShowFOV);
                    CelexCheck(lix,cy,"##cr","Crosshair",  "Custom crosshair",   &Options::Crosshair::Enabled);
                    CelexCheck(lix,cy,"##sp","Stream Proof","Hide from capture", &Options::Misc::StreamProof);
                    CelexCheck(lix,cy,"##kl","Keybind List","Show active binds", &Options::Misc::KeybindList);
                    CelexCheck(lix,cy,"##cn","Cache NPCs",  "Cache NPC entities",&Options::Misc::CacheNPCs);
                    ImGui::EndChild();
                    
                    ImGui::SetCursorScreenPos(ImVec2(px2, py2 + 45.0f));
                    ImGui::BeginChild("###MiscRight1", ImVec2(rpw, ph1 - 45.0f), false, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                    float rix = ImGui::GetCursorScreenPos().x + 16.0f;
                    float ry = ImGui::GetCursorScreenPos().y + 9.0f;
                    riw = rpw - 32.0f;
                    CelexBind(rix, ry, "Menu Keybind", &Options::Misc::MenuKey);
                    CelexSlider(rix,ry,"Camera FOV",&Options::Misc::FOV,70.0f,120.0f,"%.0f");
                    CelexSlider(rix,ry,"Keybind List X",&Options::Misc::KeybindListX,0.0f,1920.0f,"%.0f");
                    CelexSlider(rix,ry,"Keybind List Y",&Options::Misc::KeybindListY,0.0f,1080.0f,"%.0f");
                    auto drawL = ImGui::GetWindowDrawList();
                    drawL->AddText(ImVec2(rix,ry),textDim,"Menu Accent Color"); ry+=24.0f;
                    ImGui::SetCursorScreenPos(ImVec2(rix,ry));
                    ImGui::SetNextItemWidth(riw);
                    if(ImGui::ColorEdit3("##ac",Options::Misc::MenuAccentColor,ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop))
                      main_color=ImVec4(Options::Misc::MenuAccentColor[0],Options::Misc::MenuAccentColor[1],Options::Misc::MenuAccentColor[2],1.0f);
                    ry += 40.0f;
                    ImGui::EndChild();

                    // Misc Row 2 (Desync)
                    float py2_r2 = py2 + ph1 + 16.0f;
                    DrawPanelBg(px1, py2_r2, lpw, ph2, ICON_FA_BOLT, "Desync");
                    DrawPanelBg(px2, py2_r2, rpw, ph2, ICON_FA_SLIDERS, "Modifiers");
                    
                    ImGui::SetCursorScreenPos(ImVec2(px1, py2_r2 + 45.0f));
                    ImGui::BeginChild("###MiscLeft2", ImVec2(lpw, ph2 - 45.0f), false, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                    lix = ImGui::GetCursorScreenPos().x + 16.0f;
                    cy = ImGui::GetCursorScreenPos().y + 9.0f;
                    riw = lpw - 32.0f;
                    CelexCheck(lix,cy,"##ds","Desync",     "Desynchronize player state", &Options::Desync::Enabled);
                    CelexCheck(lix,cy,"##dsv","Desync Vis.", "Show server pos", &Options::Desync::Visualizer);
                    auto drawL2 = ImGui::GetWindowDrawList();
                    drawL2->AddText(ImVec2(lix,cy),textDim,"Visualizer Color"); cy+=24.0f;
                    ImGui::SetCursorScreenPos(ImVec2(lix,cy));
                    ImGui::SetNextItemWidth(riw);
                    ImGui::ColorEdit3("##dvc",Options::Desync::VisualizerColor,ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoInputs);
                    cy += 40.0f;
                    ImGui::EndChild();
                    
                    ImGui::SetCursorScreenPos(ImVec2(px2, py2_r2 + 45.0f));
                    ImGui::BeginChild("###MiscRight2", ImVec2(rpw, ph2 - 45.0f), false, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                    rix = ImGui::GetCursorScreenPos().x + 16.0f;
                    ry = ImGui::GetCursorScreenPos().y + 9.0f;
                    riw = rpw - 32.0f;
                    CelexBind(rix, ry, "Desync Keybind", &Options::Desync::Key);
                    static const char* tm[] = {"Hold", "Toggle"};
                    CelexCombo(rix, ry, "Desync Mode", "", &Options::Desync::ToggleType, tm, 2);
                    ImGui::EndChild();
                    
                    ImGui::SetCursorScreenPos(ImVec2(px1, py2_r2 + ph2 + 16.0f));
                    ImGui::Dummy(ImVec2(10, 10)); // Ensure we can scroll down to the bottom
                }
                ImGui::EndChild(); // ###TabScroll
                ImGui::PopFont();
            }
            ImGui::PopStyleVar();
            ImGui::End();
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
