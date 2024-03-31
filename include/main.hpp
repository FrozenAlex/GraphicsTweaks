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
#include "GlobalNamespace/MirrorRendererSO.hpp"
#include "GlobalNamespace/MirrorRendererGraphicsSettingsPresets.hpp"

#include "GraphicsTweaksConfig.hpp"

namespace GraphicsTweaks {
    namespace Hooks {
        void VRRenderingParamsSetup();

    }

    namespace VRRenderingParamsSetup {
        void Reload(std::optional<float> vrResolutionScale = std::nullopt);
    }

    class MirrorsData {
        public:
            static void ApplySettings();
            inline static SafePtrUnity<GlobalNamespace::MirrorRendererSO> mirrorRenderer;
            inline static SafePtrUnity<GlobalNamespace::MirrorRendererGraphicsSettingsPresets> mirrorRendererGraphicsSettingsPresets;
    };

    class BloomData {
        public:
            static void ApplySettings();
            inline static SafePtrUnity<GlobalNamespace::MainEffectContainerSO> mainEffectContainer;
            inline static SafePtrUnity<GlobalNamespace::MainEffectSO> noEffect;
            //Should we support both pyramid and kawase bloom?
            inline static SafePtrUnity<GlobalNamespace::MainEffectSO> ldBloomEffect;
            inline static SafePtrUnity<GlobalNamespace::MainEffectSO> hdBloomEffect;
    };
}