#include <cassert>
#include <cmath>
#include <filesystem>

#include "star/rbx/configs/configs.h"

// Config tests do not instantiate process access. This definition only allows
// the inline global MemoryManager owner to be destroyed by the test binary.
MemoryManager::~MemoryManager() {}

int main()
{
    Options::Misc::FOVEnabled = true;
    Options::Misc::MenuKey = 77;
    Options::ESP::SkeletonThickness = 4.5f;
    Options::Aimbot::FOVFillColor[3] = 0.42f;
    Options::Aimbot::CustomCurveP2[1] = 0.83f;
    Options::Triggerbot::AdvancedFOV = true;
    Options::Triggerbot::RightFootFOV_Y = 37.0f;
    Options::Desync::Visualizer = true;

    const auto document = Config::Capture();
    assert(document.at("schemaVersion") == 2);
    assert(document.at("Misc").at("FOV Enabled") == true);
    assert(document.at("Triggerbot").at("Right Foot FOV Y") == 37.0f);

    Options::Misc::FOVEnabled = false;
    Options::Misc::MenuKey = 0;
    Options::ESP::SkeletonThickness = 1.0f;
    Options::Aimbot::FOVFillColor[3] = 0.0f;
    Options::Aimbot::CustomCurveP2[1] = 0.0f;
    Options::Triggerbot::AdvancedFOV = false;
    Options::Triggerbot::RightFootFOV_Y = 0.0f;
    Options::Desync::Visualizer = false;

    const auto applied = Config::Apply(document);
    assert(applied.success);
    assert(Options::Misc::FOVEnabled);
    assert(Options::Misc::MenuKey == 77);
    assert(std::abs(Options::ESP::SkeletonThickness - 4.5f) < 0.001f);
    assert(std::abs(Options::Aimbot::FOVFillColor[3] - 0.42f) < 0.001f);
    assert(std::abs(Options::Aimbot::CustomCurveP2[1] - 0.83f) < 0.001f);
    assert(Options::Triggerbot::AdvancedFOV);
    assert(std::abs(Options::Triggerbot::RightFootFOV_Y - 37.0f) < 0.001f);
    assert(Options::Desync::Visualizer);

    Config::json invalid = {
        {"schemaVersion", 2},
        {"Misc", {{"FOV", 9999.0}, {"Menu Key", 9999}}}
    };
    assert(Config::Apply(invalid).success);
    assert(Options::Misc::FOV == 120.0f);
    assert(Options::Misc::MenuKey == 255);

    Config::json future = {{"schemaVersion", 999}};
    assert(!Config::Apply(future).success);

    const auto testDirectory = std::filesystem::temp_directory_path() / "celex-config-tests";
    std::error_code fileError;
    std::filesystem::remove_all(testDirectory, fileError);
    Globals::configsPath = testDirectory.string();
    const auto saved = Config::Save("round-trip");
    assert(saved.success);
    assert(saved.fileName == "round-trip.json");
    assert(std::filesystem::exists(testDirectory / saved.fileName));
    Options::Misc::MenuKey = 12;
    const auto loaded = Config::Load("round-trip.json");
    assert(loaded.success);
    assert(Options::Misc::MenuKey == 255);
    assert(Config::Save("round-trip.json").success);
    assert(std::filesystem::exists(testDirectory / "round-trip.json.bak"));
    assert(!Config::Save("../invalid.json").success);
    std::filesystem::remove_all(testDirectory, fileError);

    Config::json legacy = {{"ESP", {{"Box", true}}}};
    assert(Config::Apply(legacy).success);
    assert(Options::ESP::BoxType == 1);

    return 0;
}
