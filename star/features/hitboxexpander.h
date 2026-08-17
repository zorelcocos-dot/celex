#pragma once

#include "../rbx/globals/options.h"
#include "../rbx/globals/globals.h"
#include "../rbx/globals/runtime.h"

#include <chrono>
#include <stop_token>

inline void RunHitboxExpander(std::stop_token stopToken)
{
    while (!Globals::Runtime::ShouldStop(stopToken))
    {
        bool enabled = false;
        float horizontalSize = 10.0f;
        float verticalSize = 10.0f;
        bool showHitbox = false;
        float transparency = 0.5f;
        bool walkThrough = false;
        {
            std::lock_guard<std::recursive_mutex> lock(Options::Mutex);
            enabled = Options::HitboxExpander::Enabled;
            horizontalSize = Options::HitboxExpander::HorizontalSize;
            verticalSize = Options::HitboxExpander::VerticalSize;
            showHitbox = Options::HitboxExpander::ShowHitbox;
            transparency = Options::HitboxExpander::HitboxTransparency;
            walkThrough = Options::HitboxExpander::WalkThrough;
        }

        if (!enabled)
        {
            Globals::Runtime::Sleep(stopToken, std::chrono::milliseconds(100));
            continue;
        }

        const auto state = Globals::Roblox::Snapshot();
        if (!state.Players.address)
        {
            Globals::Runtime::Sleep(stopToken, std::chrono::milliseconds(100));
            continue;
        }

        try
        {
            const auto players = state.Players.GetChildren();
            for (const auto& player : players)
            {
                if (Globals::Runtime::ShouldStop(stopToken))
                    break;
                if (!player.address || player.address == state.LocalPlayer.address)
                    continue;

                const auto character = player.Character();
                if (!character.address)
                    continue;

                const auto humanoidRootPart = character.FindFirstChild("HumanoidRootPart");
                if (!humanoidRootPart.address)
                    continue;

                const uintptr_t primitive = Memory->read<uintptr_t>(
                    humanoidRootPart.address + Offsets::BasePart::Primitive);
                if (!primitive)
                    continue;

                const Vectors::Vector3 newSize = {
                    horizontalSize,
                    verticalSize,
                    horizontalSize
                };
                Memory->write<Vectors::Vector3>(primitive + Offsets::Primitive::Size, newSize);

                const uintptr_t canCollideAddress = primitive + Offsets::Primitive::Flags;
                const uint8_t currentFlags = Memory->read<uint8_t>(canCollideAddress);
                constexpr uint8_t canCollideBit = 0x8;
                const uint8_t newFlags = walkThrough
                    ? static_cast<uint8_t>(currentFlags & ~canCollideBit)
                    : static_cast<uint8_t>(currentFlags | canCollideBit);
                if (newFlags != currentFlags)
                    Memory->write<uint8_t>(canCollideAddress, newFlags);

                Memory->write<float>(
                    humanoidRootPart.address + Offsets::BasePart::Transparency,
                    showHitbox ? transparency : 1.0f);
            }
        }
        catch (...)
        {
            // A stale instance can disappear between cache updates. The next
            // iteration retries against a fresh state snapshot.
        }

        Globals::Runtime::Sleep(stopToken, std::chrono::milliseconds(50));
    }
}
