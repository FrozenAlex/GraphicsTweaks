#include "main.hpp"
#include "_config.hpp"
#include "Unity/XR/Oculus/OculusLoader.hpp"
#include "GlobalNamespace/MainSystemInit.hpp"
#include "GlobalNamespace/ConditionalActivation.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/Renderer.hpp"
#include "UnityEngine/Object.hpp"
#include "UnityEngine/Camera.hpp"
#include "UnityEngine/ScriptableObject.hpp"
#include "UnityEngine/XR/XRSettings.hpp"
#include "UnityEngine/QualitySettings.hpp"
#include "UnityEngine/CameraClearFlags.hpp"
#include "UnityEngine/Renderer.hpp"
#include "GlobalNamespace/MainSettingsModelSO.hpp"
#include "GlobalNamespace/ObservableVariableSO_1.hpp"
#include "GlobalNamespace/MainCamera.hpp"
#include "GlobalNamespace/IFileStorage.hpp"
#include "GlobalNamespace/ConditionalMaterialSwitcher.hpp"
#include "GlobalNamespace/AlwaysVisibleQuad.hpp"
#include "GlobalNamespace/OVRManager.hpp"
#include "GlobalNamespace/OVRPassthroughLayer.hpp"
#include "GlobalNamespace/OVROverlay.hpp"
#include "GlobalNamespace/BloomPrePass.hpp"
#include "GlobalNamespace/OVRPlugin.hpp"
#include "GlobalNamespace/ObstacleMaterialSetter.hpp"
#include "bsml/shared/BSML/MainThreadScheduler.hpp"
#include "bsml/shared/BSML.hpp"
#include "UI/GraphicsTweaksFlowCoordinator.hpp"
#include "logging.hpp"

inline modloader::ModInfo modInfo = {MOD_ID, VERSION, GIT_COMMIT}; // Stores the ID and version of our mod, and is sent to the modloader upon startup


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
    INFO("OculusLoader_Initialize hook called!");
    auto settings = self->GetSettings();
    settings->___PhaseSync = true;
    return OculusLoader_Initialize(self);
}


// Sabers burn marks
MAKE_HOOK_MATCH(MainSystemInit_Init, &GlobalNamespace::MainSystemInit::Init, void, GlobalNamespace::MainSystemInit* self) {
    INFO("MainSystemInit_Init hook called!");
   
    if (getGraphicsTweaksConfig().Bloom.GetValue()) {
        auto scriptableObjects = UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::ScriptableObject*>();

        for (int i = 0; i < scriptableObjects->get_Length(); i++) {

            auto scriptableObject = scriptableObjects[i];
            auto csName = scriptableObject->get_name();
            auto name = csName;
            DEBUG("Scriptable Object Name: {}", name);
            if (name == "PyramidBloomMainEffect") {
                INFO("Found PyramidBloomMainEffect!");
                auto mainEffectGraphicsSettingsPresets = self->____mainEffectGraphicsSettingsPresets;
                auto presets = mainEffectGraphicsSettingsPresets->____presets;
                presets->_values[0]->___mainEffect = reinterpret_cast<GlobalNamespace::MainEffectSO*>(scriptableObject);
                break;
            }
        }
    }
    MainSystemInit_Init(self);
}

// Set the ConditionalActivation to always be active and set the eyeTextureResolutionScale to 1.0 and antiAliasing to 0
MAKE_HOOK_MATCH(ConditionalActivation_Awake, &GlobalNamespace::ConditionalActivation::Awake, void, GlobalNamespace::ConditionalActivation* self) {
    DEBUG("ConditionalActivation_Awake hook called!");
    self->get_gameObject()->SetActive(true);
    UnityEngine::XR::XRSettings::set_eyeTextureResolutionScale(1.0f);
    // ConditionalActivation_Awake(self);
}

// Graphics Tweaks
MAKE_HOOK_MATCH(MainSettingsModelSO_SetSaveConfig, &GlobalNamespace::MainSettingsModelSO::SetSaveConfig, void, GlobalNamespace::MainSettingsModelSO* self, GlobalNamespace::MainSettingsModelSO::Config* config) {
    DEBUG("MainSettingsModelSO_SetSaveConfig hook called!");
    MainSettingsModelSO_SetSaveConfig(self, config);
    auto vrResolutionScale = self->___vrResolutionScale;
    auto smokeGraphicsSettings = self->___smokeGraphicsSettings;
    auto depthTextureEnabled = self->___depthTextureEnabled;
    auto mainEffectGraphicsSettings = self->___mainEffectGraphicsSettings;
    auto mirrorGraphicsSettings = self->___mirrorGraphicsSettings;
    auto screenDisplacementEffectsEnabled = self->___screenDisplacementEffectsEnabled;
    auto antiAliasingLevel = self->___antiAliasingLevel;
    auto enableFPSCounter = self->___enableFPSCounter;
    auto maxShockwaveParticles = self->___maxShockwaveParticles;
    auto targetFramerate = self->___targetFramerate;
    auto obstaclesQuality = self->___obstaclesQuality;
    
    bool value = depthTextureEnabled->get_value();

    DEBUG("Depth Texture Enabled before: {}", value);

    vrResolutionScale->set_value(1.0f);
    targetFramerate->set_value(120);
    // Improves smoke quality
    depthTextureEnabled->set_value(true);
    enableFPSCounter->set_value(false);
    smokeGraphicsSettings->set_value(true);
    antiAliasingLevel->set_value(getGraphicsTweaksConfig().AntiAliasing.GetValue());
    screenDisplacementEffectsEnabled->set_value(getGraphicsTweaksConfig().ScreenDistortion.GetValue());
    maxShockwaveParticles->set_value(getGraphicsTweaksConfig().Shockwave.GetValue());
    mirrorGraphicsSettings->set_value(getGraphicsTweaksConfig().Mirror.GetValue());
    
    GlobalNamespace::OVRPlugin::set_cpuLevel(getGraphicsTweaksConfig().CpuLevel.GetValue());
    GlobalNamespace::OVRPlugin::set_gpuLevel(getGraphicsTweaksConfig().GpuLevel.GetValue());

    DEBUG("Depth Texture Enabled after: {}", depthTextureEnabled->get_value());
}

MAKE_HOOK_MATCH(ObstacleMaterialSetter_SetCoreMaterial, &GlobalNamespace::ObstacleMaterialSetter::SetCoreMaterial, void, GlobalNamespace::ObstacleMaterialSetter* self, UnityEngine::Renderer* renderer, BeatSaber::PerformancePresets::ObstaclesQuality obstaclesQuality) {
    DEBUG("ObstacleMaterialSetter_SetCoreMaterial hook called! >_<");

    BeatSaber::PerformancePresets::ObstaclesQuality quality;

    DEBUG("config quaality {} :3", getGraphicsTweaksConfig().WallQuality.GetValue());

    switch(getGraphicsTweaksConfig().WallQuality.GetValue()) {
        case 0:
            quality = ::BeatSaber::PerformancePresets::ObstaclesQuality::ObstacleLW;
            break;
        case 1:
            quality = ::BeatSaber::PerformancePresets::ObstaclesQuality::TexturedObstacle;
            break;
        case 2:
            quality = ::BeatSaber::PerformancePresets::ObstaclesQuality::ObstacleHW;
            break;
    }

    DEBUG("quaality {} :3", quality.value__);

    ObstacleMaterialSetter_SetCoreMaterial(self, renderer, quality);
}

// Not sure what this does, but it's a hook
MAKE_HOOK_MATCH(ConditionalMaterialSwitcher_Awake, &GlobalNamespace::ConditionalMaterialSwitcher::Awake, void, GlobalNamespace::ConditionalMaterialSwitcher* self) {
    DEBUG("ConditionalMaterialSwitcher_Awake hook called!");
    auto renderer = self->____renderer;
    auto material1 = self->____material1;
    renderer->set_sharedMaterial(material1);
    // Don't call the original method, as it's not needed?
    // ConditionalMaterialSwitcher_Awake(self);
}

bool firstTimeInit = false;

MAKE_HOOK_MATCH(AlwaysVisibleQuad_OnEnable, &GlobalNamespace::AlwaysVisibleQuad::OnEnable, void, GlobalNamespace::AlwaysVisibleQuad* self) {
    DEBUG("AlwaysVisibleQuad_OnEnable hook called!");
    auto gameObject = self->get_gameObject();
    gameObject->SetActive(false);

}

MAKE_HOOK_MATCH(MainCamera_Awake, &GlobalNamespace::MainCamera::Awake, void, GlobalNamespace::MainCamera* self) {
    MainCamera_Awake(self);

    if (!firstTimeInit) {
        auto ovrManagerGO = UnityEngine::GameObject::New_ctor("OVRManager");
        UnityEngine::Object::DontDestroyOnLoad(ovrManagerGO);
        ovrManagerGO->SetActive(false);
        auto ovrManager = ovrManagerGO->AddComponent<GlobalNamespace::OVRManager*>();
        ovrManager->___useRecommendedMSAALevel = false;
        // ovrManager->___isInsightPassthroughEnabled = true;
        // ovrManager->set_trackingOriginType(GlobalNamespace::__OVRManager__TrackingOrigin::FloorLevel);
        // ovrManager->SetSpaceWarp(true);
        ovrManager->set_cpuLevel(4);
        ovrManager->set_gpuLevel(4);
        ovrManagerGO->SetActive(true);

        firstTimeInit = true;
        DEBUG("Initialized OVRManager!");
    }

    // auto mainCamera = self->get_camera();
    // auto mainCameraGO = mainCamera->get_gameObject();

    // // Bloom prepass, not sure if this is needed (it looks kinda bad with it on?)
    // // mainCameraGO->GetComponent<GlobalNamespace::BloomPrePass*>()->set_enabled(false);

    // auto backgroundColor = UnityEngine::Color(
    //     0.0f,
    //     0.0f,
    //     0.0f,
    //     0.0f
    // );

    // mainCamera->set_clearFlags(UnityEngine::CameraClearFlags::SolidColor);
    // mainCamera->set_backgroundColor(backgroundColor);

    // auto ovrPassthroughLayer = mainCameraGO->AddComponent<GlobalNamespace::OVRPassthroughLayer*>();
    // ovrPassthroughLayer->___overlayType = GlobalNamespace::OVROverlay::OverlayType::Underlay;
    // ovrPassthroughLayer->___textureOpacity_ = 0.1f;
}

// Called later on in the game loading - a good time to install function hooks
GT_EXPORT_FUNC void load() {
    INFO("Loading GraphicsTweaks...");
    il2cpp_functions::Init();
    custom_types::Register::AutoRegister();
    BSML::Init();

    auto logger = Paper::ConstLoggerContext("GraphicsTweaks");

    INFO("Installing hooks...");
    // Phase Sync (latency reduction)
    INSTALL_HOOK(logger, OculusLoader_Initialize);
    // Bloom (expensive, not sure if it's worth it?)
    INSTALL_HOOK(logger, MainSystemInit_Init);
    INSTALL_HOOK(logger, MainSettingsModelSO_SetSaveConfig);
    INSTALL_HOOK(logger, ConditionalActivation_Awake);
    INSTALL_HOOK(logger, ConditionalMaterialSwitcher_Awake);
    INSTALL_HOOK(logger, ObstacleMaterialSetter_SetCoreMaterial);
    
    // Passthrough
    // INSTALL_HOOK(logger, MainCamera_Awake);
    // INSTALL_HOOK(logger, AlwaysVisibleQuad_OnEnable);

    INFO("Installed all hooks!");


    // BSML::Register::RegisterMenuButton<
    BSML::Register::RegisterSettingsMenu<GraphicsTweaks::UI::GraphicsTweaksFlowCoordinator*>("GraphicsTweaks");
    INFO("Registered settings menu!");
    INFO("GraphicsTweaks loaded!");
}