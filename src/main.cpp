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
#include "GlobalNamespace/MainSystemInit.hpp"
#include "GlobalNamespace/MirrorRendererSO.hpp"
#include "GlobalNamespace/MirrorRendererGraphicsSettingsPresets.hpp"
#include "bsml/shared/BSML/MainThreadScheduler.hpp"
#include "bsml/shared/BSML/SharedCoroutineStarter.hpp"
#include "bsml/shared/BSML.hpp"
#include "UI/GraphicsTweaksFlowCoordinator.hpp"
#include "logging.hpp"
#include "GlobalNamespace/FPSCounter.hpp"
#include "GlobalNamespace/FPSCounterUIController.hpp"
#include "GlobalNamespace/GameplayCoreInstaller.hpp"
#include "GlobalNamespace/PerformanceVisualizer.hpp"


#include "Tayx/Graphy/GraphyManager.hpp"
#include "Tayx/Graphy/Fps/G_FpsMonitor.hpp"
#include "Tayx/Graphy/Ram/G_RamMonitor.hpp"
#include "Tayx/Graphy/Audio/G_AudioMonitor.hpp"
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
    INFO("OculusLoader_Initialize hook called!");
    auto settings = self->GetSettings();
    settings->___PhaseSync = true;
    return OculusLoader_Initialize(self);
}


void GraphicsTweaks::MirrorsData::ApplySettings(){
    if (!GraphicsTweaks::MirrorsData::mirrorRenderer) {
        return;
    }
    if (!GraphicsTweaks::MirrorsData::mirrorRendererGraphicsSettingsPresets) {
        return;
    }

    // Set the mirror renderer settings to the ones we want
    auto currentPreset = getGraphicsTweaksConfig().Mirror.GetValue();

    // Loop through presets and print them
    auto presets = GraphicsTweaks::MirrorsData::mirrorRendererGraphicsSettingsPresets->get_namedPresets();
    for (const auto& item : presets) {
        DEBUG("Preset: {}", item->get_presetNameLocalizationKey());
        //  item->ToString();
    }

    // Init the mirror renderer with the settings from the preset
    if (currentPreset >= GraphicsTweaks::MirrorsData::mirrorRendererGraphicsSettingsPresets->get_presets().size()) {
        currentPreset = 3;
    }
    auto presetObj = GraphicsTweaks::MirrorsData::mirrorRendererGraphicsSettingsPresets->get_presets()[currentPreset];
    GraphicsTweaks::MirrorsData::mirrorRenderer->Init(
        presetObj->___reflectLayers,
        presetObj->___stereoTextureWidth,
        presetObj->___stereoTextureHeight,
        presetObj->___monoTextureWidth,
        presetObj->___monoTextureHeight,
        presetObj->___maxAntiAliasing,
        presetObj->___enableBloomPrePassFog
    );
};

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

    // MirrorRendererSO
    // Save the MirrorRendererSO instance for later use (get graphics settings presets, etc.)
    GraphicsTweaks::MirrorsData::mirrorRenderer = self->____mirrorRenderer;
    GraphicsTweaks::MirrorsData::mirrorRendererGraphicsSettingsPresets = self->____mirrorRendererGraphicsSettingsPresets;

    // Apply the settings
    GraphicsTweaks::MirrorsData::ApplySettings();
   
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
    if((name == "FakeGlow0" || name == "FakeGlow1" || name == "ObstacleFakeGlow") && getGraphicsTweaksConfig().Bloom.GetValue()) {
        self->get_gameObject()->SetActive(false);
    }
   
    bool showAdvancedCounter = getGraphicsTweaksConfig().FpsCounterAdvanced.GetValue();
    if (FPSCounter::counter) {
        FPSCounter::counter->SetActive(showAdvancedCounter);
    }
    if(showAdvancedCounter && !GraphicsTweaks::FPSCounter::counter) {
        // Load the FPS counter
        BSML::SharedCoroutineStarter::get_instance()->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(GraphicsTweaks::FPSCounter::LoadBund()));
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
    auto renderer = self->____renderer;
    auto material1 = self->____material1;
    renderer->set_sharedMaterial(material1);
}

//Force depth to on if using high quality smoke.
MAKE_HOOK_MATCH(VisualEffectsController_OnPreRender, &GlobalNamespace::VisualEffectsController::OnPreRender, void, GlobalNamespace::VisualEffectsController* self) {
    self->SetShaderKeyword("DEPTH_TEXTURE_ENABLED", getGraphicsTweaksConfig().SmokeQuality.GetValue() > 1);
}

MAKE_HOOK_MATCH(VisualEffectsController_HandleDepthTextureEnabledDidChange, &GlobalNamespace::VisualEffectsController::HandleDepthTextureEnabledDidChange, void, GlobalNamespace::VisualEffectsController* self) {
    if(getGraphicsTweaksConfig().SmokeQuality.GetValue() > 1) {
        self->____camera->set_depthTextureMode(UnityEngine::DepthTextureMode::Depth);
    } else {
        self->____camera->set_depthTextureMode(UnityEngine::DepthTextureMode::None);
    }
}

MAKE_HOOK_MATCH(GraphyManager_Init, &Tayx::Graphy::GraphyManager::Init, void, Tayx::Graphy::GraphyManager* self) {
    if (self->m_keepAlive)
	{
		UnityEngine::Object::DontDestroyOnLoad(self->transform->root->gameObject);
	}
    self->m_fpsMonitor = self->GetComponentInChildren<Tayx::Graphy::Fps::G_FpsMonitor*>(true);
	self->m_fpsMonitor->Init();
	self->m_ramMonitor = self->GetComponentInChildren<Tayx::Graphy::Ram::G_RamMonitor*>(true);
	self->m_ramMonitor->Init();
    self->m_audioMonitor = self->GetComponentInChildren<Tayx::Graphy::Audio::G_AudioMonitor*>(true);
	self->m_audioMonitor->Init();
}


MAKE_HOOK_MATCH(
    GameplayCoreInstaller_InstallBindings,
    &GlobalNamespace::GameplayCoreInstaller::InstallBindings,
    void,
    GlobalNamespace::GameplayCoreInstaller* self
) {
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



// Called later on in the game loading - a good time to install function hooks
GT_EXPORT_FUNC void load() {
    INFO("Loading GraphicsTweaks...");
    il2cpp_functions::Init();
    custom_types::Register::AutoRegister();
    BSML::Init();

    

    INFO("Installing hooks...");
    // Phase Sync (latency reduction)
    INSTALL_HOOK(Logger, OculusLoader_Initialize);
    // Bloom (expensive, not sure if it's worth it?)

    INSTALL_HOOK(Logger, MainSystemInit_Init);
    INSTALL_HOOK(Logger, ConditionalActivation_Awake);
    INSTALL_HOOK(Logger, ConditionalMaterialSwitcher_Awake);
    INSTALL_HOOK(Logger, ObstacleMaterialSetter_SetCoreMaterial);
    INSTALL_HOOK(Logger, ShockwaveEffect_Start);
    INSTALL_HOOK(Logger, VisualEffectsController_OnPreRender);
    INSTALL_HOOK(Logger, VisualEffectsController_HandleDepthTextureEnabledDidChange);
    INSTALL_HOOK(Logger, GameplayCoreInstaller_InstallBindings);
    // INSTALL_HOOK(Logger, GraphyManager_Init);

    GraphicsTweaks::Hooks::VRRenderingParamsSetup();

    INFO("Installed all hooks!");

    BSML::Register::RegisterMainMenu<GraphicsTweaks::UI::GraphicsTweaksFlowCoordinator*>("<color=#f0cdff>Graphics Tweaks", "Tweak your graphics");
    INFO("Registered settings menu!");
    INFO("GraphicsTweaks loaded!");
}