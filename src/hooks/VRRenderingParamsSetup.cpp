#include "main.hpp"
#include "GraphicsTweaksConfig.hpp"

#include "GlobalNamespace/OVRManager.hpp"
#include "GlobalNamespace/OVRPlugin.hpp"
#include "GlobalNamespace/VRRenderingParamsSetup.hpp"
#include "GlobalNamespace/MenuShockwave.hpp"
#include "UnityEngine/Mathf.hpp"
#include "UnityEngine/Resources.hpp"
#include "GlobalNamespace/ShockwaveEffect.hpp"
#include "UnityEngine/Rendering/GraphicsSettings.hpp"
#include "UnityEngine/Rendering/Universal/UniversalRenderPipelineAsset.hpp"
#include "logging.hpp"
#include "bsml/shared/BSML/MainThreadScheduler.hpp"
#include "UnityEngine/QualitySettings.hpp"
#include "beatsaber-hook/shared/hooking.hpp"
#include "PlatformDetector.hpp"
#include "Utils.hpp"
using namespace GlobalNamespace;

safe_ptr<GlobalNamespace::VRRenderingParamsSetup*> vrRenderingParamsSetup;

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

void GraphicsTweaks::VRRenderingParamsSetup::ApplyResolutionScale(GlobalNamespace::SceneType sceneType) {
    if (GraphicsTweaks::IsInRender()) return;

    // Undefined is used when changing settings without changing scenes.
    static GlobalNamespace::SceneType currentSceneType = GlobalNamespace::SceneType::Menu;
    if (sceneType != GlobalNamespace::SceneType::Undefined) currentSceneType = sceneType;
    float scale = currentSceneType == GlobalNamespace::SceneType::Game
        ? getGraphicsTweaksConfig().GameResolution.GetValue()
        : getGraphicsTweaksConfig().MenuResolution.GetValue();
    if (scale == 0) scale = 1;

    // URP reapplies this field to XR render targets when it prepares each camera.
    auto pipeline = UnityEngine::Rendering::GraphicsSettings::get_currentRenderPipeline()
        .try_cast<UnityEngine::Rendering::Universal::UniversalRenderPipelineAsset>();
    if (pipeline) pipeline->m_RenderScale = scale;
}

void GraphicsTweaks::VRRenderingParamsSetup::Reload(std::optional<float> vrResolutionScale) {
    using namespace GlobalNamespace;
    using namespace UnityEngine;

    if (GraphicsTweaks::IsInRender()) {
        INFO("Skipping VRRenderingParamsSetup::Reload because we are in render state.");
        return;
    }

    // Set defaults for resolution scale.
    if (getGraphicsTweaksConfig().MenuResolution.GetValue() == 0) {
        getGraphicsTweaksConfig().MenuResolution.SetValue(1);
    }
    if (getGraphicsTweaksConfig().GameResolution.GetValue() == 0) {
        getGraphicsTweaksConfig().GameResolution.SetValue(1);
    }

    GlobalNamespace::SceneType sceneType = vrRenderingParamsSetup->____sceneType;

    ApplyResolutionScale(sceneType);

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
   

    bool distortionsUsed = getGraphicsTweaksConfig().WallQuality.GetValue() == 2 || getGraphicsTweaksConfig().MenuShockwaves.GetValue() || getGraphicsTweaksConfig().GameShockwaves.GetValue();
    // Quest 1 has bugged distortions, so we disable them.
    if (isQuest1) {
        distortionsUsed = false;
    }

    auto aaValue = distortionsUsed ? 0 : getGraphicsTweaksConfig().AntiAliasing.GetValue()*2;
    if (QualitySettings::get_antiAliasing() != aaValue) {
        UnityEngine::QualitySettings::set_antiAliasing(aaValue);
    }

    // Apply device performance settings after scene initialization.
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
        auto maxRefreshRate = predefinedRefreshRates.empty()
            ? Mathf::Max(OVRPlugin::get_systemDisplayFrequenciesAvailable())
            : predefinedRefreshRates.back();
        auto defaultRefreshRate = model == "Quest 3" ? 120.0f : maxRefreshRate;

        // Set defaults and clamp to max refresh rate.
        if (getGraphicsTweaksConfig().MenuRefreshRate.GetValue() > maxRefreshRate) {
            getGraphicsTweaksConfig().MenuRefreshRate.SetValue(maxRefreshRate);
        } else if (getGraphicsTweaksConfig().MenuRefreshRate.GetValue() == -1) {
            getGraphicsTweaksConfig().MenuRefreshRate.SetValue(defaultRefreshRate);
        }
        if (getGraphicsTweaksConfig().GameRefreshRate.GetValue() > maxRefreshRate) {
            getGraphicsTweaksConfig().GameRefreshRate.SetValue(maxRefreshRate);
        } else if (getGraphicsTweaksConfig().GameRefreshRate.GetValue() == -1) {
            getGraphicsTweaksConfig().GameRefreshRate.SetValue(defaultRefreshRate);
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
