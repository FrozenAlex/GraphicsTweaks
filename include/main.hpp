#pragma once

// Include the modloader header, which allows us to tell the modloader which mod this is, and the version etc.
#include "scotland2/shared/loader.hpp"

// beatsaber-hook is a modding framework that lets us call functions and fetch field values from in the game
// It also allows creating objects, configuration, and importantly, hooking methods to modify their values
#include "beatsaber-hook/shared/utils/logging.hpp"
#include "beatsaber-hook/shared/config/config-utils.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-functions.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "GlobalNamespace/MainSystemInit.hpp"
#include "GlobalNamespace/MirrorRendererSO.hpp"
#include "GlobalNamespace/MirrorRendererGraphicsSettingsPresets.hpp"
#include "GlobalNamespace/MainEffectContainerSO.hpp"
#include "GlobalNamespace/SettingsApplicatorSO.hpp"
#include "GlobalNamespace/MirrorRendererGraphicsSettingsPresets.hpp"
#include "BeatSaber/PerformancePresets/PerformancePreset.hpp"
#include "GraphicsTweaksConfig.hpp"

namespace GraphicsTweaks {
    namespace Hooks {
        void VRRenderingParamsSetup();

    }

    namespace VRRenderingParamsSetup {
        void Reload(std::optional<float> vrResolutionScale = std::nullopt);
    }

    class PerformancePreset {
        public:
            static void ApplySettings();
            static UnityW<BeatSaber::PerformancePresets::PerformancePreset> GetCustomPreset();
            inline static SafePtrUnity<GlobalNamespace::SettingsApplicatorSO> settingsApplicatorSO;
            inline static SafePtr<BeatSaber::PerformancePresets::PerformancePreset> customPreset;
    };
}
