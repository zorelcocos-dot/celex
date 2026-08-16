#pragma once
#include "../globals/options.h"
#include "../globals/globals.h"
#include <thread>
#include <vector>


inline void TPHandler()
{
	while (true)
	{
		auto fakeDataModel = Memory->read<uintptr_t>(Memory->getBaseAddress() + Offsets::FakeDataModel::Pointer);
		auto dataModel = RobloxInstance(0);
		if (fakeDataModel != 0)
			dataModel = RobloxInstance(Memory->read<uintptr_t>(fakeDataModel + Offsets::FakeDataModel::RealDataModel));
		int placeId = 0;
		if (dataModel.address != 0)
			placeId = Memory->read<int>(dataModel.address + Offsets::DataModel::PlaceId);
		uintptr_t visualEngine = 0;

		bool shouldReinit = false;
		if (!dataModel || dataModel.address == 0)
			shouldReinit = true;
		else {
			std::string dmName = dataModel.Name();
			if (dmName == "LuaApp" || placeId != Globals::Roblox::lastPlaceID)
				shouldReinit = true;
		}

		if (shouldReinit) // player left the game / teleported
		{
			// Wait for new DataModel to be Ugc
			int retries = 0;
			while (retries < 60) {
				fakeDataModel = Memory->read<uintptr_t>(Memory->getBaseAddress() + Offsets::FakeDataModel::Pointer);
				if (fakeDataModel != 0) {
					dataModel = RobloxInstance(Memory->read<uintptr_t>(fakeDataModel + Offsets::FakeDataModel::RealDataModel));
					if (dataModel.address != 0 && dataModel.Name() == "Ugc")
						break;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				retries++;
			}
			if (dataModel.address == 0 || dataModel.Name() != "Ugc") {
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				continue;
			}

			Globals::Roblox::DataModel = dataModel;

			visualEngine = Memory->read<uintptr_t>(Memory->getBaseAddress() + Offsets::VisualEngine::Pointer);
			int veRetries = 0;
			while (visualEngine == 0 && veRetries < 60)
			{
				visualEngine = Memory->read<uintptr_t>(Memory->getBaseAddress() + Offsets::VisualEngine::Pointer);
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				veRetries++;
			}
			Globals::Roblox::VisualEngine = visualEngine;

			Globals::Roblox::Workspace = Globals::Roblox::DataModel.FindFirstChildWhichIsA("Workspace");
			Globals::Roblox::Players = Globals::Roblox::DataModel.FindFirstChildWhichIsA("Players");
			if (Globals::Roblox::Workspace.address != 0)
				Globals::Roblox::Camera = Globals::Roblox::Workspace.FindFirstChildWhichIsA("Camera");
			else
				Globals::Roblox::Camera = RobloxInstance(0);

			if (Globals::Roblox::Players.address != 0)
				Globals::Roblox::LocalPlayer = RobloxInstance(Memory->read<uintptr_t>(Globals::Roblox::Players.address + Offsets::Player::LocalPlayer));
			else
				Globals::Roblox::LocalPlayer = RobloxInstance(0);

			// FIX: Read NEW placeId after reinit, not old one
			int newPlaceId = 0;
			if (Globals::Roblox::DataModel.address != 0)
				newPlaceId = Memory->read<int>(Globals::Roblox::DataModel.address + Offsets::DataModel::PlaceId);
			Globals::Roblox::lastPlaceID = newPlaceId;

			{
				std::lock_guard<std::mutex> lock1(Globals::Caches::PlayersMutex);
				Globals::Caches::CachedPlayers.clear();
			}
			{
				std::lock_guard<std::mutex> lock2(Globals::Caches::PlayerObjectsMutex);
				Globals::Caches::CachedPlayerObjects.clear();
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

