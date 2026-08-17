#pragma once

#include "../rbx/globals/options.h"
#include "../rbx/globals/globals.h"
#include "../rbx/globals/runtime.h"

#include <chrono>
#include <stop_token>

inline void MiscLoop(std::stop_token stopToken)
{
    while (!Globals::Runtime::ShouldStop(stopToken))
    {
        bool fovEnabled = false;
        float fov = 70.0f;
        bool headless = false;
        {
            std::lock_guard<std::recursive_mutex> lock(Options::Mutex);
            fovEnabled = Options::Misc::FOVEnabled;
            fov = Options::Misc::FOV;
            headless = Options::ESP::Headless;
        }

        const auto state = Globals::Roblox::Snapshot();
        const auto character = state.LocalPlayer.Character();

        if (fovEnabled && state.Camera.address != 0)
            state.Camera.SetFOV(fov);

        const auto head = character.FindFirstChild("Head");
        if (head.address != 0)
        {
            if (headless)
            {
                Memory->write<float>(head.address + Offsets::BasePart::Transparency, 1.0f);
            }
            else
            {
                const float currentTransparency = Memory->read<float>(
                    head.address + Offsets::BasePart::Transparency);
                if (currentTransparency >= 0.99f)
                    Memory->write<float>(head.address + Offsets::BasePart::Transparency, 0.0f);
            }
        }

        Globals::Runtime::Sleep(stopToken, std::chrono::milliseconds(500));
    }
}
