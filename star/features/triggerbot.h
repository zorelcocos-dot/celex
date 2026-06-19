#pragma once
#include "../overlay/utils/W2S.h"
#include "../overlay/imgui/imgui.h"
#include "../rbx/globals/options.h"
#include "../rbx/globals/globals.h"
#include <windows.h>
#include <chrono>

inline void RenderAdvancedFOV(ImDrawList* drawList)
{
    if (!Options::Triggerbot::AdvancedFOV || !Options::Triggerbot::ShowAdvancedFOV)
        return;

    auto localTeam = Globals::Roblox::LocalPlayer.Team();
    
    for (auto& player : Globals::Caches::CachedPlayerObjects)
    {
        if (player.address == Globals::Roblox::LocalPlayer.address)
            continue;

        if (player.Health <= 0)
            continue;

        if (player.Team.address == localTeam.address && Options::Triggerbot::TeamCheck)
            continue;

        // Helper lambda to draw 3D box around a body part with FOV expansion
        auto drawPartFOV = [&](const RobloxInstance& part, float fovX, float fovY) {
            if (part.address == 0 || (fovX == 0.0f && fovY == 0.0f))
                return;

            Vectors::Vector3 partPos = part.Position();
            sCFrame partCFrame = part.CFrame();
            
            Vectors::Vector3 rightVec = partCFrame.GetRightVector();
            Vectors::Vector3 upVec = partCFrame.GetUpVector();
            Vectors::Vector3 lookVec = partCFrame.GetLookVector();

            // Convert screen-space FOV to world-space expansion
            float worldExpansionX = fovX * 0.02f; // Scale factor for conversion
            float worldExpansionY = fovY * 0.02f;
            
            // Base size for body parts
            float baseWidth = 1.0f + worldExpansionX;
            float baseHeight = 1.0f + worldExpansionY;
            float baseDepth = 1.0f + worldExpansionX;

            // Create 8 corners of the expanded bounding box
            std::array<Vectors::Vector3, 8> corners3D = {
                partPos + rightVec * baseWidth + upVec * baseHeight + lookVec * baseDepth,
                partPos - rightVec * baseWidth + upVec * baseHeight + lookVec * baseDepth,
                partPos + rightVec * baseWidth - upVec * baseHeight + lookVec * baseDepth,
                partPos - rightVec * baseWidth - upVec * baseHeight + lookVec * baseDepth,
                partPos + rightVec * baseWidth + upVec * baseHeight - lookVec * baseDepth,
                partPos - rightVec * baseWidth + upVec * baseHeight - lookVec * baseDepth,
                partPos + rightVec * baseWidth - upVec * baseHeight - lookVec * baseDepth,
                partPos - rightVec * baseWidth - upVec * baseHeight - lookVec * baseDepth
            };

            // Convert to 2D screen space
            std::array<ImVec2, 8> corners2D;
            bool valid = true;
            for (size_t i = 0; i < 8; ++i)
            {
                auto screenPos = WorldToScreen(corners3D[i]);
                if (screenPos.x != -1 && screenPos.y != -1)
                {
                    corners2D[i] = ImVec2(screenPos.x, screenPos.y);
                }
                else
                {
                    valid = false;
                    break;
                }
            }

            if (!valid)
                return false;

            // Find bounding rectangle of the 3D box projection
            float minX = FLT_MAX, minY = FLT_MAX;
            float maxX = -FLT_MAX, maxY = -FLT_MAX;
            
            for (const auto& corner : corners2D)
            {
                minX = (std::min)(minX, corner.x);
                maxX = (std::max)(maxX, corner.x);
                minY = (std::min)(minY, corner.y);
                maxY = (std::max)(maxY, corner.y);
            }

            // Check if cursor is inside the bounding rectangle
            return (cursorPos.x >= minX
                    cursorPos.y >= minY && cursorPos.y <= maxY);
        };

        // Check each body part with its specific FOV
        if (checkPartFOV(player.Head, Options::Triggerbot::HeadFOV_X, Options::Triggerbot::HeadFOV_Y))
            foundTarget = true;
        
        if (!foundTarget && player.RigType == 0) // R6
        {
            if (checkPartFOV(player.Torso, Options::Triggerbot::TorsoFOV_X, Options::Triggerbot::TorsoFOV_Y))
                foundTarget = true;
            else if (checkPartFOV(player.Left_Arm, Options::Triggerbot::LeftUpperArmFOV_X, Options::Triggerbot::LeftUpperArmFOV_Y))
                foundTarget = true;
            else if (checkPartFOV(player.Right_Arm, Options::Triggerbot::RightUpperArmFOV_X, Options::Triggerbot::RightUpperArmFOV_Y))
                foundTarget = true;
            else if (checkPartFOV(player.Left_Leg, Options::Triggerbot::LeftUpperLegFOV_X, Options::Triggerbot::LeftUpperLegFOV_Y))
                foundTarget = true;
            else if (checkPartFOV(player.Right_Leg, Options::Triggerbot::RightUpperLegFOV_X, Options::Triggerbot::RightUpperLegFOV_Y))
                foundTarget = true;
        }
        else if (!foundTarget) // R15
        {
            // If Torso FOV is set, use it for both Upper and Lower Torso, otherwise use individual settings
            float upperTorsoX = (Options::Triggerbot::TorsoFOV_X > 0.0f) ? Options::Triggerbot::TorsoFOV_X : Options::Triggerbot::UpperTorsoFOV_X;
            float upperTorsoY = (Options::Triggerbot::TorsoFOV_Y > 0.0f) ? Options::Triggerbot::TorsoFOV_Y : Options::Triggerbot::UpperTorsoFOV_Y;
            float lowerTorsoX = (Options::Triggerbot::TorsoFOV_X > 0.0f) ? Options::Triggerbot::TorsoFOV_X : Options::Triggerbot::LowerTorsoFOV_X;
            float lowerTorsoY = (Options::Triggerbot::TorsoFOV_Y > 0.0f) ? Options::Triggerbot::TorsoFOV_Y : Options::Triggerbot::LowerTorsoFOV_Y;
            
            if (checkPartFOV(player.Upper_Torso, upperTorsoX, upperTorsoY))
                foundTarget = true;
            else if (checkPartFOV(player.Lower_Torso, lowerTorsoX, lowerTorsoY))
                foundTarget = true;
            else if (checkPartFOV(player.Left_Upper_Arm, Options::Triggerbot::LeftUpperArmFOV_X, Options::Triggerbot::LeftUpperArmFOV_Y))
                foundTarget = true;
            else if (checkPartFOV(player.Left_Lower_Arm, Options::Triggerbot::LeftLowerArmFOV_X, Options::Triggerbot::LeftLowerArmFOV_Y))
                foundTarget = true;
            else if (checkPartFOV(player.Left_Hand, Options::Triggerbot::LeftHandFOV_X, Options::Triggerbot::LeftHandFOV_Y))
                foundTarget = true;
            else if (checkPartFOV(player.Right_Upper_Arm, Options::Triggerbot::RightUpperArmFOV_X, Options::Triggerbot::RightUpperArmFOV_Y))
                foundTarget = true;
            else if (checkPartFOV(player.Right_Lower_Arm, Options::Triggerbot::RightLowerArmFOV_X, Options::Triggerbot::RightLowerArmFOV_Y))
                foundTarget = true;
            else if (checkPartFOV(player.Right_Hand, Options::Triggerbot::RightHandFOV_X, Options::Triggerbot::RightHandFOV_Y))
                foundTarget = true;
            else if (checkPartFOV(player.Left_Upper_Leg, Options::Triggerbot::LeftUpperLegFOV_X, Options::Triggerbot::LeftUpperLegFOV_Y))
                foundTarget = true;
            else if (checkPartFOV(player.Left_Lower_Leg, Options::Triggerbot::LeftLowerLegFOV_X, Options::Triggerbot::LeftLowerLegFOV_Y))
                foundTarget = true;
            else if (checkPartFOV(player.Left_Foot, Options::Triggerbot::LeftFootFOV_X, Options::Triggerbot::LeftFootFOV_Y))
                foundTarget = true;
            else if (checkPartFOV(player.Right_Upper_Leg, Options::Triggerbot::RightUpperLegFOV_X, Options::Triggerbot::RightUpperLegFOV_Y))
                foundTarget = true;
            else if (checkPartFOV(player.Right_Lower_Leg, Options::Triggerbot::RightLowerLegFOV_X, Options::Triggerbot::RightLowerLegFOV_Y))
                foundTarget = true;
            else if (checkPartFOV(player.Right_Foot, Options::Triggerbot::RightFootFOV_X, Options::Triggerbot::RightFootFOV_Y))
                foundTarget = true;
        }

        if (foundTarget)
        {
            // Delay before shooting
            static auto lastFireTime = std::chrono::steady_clock::now();
            auto currentTime = std::chrono::steady_clock::now();
            auto timeSinceLastFire = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastFireTime).count();

            if (timeSinceLastFire >= Options::Triggerbot::Delay)
            {
                // Simulate mouse click
                INPUT input = { 0 };
                input.type = INPUT_MOUSE;
                input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
                SendInput(1, &input, sizeof(INPUT));

                Sleep(20);

                input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
                SendInput(1, &input, sizeof(INPUT));

                lastFireTime = currentTime;
            }
            
            return; // Only shoot at one target at a time
        }
    }
}
