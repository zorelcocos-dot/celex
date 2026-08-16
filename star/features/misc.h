#pragma once

#include "../rbx/globals/options.h"
#include "../rbx/globals/globals.h"

#include <thread>

inline void MiscLoop()
{
	while (true)
	{
		auto character = Globals::Roblox::LocalPlayer.Character();
		
		// Camera FOV
		if (Options::Misc::FOVEnabled)
		{
			Globals::Roblox::Camera.SetFOV(Options::Misc::FOV);
		}
		
		// Headless feature: toggle transparency, restore when disabled
		{
			auto head = character.FindFirstChild("Head");
			if (head.address != 0)
			{
				if (Options::ESP::Headless)
					Memory->write<float>(head.address + Offsets::BasePart::Transparency, 1.0f);
				else {
					// Restore to visible if previously hidden (check current transparency)
					float curTrans = Memory->read<float>(head.address + Offsets::BasePart::Transparency);
					if (curTrans >= 0.99f)
						Memory->write<float>(head.address + Offsets::BasePart::Transparency, 0.0f);
				}
			}
		}
		
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}
}