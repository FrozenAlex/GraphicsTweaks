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
#include "UnityEngine/Rendering/CommandBuffer.hpp"
#include "UnityEngine/RenderTexture.hpp"
#include "UnityEngine/RenderTextureFormat.hpp"
#include "UnityEngine/RenderTextureReadWrite.hpp"
#include "UnityEngine/VRTextureUsage.hpp"
#include "UnityEngine/ParticleSystem.hpp"
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
#include "GlobalNamespace/ShockwaveEffect.hpp"
#include "GlobalNamespace/VisualEffectsController.hpp"
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

    UnityEngine::QualitySettings::set_antiAliasing(0);
}

// Enable or disable shockwaves
MAKE_HOOK_MATCH(ConditionalActivation_Awake, &GlobalNamespace::ConditionalActivation::Awake, void, GlobalNamespace::ConditionalActivation* self) {
    DEBUG("ConditionalActivation_Awake hook called! {}", self->get_gameObject()->get_name());
    auto name = self->get_gameObject()->get_name();
    if(name == "ShockwaveEffect" && getGraphicsTweaksConfig().GameShockwaves.GetValue()) {
        self->get_gameObject()->SetActive(true);
    } else if(name == "ShockwaveEffect" && !getGraphicsTweaksConfig().GameShockwaves.GetValue()) {
        self->get_gameObject()->SetActive(false);
    }

    if(name == "MenuShockwave" && getGraphicsTweaksConfig().MenuShockwaves.GetValue()) {
        self->get_gameObject()->SetActive(true);
    } else if(name == "MenuShockwave" && !getGraphicsTweaksConfig().MenuShockwaves.GetValue()) {
        self->get_gameObject()->SetActive(false);
    }

    //Disable fake glow
    if(std::find(name.begin(), name.end(), "Fake") != name.end()) {
        self->get_gameObject()->SetActive(false);
    }
}

MAKE_HOOK_MATCH(ShockwaveEffect_Start, &GlobalNamespace::ShockwaveEffect::Start, void, GlobalNamespace::ShockwaveEffect* self) {
    DEBUG("ShockwaveEffect_Start hook called!");
    ShockwaveEffect_Start(self);
    self->____shockwavePS->get_main().set_maxParticles(getGraphicsTweaksConfig().NumShockwaves.GetValue());
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

    switch (quality.value__)
    {
    case 0:
    case 1:
        renderer->set_sharedMaterial(self->____texturedCoreMaterial);
        DEBUG("TEXTUREDDDDD");
        break;
    case 2:
        renderer->set_sharedMaterial(self->____lwCoreMaterial);
        DEBUG("TRANSPARENTTTTTT");
        break;
    case 3:
        renderer->set_sharedMaterial(self->____hwCoreMaterial);
        DEBUG("DISTORTEDDDDD");
        break;
    default:
        renderer->set_sharedMaterial(self->____lwCoreMaterial);
        DEBUG("TRANSPARENTTTTTT");
        break;
    }
}

// Not sure what this does, but it's a hook
MAKE_HOOK_MATCH(ConditionalMaterialSwitcher_Awake, &GlobalNamespace::ConditionalMaterialSwitcher::Awake, void, GlobalNamespace::ConditionalMaterialSwitcher* self) {
    DEBUG("ConditionalMaterialSwitcher_Awake hook called!");
    auto renderer = self->____renderer;
    auto material1 = self->____material1;
    renderer->set_sharedMaterial(material1);
}

//Force depth to on if using high quality smoke.
MAKE_HOOK_MATCH(VisualEffectsController_OnPreRender, &GlobalNamespace::VisualEffectsController::OnPreRender, void, GlobalNamespace::VisualEffectsController* self) {
    DEBUG("VisualEffectsController_OnPreRender hook called!");
    self->SetShaderKeyword("DEPTH_TEXTURE_ENABLED", getGraphicsTweaksConfig().SmokeQuality.GetValue() > 1);
}

MAKE_HOOK_MATCH(VisualEffectsController_HandleDepthTextureEnabledDidChange, &GlobalNamespace::VisualEffectsController::HandleDepthTextureEnabledDidChange, void, GlobalNamespace::VisualEffectsController* self) {
    DEBUG("VisualEffectsController_HandleDepthTextureEnabledDidChange hook called!");
    if(getGraphicsTweaksConfig().SmokeQuality.GetValue() > 1) {
        self->____camera->set_depthTextureMode(UnityEngine::DepthTextureMode::Depth);
    } else {
        self->____camera->set_depthTextureMode(UnityEngine::DepthTextureMode::None);
    }
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
    INSTALL_HOOK(logger, ConditionalActivation_Awake);
    INSTALL_HOOK(logger, ConditionalMaterialSwitcher_Awake);
    INSTALL_HOOK(logger, ObstacleMaterialSetter_SetCoreMaterial);
    INSTALL_HOOK(logger, ShockwaveEffect_Start);
    INSTALL_HOOK(logger, VisualEffectsController_OnPreRender);
    INSTALL_HOOK(logger, VisualEffectsController_HandleDepthTextureEnabledDidChange);

    INFO("Installed all hooks!");

    BSML::Register::RegisterMainMenu<GraphicsTweaks::UI::GraphicsTweaksFlowCoordinator*>("<color=#f0cdff>Graphics Tweaks", "Tweak your graphics");
    INFO("Registered settings menu!");
    INFO("GraphicsTweaks loaded!");
}