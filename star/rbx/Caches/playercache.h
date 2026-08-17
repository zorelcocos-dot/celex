#pragma once
#include "../globals/options.h"
#include "../globals/globals.h"
#include "../globals/runtime.h"
#include <stop_token>
#include <thread>
#include <vector>


inline void CachePlayers(std::stop_token stopToken)
{
	std::vector<RobloxInstance> tempList;

	while (!Globals::Runtime::ShouldStop(stopToken))
	{
		try {
		tempList.clear();
		tempList.reserve(64);

		const auto state = Globals::Roblox::Snapshot();
		bool cacheNpcs = false;
		{
			std::lock_guard<std::recursive_mutex> lock(Options::Mutex);
			cacheNpcs = Options::Misc::CacheNPCs;
		}

		// Get local player's character to exclude it from NPC detection
		RobloxInstance localCharacter(0);
		if (state.LocalPlayer.address != 0)
			localCharacter = state.LocalPlayer.Character();

		// Cache real players from Players service
		if (state.Players.address != 0) {
			auto children = state.Players.GetChildren();
			for (auto& player : children)
			{
				if (!player.address) continue;
				tempList.push_back(player);
			}
		}

		// Cache NPCs from Workspace - look for any Model with a Humanoid
		auto workspace = state.Workspace;
		if (workspace.address != 0 && cacheNpcs)
		{
			auto workspaceChildren = workspace.GetChildren();
			
			for (auto& child : workspaceChildren)
			{
				std::string childClass = child.Class();
				
				// If it's a Folder, check inside it
				if (childClass == "Folder")
				{
					auto folderChildren = child.GetChildren();
					for (auto& folderChild : folderChildren)
					{
						std::string folderChildClass = folderChild.Class();
						
						if (folderChildClass == "Model")
						{
							// Skip if this is the local player's character
							if (folderChild.address == localCharacter.address)
								continue;
								
							auto humanoid = folderChild.FindFirstChildWhichIsA("Humanoid");
							if (humanoid.address != 0)
							{
								auto hrp = folderChild.FindFirstChild("HumanoidRootPart");
								if (hrp.address != 0)
								{
									tempList.push_back(folderChild);
								}
							}
						}
					}
				}
				
				// Check if it's a Model directly in workspace
				if (childClass == "Model")
				{
					// Skip if this is the local player's character
					if (child.address == localCharacter.address)
						continue;
						
					auto humanoid = child.FindFirstChildWhichIsA("Humanoid");
					if (humanoid.address != 0)
					{
						auto hrp = child.FindFirstChild("HumanoidRootPart");
						if (hrp.address != 0)
						{
							tempList.push_back(child);
						}
					}
				}
			}
		}

		{
			std::lock_guard<std::mutex> lock(Globals::Caches::PlayersMutex);
			Globals::Caches::CachedPlayers = tempList;
		}
			} catch (...) {}
			Globals::Runtime::Sleep(stopToken, std::chrono::milliseconds(5000));
		}
	}

