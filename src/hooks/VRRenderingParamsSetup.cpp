#include "main.hpp"
#include "GraphicsTweaksConfig.hpp"

#include "GlobalNamespace/OVRManager.hpp"
#include "GlobalNamespace/OVRPlugin.hpp"
#include "GlobalNamespace/VRRenderingParamsSetup.hpp"
#include "GlobalNamespace/MenuShockwave.hpp"
#include "UnityEngine/Mathf.hpp"
#include "UnityEngine/Resources.hpp"
#include "GlobalNamespace/ShockwaveEffect.hpp"
#include "UnityEngine/XR/XRSettings.hpp"
#include "logging.hpp"
#include "bsml/shared/BSML/MainThreadScheduler.hpp"
#include "UnityEngine/QualitySettings.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "PlatformDetector.hpp"
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

::GlobalNamespace::OVRPlugin_ProcessorPerformanceLevel getPerfLevel(int level) {
    switch (level) {
        case 0:
            return ::GlobalNamespace::OVRPlugin_ProcessorPerformanceLevel::PowerSavings;
        case 1:
            return ::GlobalNamespace::OVRPlugin_ProcessorPerformanceLevel::SustainedLow;
        case 2:
            return ::GlobalNamespace::OVRPlugin_ProcessorPerformanceLevel::SustainedHigh;
        case 3:
            return ::GlobalNamespace::OVRPlugin_ProcessorPerformanceLevel::Boost;
        default:
            return ::GlobalNamespace::OVRPlugin_ProcessorPerformanceLevel::SustainedHigh;
    }
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

    GlobalNamespace::SceneType sceneType = vrRenderingParamsSetup->____sceneType;

    // Set resolution scale based on scene type.
    auto resolutionMultiplier = sceneType == GlobalNamespace::SceneType::Game ? getGraphicsTweaksConfig().GameResolution.GetValue() : getGraphicsTweaksConfig().MenuResolution.GetValue();
    auto currentEyeTextureResolutionScale = XRSettings::get_eyeTextureResolutionScale();

    // Enable dynamic resolution.
    // Seems to introduce screen distortion, so it's disabled for now.
    // {
    //     auto instance = OVRManager::get_instance();
    //     if (!instance)
    //     {
    //         DEBUG("OVRManager instance is null, creating new one...");
    //         auto ovrManagerGO = UnityEngine::GameObject::New_ctor("OVRManager");
    //         UnityEngine::Object::DontDestroyOnLoad(ovrManagerGO);
    //         ovrManagerGO->SetActive(false);
    //         auto instance = ovrManagerGO->AddComponent<GlobalNamespace::OVRManager*>();

        
    //         instance->set_trackingOriginType(GlobalNamespace::OVRManager_TrackingOrigin::FloorLevel);
    
    //         ovrManagerGO->SetActive(true);
    //         DEBUG("Initialized OVRManager!");
    //     }
        

    //     if (instance) {
    //         // instance->enableDynamicResolution = getGraphicsTweaksConfig().DynamicResolution.GetValue();
    //         // instance->__cordl_internal_set_maxDynamicResolutionScale(1.0f);
    //         // instance->__cordl_internal_set_minDynamicResolutionScale(0.6f);
    //     } else {
    //         WARNING("OVRManager instance is null!");
    //     }
    // }
   

    if (currentEyeTextureResolutionScale != resolutionMultiplier) {
        DEBUG("Setting resolution scale to {}", resolutionMultiplier);
        XRSettings::set_eyeTextureResolutionScale(resolutionMultiplier);
    }

    bool distortionsUsed = getGraphicsTweaksConfig().WallQuality.GetValue() == 2 || getGraphicsTweaksConfig().MenuShockwaves.GetValue() || getGraphicsTweaksConfig().GameShockwaves.GetValue();
    // Quest 1 has bugged distortions, so we disable them.
    if (isQuest1) {
        distortionsUsed = false;
    }

    auto aaValue = distortionsUsed ? 0 : getGraphicsTweaksConfig().AntiAliasing.GetValue()*2;
    if (QualitySettings::get_antiAliasing() != aaValue) {
        UnityEngine::QualitySettings::set_antiAliasing(aaValue);
    }

    // Next frame is needed because the game sets the resolution scale after this function is called too.
    BSML::MainThreadScheduler::ScheduleNextFrame([]() {
        GlobalNamespace::SceneType sceneType = vrRenderingParamsSetup->____sceneType;

        // Fixed foveated rendering
        if (OVRManager::get_fixedFoveatedRenderingSupported()) {
            auto foveationLevel = sceneType != GlobalNamespace::SceneType::Game ? getGraphicsTweaksConfig().MenuFoveatedRenderingLevel.GetValue() : getGraphicsTweaksConfig().InGameFoveatedRenderingLevel.GetValue();
            auto currentLevel = static_cast<int>(OVRManager::get_fixedFoveatedRenderingLevel());
            if (currentLevel != foveationLevel) {
                DEBUG("Setting fixed foveated rendering level to {}", foveationLevel);
                OVRManager::set_fixedFoveatedRenderingLevel(foveationLevel);
            }
        }

        OVRPlugin::SetClientColorDesc(getGraphicsTweaksConfig().ColorSpace.GetValue());

        // Set our CPU/GPU levels. (we subtract 1 because the game uses 0-indexed values)
        OVRPlugin::set_suggestedCpuPerfLevel(getPerfLevel(getGraphicsTweaksConfig().CpuPerfLevel.GetValue()));
        OVRPlugin::set_suggestedGpuPerfLevel(getPerfLevel(getGraphicsTweaksConfig().GpuPerfLevel.GetValue()));

        // Check if our (default) refresh rate has been set.
        auto maxRefreshRate = Mathf::Max(OVRPlugin::get_systemDisplayFrequenciesAvailable());

        // Set defaults and clamp to max refresh rate.
        if (getGraphicsTweaksConfig().MenuRefreshRate.GetValue() > maxRefreshRate || getGraphicsTweaksConfig().GameRefreshRate.GetValue() == -1) {
            getGraphicsTweaksConfig().MenuRefreshRate.SetValue(maxRefreshRate);
        }
        if (getGraphicsTweaksConfig().GameRefreshRate.GetValue() > maxRefreshRate || getGraphicsTweaksConfig().GameRefreshRate.GetValue() == -1) {
            getGraphicsTweaksConfig().GameRefreshRate.SetValue(maxRefreshRate);
        }

        float refreshRate = sceneType == GlobalNamespace::SceneType::Game ? getGraphicsTweaksConfig().GameRefreshRate.GetValue() : getGraphicsTweaksConfig().MenuRefreshRate.GetValue();
        auto currentRefreshRate = OVRPlugin::get_systemDisplayFrequency();
        if (currentRefreshRate != refreshRate) {
            DEBUG("Setting refresh rate to {}", refreshRate);
            OVRPlugin::set_systemDisplayFrequency(refreshRate);
        }

        // Handle shockwaves.
        auto shockwaveValue = sceneType == GlobalNamespace::SceneType::Game ? getGraphicsTweaksConfig().GameShockwaves.GetValue() : getGraphicsTweaksConfig().MenuShockwaves.GetValue();
        auto shockwaveEffectElements = UnityEngine::Resources::FindObjectsOfTypeAll<GlobalNamespace::ShockwaveEffect*>();
        for (UnityW<GlobalNamespace::ShockwaveEffect> shockwaveElement : shockwaveEffectElements) {
            if (shockwaveElement) shockwaveElement->set_enabled(shockwaveValue);
        }

        if (!isQuest1) {
            // Game specific shockwaves.
            if (sceneType == GlobalNamespace::SceneType::Game) {
                auto shockwaveEffectElements = UnityEngine::Resources::FindObjectsOfTypeAll<GlobalNamespace::ShockwaveEffect*>();
                for (UnityW<GlobalNamespace::ShockwaveEffect> shockwaveElement : shockwaveEffectElements) {
                    if (shockwaveElement) {
                        shockwaveElement->set_enabled(shockwaveValue);
                    }
                }
            }

            // Menu specific shockwaves.
            if (sceneType == GlobalNamespace::SceneType::Menu) {
                auto shockwaveEffectElements = UnityEngine::Resources::FindObjectsOfTypeAll<GlobalNamespace::MenuShockwave*>();
                for (UnityW<GlobalNamespace::MenuShockwave> shockwaveElement : shockwaveEffectElements) {
                    if (shockwaveElement) {
                        shockwaveElement->set_enabled(shockwaveValue);
                    }
                }
            }    
        }
        
    });
}

void GraphicsTweaks::Hooks::VRRenderingParamsSetup() {
    INSTALL_HOOK(Logger, VRRenderingParamsSetup_OnEnable);
}