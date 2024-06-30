#include "main.hpp"
#include "_config.hpp"
#include "Unity/XR/Oculus/OculusLoader.hpp"
#include "GlobalNamespace/MainSystemInit.hpp"
#include "GlobalNamespace/ConditionalActivation.hpp"
#include "BeatSaber/PerformancePresets/ObstaclesQuality.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/Renderer.hpp"
#include "UnityEngine/Object.hpp"
#include "UnityEngine/Camera.hpp"
#include "UnityEngine/ScriptableObject.hpp"
#include "UnityEngine/XR/XRSettings.hpp"
#include "UnityEngine/QualitySettings.hpp"
#include "UnityEngine/CameraClearFlags.hpp"
#include "UnityEngine/Renderer.hpp"
#include "UnityEngine/Rendering/CommandBuffer.hpp"
#include "UnityEngine/RenderTexture.hpp"
#include "UnityEngine/RenderTextureFormat.hpp"
#include "UnityEngine/RenderTextureReadWrite.hpp"
#include "UnityEngine/VRTextureUsage.hpp"
#include "UnityEngine/ParticleSystem.hpp"
#include "GlobalNamespace/ObservableVariableSO_1.hpp"
#include "GlobalNamespace/MainCamera.hpp"
#include "GlobalNamespace/IFileStorage.hpp"
#include "GlobalNamespace/ConditionalMaterialSwitcher.hpp"
#include "GlobalNamespace/AlwaysVisibleQuad.hpp"
#include "GlobalNamespace/OVRManager.hpp"
#include "GlobalNamespace/OVRPassthroughLayer.hpp"
#include "GlobalNamespace/OVROverlay.hpp"
#include "GlobalNamespace/BloomPrePass.hpp"
#include "GlobalNamespace/BoolSO.hpp"
#include "GlobalNamespace/OVRPlugin.hpp"
#include "GlobalNamespace/ObstacleMaterialSetter.hpp"
#include "GlobalNamespace/ShockwaveEffect.hpp"
#include "GlobalNamespace/MainSystemInit.hpp"
#include "GlobalNamespace/MirrorRendererSO.hpp"
#include "GlobalNamespace/MirrorRendererGraphicsSettingsPresets.hpp"
#include "GlobalNamespace/BloomPrePassEffectContainerSO.hpp"
#include "GlobalNamespace/BloomPrePassEffectSO.hpp"
#include "GlobalNamespace/FakeReflectionDynamicObjectsState.hpp"
#include "GlobalNamespace/PyramidBloomRendererSO.hpp"
#include "GlobalNamespace/FPSCounter.hpp"
#include "GlobalNamespace/VRRenderingParamsSetup.hpp"
#include "GlobalNamespace/FPSCounterUIController.hpp"
#include "GlobalNamespace/GameplayCoreInstaller.hpp"
#include "GlobalNamespace/PerformanceVisualizer.hpp"
#include "GlobalNamespace/FakeMirrorObjectsInstaller.hpp"
#include "GlobalNamespace/MirroredGameNoteController.hpp"
#include "GlobalNamespace/MirroredBombNoteController.hpp"
#include "GlobalNamespace/MirroredObstacleController.hpp"
#include "GlobalNamespace/MirroredSliderController.hpp"
#include "GlobalNamespace/DepthTextureController.hpp"
#include "GlobalNamespace/MirroredBeatmapObjectManager.hpp"
#include "GlobalNamespace/MirrorRendererGraphicsSettingsPresets.hpp"
#include "GlobalNamespace/MainFlowCoordinator.hpp"
#include "GlobalNamespace/PyramidBloomMainEffectSO.hpp"
#include "Zenject/DiContainer.hpp"
#include "Zenject/ScopeConcreteIdArgConditionCopyNonLazyBinder.hpp"
#include "Zenject/FromBinderGeneric_1.hpp"
#include "bsml/shared/BSML/MainThreadScheduler.hpp"
#include "bsml/shared/BSML/SharedCoroutineStarter.hpp"
#include "bsml/shared/BSML.hpp"
#include "UI/GraphicsTweaksFlowCoordinator.hpp"
#include "GlobalNamespace/BloomPrePass.hpp"
#include "UnityEngine/Camera.hpp"
#include "UnityEngine/GameObject.hpp"
#include "BeatSaber/PerformancePresets/PerformancePreset.hpp"
#include "BeatSaber/PerformancePresets/QuestPerformanceSettings.hpp"
#include "BeatSaber/GameSettings/GraphicSettingsHandler.hpp"
#include "logging.hpp"


#include "FPSCounter.hpp"

inline modloader::ModInfo modInfo = {MOD_ID, VERSION, GIT_COMMIT}; // Stores the ID and version of our mod, and is sent to the modloader upon startup

using namespace GraphicsTweaks;

// Called at the early stages of game loading
GT_EXPORT_FUNC void setup(CModInfo& info) {
    info.id = MOD_ID;
    info.version = VERSION;
    info.version_long = GIT_COMMIT;
    modInfo.assign(info);
	
    INFO("Setting up GraphicsTweaks  config...");
    getGraphicsTweaksConfig().Init(modInfo);

    INFO("Completed setup!");
}

// Enable Phase Sync (latency reduction) by default
MAKE_HOOK_MATCH(OculusLoader_Initialize, &Unity::XR::Oculus::OculusLoader::Initialize, bool, Unity::XR::Oculus::OculusLoader* self) {
    // INFO("OculusLoader_Initialize hook called!");
    auto settings = self->GetSettings();
    settings->___PhaseSync = true;
    return OculusLoader_Initialize(self);
}

void GrabObjects() {
    auto scriptableObjects = UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::ScriptableObject*>();

    for (int i = 0; i < scriptableObjects->get_Length(); i++) {
        auto scriptableObject = scriptableObjects[i];
        auto name = scriptableObject->get_name();
        //DEBUG("Scriptable Object Name: {}", name);
        if(name == "NoPostProcessMainEffect") {
            INFO("Found NoPostProcessMainEffect!");
            auto noPostProcessMainEffect = reinterpret_cast<GlobalNamespace::MainEffectSO*>(scriptableObject);
            GraphicsTweaks::BloomData::noEffect = noPostProcessMainEffect;
        }
        if(name == "PyramidBloomMainEffect") {
            INFO("Found PyramidBloomMainEffect!");
            auto pyramidBloomMainEffect = reinterpret_cast<GlobalNamespace::MainEffectSO*>(scriptableObject);
            GraphicsTweaks::BloomData::hdBloomEffect = pyramidBloomMainEffect;
            GraphicsTweaks::BloomData::ldBloomEffect = UnityEngine::Object::Instantiate(pyramidBloomMainEffect);
            auto effect = reinterpret_cast<GlobalNamespace::PyramidBloomMainEffectSO*>(GraphicsTweaks::BloomData::ldBloomEffect.ptr());
            effect->____bloomTextureWidth /= 2.0f;
        }
        if(name == "MainCameraMainEffectContainer") {
            INFO("Found MainCameraMainEffectContainer!");
            auto mainEffectContainer = reinterpret_cast<GlobalNamespace::MainEffectContainerSO*>(scriptableObject);
            GraphicsTweaks::BloomData::mainEffectContainer = mainEffectContainer;
        }
    }
}

void GraphicsTweaks::PerformancePreset::ApplySettings() {
    bool isMainThread = BSML::MainThreadScheduler::CurrentThreadIsMainThread();
    if (!isMainThread) {
        INFO("Not on main thread, skipping PerformancePreset::ApplySettings");
        return;
    }

    auto preset = GraphicsTweaks::PerformancePreset::GetCustomPreset();
    if (!GraphicsTweaks::PerformancePreset::settingsApplicatorSO) {
      return ERROR("Failed to get settingsApplicatorSO!");
    }

    GraphicsTweaks::PerformancePreset::settingsApplicatorSO->ApplyPerformancePreset(preset, GlobalNamespace::SceneType::Undefined);
}


UnityW<BeatSaber::PerformancePresets::PerformancePreset> GraphicsTweaks::PerformancePreset::GetCustomPreset() {
    if (!GraphicsTweaks::PerformancePreset::customPreset) {
        auto preset = BeatSaber::PerformancePresets::PerformancePreset::New_ctor();
        preset->____presetName = "GraphicsTweaks Custom Preset";
        GraphicsTweaks::PerformancePreset::customPreset = preset;
        GraphicsTweaks::PerformancePreset::customPreset->____bloomPrePassTextureEffect = ::BeatSaber::PerformancePresets::BloomPrepassTextureEffectPreset::HD;

        auto questSettings = BeatSaber::PerformancePresets::QuestPerformanceSettings::New_ctor();
        preset->____questSettings = questSettings;
    }

    // Update the preset with the current settings
    auto preset = GraphicsTweaks::PerformancePreset::customPreset.ptr();

    // Set the foveated rendering level based on the current settings
    preset->____questSettings->____foveatedRenderingLevelGameplay = getGraphicsTweaksConfig().InGameFoveatedRenderingLevel.GetValue();
    preset->____questSettings->____foveatedRenderingLevelMenu = getGraphicsTweaksConfig().MenuFoveatedRenderingLevel.GetValue();

    preset->____smokeGraphics = getGraphicsTweaksConfig().SmokeQuality.GetValue() > 0;
    // Enable depth texture if smoke quality is high
    preset->____depthTexture = getGraphicsTweaksConfig().SmokeQuality.GetValue() > 1;

    preset->____maxShockwaveParticles = getGraphicsTweaksConfig().NumShockwaves.GetValue();
    preset->____burnMarkTrails = getGraphicsTweaksConfig().Burnmarks.GetValue();

    // Handle bloom quality
    int bloomQuality = getGraphicsTweaksConfig().BloomQuality.GetValue();
    switch (bloomQuality)
    {
        case 0:
            preset->____mainEffectGraphics = ::BeatSaber::PerformancePresets::MainEffectPreset::Off;
            break;
        case 1:
            preset->____mainEffectGraphics = ::BeatSaber::PerformancePresets::MainEffectPreset::Pyramid;
            break;
        case 2:
            preset->____mainEffectGraphics = ::BeatSaber::PerformancePresets::MainEffectPreset::Pyramid;
            break;
        default:
            preset->____mainEffectGraphics = ::BeatSaber::PerformancePresets::MainEffectPreset::Off;
    }


    auto mirrorQuality = getGraphicsTweaksConfig().Mirror.GetValue();
    switch (mirrorQuality)
    {
        case 0:
            preset->____mirrorGraphics = ::BeatSaber::PerformancePresets::MirrorQualityPreset::Off;
            break;
        case 1:
            preset->____mirrorGraphics = ::BeatSaber::PerformancePresets::MirrorQualityPreset::Fake;
            break;
        case 2:
            preset->____mirrorGraphics = ::BeatSaber::PerformancePresets::MirrorQualityPreset::RenderedLQ;
            break;
        case 3:
            preset->____mirrorGraphics = ::BeatSaber::PerformancePresets::MirrorQualityPreset::RenderedHQ;
            break;
        default:
            preset->____mirrorGraphics = ::BeatSaber::PerformancePresets::MirrorQualityPreset::Fake;
    }

    auto wallQuality = getGraphicsTweaksConfig().WallQuality.GetValue();
    switch (wallQuality)
    {
        case 0:
            preset->____obstaclesQuality = ::BeatSaber::PerformancePresets::ObstaclesQuality::ObstacleLW;
            break;
        case 1:
            preset->____obstaclesQuality = ::BeatSaber::PerformancePresets::ObstaclesQuality::TexturedObstacle;
            break;
        case 2:
            preset->____obstaclesQuality = ::BeatSaber::PerformancePresets::ObstaclesQuality::ObstacleHW;
            break;
        default:
            preset->____obstaclesQuality = ::BeatSaber::PerformancePresets::ObstaclesQuality::ObstacleLW;
    }

    // If the wall quality is ObstacleHW, enable screen displacement effects
    preset->____screenDisplacementEffects = wallQuality >= 2;
    

    return GraphicsTweaks::PerformancePreset::customPreset.ptr();
}


void GraphicsTweaks::BloomData::ApplySettings() {
    // if (!GraphicsTweaks::BloomData::noEffect || !GraphicsTweaks::BloomData::hdBloomEffect || !GraphicsTweaks::BloomData::ldBloomEffect) {
    //     GrabObjects();
    //     if (!GraphicsTweaks::BloomData::noEffect || !GraphicsTweaks::BloomData::hdBloomEffect || !GraphicsTweaks::BloomData::ldBloomEffect) {
    //         ERROR("Failed to get bloom effects!");
    //         return;
    //     }
    // }

    // GlobalNamespace::MainEffectSO* mainEffect = nullptr;

    // // Set the bloom settings to the ones we want
    // auto bloom = getGraphicsTweaksConfig().Bloom.GetValue();
    // bool hd = getGraphicsTweaksConfig().BloomQuality.GetValue() == 2;
    // if (bloom) {
    //     mainEffect = hd ? GraphicsTweaks::BloomData::hdBloomEffect.ptr() : GraphicsTweaks::BloomData::ldBloomEffect.ptr();
    // } else {
    //     mainEffect = GraphicsTweaks::BloomData::noEffect.ptr();
    // }

    // GraphicsTweaks::BloomData::mainEffectContainer->Init(mainEffect);
};

// Sabers burn marks
MAKE_HOOK_MATCH(MainSystemInit_Init, &GlobalNamespace::MainSystemInit::Init, void, GlobalNamespace::MainSystemInit* self, ::GlobalNamespace::SettingsApplicatorSO* settingsApplicator) {
    INFO("MainSystemInit_Init hook called!");
    MainSystemInit_Init(self, settingsApplicator);

    GrabObjects();

    // Save the settings applicator
    GraphicsTweaks::PerformancePreset::settingsApplicatorSO = settingsApplicator;

    GraphicsTweaks::PerformancePreset::ApplySettings();
    GraphicsTweaks::BloomData::ApplySettings();

    bool distortionsUsed = getGraphicsTweaksConfig().WallQuality.GetValue() == 2 || getGraphicsTweaksConfig().MenuShockwaves.GetValue() || getGraphicsTweaksConfig().GameShockwaves.GetValue();
    
    UnityEngine::QualitySettings::set_antiAliasing(distortionsUsed ? 0 : getGraphicsTweaksConfig().AntiAliasing.GetValue());
}

// Enable or disable shockwaves
MAKE_HOOK_MATCH(ConditionalActivation_Awake, &GlobalNamespace::ConditionalActivation::Awake, void, GlobalNamespace::ConditionalActivation* self) {
    DEBUG("ConditionalActivation_Awake hook called! {}", self->get_gameObject()->get_name());
    auto name = self->get_gameObject()->get_name();
    if(name == "ShockwaveEffect") {
        self->get_gameObject()->SetActive(getGraphicsTweaksConfig().GameShockwaves.GetValue());
    }

    if(name == "MenuShockwave") {
        self->get_gameObject()->SetActive(getGraphicsTweaksConfig().MenuShockwaves.GetValue());
    }

    // Disable fake glow    
    if((name == "FakeGlow0" || name == "FakeGlow1" || name == "ObstacleFakeGlow")) {
        self->get_gameObject()->SetActive(!getGraphicsTweaksConfig().Bloom.GetValue());
    }

    DEBUG("ConditionalActivation_Awake hook called! {}", self->get_gameObject()->get_name());



    if(name == "BigSmokePS") {
        self->get_gameObject()->SetActive(getGraphicsTweaksConfig().SmokeQuality.GetValue() > 0);
    }

    if(name == "SaberBurnMarksArea") {
        self->get_gameObject()->SetActive(getGraphicsTweaksConfig().Burnmarks.GetValue());
    }
}

MAKE_HOOK_MATCH(ShockwaveEffect_Start, &GlobalNamespace::ShockwaveEffect::Start, void, GlobalNamespace::ShockwaveEffect* self) {
    DEBUG("ShockwaveEffect_Start hook called!");
    ShockwaveEffect_Start(self);
    self->____shockwavePS->get_main().set_maxParticles(getGraphicsTweaksConfig().NumShockwaves.GetValue());
}

MAKE_HOOK_MATCH(GraphicsSettingsHandler_TryGetCurrentPerformancePreset, static_cast<bool (BeatSaber::GameSettings::GraphicSettingsHandler::*)(ByRef<::BeatSaber::PerformancePresets::PerformancePreset*>)>(&BeatSaber::GameSettings::GraphicSettingsHandler::TryGetCurrentPerformancePreset), bool, BeatSaber::GameSettings::GraphicSettingsHandler* self, ByRef<::BeatSaber::PerformancePresets::PerformancePreset*> currentPreset) {
    currentPreset = GraphicsTweaks::PerformancePreset::GetCustomPreset();
    return true;
}

// Not sure what this does, but it's a hook
MAKE_HOOK_MATCH(ConditionalMaterialSwitcher_Awake, &GlobalNamespace::ConditionalMaterialSwitcher::Awake, void, GlobalNamespace::ConditionalMaterialSwitcher* self) {
    DEBUG("ConditionalMaterialSwitcher_Awake hook called! {}", self->get_gameObject()->get_name());
    auto renderer = self->____renderer;
    auto material1 = self->____material1;
    renderer->set_sharedMaterial(material1);
}

//Force depth to on if using high quality smoke.
MAKE_HOOK_MATCH(DepthTextureController_OnPreRender, &GlobalNamespace::DepthTextureController::OnPreRender, void, GlobalNamespace::DepthTextureController* self) {
    self->SetShaderKeyword("DEPTH_TEXTURE_ENABLED", getGraphicsTweaksConfig().SmokeQuality.GetValue() > 1);
}

// MAKE_HOOK_MATCH(PyramidBloom_PreRender, &GlobalNamespace::PyramidBloomMainEffectSO::PreRender, void, GlobalNamespace::PyramidBloomMainEffectSO* self) {
//     // self->____bloomIntensity = 0.1f;
//     // self->____baseColorBoost = 1.0f;
//     // self->____baseColorBoostThreshold = 0.0f;
//     PyramidBloom_PreRender(self);
// };


// MAKE_HOOK_MATCH(
//     PyramidBloomRendererSO_RenderBloom, 
//     static_cast<void (GlobalNamespace::PyramidBloomRendererSO::*)(
//         ::UnityEngine::RenderTexture*, ::UnityEngine::RenderTexture*, float_t, float_t, float_t,
//         float_t, bool , bool, float_t, float_t,
//         float_t, float_t , ::GlobalNamespace::__PyramidBloomRendererSO__Pass ,
//         ::GlobalNamespace::__PyramidBloomRendererSO__Pass , ::GlobalNamespace::__PyramidBloomRendererSO__Pass ,
//         ::GlobalNamespace::__PyramidBloomRendererSO__Pass , bool , bool 

//     )>(&GlobalNamespace::PyramidBloomRendererSO::RenderBloom), 
//     void, 
//     GlobalNamespace::PyramidBloomRendererSO* self,
//     ::UnityEngine::RenderTexture* src, ::UnityEngine::RenderTexture* dest, float_t radius, float_t intensity, float_t autoExposureLimit,
//     float_t downIntensityOffset, bool uniformPyramidWeights, bool downsampleOnFirstPass, float_t pyramidWeightsParam, float_t alphaWeights,
//     float_t firstUpsampleBrightness, float_t finalUpsampleBrightness, ::GlobalNamespace::__PyramidBloomRendererSO__Pass preFilterPass,
//     ::GlobalNamespace::__PyramidBloomRendererSO__Pass downsamplePass, ::GlobalNamespace::__PyramidBloomRendererSO__Pass upsamplePass,
//     ::GlobalNamespace::__PyramidBloomRendererSO__Pass finalUpsamplePass, bool legacyAutoExposure, bool isScreenspaceEffect
// ) {
//     // DEBUG("PyramidBloomRendererSO_RenderBloom hook called!");
//     // DEBUG("radius: {}", radius);
//     // DEBUG("intensity: {}", intensity);
//     // DEBUG("autoExposureLimit: {}", autoExposureLimit);
//     // DEBUG("downIntensityOffset: {}", downIntensityOffset);
//     // DEBUG("uniformPyramidWeights: {}", uniformPyramidWeights);
//     // DEBUG("downsampleOnFirstPass: {}", downsampleOnFirstPass);
//     // DEBUG("pyramidWeightsParam: {}", pyramidWeightsParam);
//     // DEBUG("alphaWeights: {}", alphaWeights);
//     // DEBUG("firstUpsampleBrightness: {}", firstUpsampleBrightness);
//     // DEBUG("finalUpsampleBrightness: {}", finalUpsampleBrightness);
//     // DEBUG("legacyAutoExposure: {}", legacyAutoExposure);
//     // DEBUG("isScreenspaceEffect: {}", isScreenspaceEffect);

//     if (isScreenspaceEffect) {
//         // firstUpsampleBrightness = 1.0f;
//         // finalUpsampleBrightness = 1.0f;
//         intensity = 0.00001f;
//     }
//     // ;
//     self->____material->SetFloat(self->getStaticF__combineSrcID(), intensity);
//     PyramidBloomRendererSO_RenderBloom(self, src, dest, radius, intensity, autoExposureLimit, downIntensityOffset, uniformPyramidWeights, downsampleOnFirstPass, pyramidWeightsParam, alphaWeights, firstUpsampleBrightness, finalUpsampleBrightness, preFilterPass, downsamplePass, upsamplePass, finalUpsamplePass, legacyAutoExposure, isScreenspaceEffect);
// };



// MAKE_HOOK_MATCH(VisualEffectsController_HandleDepthTextureEnabledDidChange, &GlobalNamespace::VisualEffectsController::HandleDepthTextureEnabledDidChange, void, GlobalNamespace::VisualEffectsController* self) {
//     if(getGraphicsTweaksConfig().SmokeQuality.GetValue() > 1) {
//         self->____camera->set_depthTextureMode(UnityEngine::DepthTextureMode::Depth);
//     } else {
//         self->____camera->set_depthTextureMode(UnityEngine::DepthTextureMode::None);
//     }
// }

MAKE_HOOK_MATCH(
    GameplayCoreInstaller_InstallBindings,
    &GlobalNamespace::GameplayCoreInstaller::InstallBindings,
    void,
    GlobalNamespace::GameplayCoreInstaller* self
) {
    DEBUG("GameplayCoreInstaller_InstallBindings hook called!");
    using namespace GlobalNamespace;
    using namespace UnityEngine;

    GameplayCoreInstaller_InstallBindings(self);

    if (getGraphicsTweaksConfig().FpsCounter.GetValue()) {
        UnityW<FPSCounterUIController> fpsCounterUIController = Object::Instantiate(Resources::FindObjectsOfTypeAll<FPSCounterUIController*>()[0]);
        if (fpsCounterUIController->____fpsCounter) {
            fpsCounterUIController->____fpsCounter->set_enabled(true);
        }
    }
}

MAKE_HOOK_MATCH(MainFlowCoordinator_DidActivate, &GlobalNamespace::MainFlowCoordinator::DidActivate, void, GlobalNamespace::MainFlowCoordinator* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    MainFlowCoordinator_DidActivate(self, firstActivation, addedToHierarchy, screenSystemEnabling);
    DEBUG("MainFlowCoordinator_DidActivate");

    if(!GraphicsTweaks::FPSCounter::counter) {
        // Load the FPS counter
        self->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(GraphicsTweaks::FPSCounter::LoadBund()));
    }

}



// Called later on in the game loading - a good time to install function hooks
GT_EXPORT_FUNC void load() {
    INFO("Loading GraphicsTweaks...");
    il2cpp_functions::Init();
    custom_types::Register::AutoRegister();
    BSML::Init();

    

    INFO("Installing hooks...");
    // Phase Sync (latency reduction)
    INSTALL_HOOK(Logger, OculusLoader_Initialize);
    INSTALL_HOOK(Logger, MainSystemInit_Init);
    INSTALL_HOOK(Logger, ConditionalActivation_Awake);
    INSTALL_HOOK(Logger, ConditionalMaterialSwitcher_Awake);
    INSTALL_HOOK(Logger, ShockwaveEffect_Start);
    INSTALL_HOOK(Logger, DepthTextureController_OnPreRender);
    INSTALL_HOOK(Logger, GraphicsSettingsHandler_TryGetCurrentPerformancePreset);
    // INSTALL_HOOK(Logger, VisualEffectsController_HandleDepthTextureEnabledDidChange);
    INSTALL_HOOK(Logger, GameplayCoreInstaller_InstallBindings);
    INSTALL_HOOK(Logger, MainFlowCoordinator_DidActivate);

    // Debugging hooks
    // INSTALL_HOOK(Logger, PyramidBloom_PreRender);
    // INSTALL_HOOK(Logger, PyramidBloomRendererSO_RenderBloom);

    GraphicsTweaks::Hooks::VRRenderingParamsSetup();

    INFO("Installed all hooks!");
    BSML::Register::RegisterMainMenu<GraphicsTweaks::UI::GraphicsTweaksFlowCoordinator*>("<color=#D1ACFF>Graphics Tweaks", "Tweak your graphics");
    INFO("GraphicsTweaks loaded!");
}