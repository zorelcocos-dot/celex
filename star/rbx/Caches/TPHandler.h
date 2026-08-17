#pragma once

#include "../globals/globals.h"
#include "../globals/runtime.h"

#include <chrono>
#include <stop_token>
#include <thread>

inline void TPHandler(std::stop_token stopToken)
{
    while (!Globals::Runtime::ShouldStop(stopToken))
    {
        const auto currentState = Globals::Roblox::Snapshot();
        const auto fakeDataModel = Memory->read<uintptr_t>(
            Memory->getBaseAddress() + Offsets::FakeDataModel::Pointer);

        RobloxInstance dataModel(0);
        if (fakeDataModel != 0)
        {
            dataModel = RobloxInstance(Memory->read<uintptr_t>(
                fakeDataModel + Offsets::FakeDataModel::RealDataModel));
        }

        int placeId = 0;
        if (dataModel.address != 0)
            placeId = Memory->read<int>(dataModel.address + Offsets::DataModel::PlaceId);

        bool shouldReinitialize = dataModel.address == 0;
        if (!shouldReinitialize)
        {
            const std::string dataModelName = dataModel.Name();
            shouldReinitialize = dataModelName == "LuaApp" ||
                (currentState.LastPlaceId != 0 && placeId != currentState.LastPlaceId);
        }

        if (shouldReinitialize)
        {
            dataModel = RobloxInstance(0);
            for (int retry = 0; retry < 60 && !Globals::Runtime::ShouldStop(stopToken); ++retry)
            {
                const auto candidateFakeDataModel = Memory->read<uintptr_t>(
                    Memory->getBaseAddress() + Offsets::FakeDataModel::Pointer);
                if (candidateFakeDataModel != 0)
                {
                    RobloxInstance candidate(Memory->read<uintptr_t>(
                        candidateFakeDataModel + Offsets::FakeDataModel::RealDataModel));
                    if (candidate.address != 0 && candidate.Name() == "Ugc")
                    {
                        dataModel = candidate;
                        break;
                    }
                }
                Globals::Runtime::Sleep(stopToken, std::chrono::milliseconds(1000));
            }

            if (Globals::Runtime::ShouldStop(stopToken))
                break;
            if (dataModel.address == 0)
            {
                Globals::Runtime::Sleep(stopToken, std::chrono::milliseconds(1000));
                continue;
            }

            uintptr_t visualEngine = 0;
            for (int retry = 0; retry < 60 && visualEngine == 0 &&
                 !Globals::Runtime::ShouldStop(stopToken); ++retry)
            {
                visualEngine = Memory->read<uintptr_t>(
                    Memory->getBaseAddress() + Offsets::VisualEngine::Pointer);
                if (visualEngine == 0)
                    Globals::Runtime::Sleep(stopToken, std::chrono::milliseconds(1000));
            }

            if (Globals::Runtime::ShouldStop(stopToken))
                break;

            Globals::Roblox::State nextState;
            nextState.DataModel = dataModel;
            nextState.VisualEngine = visualEngine;
            nextState.Workspace = dataModel.FindFirstChildWhichIsA("Workspace");
            nextState.Players = dataModel.FindFirstChildWhichIsA("Players");
            if (nextState.Workspace.address != 0)
                nextState.Camera = nextState.Workspace.FindFirstChildWhichIsA("Camera");
            if (nextState.Players.address != 0)
            {
                nextState.LocalPlayer = RobloxInstance(Memory->read<uintptr_t>(
                    nextState.Players.address + Offsets::Player::LocalPlayer));
            }
            nextState.LastPlaceId = Memory->read<int>(
                dataModel.address + Offsets::DataModel::PlaceId);

            // Publish a complete state in one operation so readers never observe
            // a mixture of addresses from two different game sessions.
            Globals::Roblox::Replace(nextState);

            {
                std::lock_guard<std::mutex> lock(Globals::Caches::PlayersMutex);
                Globals::Caches::CachedPlayers.clear();
            }
            {
                std::lock_guard<std::mutex> lock(Globals::Caches::PlayerObjectsMutex);
                Globals::Caches::CachedPlayerObjects.clear();
            }
        }

        Globals::Runtime::Sleep(stopToken, std::chrono::milliseconds(100));
    }
}
