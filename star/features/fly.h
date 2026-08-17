#pragma once

#include "../rbx/globals/globals.h"
#include "../rbx/globals/options.h"
#include "../rbx/globals/runtime.h"

#include <chrono>
#include <stop_token>

inline void FlyLoop(std::stop_token stopToken)
{
    bool wasKeyPressed = false;

    while (Globals::Runtime::Sleep(stopToken, std::chrono::milliseconds(10)))
    {
        int key = 0;
        int toggleType = 0;
        bool enabled = false;
        bool toggled = false;
        float speed = 50.0f;
        {
            std::lock_guard<std::recursive_mutex> lock(Options::Mutex);
            key = Options::Fly::FlyKey;
            toggleType = Options::Fly::ToggleType;
            enabled = Options::Fly::Enabled;
            toggled = Options::Fly::Toggled;
            speed = Options::Fly::Speed;
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
            Options::Fly::Toggled = toggled;
        }

        if (!enabled || !toggled)
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

            const auto humanoidRootPart = character.FindFirstChild("HumanoidRootPart");
            if (!humanoidRootPart.address || !state.Camera.address)
                continue;

            const auto cameraMatrix = state.Camera.CFrame();
            const Vectors::Vector3 forward(
                -cameraMatrix.r02, -cameraMatrix.r12, -cameraMatrix.r22);
            const Vectors::Vector3 up(0, 1, 0);

            const uintptr_t primitive = Memory->read<uintptr_t>(
                humanoidRootPart.address + Offsets::BasePart::Primitive);
            if (!primitive)
                continue;

            Vectors::Vector3 velocity(0, 0, 0);
            const Vectors::Vector3 right(-forward.z, 0.0f, forward.x);

            if (GetAsyncKeyState('W') & 0x8000)
                velocity = velocity - forward * speed;
            if (GetAsyncKeyState('S') & 0x8000)
                velocity = velocity + forward * speed;
            if (GetAsyncKeyState('A') & 0x8000)
                velocity = velocity - right * speed;
            if (GetAsyncKeyState('D') & 0x8000)
                velocity = velocity + right * speed;
            if (GetAsyncKeyState(VK_SPACE) & 0x8000)
                velocity = velocity + up * speed;
            if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
                velocity = velocity - up * speed;

            Memory->write<Vectors::Vector3>(
                primitive + Offsets::Primitive::AssemblyLinearVelocity, velocity);
        }
        catch (...)
        {
            // The state is refreshed by TPHandler when instances become stale.
        }
    }

    std::lock_guard<std::recursive_mutex> lock(Options::Mutex);
    Options::Fly::Toggled = false;
}
