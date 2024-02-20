#include "main.hpp"
#include "_config.hpp"
#include "Unity/XR/Oculus/OculusLoader.hpp"
#include "GlobalNamespace/MainSystemInit.hpp"
#include "GlobalNamespace/ConditionalActivation.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/Object.hpp"
#include "UnityEngine/Camera.hpp"
#include "UnityEngine/ScriptableObject.hpp"
#include "UnityEngine/XR/XRSettings.hpp"
#include "UnityEngine/QualitySettings.hpp"
#include "UnityEngine/CameraClearFlags.hpp"
#include "GlobalNamespace/MainSettingsModelSO.hpp"
#include "GlobalNamespace/ISaveData.hpp"
#include "GlobalNamespace/ObservableVariableSO_1.hpp"
#include "GlobalNamespace/MainCamera.hpp"
#include "GlobalNamespace/ConditionalMaterialSwitcher.hpp"
#include "GlobalNamespace/AlwaysVisibleQuad.hpp"
#include "GlobalNamespace/OVRManager.hpp"
#include "GlobalNamespace/OVRPassthroughLayer.hpp"
#include "GlobalNamespace/OVROverlay.hpp"
#include "GlobalNamespace/BloomPrePass.hpp"

inline modloader::ModInfo modInfo = {MOD_ID, VERSION, GIT_COMMIT}; // Stores the ID and version of our mod, and is sent to the modloader upon startup

// Returns a logger, useful for printing debug messages
Paper::ConstLoggerContext<15UL> getLogger() {
    static auto fastContext = Paper::Logger::WithContext<MOD_ID>();
    return fastContext;
}

// Returns a logger, useful for printing debug messages (only used for hooks)
Logger& getLoggerOld() {
    static auto* logger = new Logger(modInfo, LoggerOptions(false, true));
    return *logger;
}


// Called at the early stages of game loading
GT_EXPORT_FUNC void setup(CModInfo& info) {
    info.id = MOD_ID;
    info.version = VERSION;
    info.version_long = GIT_COMMIT;
    modInfo.assign(info);
	
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


// Set the main effect to the PyramidBloomMainEffectSO (bloom effect on sabers?)
MAKE_HOOK_MATCH(MainSystemInit_Init, &GlobalNamespace::MainSystemInit::Init, void, GlobalNamespace::MainSystemInit* self) {
    INFO("MainSystemInit_Init hook called!");
   
    auto scriptableObjects = UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::ScriptableObject*>();

    for (int i = 0; i < scriptableObjects->get_Length(); i++) {

        auto scriptableObject = scriptableObjects[i];
        auto csName = scriptableObject->get_name();
        auto name = to_utf8(csstrtostr(csName));
        DEBUG("Scriptable Object Name: {}", name.c_str());
        if (name == "PyramidBloomMainEffect") {
            INFO("Found PyramidBloomMainEffect!");
            auto mainEffectGraphicsSettingsPresets = self->____mainEffectGraphicsSettingsPresets;
            auto presets = mainEffectGraphicsSettingsPresets->____presets;
            presets->_values[0]->___mainEffect = reinterpret_cast<GlobalNamespace::MainEffectSO*>(scriptableObject);
            break;
        }
    }
    MainSystemInit_Init(self);
}

// Set the ConditionalActivation to always be active and set the eyeTextureResolutionScale to 1.0 and antiAliasing to 0
MAKE_HOOK_MATCH(ConditionalActivation_Awake, &GlobalNamespace::ConditionalActivation::Awake, void, GlobalNamespace::ConditionalActivation* self) {
    self->get_gameObject()->SetActive(true);
    UnityEngine::XR::XRSettings::set_eyeTextureResolutionScale(1.0f);
    // ConditionalActivation_Awake(self);
}

// Graphics Tweaks
MAKE_HOOK_MATCH(MainSettingsModelSO_Load, &GlobalNamespace::MainSettingsModelSO::Load, void, GlobalNamespace::MainSettingsModelSO* self, ::GlobalNamespace::ISaveData* saveData, bool forced) {
    MainSettingsModelSO_Load(self, saveData, forced);

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

    obstaclesQuality->set_value(GlobalNamespace::ObstaclesQuality::TexturedObstacle);
    vrResolutionScale->set_value(1.0f);
    targetFramerate->set_value(120);
    depthTextureEnabled->set_value(false);
    enableFPSCounter->set_value(true);
    smokeGraphicsSettings->set_value(true);
    antiAliasingLevel->set_value(1);
    screenDisplacementEffectsEnabled->set_value(false);
    maxShockwaveParticles->set_value(0);
    mirrorGraphicsSettings->set_value(0);

    DEBUG("Depth Texture Enabled after: {}", depthTextureEnabled->get_value());

}

// Not sure what this does, but it's a hook
MAKE_HOOK_MATCH(ConditionalMaterialSwitcher_Awake, &GlobalNamespace::ConditionalMaterialSwitcher::Awake, void, GlobalNamespace::ConditionalMaterialSwitcher* self) {
    auto renderer = self->____renderer;
    auto material1 = self->____material1;
    renderer->set_sharedMaterial(material1);
    // Don't call the original method, as it's not needed?
    // ConditionalMaterialSwitcher_Awake(self);
}

bool firstTimeInit = false;

MAKE_HOOK_MATCH(AlwaysVisibleQuad_OnEnable, &GlobalNamespace::AlwaysVisibleQuad::OnEnable, void, GlobalNamespace::AlwaysVisibleQuad* self) {
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
        ovrManager->___isInsightPassthroughEnabled = true;
        ovrManager->set_trackingOriginType(GlobalNamespace::__OVRManager__TrackingOrigin::FloorLevel);
        ovrManagerGO->SetActive(true);
        firstTimeInit = true;
        DEBUG("Initialized OVRManager!");
    }

    auto mainCamera = self->get_camera();
    auto mainCameraGO = mainCamera->get_gameObject();

    // Bloom prepass, not sure if this is needed (it looks kinda bad with it on?)
    // mainCameraGO->GetComponent<GlobalNamespace::BloomPrePass*>()->set_enabled(false);

    auto backgroundColor = UnityEngine::Color(
        0.0f,
        0.0f,
        0.0f,
        0.0f
    );

    mainCamera->set_clearFlags(UnityEngine::CameraClearFlags::SolidColor);
    mainCamera->set_backgroundColor(backgroundColor);

    auto ovrPassthroughLayer = mainCameraGO->AddComponent<GlobalNamespace::OVRPassthroughLayer*>();
    ovrPassthroughLayer->___overlayType = GlobalNamespace::OVROverlay::OverlayType::Underlay;
    ovrPassthroughLayer->___textureOpacity_ = 0.1f;
}

// Called later on in the game loading - a good time to install function hooks
GT_EXPORT_FUNC void load() {
    il2cpp_functions::Init();
    

    INFO("Installing hooks...");
    // Phase Sync (latency reduction)
    INSTALL_HOOK(getLoggerOld(), OculusLoader_Initialize);
    // Bloom (expensive, not sure if it's worth it?)
    INSTALL_HOOK(getLoggerOld(), MainSystemInit_Init);
    INSTALL_HOOK(getLoggerOld(), MainSettingsModelSO_Load);
    INSTALL_HOOK(getLoggerOld(), ConditionalActivation_Awake);
    INSTALL_HOOK(getLoggerOld(), ConditionalMaterialSwitcher_Awake);
    
    // Passthrough
    // INSTALL_HOOK(getLoggerOld(), MainCamera_Awake);
    // INSTALL_HOOK(getLoggerOld(), AlwaysVisibleQuad_OnEnable);

    INFO("Installed all hooks!");
    
    custom_types::Register::AutoRegister();
}