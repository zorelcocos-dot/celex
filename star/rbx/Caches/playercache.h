#pragma once
#include "../globals/options.h"
#include "../globals/globals.h"
#include <thread>
#include <vector>


inline void CachePlayers()
{
	std::vector<RobloxInstance> tempList;

	while (true)
	{
		try {
		tempList.clear();
		tempList.reserve(64);

		// Get local player's character to exclude it from NPC detection
		RobloxInstance localCharacter(0);
		if (Globals::Roblox::LocalPlayer.address != 0)
			localCharacter = Globals::Roblox::LocalPlayer.Character();

		// Cache real players from Players service
		if (Globals::Roblox::Players.address != 0) {
			auto children = Globals::Roblox::Players.GetChildren();
			for (auto& player : children)
			{
				if (!player.address) continue;
				tempList.push_back(player);
			}
		}

		// Cache NPCs from Workspace - look for any Model with a Humanoid
		auto workspace = Globals::Roblox::Workspace;
		if (workspace.address != 0 && Options::Misc::CacheNPCs)
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
		std::this_thread::sleep_for(std::chrono::milliseconds(5000));
	}
}

