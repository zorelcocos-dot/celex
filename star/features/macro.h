#pragma once
#include "../rbx/globals/options.h"
#include "../overlay/imgui/KeyBind.h"
#include <windows.h>
#include <chrono>

inline void RunMacro()
{
    if (!Options::Macro::Enabled)
        return;

    // Check keybind
    static bool wasKeyPressed = false;
    bool isKeyPressed = KeyBind::IsPressed(Options::Macro::MacroKey);

    if (Options::Macro::ToggleType == 1)
    {
        // Toggle mode
        if (isKeyPressed && !wasKeyPressed)
        {
            Options::Macro::Toggled = !Options::Macro::Toggled;
        }
        wasKeyPressed = isKeyPressed;

        if (!Options::Macro::Toggled)
            return;
    }
    else
    {
        // Hold mode
        if (!isKeyPressed)
        {
            Options::Macro::Toggled = false;
            return;
        }
    }

    // Non-blocking press/release state machine. RunMacro is called once per
    // frame on the render thread, so a blocking Sleep() here would stall the
    // entire overlay. Instead we track when to release the pending key.
    enum class Phase { Idle, Holding };
    static Phase phase = Phase::Idle;
    static bool pressI = true; // Alternate between I and O keys
    static auto lastPressTime = std::chrono::steady_clock::now();
    static auto releaseTime = std::chrono::steady_clock::now();
    static WORD activeKey = 0;

    const auto currentTime = std::chrono::steady_clock::now();

    if (phase == Phase::Holding)
    {
        if (currentTime >= releaseTime)
        {
            INPUT up = { 0 };
            up.type = INPUT_KEYBOARD;
            up.ki.wVk = activeKey;
            up.ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(1, &up, sizeof(INPUT));
            activeKey = 0;
            phase = Phase::Idle;
        }
        return;
    }

    const auto timeSinceLastPress =
        std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastPressTime).count();

    if (timeSinceLastPress >= Options::Macro::Delay)
    {
        activeKey = pressI ? 0x49 : 0x4F; // 'I' or 'O'

        INPUT down = { 0 };
        down.type = INPUT_KEYBOARD;
        down.ki.wVk = activeKey;
        down.ki.dwFlags = 0; // Key down
        SendInput(1, &down, sizeof(INPUT));

        releaseTime = currentTime + std::chrono::milliseconds(20);
        phase = Phase::Holding;
        pressI = !pressI;
        lastPressTime = currentTime;
    }
}
