#pragma once

#include "../configs/json.hpp"
#include "../globals/options.h"
#include "../globals/globals.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <mutex>
#include <string>
#include <system_error>
#include <vector>

namespace Config
{
    using json = nlohmann::json;

    inline constexpr int CurrentSchemaVersion = 2;

    struct Result
    {
        bool success = false;
        std::string message;
        std::string fileName;
    };

    template <std::size_t N>
    inline json FloatArray(const float (&values)[N])
    {
        json result = json::array();
        for (const float value : values)
            result.push_back(value);
        return result;
    }

    inline json Capture()
    {
        std::lock_guard<std::recursive_mutex> lock(Options::Mutex);

        json j;
        j["schemaVersion"] = CurrentSchemaVersion;

        j["ESP"] = {
            {"Team Check", Options::ESP::TeamCheck},
            {"Box Type", Options::ESP::BoxType},
            {"Tracers", Options::ESP::Tracers},
            {"TracersStart", Options::ESP::TracersStart},
            {"Skeleton", Options::ESP::Skeleton},
            {"Name", Options::ESP::Name},
            {"Distance", Options::ESP::Distance},
            {"Health", Options::ESP::Health},
            {"Head Circles", Options::ESP::HeadCircle},
            {"Corner ESP", Options::ESP::CornerESP},
            {"Headless", Options::ESP::Headless},
            {"Remove Borders", Options::ESP::RemoveBorders},
            {"Tracer Thickness", Options::ESP::TracerThickness},
            {"Box Thickness", Options::ESP::BoxThickness},
            {"Skeleton Thickness", Options::ESP::SkeletonThickness},
            {"3D ESP Thickness", Options::ESP::ESP3DThickness},
            {"Head Circle Thickness", Options::ESP::HeadCircleThickness},
            {"Head Circle Max Scale", Options::ESP::HeadCircleMaxScale},
            {"Name Color", FloatArray(Options::ESP::Color)},
            {"Box Color", FloatArray(Options::ESP::BoxColor)},
            {"Corner Color", FloatArray(Options::ESP::CornerColor)},
            {"Skeleton Color", FloatArray(Options::ESP::SkeletonColor)},
            {"Distance Color", FloatArray(Options::ESP::DistanceColor)},
            {"Tracers Color", FloatArray(Options::ESP::TracerColor)},
            {"3D ESP Color", FloatArray(Options::ESP::ESP3DColor)},
            {"Head Circles Color", FloatArray(Options::ESP::HeadCircleColor)},
            {"Chams Color", FloatArray(Options::ESP::ChamsColor)}
        };

        j["Aimbot"] = {
            {"Aimbot Key", Options::Aimbot::AimbotKey},
            {"Aiming Type", Options::Aimbot::AimingType},
            {"Toggle Type", Options::Aimbot::ToggleType},
            {"Aimbot", Options::Aimbot::Aimbot},
            {"Team Check", Options::Aimbot::TeamCheck},
            {"Downed Check", Options::Aimbot::DownedCheck},
            {"Sticky Aim", Options::Aimbot::StickyAim},
            {"Target Bone", Options::Aimbot::TargetBone},
            {"Air Target Bone", Options::Aimbot::AirTargetBone},
            {"FOV", Options::Aimbot::FOV},
            {"Show FOV", Options::Aimbot::ShowFOV},
            {"Show FOV Fill", Options::Aimbot::ShowFOVFill},
            {"FOV Color", FloatArray(Options::Aimbot::FOVColor)},
            {"FOV Fill Color", FloatArray(Options::Aimbot::FOVFillColor)},
            {"FOV Thickness", Options::Aimbot::FOVThickness},
            {"Smoothness", Options::Aimbot::Smoothness},
            {"Smoothness Curve", Options::Aimbot::SmoothnessCurve},
            {"Custom Curve Enabled", Options::Aimbot::CustomCurveEnabled},
            {"Custom Curve P1", FloatArray(Options::Aimbot::CustomCurveP1)},
            {"Custom Curve P2", FloatArray(Options::Aimbot::CustomCurveP2)},
            {"Range", Options::Aimbot::Range},
            {"Prediction", Options::Aimbot::Prediction},
            {"Prediction X", Options::Aimbot::PredictionX},
            {"Prediction Y", Options::Aimbot::PredictionY},
            {"Shake", Options::Aimbot::Shake},
            {"Shake Intensity", Options::Aimbot::ShakeIntensity},
            {"Stutter", Options::Aimbot::Stutter},
            {"Stutter Ticks", Options::Aimbot::StutterTicks}
        };

        j["Triggerbot"] = {
            {"Triggerbot Key", Options::Triggerbot::TriggerbotKey},
            {"Toggle Type", Options::Triggerbot::ToggleType},
            {"Enabled", Options::Triggerbot::Enabled},
            {"Team Check", Options::Triggerbot::TeamCheck},
            {"Downed Check", Options::Triggerbot::DownedCheck},
            {"Radius", Options::Triggerbot::Radius},
            {"Range", Options::Triggerbot::Range},
            {"Delay", Options::Triggerbot::Delay},
            {"Advanced FOV", Options::Triggerbot::AdvancedFOV},
            {"Show Advanced FOV", Options::Triggerbot::ShowAdvancedFOV},
            {"Head FOV X", Options::Triggerbot::HeadFOV_X},
            {"Head FOV Y", Options::Triggerbot::HeadFOV_Y},
            {"Torso FOV X", Options::Triggerbot::TorsoFOV_X},
            {"Torso FOV Y", Options::Triggerbot::TorsoFOV_Y},
            {"Upper Torso FOV X", Options::Triggerbot::UpperTorsoFOV_X},
            {"Upper Torso FOV Y", Options::Triggerbot::UpperTorsoFOV_Y},
            {"Lower Torso FOV X", Options::Triggerbot::LowerTorsoFOV_X},
            {"Lower Torso FOV Y", Options::Triggerbot::LowerTorsoFOV_Y},
            {"Left Upper Arm FOV X", Options::Triggerbot::LeftUpperArmFOV_X},
            {"Left Upper Arm FOV Y", Options::Triggerbot::LeftUpperArmFOV_Y},
            {"Left Lower Arm FOV X", Options::Triggerbot::LeftLowerArmFOV_X},
            {"Left Lower Arm FOV Y", Options::Triggerbot::LeftLowerArmFOV_Y},
            {"Left Hand FOV X", Options::Triggerbot::LeftHandFOV_X},
            {"Left Hand FOV Y", Options::Triggerbot::LeftHandFOV_Y},
            {"Right Upper Arm FOV X", Options::Triggerbot::RightUpperArmFOV_X},
            {"Right Upper Arm FOV Y", Options::Triggerbot::RightUpperArmFOV_Y},
            {"Right Lower Arm FOV X", Options::Triggerbot::RightLowerArmFOV_X},
            {"Right Lower Arm FOV Y", Options::Triggerbot::RightLowerArmFOV_Y},
            {"Right Hand FOV X", Options::Triggerbot::RightHandFOV_X},
            {"Right Hand FOV Y", Options::Triggerbot::RightHandFOV_Y},
            {"Left Upper Leg FOV X", Options::Triggerbot::LeftUpperLegFOV_X},
            {"Left Upper Leg FOV Y", Options::Triggerbot::LeftUpperLegFOV_Y},
            {"Left Lower Leg FOV X", Options::Triggerbot::LeftLowerLegFOV_X},
            {"Left Lower Leg FOV Y", Options::Triggerbot::LeftLowerLegFOV_Y},
            {"Left Foot FOV X", Options::Triggerbot::LeftFootFOV_X},
            {"Left Foot FOV Y", Options::Triggerbot::LeftFootFOV_Y},
            {"Right Upper Leg FOV X", Options::Triggerbot::RightUpperLegFOV_X},
            {"Right Upper Leg FOV Y", Options::Triggerbot::RightUpperLegFOV_Y},
            {"Right Lower Leg FOV X", Options::Triggerbot::RightLowerLegFOV_X},
            {"Right Lower Leg FOV Y", Options::Triggerbot::RightLowerLegFOV_Y},
            {"Right Foot FOV X", Options::Triggerbot::RightFootFOV_X},
            {"Right Foot FOV Y", Options::Triggerbot::RightFootFOV_Y}
        };

        j["Macro"] = {
            {"Macro Key", Options::Macro::MacroKey},
            {"Toggle Type", Options::Macro::ToggleType},
            {"Enabled", Options::Macro::Enabled},
            {"Delay", Options::Macro::Delay}
        };

        j["Crosshair"] = {
            {"Enabled", Options::Crosshair::Enabled},
            {"Style", Options::Crosshair::Style},
            {"Size", Options::Crosshair::Size},
            {"Gap", Options::Crosshair::Gap},
            {"Thickness", Options::Crosshair::Thickness},
            {"Spin Speed", Options::Crosshair::SpinSpeed},
            {"Gap Speed", Options::Crosshair::GapSpeed},
            {"Gap Tween", Options::Crosshair::GapTween},
            {"Show Text", Options::Crosshair::ShowText},
            {"Color", FloatArray(Options::Crosshair::Color)}
        };

        j["Misc"] = {
            {"Bypass", Options::Misc::Bypass},
            {"FOV Enabled", Options::Misc::FOVEnabled},
            {"FOV", Options::Misc::FOV},
            {"Cache NPCs", Options::Misc::CacheNPCs},
            {"Keybind List", Options::Misc::KeybindList},
            {"Keybind List X", Options::Misc::KeybindListX},
            {"Keybind List Y", Options::Misc::KeybindListY},
            {"Stream Proof", Options::Misc::StreamProof},
            {"Menu Accent Color", FloatArray(Options::Misc::MenuAccentColor)},
            {"Menu Key", Options::Misc::MenuKey}
        };

        j["HitboxExpander"] = {
            {"Enabled", Options::HitboxExpander::Enabled},
            {"Horizontal Size", Options::HitboxExpander::HorizontalSize},
            {"Vertical Size", Options::HitboxExpander::VerticalSize},
            {"Show Hitbox", Options::HitboxExpander::ShowHitbox},
            {"Transparency", Options::HitboxExpander::HitboxTransparency},
            {"Walk Through", Options::HitboxExpander::WalkThrough}
        };

        j["Fly"] = {
            {"Fly Key", Options::Fly::FlyKey},
            {"Toggle Type", Options::Fly::ToggleType},
            {"Enabled", Options::Fly::Enabled},
            {"Speed", Options::Fly::Speed}
        };

        j["WalkSpeed"] = {
            {"WalkSpeed Key", Options::WalkSpeed::WalkSpeedKey},
            {"Toggle Type", Options::WalkSpeed::ToggleType},
            {"Enabled", Options::WalkSpeed::Enabled},
            {"Speed", Options::WalkSpeed::Speed}
        };

        j["Desync"] = {
            {"Enabled", Options::Desync::Enabled},
            {"Key", Options::Desync::Key},
            {"Toggle Type", Options::Desync::ToggleType},
            {"Visualizer", Options::Desync::Visualizer},
            {"Visualizer Color", FloatArray(Options::Desync::VisualizerColor)}
        };

        return j;
    }

    inline const json* Section(const json& root, const char* name)
    {
        const auto it = root.find(name);
        return it != root.end() && it->is_object() ? &(*it) : nullptr;
    }

    inline void ReadBool(const json* section, const char* key, bool& target)
    {
        if (!section)
            return;
        const auto it = section->find(key);
        if (it != section->end() && it->is_boolean())
            target = it->get<bool>();
    }

    inline void ReadInt(const json* section, const char* key, int& target, int minimum, int maximum)
    {
        if (!section)
            return;
        const auto it = section->find(key);
        if (it == section->end() || !(it->is_number_integer() || it->is_number_unsigned()))
            return;
        const long double value = it->get<long double>();
        if (!std::isfinite(value))
            return;
        target = static_cast<int>(std::clamp(
            value, static_cast<long double>(minimum), static_cast<long double>(maximum)));
    }

    inline void ReadFloat(const json* section, const char* key, float& target, float minimum, float maximum)
    {
        if (!section)
            return;
        const auto it = section->find(key);
        if (it == section->end() || !it->is_number())
            return;
        const double value = it->get<double>();
        if (std::isfinite(value))
            target = static_cast<float>(std::clamp(value, static_cast<double>(minimum), static_cast<double>(maximum)));
    }

    template <std::size_t N>
    inline void ReadFloatArray(const json* section, const char* key, float (&target)[N], float minimum = 0.0f, float maximum = 1.0f)
    {
        if (!section)
            return;
        const auto it = section->find(key);
        if (it == section->end() || !it->is_array() || it->size() != N)
            return;

        std::array<float, N> values{};
        for (std::size_t index = 0; index < N; ++index)
        {
            if (!(*it)[index].is_number())
                return;
            const double value = (*it)[index].get<double>();
            if (!std::isfinite(value))
                return;
            values[index] = static_cast<float>(std::clamp(value, static_cast<double>(minimum), static_cast<double>(maximum)));
        }

        std::copy(values.begin(), values.end(), target);
    }

    inline Result Apply(const json& data)
    {
        if (!data.is_object())
            return {false, "Config root must be a JSON object", {}};

        int schemaVersion = 1;
        const auto versionIt = data.find("schemaVersion");
        if (versionIt != data.end())
        {
            if (!(versionIt->is_number_integer() || versionIt->is_number_unsigned()))
                return {false, "Invalid config schema version", {}};
            const long double version = versionIt->get<long double>();
            if (!std::isfinite(version) || version < 1.0L ||
                version > static_cast<long double>((std::numeric_limits<int>::max)()))
                return {false, "Invalid config schema version", {}};
            schemaVersion = static_cast<int>(version);
        }
        if (schemaVersion < 1 || schemaVersion > CurrentSchemaVersion)
            return {false, "Unsupported config schema version", {}};

        std::lock_guard<std::recursive_mutex> lock(Options::Mutex);

        const json* esp = Section(data, "ESP");
        ReadBool(esp, "Team Check", Options::ESP::TeamCheck);
        ReadInt(esp, "Box Type", Options::ESP::BoxType, 0, 2);
        if (esp && !esp->contains("Box Type"))
        {
            bool oldBox = Options::ESP::BoxType != 0;
            ReadBool(esp, "Box", oldBox);
            Options::ESP::BoxType = oldBox ? 1 : 0;
        }
        ReadBool(esp, "Tracers", Options::ESP::Tracers);
        ReadInt(esp, "TracersStart", Options::ESP::TracersStart, 0, 2);
        ReadBool(esp, "Skeleton", Options::ESP::Skeleton);
        ReadBool(esp, "Name", Options::ESP::Name);
        ReadBool(esp, "Distance", Options::ESP::Distance);
        ReadBool(esp, "Health", Options::ESP::Health);
        ReadBool(esp, "Head Circles", Options::ESP::HeadCircle);
        ReadBool(esp, "Corner ESP", Options::ESP::CornerESP);
        ReadBool(esp, "Headless", Options::ESP::Headless);
        ReadBool(esp, "Remove Borders", Options::ESP::RemoveBorders);
        ReadFloat(esp, "Tracer Thickness", Options::ESP::TracerThickness, 0.5f, 10.0f);
        ReadFloat(esp, "Box Thickness", Options::ESP::BoxThickness, 0.5f, 10.0f);
        ReadFloat(esp, "Skeleton Thickness", Options::ESP::SkeletonThickness, 0.5f, 10.0f);
        ReadFloat(esp, "3D ESP Thickness", Options::ESP::ESP3DThickness, 0.5f, 10.0f);
        ReadFloat(esp, "Head Circle Thickness", Options::ESP::HeadCircleThickness, 0.5f, 10.0f);
        ReadFloat(esp, "Head Circle Max Scale", Options::ESP::HeadCircleMaxScale, 0.1f, 10.0f);
        ReadFloatArray(esp, "Name Color", Options::ESP::Color);
        ReadFloatArray(esp, "Box Color", Options::ESP::BoxColor);
        ReadFloatArray(esp, "Corner Color", Options::ESP::CornerColor);
        ReadFloatArray(esp, "Skeleton Color", Options::ESP::SkeletonColor);
        ReadFloatArray(esp, "Distance Color", Options::ESP::DistanceColor);
        ReadFloatArray(esp, "Tracers Color", Options::ESP::TracerColor);
        ReadFloatArray(esp, "3D ESP Color", Options::ESP::ESP3DColor);
        ReadFloatArray(esp, "Head Circles Color", Options::ESP::HeadCircleColor);
        ReadFloatArray(esp, "Chams Color", Options::ESP::ChamsColor);

        const json* aimbot = Section(data, "Aimbot");
        ReadInt(aimbot, "Aimbot Key", Options::Aimbot::AimbotKey, 0, 255);
        ReadInt(aimbot, "Aiming Type", Options::Aimbot::AimingType, 0, 1);
        ReadInt(aimbot, "Toggle Type", Options::Aimbot::ToggleType, 0, 1);
        ReadBool(aimbot, "Aimbot", Options::Aimbot::Aimbot);
        ReadBool(aimbot, "Team Check", Options::Aimbot::TeamCheck);
        ReadBool(aimbot, "Downed Check", Options::Aimbot::DownedCheck);
        ReadBool(aimbot, "Sticky Aim", Options::Aimbot::StickyAim);
        ReadInt(aimbot, "Target Bone", Options::Aimbot::TargetBone, 0, 7);
        ReadInt(aimbot, "Air Target Bone", Options::Aimbot::AirTargetBone, 0, 7);
        ReadFloat(aimbot, "FOV", Options::Aimbot::FOV, 1.0f, 2000.0f);
        ReadBool(aimbot, "Show FOV", Options::Aimbot::ShowFOV);
        ReadBool(aimbot, "Show FOV Fill", Options::Aimbot::ShowFOVFill);
        ReadFloatArray(aimbot, "FOV Color", Options::Aimbot::FOVColor);
        ReadFloatArray(aimbot, "FOV Fill Color", Options::Aimbot::FOVFillColor);
        ReadFloat(aimbot, "FOV Thickness", Options::Aimbot::FOVThickness, 0.5f, 10.0f);
        ReadFloat(aimbot, "Smoothness", Options::Aimbot::Smoothness, 0.0f, 1.0f);
        ReadInt(aimbot, "Smoothness Curve", Options::Aimbot::SmoothnessCurve, 0, 4);
        ReadBool(aimbot, "Custom Curve Enabled", Options::Aimbot::CustomCurveEnabled);
        ReadFloatArray(aimbot, "Custom Curve P1", Options::Aimbot::CustomCurveP1);
        ReadFloatArray(aimbot, "Custom Curve P2", Options::Aimbot::CustomCurveP2);
        ReadFloat(aimbot, "Range", Options::Aimbot::Range, 1.0f, 100000.0f);
        ReadBool(aimbot, "Prediction", Options::Aimbot::Prediction);
        ReadFloat(aimbot, "Prediction X", Options::Aimbot::PredictionX, 0.01f, 1000.0f);
        ReadFloat(aimbot, "Prediction Y", Options::Aimbot::PredictionY, 0.01f, 1000.0f);
        ReadBool(aimbot, "Shake", Options::Aimbot::Shake);
        ReadFloat(aimbot, "Shake Intensity", Options::Aimbot::ShakeIntensity, 0.0f, 100.0f);
        ReadBool(aimbot, "Stutter", Options::Aimbot::Stutter);
        ReadInt(aimbot, "Stutter Ticks", Options::Aimbot::StutterTicks, 1, 1000);

        const json* triggerbot = Section(data, "Triggerbot");
        ReadInt(triggerbot, "Triggerbot Key", Options::Triggerbot::TriggerbotKey, 0, 255);
        ReadInt(triggerbot, "Toggle Type", Options::Triggerbot::ToggleType, 0, 1);
        ReadBool(triggerbot, "Enabled", Options::Triggerbot::Enabled);
        ReadBool(triggerbot, "Team Check", Options::Triggerbot::TeamCheck);
        ReadBool(triggerbot, "Downed Check", Options::Triggerbot::DownedCheck);
        ReadFloat(triggerbot, "Radius", Options::Triggerbot::Radius, 0.0f, 2000.0f);
        ReadFloat(triggerbot, "Range", Options::Triggerbot::Range, 1.0f, 100000.0f);
        ReadInt(triggerbot, "Delay", Options::Triggerbot::Delay, 0, 5000);
        ReadBool(triggerbot, "Advanced FOV", Options::Triggerbot::AdvancedFOV);
        ReadBool(triggerbot, "Show Advanced FOV", Options::Triggerbot::ShowAdvancedFOV);

        auto readPartFov = [triggerbot](const char* xName, float& x, const char* yName, float& y)
        {
            ReadFloat(triggerbot, xName, x, 0.0f, 2000.0f);
            ReadFloat(triggerbot, yName, y, 0.0f, 2000.0f);
        };
        readPartFov("Head FOV X", Options::Triggerbot::HeadFOV_X, "Head FOV Y", Options::Triggerbot::HeadFOV_Y);
        readPartFov("Torso FOV X", Options::Triggerbot::TorsoFOV_X, "Torso FOV Y", Options::Triggerbot::TorsoFOV_Y);
        readPartFov("Upper Torso FOV X", Options::Triggerbot::UpperTorsoFOV_X, "Upper Torso FOV Y", Options::Triggerbot::UpperTorsoFOV_Y);
        readPartFov("Lower Torso FOV X", Options::Triggerbot::LowerTorsoFOV_X, "Lower Torso FOV Y", Options::Triggerbot::LowerTorsoFOV_Y);
        readPartFov("Left Upper Arm FOV X", Options::Triggerbot::LeftUpperArmFOV_X, "Left Upper Arm FOV Y", Options::Triggerbot::LeftUpperArmFOV_Y);
        readPartFov("Left Lower Arm FOV X", Options::Triggerbot::LeftLowerArmFOV_X, "Left Lower Arm FOV Y", Options::Triggerbot::LeftLowerArmFOV_Y);
        readPartFov("Left Hand FOV X", Options::Triggerbot::LeftHandFOV_X, "Left Hand FOV Y", Options::Triggerbot::LeftHandFOV_Y);
        readPartFov("Right Upper Arm FOV X", Options::Triggerbot::RightUpperArmFOV_X, "Right Upper Arm FOV Y", Options::Triggerbot::RightUpperArmFOV_Y);
        readPartFov("Right Lower Arm FOV X", Options::Triggerbot::RightLowerArmFOV_X, "Right Lower Arm FOV Y", Options::Triggerbot::RightLowerArmFOV_Y);
        readPartFov("Right Hand FOV X", Options::Triggerbot::RightHandFOV_X, "Right Hand FOV Y", Options::Triggerbot::RightHandFOV_Y);
        readPartFov("Left Upper Leg FOV X", Options::Triggerbot::LeftUpperLegFOV_X, "Left Upper Leg FOV Y", Options::Triggerbot::LeftUpperLegFOV_Y);
        readPartFov("Left Lower Leg FOV X", Options::Triggerbot::LeftLowerLegFOV_X, "Left Lower Leg FOV Y", Options::Triggerbot::LeftLowerLegFOV_Y);
        readPartFov("Left Foot FOV X", Options::Triggerbot::LeftFootFOV_X, "Left Foot FOV Y", Options::Triggerbot::LeftFootFOV_Y);
        readPartFov("Right Upper Leg FOV X", Options::Triggerbot::RightUpperLegFOV_X, "Right Upper Leg FOV Y", Options::Triggerbot::RightUpperLegFOV_Y);
        readPartFov("Right Lower Leg FOV X", Options::Triggerbot::RightLowerLegFOV_X, "Right Lower Leg FOV Y", Options::Triggerbot::RightLowerLegFOV_Y);
        readPartFov("Right Foot FOV X", Options::Triggerbot::RightFootFOV_X, "Right Foot FOV Y", Options::Triggerbot::RightFootFOV_Y);

        const json* macro = Section(data, "Macro");
        ReadInt(macro, "Macro Key", Options::Macro::MacroKey, 0, 255);
        ReadInt(macro, "Toggle Type", Options::Macro::ToggleType, 0, 1);
        ReadBool(macro, "Enabled", Options::Macro::Enabled);
        ReadInt(macro, "Delay", Options::Macro::Delay, 0, 5000);

        const json* crosshair = Section(data, "Crosshair");
        ReadBool(crosshair, "Enabled", Options::Crosshair::Enabled);
        ReadInt(crosshair, "Style", Options::Crosshair::Style, 0, 1);
        ReadFloat(crosshair, "Size", Options::Crosshair::Size, 1.0f, 500.0f);
        ReadFloat(crosshair, "Gap", Options::Crosshair::Gap, 0.0f, 500.0f);
        ReadFloat(crosshair, "Thickness", Options::Crosshair::Thickness, 0.5f, 50.0f);
        ReadFloat(crosshair, "Spin Speed", Options::Crosshair::SpinSpeed, 0.0f, 1000.0f);
        ReadFloat(crosshair, "Gap Speed", Options::Crosshair::GapSpeed, 0.0f, 100.0f);
        ReadBool(crosshair, "Gap Tween", Options::Crosshair::GapTween);
        ReadBool(crosshair, "Show Text", Options::Crosshair::ShowText);
        ReadFloatArray(crosshair, "Color", Options::Crosshair::Color);

        const json* misc = Section(data, "Misc");
        ReadBool(misc, "Bypass", Options::Misc::Bypass);
        ReadBool(misc, "FOV Enabled", Options::Misc::FOVEnabled);
        ReadFloat(misc, "FOV", Options::Misc::FOV, 20.0f, 120.0f);
        ReadBool(misc, "Cache NPCs", Options::Misc::CacheNPCs);
        ReadBool(misc, "Keybind List", Options::Misc::KeybindList);
        ReadFloat(misc, "Keybind List X", Options::Misc::KeybindListX, 0.0f, 10000.0f);
        ReadFloat(misc, "Keybind List Y", Options::Misc::KeybindListY, 0.0f, 10000.0f);
        ReadBool(misc, "Stream Proof", Options::Misc::StreamProof);
        ReadFloatArray(misc, "Menu Accent Color", Options::Misc::MenuAccentColor);
        ReadInt(misc, "Menu Key", Options::Misc::MenuKey, 0, 255);

        const json* hitbox = Section(data, "HitboxExpander");
        ReadBool(hitbox, "Enabled", Options::HitboxExpander::Enabled);
        ReadFloat(hitbox, "Horizontal Size", Options::HitboxExpander::HorizontalSize, 1.0f, 30.0f);
        ReadFloat(hitbox, "Vertical Size", Options::HitboxExpander::VerticalSize, 1.0f, 30.0f);
        ReadBool(hitbox, "Show Hitbox", Options::HitboxExpander::ShowHitbox);
        ReadFloat(hitbox, "Transparency", Options::HitboxExpander::HitboxTransparency, 0.0f, 1.0f);
        ReadBool(hitbox, "Walk Through", Options::HitboxExpander::WalkThrough);

        const json* fly = Section(data, "Fly");
        ReadInt(fly, "Fly Key", Options::Fly::FlyKey, 0, 255);
        ReadInt(fly, "Toggle Type", Options::Fly::ToggleType, 0, 1);
        ReadBool(fly, "Enabled", Options::Fly::Enabled);
        ReadFloat(fly, "Speed", Options::Fly::Speed, 1.0f, 1000.0f);

        const json* walkSpeed = Section(data, "WalkSpeed");
        ReadInt(walkSpeed, "WalkSpeed Key", Options::WalkSpeed::WalkSpeedKey, 0, 255);
        ReadInt(walkSpeed, "Toggle Type", Options::WalkSpeed::ToggleType, 0, 1);
        ReadBool(walkSpeed, "Enabled", Options::WalkSpeed::Enabled);
        ReadFloat(walkSpeed, "Speed", Options::WalkSpeed::Speed, 1.0f, 1000.0f);

        const json* desync = Section(data, "Desync");
        ReadBool(desync, "Enabled", Options::Desync::Enabled);
        ReadInt(desync, "Key", Options::Desync::Key, 0, 255);
        ReadInt(desync, "Toggle Type", Options::Desync::ToggleType, 0, 1);
        ReadBool(desync, "Visualizer", Options::Desync::Visualizer);
        ReadFloatArray(desync, "Visualizer Color", Options::Desync::VisualizerColor);

        // Runtime-only state must never leak from one profile into another.
        Options::Aimbot::Toggled = false;
        Options::Aimbot::CurrentTarget = RobloxPlayer{};
        Options::Triggerbot::Toggled = false;
        Options::Macro::Toggled = false;
        Options::Fly::Toggled = false;
        Options::WalkSpeed::Toggled = false;
        Options::Desync::Toggled = false;

        return {true, schemaVersion == CurrentSchemaVersion ? "Config loaded" : "Legacy config migrated", {}};
    }

    inline Result NormalizeFileName(std::string name)
    {
        if (name.empty())
            return {false, "Config name cannot be empty", {}};
        if (name == "." || name == ".." || name.find_first_of("\\/:*?\"<>|") != std::string::npos)
            return {false, "Config name contains invalid characters", {}};
        if (name.size() > 120)
            return {false, "Config name is too long", {}};

        std::filesystem::path path(name);
        if (path.extension().empty())
            name += ".json";
        else if (path.extension() != ".json")
            return {false, "Config file must use the .json extension", {}};

        return {true, {}, name};
    }

    inline Result Save(const std::string& requestedName)
    {
        Result normalized = NormalizeFileName(requestedName);
        if (!normalized.success)
            return normalized;

        std::error_code error;
        const std::filesystem::path directory(Globals::configsPath);
        std::filesystem::create_directories(directory, error);
        if (error)
            return {false, "Could not create the configs directory", normalized.fileName};

        const std::filesystem::path target = directory / normalized.fileName;
        std::filesystem::path temporary = target;
        temporary += ".tmp";
        std::filesystem::path backup = target;
        backup += ".bak";

        {
            std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
            if (!output.is_open())
                return {false, "Could not open the temporary config file", normalized.fileName};
            output << Capture().dump(4);
            output.flush();
            if (!output.good())
            {
                output.close();
                std::filesystem::remove(temporary, error);
                return {false, "Could not write the config file", normalized.fileName};
            }
        }

        error.clear();
        if (std::filesystem::exists(target, error))
        {
            error.clear();
            std::filesystem::copy_file(target, backup, std::filesystem::copy_options::overwrite_existing, error);
            if (error)
            {
                std::filesystem::remove(temporary, error);
                return {false, "Could not create a config backup", normalized.fileName};
            }
            error.clear();
            std::filesystem::remove(target, error);
            if (error)
            {
                std::filesystem::remove(temporary, error);
                return {false, "Could not replace the existing config", normalized.fileName};
            }
        }

        error.clear();
        std::filesystem::rename(temporary, target, error);
        if (error)
        {
            std::error_code restoreError;
            if (std::filesystem::exists(backup, restoreError))
                std::filesystem::copy_file(backup, target, std::filesystem::copy_options::overwrite_existing, restoreError);
            std::filesystem::remove(temporary, restoreError);
            return {false, "Could not finalize the config file", normalized.fileName};
        }

        return {true, "Config saved", normalized.fileName};
    }

    inline Result Load(const std::string& requestedName)
    {
        Result normalized = NormalizeFileName(requestedName);
        if (!normalized.success)
            return normalized;

        const std::filesystem::path path = std::filesystem::path(Globals::configsPath) / normalized.fileName;
        std::ifstream input(path, std::ios::binary);
        if (!input.is_open())
            return {false, "Config file was not found", normalized.fileName};

        json data;
        try
        {
            data = json::parse(input);
        }
        catch (const json::exception& exception)
        {
            return {false, std::string("Invalid JSON: ") + exception.what(), normalized.fileName};
        }

        Result result = Apply(data);
        result.fileName = normalized.fileName;
        return result;
    }

    inline std::vector<std::string> List()
    {
        std::vector<std::string> result;
        std::error_code error;
        const std::filesystem::path directory(Globals::configsPath);
        if (!std::filesystem::exists(directory, error))
            return result;

        for (std::filesystem::directory_iterator it(directory, error), end; !error && it != end; it.increment(error))
        {
            if (it->is_regular_file(error) && it->path().extension() == ".json")
                result.push_back(it->path().filename().string());
        }
        std::sort(result.begin(), result.end());
        return result;
    }
}
