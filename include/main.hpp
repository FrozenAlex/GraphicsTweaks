#pragma once
#include "beatsaber-hook/shared/safeptr.hpp"

// Include the modloader header, which allows us to tell the modloader which mod this is, and the version etc.
#include "scotland2/shared/loader.hpp"

// beatsaber-hook is a modding framework that lets us call functions and fetch field values from in the game
// It also allows creating objects, configuration, and importantly, hooking methods to modify their values
#include "GlobalNamespace/MainSystemInit.hpp"
#include "GlobalNamespace/SettingsManager.hpp"
#include "GlobalNamespace/SettingsApplicatorSO.hpp"
#include "GlobalNamespace/SceneType.hpp"


namespace GraphicsTweaks {
    
    namespace Hooks {
        void VRRenderingParamsSetup();

    }

    namespace VRRenderingParamsSetup {
        float GetConfiguredRefreshRate(GlobalNamespace::SceneType sceneType = GlobalNamespace::SceneType::Undefined);
        void ApplyResolutionScale(GlobalNamespace::SceneType sceneType);
        void Reload(std::optional<float> vrResolutionScale = std::nullopt);
    }

    class PerformancePreset {
        public:
            static void ApplySettings();
            static BeatSaber::Settings::Settings* GetCustomPreset();
            inline static safe_ptr<GlobalNamespace::SettingsApplicatorSO*> settingsApplicatorSO;
            inline static BeatSaber::Settings::Settings* customPreset;
    };
}
