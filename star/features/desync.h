#pragma once
#include "../rbx/globals/globals.h"
#include "../rbx/globals/options.h"
#include "../rbx/globals/runtime.h"
#include <thread>
#include <chrono>
#include <mutex>
#include <stop_token>

// Desync implementation matching Foulz reference exactly:
// - Uses CachedPlayerObjects to get local player HRP position (more reliable)
// - Has keybind change cooldown (300ms debounce)
// - 30ms poll interval
// - Writes NextGenReplicatorEnabledWrite4 FFlag to disable/enable replication

inline std::mutex desync_mutex;

inline Vectors::Vector3 DesyncGetCoords()
{
    try {
        const auto state = Globals::Roblox::Snapshot();
        auto lp = state.LocalPlayer;
        if (lp.address == 0) return { 0.0f, 0.0f, 0.0f };
        
        auto character = lp.Character();
        if (character.address == 0) return { 0.0f, 0.0f, 0.0f };

        auto hrp = character.FindFirstChild("HumanoidRootPart");
        if (hrp.address != 0)
        {
            return hrp.Position();
        }
        return { 0.0f, 0.0f, 0.0f };
    }
    catch (...) { return { 0.0f, 0.0f, 0.0f }; }
}

inline void DesyncLoop(std::stop_token stopToken)
{
    bool held = false;
    int old_key_hash = 0;
    std::chrono::steady_clock::time_point chill_out{};

    while (!Globals::Runtime::ShouldStop(stopToken))
    {
        try
        {
            std::unique_lock<std::recursive_mutex> optionsLock(Options::Mutex);
            if (!Options::Desync::Enabled)
            {
                if (Options::Desync::Active)
                {
                    Options::Desync::Active = false;
                    {
                        std::lock_guard<std::mutex> lock(desync_mutex);
                        Options::Desync::SpawnPos = { 0.0f, 0.0f, 0.0f };
                    }
                    try {
                        Memory->write<bool>(Memory->getBaseAddress() + Options::Desync::NextGenReplicatorOffset + 0xC0, false);
                    } catch (...) {}
                    held = false;
                    Options::Desync::Toggled = false;
                }
                optionsLock.unlock();
                Globals::Runtime::Sleep(stopToken, std::chrono::milliseconds(200));
                continue;
            }

            // Keybind change cooldown (300ms debounce like Foulz reference)
            int cur_hash = Options::Desync::Key;
            if (cur_hash != old_key_hash)
            {
                old_key_hash = cur_hash;
                chill_out = std::chrono::steady_clock::now() + std::chrono::milliseconds(300);
            }

            // Handle keybind state
            bool key_pressed = false;
            if (Options::Desync::Key != 0)
            {
                key_pressed = (GetAsyncKeyState(Options::Desync::Key) & 0x8000) != 0;
            }

            // Apply cooldown
            if (std::chrono::steady_clock::now() < chill_out)
                key_pressed = false;

            if (Options::Desync::ToggleType == 1) // Toggle Mode
            {
                static bool wasKeyPressed = false;
                if (key_pressed && !wasKeyPressed)
                {
                    Options::Desync::Toggled = !Options::Desync::Toggled;
                }
                wasKeyPressed = key_pressed;
                key_pressed = Options::Desync::Toggled;
            }

            if (key_pressed && !held)
            {
                held = true;
                Options::Desync::Active = true;

                // Get coordinates from CachedPlayerObjects (like Foulz reference)
                auto pos = DesyncGetCoords();
                {
                    std::lock_guard<std::mutex> lock(desync_mutex);
                    Options::Desync::SpawnPos = pos;
                }

                // Write flag to true - disable replication
                try {
                    Memory->write<bool>(Memory->getBaseAddress() + Options::Desync::NextGenReplicatorOffset + 0xC0, true);
                } catch (...) {}
            }
            else if (!key_pressed && held)
            {
                held = false;
                Options::Desync::Active = false;
                {
                    std::lock_guard<std::mutex> lock(desync_mutex);
                    Options::Desync::SpawnPos = { 0.0f, 0.0f, 0.0f };
                }

                // Write flag to false - re-enable replication
                try {
                    Memory->write<bool>(Memory->getBaseAddress() + Options::Desync::NextGenReplicatorOffset + 0xC0, false);
                } catch (...) {}
            }

            optionsLock.unlock();
            Globals::Runtime::Sleep(stopToken, std::chrono::milliseconds(30));
        }
        catch (...) { Globals::Runtime::Sleep(stopToken, std::chrono::milliseconds(100)); }
    }

    std::lock_guard<std::recursive_mutex> optionsLock(Options::Mutex);
    if (Options::Desync::Active)
    {
        Memory->write<bool>(Memory->getBaseAddress() + Options::Desync::NextGenReplicatorOffset + 0xC0, false);
        Options::Desync::Active = false;
    }
    Options::Desync::Toggled = false;
    Options::Desync::SpawnPos = { 0.0f, 0.0f, 0.0f };
}
