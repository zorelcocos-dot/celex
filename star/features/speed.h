#pragma once

#include "../rbx/globals/globals.h"
#include "../rbx/globals/options.h"
#include "../rbx/globals/runtime.h"

#include <chrono>
#include <stop_token>

inline void SpeedLoop(std::stop_token stopToken)
{
    bool wasKeyPressed = false;

    while (Globals::Runtime::Sleep(stopToken, std::chrono::milliseconds(10)))
    {
        int key = 0;
        int toggleType = 0;
        bool enabled = false;
        bool toggled = false;
        float speed = 16.0f;
        {
            std::lock_guard<std::recursive_mutex> lock(Options::Mutex);
            key = Options::WalkSpeed::WalkSpeedKey;
            toggleType = Options::WalkSpeed::ToggleType;
            enabled = Options::WalkSpeed::Enabled;
            toggled = Options::WalkSpeed::Toggled;
            speed = Options::WalkSpeed::Speed;
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
            Options::WalkSpeed::Toggled = toggled;
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

            const auto humanoid = character.FindFirstChildWhichIsA("Humanoid");
            const auto humanoidRootPart = character.FindFirstChild("HumanoidRootPart");
            if (!humanoid.address || !humanoidRootPart.address)
                continue;

            const uintptr_t primitive = Memory->read<uintptr_t>(
                humanoidRootPart.address + Offsets::BasePart::Primitive);
            if (!primitive)
                continue;

            const Vectors::Vector3 moveDirection = Memory->read<Vectors::Vector3>(
                humanoid.address + Offsets::Humanoid::MoveDirection);
            const Vectors::Vector3 currentVelocity = Memory->read<Vectors::Vector3>(
                primitive + Offsets::Primitive::AssemblyLinearVelocity);
            const Vectors::Vector3 newVelocity(
                moveDirection.x * speed,
                currentVelocity.y,
                moveDirection.z * speed);

            Memory->write<Vectors::Vector3>(
                primitive + Offsets::Primitive::AssemblyLinearVelocity, newVelocity);
        }
        catch (...)
        {
            // The state is refreshed by TPHandler when instances become stale.
        }
    }

    std::lock_guard<std::recursive_mutex> lock(Options::Mutex);
    Options::WalkSpeed::Toggled = false;
}
