#include <windows.h>

#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stop_token>
#include <string>
#include <thread>
#include <vector>

#include "Memory/MemoryManager.h"
#include "features/desync.h"
#include "features/fly.h"
#include "features/hitboxexpander.h"
#include "features/infinitejump.h"
#include "features/misc.h"
#include "features/speed.h"
#include "overlay/renderer.h"
#include "rbx/Caches/TPHandler.h"
#include "rbx/Caches/playercache.h"
#include "rbx/Caches/playerobjectscache.h"
#include "rbx/globals/globals.h"
#include "rbx/globals/runtime.h"

namespace
{
    constexpr auto ProcessName = "RobloxPlayerBeta.exe";
    constexpr auto WindowTitle = L"Roblox";

    bool IsGameRunning()
    {
        return FindWindowW(nullptr, WindowTitle) != nullptr;
    }

    std::string GetExecutableDirectory()
    {
        char path[MAX_PATH]{};
        const DWORD length = GetModuleFileNameA(nullptr, path, MAX_PATH);
        if (length == 0 || length == MAX_PATH)
            return std::filesystem::current_path().string();
        return std::filesystem::path(path).parent_path().string();
    }

    void Log(const std::string& message, int type = 0)
    {
        const char* prefix = "[*]";
        if (type == 1)
            prefix = "[+]";
        else if (type == 2)
            prefix = "[-]";
        std::cout << prefix << ' ' << message << std::endl;
    }

    template <typename T>
    std::string ToHexString(T value)
    {
        std::ostringstream stream;
        stream << std::uppercase << std::hex << value;
        return stream.str();
    }

    BOOL WINAPI ConsoleHandler(DWORD signal)
    {
        if (signal == CTRL_C_EVENT || signal == CTRL_BREAK_EVENT ||
            signal == CTRL_CLOSE_EVENT || signal == CTRL_LOGOFF_EVENT ||
            signal == CTRL_SHUTDOWN_EVENT)
        {
            Globals::Runtime::StopRequested.store(true, std::memory_order_relaxed);
            return TRUE;
        }
        return FALSE;
    }

    bool InitializeRobloxState()
    {
        Log("Waiting for the game data model...");

        RobloxInstance dataModel(0);
        const auto dataModelDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(60);
        while (!Globals::Runtime::StopRequested.load(std::memory_order_relaxed) &&
               Memory->isProcessAlive() && std::chrono::steady_clock::now() < dataModelDeadline)
        {
            const auto fakeDataModel = Memory->read<uintptr_t>(
                Memory->getBaseAddress() + Offsets::FakeDataModel::Pointer);
            if (fakeDataModel != 0)
            {
                RobloxInstance candidate(Memory->read<uintptr_t>(
                    fakeDataModel + Offsets::FakeDataModel::RealDataModel));
                if (candidate.address != 0 && candidate.Name() == "Ugc")
                {
                    dataModel = candidate;
                    break;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }

        if (!dataModel.address || !Memory->isProcessAlive())
            return false;

        uintptr_t visualEngine = 0;
        const auto visualEngineDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
        while (!Globals::Runtime::StopRequested.load(std::memory_order_relaxed) &&
               Memory->isProcessAlive() && visualEngine == 0 &&
               std::chrono::steady_clock::now() < visualEngineDeadline)
        {
            visualEngine = Memory->read<uintptr_t>(
                Memory->getBaseAddress() + Offsets::VisualEngine::Pointer);
            if (visualEngine == 0)
                std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }
        if (visualEngine == 0)
            return false;

        Globals::Roblox::State state;
        state.DataModel = dataModel;
        state.VisualEngine = visualEngine;
        state.Workspace = dataModel.FindFirstChildWhichIsA("Workspace");
        state.Players = dataModel.FindFirstChildWhichIsA("Players");
        if (state.Workspace.address != 0)
            state.Camera = state.Workspace.FindFirstChildWhichIsA("Camera");
        if (state.Players.address != 0)
        {
            state.LocalPlayer = RobloxInstance(Memory->read<uintptr_t>(
                state.Players.address + Offsets::Player::LocalPlayer));
        }
        state.LastPlaceId = Memory->read<int>(
            dataModel.address + Offsets::DataModel::PlaceId);

        if (!state.Workspace.address || !state.Players.address ||
            !state.Camera.address || !state.LocalPlayer.address)
        {
            Log("Game state is incomplete; retrying...", 2);
            return false;
        }

        Globals::Roblox::Replace(state);

        Log("DataModel -> 0x" + ToHexString(state.DataModel.address), 1);
        Log("VisualEngine -> 0x" + ToHexString(state.VisualEngine), 1);
        Log("Workspace -> 0x" + ToHexString(state.Workspace.address), 1);
        Log("Players -> 0x" + ToHexString(state.Players.address), 1);
        Log("Camera -> 0x" + ToHexString(state.Camera.address), 1);
        Log("Logged in as " + state.LocalPlayer.Name(), 1);
        return true;
    }

    void ClearSessionState()
    {
        Globals::Runtime::GameConnected.store(false, std::memory_order_relaxed);
        Globals::Roblox::Clear();
        {
            std::lock_guard<std::mutex> lock(Globals::Caches::PlayersMutex);
            Globals::Caches::CachedPlayers.clear();
        }
        {
            std::lock_guard<std::mutex> lock(Globals::Caches::PlayerObjectsMutex);
            Globals::Caches::CachedPlayerObjects.clear();
        }
        Memory->detach();
    }
}

int main()
{
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);
    Globals::Runtime::StopRequested.store(false, std::memory_order_relaxed);
    Globals::executablePath = GetExecutableDirectory();
    Globals::configsPath = Globals::executablePath + "\\configs";

    std::error_code directoryError;
    std::filesystem::create_directories(Globals::configsPath, directoryError);
    if (directoryError)
        Log("Could not create the configs directory", 2);

    std::stop_source applicationStop;
    const std::stop_token applicationToken = applicationStop.get_token();
    std::jthread overlayThread([applicationToken]
    {
        ShowImgui(applicationToken);
    });

    while (!Globals::Runtime::StopRequested.load(std::memory_order_relaxed))
    {
        if (!IsGameRunning())
        {
            Globals::Runtime::GameConnected.store(false, std::memory_order_relaxed);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }

        Log("Roblox found!", 1);
        Log("Attaching to Roblox...");
        if (!Memory->attachToProcess(ProcessName))
        {
            Log("Attach failed; retrying...", 2);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        Log("Attached to PID " + std::to_string(Memory->getProcessId()) +
            " at 0x" + ToHexString(Memory->getBaseAddress()), 1);

        if (!InitializeRobloxState())
        {
            ClearSessionState();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }

        Globals::Runtime::GameConnected.store(true, std::memory_order_relaxed);
        Log("Starting workers...", 1);

        std::stop_source sessionStop;
        const std::stop_token sessionToken = sessionStop.get_token();
        std::vector<std::jthread> workers;
        workers.reserve(9);
        workers.emplace_back([sessionToken] { CachePlayers(sessionToken); });
        workers.emplace_back([sessionToken] { CachePlayerObjects(sessionToken); });
        workers.emplace_back([sessionToken] { TPHandler(sessionToken); });
        workers.emplace_back([sessionToken] { MiscLoop(sessionToken); });
        workers.emplace_back([sessionToken] { RunHitboxExpander(sessionToken); });
        workers.emplace_back([sessionToken] { FlyLoop(sessionToken); });
        workers.emplace_back([sessionToken] { SpeedLoop(sessionToken); });
        workers.emplace_back([sessionToken] { InfiniteJumpLoop(sessionToken); });
        workers.emplace_back([sessionToken] { DesyncLoop(sessionToken); });

        while (!Globals::Runtime::StopRequested.load(std::memory_order_relaxed) &&
               Memory->isProcessAlive())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }

        sessionStop.request_stop();
        workers.clear(); // std::jthread joins every worker here.
        ClearSessionState();

        if (!Globals::Runtime::StopRequested.load(std::memory_order_relaxed))
            Log("Roblox closed; waiting to reconnect...");
    }

    applicationStop.request_stop();
    overlayThread.join();
    ClearSessionState();
    SetConsoleCtrlHandler(ConsoleHandler, FALSE);
    Log("Shutdown complete.", 1);
    return 0;
}
