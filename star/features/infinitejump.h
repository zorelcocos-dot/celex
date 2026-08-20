#pragma once

#include "../rbx/globals/globals.h"
#include "../rbx/globals/options.h"
#include "../rbx/globals/runtime.h"

#include <chrono>
#include <stop_token>

// Repeatedly triggers the local character's jump while active. Uses the
// Humanoid "Jump" flag instead of raw velocity so it works with the game's
// own jump/fall physics and respects JumpPower/JumpHeight.
inline void InfiniteJumpLoop(std::stop_token stopToken)
{
    bool wasKeyPressed = false;
    auto lastJump = std::chrono::steady_clock::now();

    while (Globals::Runtime::Sleep(stopToken, std::chrono::milliseconds(10)))
    {
        int key = 0;
        int toggleType = 0;
        bool enabled = false;
        bool toggled = false;
        float jumpPower = 50.0f;
        float cooldown = 100.0f;
        {
            std::lock_guard<std::recursive_mutex> lock(Options::Mutex);
            key = Options::InfiniteJump::InfiniteJumpKey;
            toggleType = Options::InfiniteJump::ToggleType;
            enabled = Options::InfiniteJump::Enabled;
            toggled = Options::InfiniteJump::Toggled;
            jumpPower = Options::InfiniteJump::JumpPower;
            cooldown = Options::InfiniteJump::JumpCooldown;
        }

        if (key != 0)
        {
            const bool isKeyPressed = (GetAsyncKeyState(key) & 0x8000) != 0;
            if (toggleType == 1)
            {
                if (isKeyPressed && !wasKeyPressed)
                    toggled = !toggled;
                wasKeyPressed = isKeyPressed;
            }
            else
            {
                toggled = isKeyPressed;
            }

            std::lock_guard<std::recursive_mutex> lock(Options::Mutex);
            Options::InfiniteJump::Toggled = toggled;
        }

        if (!enabled || !toggled)
        {
            lastJump = std::chrono::steady_clock::now();
            continue;
        }

        const auto now = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastJump).count();
        if (elapsed < static_cast<long long>(cooldown))
            continue;

        try
        {
            const auto state = Globals::Roblox::Snapshot();
            const auto localPlayer = state.LocalPlayer;
            if (!localPlayer.address)
                continue;

            const auto character = localPlayer.Character();
            if (!character.address)
                continue;

            auto humanoid = character.FindFirstChildWhichIsA("Humanoid");
            if (!humanoid.address)
                continue;

            // Set a custom jump power when configured above zero.
            if (jumpPower > 0.0f)
                humanoid.SetJumpPower(jumpPower);

            // Toggle the jump flag off then on to retrigger a fresh jump.
            Memory->write<bool>(humanoid.address + Offsets::Humanoid::Jump, false);
            Memory->write<bool>(humanoid.address + Offsets::Humanoid::Jump, true);

            lastJump = now;
        }
        catch (...)
        {
            // The state is refreshed by TPHandler when instances become stale.
        }
    }

    std::lock_guard<std::recursive_mutex> lock(Options::Mutex);
    Options::InfiniteJump::Toggled = false;
}
