#include "main.hpp"
#include "GraphicsTweaksConfig.hpp"

#include "GlobalNamespace/BoolSO.hpp"
#include "GlobalNamespace/FloatSO.hpp"
#include "GlobalNamespace/IVRPlatformHelper.hpp"
#include "GlobalNamespace/OVRManager.hpp"
#include "GlobalNamespace/OVRPlugin.hpp"
#include "GlobalNamespace/VRPlatformSDK.hpp"
#include "GlobalNamespace/VRRenderingParamsSetup.hpp"
#include "GlobalNamespace/MenuShockwave.hpp"
#include "UnityEngine/Mathf.hpp"
#include "UnityEngine/Resources.hpp"
#include "GlobalNamespace/ShockwaveEffect.hpp"
#include "UnityEngine/XR/XRSettings.hpp"
#include "Unity/XR/Oculus/NativeMethods.hpp"
#include "logging.hpp"
#include "bsml/shared/BSML/MainThreadScheduler.hpp"
#include "bsml/shared/BSML/SharedCoroutineStarter.hpp"
#include "UnityEngine/QualitySettings.hpp"
#include "FPSCounter.hpp"
using namespace GlobalNamespace;

SafePtrUnity<GlobalNamespace::VRRenderingParamsSetup> vrRenderingParamsSetup;

MAKE_HOOK_MATCH(
    VRRenderingParamsSetup_OnEnable,
    &GlobalNamespace::VRRenderingParamsSetup::OnEnable,
    void,
    GlobalNamespace::VRRenderingParamsSetup* self
) {
    vrRenderingParamsSetup = self;

    GraphicsTweaks::VRRenderingParamsSetup::Reload();
}

void GraphicsTweaks::VRRenderingParamsSetup::Reload(std::optional<float> vrResolutionScale) {
    using namespace GlobalNamespace;
    using namespace UnityEngine;
    using namespace UnityEngine::XR;

    // Set defaults for resolution scale.
    if (getGraphicsTweaksConfig().MenuResolution.GetValue() == 0) {
        getGraphicsTweaksConfig().MenuResolution.SetValue(1);
    }
    if (getGraphicsTweaksConfig().GameResolution.GetValue() == 0) {
        getGraphicsTweaksConfig().GameResolution.SetValue(1);
    }

    // Set resolution scale based on scene type.
    auto resolutionMultiplier = vrRenderingParamsSetup->____sceneType == GlobalNamespace::SceneType::Game ? getGraphicsTweaksConfig().GameResolution.GetValue() : getGraphicsTweaksConfig().MenuResolution.GetValue();
    auto currentEyeTextureResolutionScale = XRSettings::get_eyeTextureResolutionScale();

    if (currentEyeTextureResolutionScale != resolutionMultiplier) {
        DEBUG("Setting resolution scale to {}", resolutionMultiplier);
        XRSettings::set_eyeTextureResolutionScale(resolutionMultiplier);
    }

    bool distortionsUsed = getGraphicsTweaksConfig().WallQuality.GetValue() == 2 || getGraphicsTweaksConfig().MenuShockwaves.GetValue() || getGraphicsTweaksConfig().GameShockwaves.GetValue();
    auto aaValue = distortionsUsed ? 0 : getGraphicsTweaksConfig().AntiAliasing.GetValue()*2;
    if (QualitySettings::get_antiAliasing() != aaValue) {
        UnityEngine::QualitySettings::set_antiAliasing(aaValue);
    }

    // Next frame is needed because the game sets the resolution scale after this function is called too.
    BSML::MainThreadScheduler::ScheduleNextFrame([]() {
        // If not Oculus, return.
        // if (vrRenderingParamsSetup->_vrPlatformHelper->get_vrPlatformSDK() != VRPlatformSDK::Oculus) {
        //     WARNING("Not Oculus, returning");
        //     return;
        // }

        // Fixed foveated rendering
        if (OVRManager::get_fixedFoveatedRenderingSupported()) {
            auto foveationLevel = vrRenderingParamsSetup->____sceneType != GlobalNamespace::SceneType::Game ? getGraphicsTweaksConfig().MenuFoveatedRenderingLevel.GetValue() : getGraphicsTweaksConfig().InGameFoveatedRenderingLevel.GetValue();
            OVRManager::set_fixedFoveatedRenderingLevel(foveationLevel);
        }

        OVRPlugin::SetClientColorDesc(getGraphicsTweaksConfig().ColorSpace.GetValue());

        // Set our CPU/GPU levels.
        OVRPlugin::set_cpuLevel(getGraphicsTweaksConfig().CpuLevel.GetValue());
        OVRPlugin::set_gpuLevel(getGraphicsTweaksConfig().GpuLevel.GetValue());

        // Check if our (default) refresh rate has been set.
        auto maxRefreshRate = Mathf::Max(OVRPlugin::get_systemDisplayFrequenciesAvailable());

        // Set defaults and clamp to max refresh rate.
        if (getGraphicsTweaksConfig().MenuRefreshRate.GetValue() > maxRefreshRate || getGraphicsTweaksConfig().GameRefreshRate.GetValue() == -1) {
            getGraphicsTweaksConfig().MenuRefreshRate.SetValue(maxRefreshRate);
        }
        if (getGraphicsTweaksConfig().GameRefreshRate.GetValue() > maxRefreshRate || getGraphicsTweaksConfig().GameRefreshRate.GetValue() == -1) {
            getGraphicsTweaksConfig().GameRefreshRate.SetValue(maxRefreshRate);
        }

        float refreshRate = vrRenderingParamsSetup->____sceneType == GlobalNamespace::SceneType::Game ? getGraphicsTweaksConfig().GameRefreshRate.GetValue() : getGraphicsTweaksConfig().MenuRefreshRate.GetValue();
        auto currentRefreshRate = OVRPlugin::get_systemDisplayFrequency();
        if (currentRefreshRate != refreshRate) {
            DEBUG("Setting refresh rate to {}", refreshRate);
            OVRPlugin::set_systemDisplayFrequency(refreshRate);
        }

        // Handle shockwaves.
        auto shockwaveValue = vrRenderingParamsSetup->____sceneType == GlobalNamespace::SceneType::Game ? getGraphicsTweaksConfig().GameShockwaves.GetValue() : getGraphicsTweaksConfig().MenuShockwaves.GetValue();
        auto shockwaveEffectElements = UnityEngine::Resources::FindObjectsOfTypeAll<GlobalNamespace::ShockwaveEffect*>();
        for (UnityW<GlobalNamespace::ShockwaveEffect> shockwaveElement : shockwaveEffectElements) {
            if (shockwaveElement) shockwaveElement->set_enabled(shockwaveValue);
        }

        // Game specific shockwaves.
        if (vrRenderingParamsSetup->____sceneType == GlobalNamespace::SceneType::Game) {
            auto shockwaveEffectElements = UnityEngine::Resources::FindObjectsOfTypeAll<GlobalNamespace::ShockwaveEffect*>();
            for (UnityW<GlobalNamespace::ShockwaveEffect> shockwaveElement : shockwaveEffectElements) {
                if (shockwaveElement) {
                    shockwaveElement->set_enabled(shockwaveValue);
                }
            }
        }

        // Menu specific shockwaves.
        if (vrRenderingParamsSetup->____sceneType == GlobalNamespace::SceneType::Menu) {
            auto shockwaveEffectElements = UnityEngine::Resources::FindObjectsOfTypeAll<GlobalNamespace::MenuShockwave*>();
            for (UnityW<GlobalNamespace::MenuShockwave> shockwaveElement : shockwaveEffectElements) {
                if (shockwaveElement) {
                    shockwaveElement->set_enabled(shockwaveValue);
                }
            }
        }
    });
}

void GraphicsTweaks::Hooks::VRRenderingParamsSetup() {
    INSTALL_HOOK(Logger, VRRenderingParamsSetup_OnEnable);
}