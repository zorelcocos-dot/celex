#pragma once
#include <windows.h>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <vector>
#include <string>

#include "../../Memory/MemoryManager.h"
#include "../SDK/SDK.h"

struct RobloxPlayer
{
    uintptr_t address = 0;
    int RigType = 0;
    std::string Name = "";
    float Health = 0.0f;
    float MaxHealth = 0.0f;
    RobloxInstance Team = RobloxInstance(0);
    RobloxInstance Character = RobloxInstance(0);
    RobloxInstance Humanoid = RobloxInstance(0);
    RobloxInstance Head = RobloxInstance(0);
    RobloxInstance HumanoidRootPart = RobloxInstance(0);
    RobloxInstance Left_Arm = RobloxInstance(0);
    RobloxInstance Left_Leg = RobloxInstance(0);
    RobloxInstance Right_Arm = RobloxInstance(0);
    RobloxInstance Right_Leg = RobloxInstance(0);
    RobloxInstance Torso = RobloxInstance(0);
    RobloxInstance Upper_Torso = RobloxInstance(0);
    RobloxInstance Lower_Torso = RobloxInstance(0);
    RobloxInstance Right_Upper_Arm = RobloxInstance(0);
    RobloxInstance Right_Lower_Arm = RobloxInstance(0);
    RobloxInstance Right_Hand = RobloxInstance(0);
    RobloxInstance Left_Upper_Arm = RobloxInstance(0);
    RobloxInstance Left_Lower_Arm = RobloxInstance(0);
    RobloxInstance Left_Hand = RobloxInstance(0);
    RobloxInstance Right_Upper_Leg = RobloxInstance(0);
    RobloxInstance Right_Lower_Leg = RobloxInstance(0);
    RobloxInstance Right_Foot = RobloxInstance(0);
    RobloxInstance Left_Upper_Leg = RobloxInstance(0);
    RobloxInstance Left_Lower_Leg = RobloxInstance(0);
    RobloxInstance Left_Foot = RobloxInstance(0);
};

namespace Globals
{
    namespace Roblox
    {
        struct State
        {
            RobloxInstance DataModel{0};
            uintptr_t VisualEngine = 0;
            RobloxInstance Workspace{0};
            RobloxInstance Players{0};
            RobloxInstance Camera{0};
            RobloxInstance LocalPlayer{0};
            int LastPlaceId = 0;
        };

        inline std::shared_mutex StateMutex;
        inline State CurrentState;

        inline State Snapshot()
        {
            std::shared_lock lock(StateMutex);
            return CurrentState;
        }

        inline void Replace(State state)
        {
            std::unique_lock lock(StateMutex);
            CurrentState = state;
        }

        inline void Clear()
        {
            Replace(State{});
        }
    }
    namespace Runtime
    {
        inline std::atomic_bool StopRequested = false;
        inline std::atomic_bool OverlayRunning = false;
        inline std::atomic_bool GameConnected = false;
    }
    namespace Caches
    {
        inline std::mutex PlayersMutex;
        inline std::mutex PlayerObjectsMutex;
        inline std::vector<RobloxInstance> CachedPlayers;
        inline std::vector<RobloxPlayer> CachedPlayerObjects;
    }
    inline std::string executablePath;
    inline std::string configsPath;
}