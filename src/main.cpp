#include "main.hpp"
#include "BeatSaber/Settings/QualitySettings.hpp"
#include "BeatSaber/Settings/QuestSettings.hpp"
#include "GlobalNamespace/ConditionalActivation.hpp"
#include "GlobalNamespace/ConditionalMaterialSwitcher.hpp"
#include "GlobalNamespace/DepthTextureController.hpp"
#include "GlobalNamespace/FPSCounter.hpp"
#include "GlobalNamespace/FPSCounterUIController.hpp"
#include "GlobalNamespace/FakeMirrorObjectsInstaller.hpp"
#include "GlobalNamespace/GameplayCoreInstaller.hpp"
#include "GlobalNamespace/MainCamera.hpp"
#include "GlobalNamespace/MainFlowCoordinator.hpp"
#include "GlobalNamespace/MainSystemInit.hpp"
#include "GlobalNamespace/MirrorRendererSO.hpp"
#include "GlobalNamespace/OVRManager.hpp"
#include "GlobalNamespace/OVROverlay.hpp"
#include "GlobalNamespace/OVRPassthroughLayer.hpp"
#include "GlobalNamespace/ObstacleMaterialSetter.hpp"
#include "GlobalNamespace/PerformanceVisualizer.hpp"
#include "GlobalNamespace/PyramidBloomMainEffectSO.hpp"
#include "GlobalNamespace/PyramidBloomRendererSO.hpp"
#include "GlobalNamespace/SaberBurnMarkArea.hpp"
#include "GlobalNamespace/SettingsApplicatorSO.hpp"
#include "GlobalNamespace/ShockwaveEffect.hpp"
#include "UI/GraphicsTweaksFlowCoordinator.hpp"
#include "UnityEngine/Camera.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Object.hpp"
#include "UnityEngine/QualitySettings.hpp"
#include "UnityEngine/Renderer.hpp"
#include "UnityEngine/Rendering/CommandBuffer.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/Time.hpp"
#include "Zenject/DiContainer.hpp"
#include "_config.hpp"
#include "bsml/shared/BSML.hpp"
#include "bsml/shared/BSML/MainThreadScheduler.hpp"
#include "logging.hpp"
#include <algorithm>
#include <cstddef>
#include "beatsaber-hook/shared/utils/il2cpp-functions.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "GlobalNamespace/MainSystemInit.hpp"
#include "GlobalNamespace/SettingsManager.hpp"
#include "GlobalNamespace/MirrorRendererSO.hpp"
#include "BeatSaber/Settings/SettingValidations.hpp"
#include "GlobalNamespace/SettingsApplicatorSO.hpp"
#include "BeatSaber/Settings/QualitySettings.hpp"
#include "GraphicsTweaksConfig.hpp"
#include "FPSCounter.hpp"
#include "basic_string.hpp"
#include "UnityEngine/Graphics.hpp"
#include "PlatformDetector.hpp"
#include "Utils.hpp"
#include "assets.hpp"
#include "GlobalNamespace/SettingsManager.hpp"

inline modloader::ModInfo modInfo = {
    MOD_ID, VERSION, GIT_COMMIT}; // Stores the ID and version of our mod, and
                                  // is sent to the modloader upon startup

using namespace GraphicsTweaks;

// Called at the early stages of game loading
GT_EXPORT_FUNC void setup(CModInfo &info) {
  info.id = MOD_ID;
  info.version = VERSION;
  info.version_long = GIT_COMMIT;
  modInfo.assign(info);

  INFO("Setting up GraphicsTweaks  config...");
  getGraphicsTweaksConfig().Init(modInfo);

  INFO("Completed setup!");
}

// Enable Phase Sync (latency reduction) by default
// TODO: Fix this hook
// MAKE_HOOK_MATCH(OculusLoader_Initialize,
//                 &Unity::XR::Oculus::OculusLoader::Initialize, bool,
//                 Unity::XR::Oculus::OculusLoader *self) {
//   // INFO("OculusLoader_Initialize hook called!");
//   auto settings = self->GetSettings();
//   settings->___PhaseSync = true;
//   return OculusLoader_Initialize(self);
// }

void ApplySettingsToPreset(::ByRef<::BeatSaber::Settings::Settings> preset) {
  bool isMainThread = BSML::MainThreadScheduler::CurrentThreadIsMainThread();
  if (!isMainThread) {
    INFO("Not on main thread, skipping ApplySettings");
    return;
  }

  if (GraphicsTweaks::IsInRender()) {
    INFO("Skipping ReApplySettings because we are in render state.");
    return;
  }

  // Set the foveated rendering level based on the current settings
  preset->quest.foveatedRenderingGameplay =
      getGraphicsTweaksConfig().InGameFoveatedRenderingLevel.GetValue();
  preset->quest.foveatedRenderingMenu =
      getGraphicsTweaksConfig().MenuFoveatedRenderingLevel.GetValue();

  preset->quality.smokeGraphics =
      getGraphicsTweaksConfig().SmokeQuality.GetValue() > 0;
  // Enable depth texture if smoke quality is high
  preset->quality.depthTexture =
      getGraphicsTweaksConfig().SmokeQuality.GetValue() > 1;

  preset->quality.maxShockwaveParticles =
      getGraphicsTweaksConfig().NumShockwaves.GetValue();

  preset->quality.burnMarkTrails =
      getGraphicsTweaksConfig().Burnmarks.GetValue();

  // Handle bloom quality
  int bloomQuality = getGraphicsTweaksConfig().BloomQuality.GetValue();
  switch (bloomQuality) {
  case 0:
    preset->quality.mainEffect =
        BeatSaber::Settings::QualitySettings::MainEffectOption::Off;
    break;
  case 1:
    preset->quality.mainEffect =
        BeatSaber::Settings::QualitySettings::MainEffectOption::Game;
    break;
  case 2:
    preset->quality.mainEffect =
        BeatSaber::Settings::QualitySettings::MainEffectOption::Game;
    break;
  default:
    preset->quality.mainEffect =
        BeatSaber::Settings::QualitySettings::MainEffectOption::Off;
  }

  auto mirrorQuality = getGraphicsTweaksConfig().Mirror.GetValue();
  switch (mirrorQuality) {
  case 0:
    preset->quality.mirror =
        BeatSaber::Settings::QualitySettings::MirrorQuality::Off;
    break;
  case 1:
    preset->quality.mirror =
        BeatSaber::Settings::QualitySettings::MirrorQuality::Low;
    break;
  case 2:
    preset->quality.mirror =
        BeatSaber::Settings::QualitySettings::MirrorQuality::Medium;
    break;
  case 3:
    preset->quality.mirror =
        BeatSaber::Settings::QualitySettings::MirrorQuality::High;
    break;
  default:
    preset->quality.mirror =
        BeatSaber::Settings::QualitySettings::MirrorQuality::Off;
  }

  auto wallQuality = getGraphicsTweaksConfig().WallQuality.GetValue();
  switch (wallQuality) {
  case 0:
    preset->quality.obstacles =
        BeatSaber::Settings::QualitySettings::ObstacleQuality::Low;
    break;
  case 1:
    preset->quality.obstacles =
        BeatSaber::Settings::QualitySettings::ObstacleQuality::Medium;
    break;
  case 2:
    preset->quality.obstacles =
        BeatSaber::Settings::QualitySettings::ObstacleQuality::High;
    break;
  default:
    preset->quality.obstacles =
        BeatSaber::Settings::QualitySettings::ObstacleQuality::Low;
  }

  bool distortionsUsed =
      getGraphicsTweaksConfig().WallQuality.GetValue() == 2 ||
      getGraphicsTweaksConfig().MenuShockwaves.GetValue() ||
      getGraphicsTweaksConfig().GameShockwaves.GetValue();

  if (isQuest1) {
    distortionsUsed = false;
  }

  // If the wall quality is ObstacleHW, enable screen displacement effects
  preset->quality.screenDisplacementEffects = distortionsUsed;
  
  preset->quality.antiAliasingLevel =
      distortionsUsed ? 0 : getGraphicsTweaksConfig().AntiAliasing.GetValue();
}


void GraphicsTweaks::PerformancePreset::ApplySettings() {
  bool isMainThread = BSML::MainThreadScheduler::CurrentThreadIsMainThread();
  if (!isMainThread) {
    INFO("Not on main thread, skipping PerformancePreset::ApplySettings");
    return;
  }

  if (GraphicsTweaks::IsInRender()) {
    INFO("Skipping ApplySettings because we are in render state.");
    return;
  }

  auto preset = GraphicsTweaks::PerformancePreset::GetCustomPreset();
  if (!GraphicsTweaks::PerformancePreset::settingsApplicatorSO) {
    return ERROR("Failed to get settingsApplicatorSO!");
  }

  auto settings = BeatSaber::Settings::Settings();
  settings.quality = preset->quality;
  settings.quest = preset->quest;

  GraphicsTweaks::PerformancePreset::settingsApplicatorSO->ApplyGraphicSettings(
      settings, GlobalNamespace::SceneType::Undefined);
}


BeatSaber::Settings::Settings *
GraphicsTweaks::PerformancePreset::GetCustomPreset() {
  // Update the preset with the current settings
  auto preset = GraphicsTweaks::PerformancePreset::customPreset;

  ApplySettingsToPreset(*preset);

  return GraphicsTweaks::PerformancePreset::customPreset;
}

// Sabers burn marks
MAKE_HOOK_MATCH(MainSystemInit_Init, &GlobalNamespace::MainSystemInit::Init,
                void, GlobalNamespace::MainSystemInit *self,
                ::GlobalNamespace::SettingsApplicatorSO *settingsApplicator) {
  INFO("MainSystemInit_Init hook called!");
  MainSystemInit_Init(self, settingsApplicator);

  // Save the settings applicator
  GraphicsTweaks::PerformancePreset::settingsApplicatorSO = settingsApplicator;
  auto preset = BeatSaber::Settings::Settings();
  preset.quality = self->_settingsManager->settings.quality;
  preset.quest = self->_settingsManager->settings.quest;

  GraphicsTweaks::PerformancePreset::customPreset =
      &self->_settingsManager->settings;

  GraphicsTweaks::PerformancePreset::ApplySettings();
}

MAKE_HOOK_MATCH(SettingsApplicatorSO_ApplyGraphicSettings,
                &GlobalNamespace::SettingsApplicatorSO::ApplyGraphicSettings,
                void, GlobalNamespace::SettingsApplicatorSO *self,
                ByRef<BeatSaber::Settings::Settings> settings,
                GlobalNamespace::SceneType sceneType) {
  INFO("SettingsApplicatorSO_ApplyGraphicSettings hook called!");
  SettingsApplicatorSO_ApplyGraphicSettings(self, settings, sceneType);

  if (GraphicsTweaks::IsInRender()) {
    INFO("Skipping SettingsApplicatorSO_ApplyGraphicSettings because we are in render state.");
    return;
  }


  bool distortionsUsed =
      getGraphicsTweaksConfig().WallQuality.GetValue() == 2 ||
      getGraphicsTweaksConfig().MenuShockwaves.GetValue() ||
      getGraphicsTweaksConfig().GameShockwaves.GetValue();

  if (isQuest1) {
    distortionsUsed = false;
  }

  UnityEngine::QualitySettings::set_antiAliasing(
      distortionsUsed ? 0 : getGraphicsTweaksConfig().AntiAliasing.GetValue());
}

// Enable or disable shockwaves
MAKE_HOOK_MATCH(ConditionalActivation_Awake,
                &GlobalNamespace::ConditionalActivation::Awake, void,
                GlobalNamespace::ConditionalActivation *self) {
  auto gameObject = self->get_gameObject();
  std::string name = gameObject->get_name();
  return;

  if (GraphicsTweaks::IsInRender()) {
    INFO("Skipping ConditionalActivation_Awake because we are in render state.");
    return;
  }
  // Disable fake glow
  if ((name == "FakeGlow0" || name == "FakeGlow1" ||
       name == "ObstacleFakeGlow")) {
    gameObject->SetActive(
        !getGraphicsTweaksConfig().Bloom.GetValue());
  }

  if (name == "BigSmokePS") {
    gameObject->SetActive(
        getGraphicsTweaksConfig().SmokeQuality.GetValue() > 0);
  }
}

//////////////////////

// Force depth to on if using high quality smoke.
// MAKE_HOOK_MATCH(DepthTextureController_OnPreRender,
// &GlobalNamespace::DepthTextureController::OnPreRender, void,
// GlobalNamespace::DepthTextureController* self) {
//     self("DEPTH_TEXTURE_ENABLED",
//     getGraphicsTweaksConfig().SmokeQuality.GetValue() > 1);
// }

// MAKE_HOOK_MATCH(PyramidBloom_PreRender,
// &GlobalNamespace::PyramidBloomMainEffectSO::PreRender, void,
// GlobalNamespace::PyramidBloomMainEffectSO* self) {
//     // self->____bloomIntensity = 0.1f;
//     // self->____baseColorBoost = 1.0f;
//     // self->____baseColorBoostThreshold = 0.0f;
//     PyramidBloom_PreRender(self);
// };

// MAKE_HOOK_MATCH(
//     PyramidBloomRendererSO_RenderBloom,
//     static_cast<void (GlobalNamespace::PyramidBloomRendererSO::*)(
//         ::UnityEngine::RenderTexture*, ::UnityEngine::RenderTexture*,
//         float_t, float_t, float_t, float_t, bool , bool, float_t, float_t,
//         float_t, float_t , ::GlobalNamespace::__PyramidBloomRendererSO__Pass
//         ,
//         ::GlobalNamespace::__PyramidBloomRendererSO__Pass ,
//         ::GlobalNamespace::__PyramidBloomRendererSO__Pass ,
//         ::GlobalNamespace::__PyramidBloomRendererSO__Pass , bool , bool

//     )>(&GlobalNamespace::PyramidBloomRendererSO::RenderBloom),
//     void,
//     GlobalNamespace::PyramidBloomRendererSO* self,
//     ::UnityEngine::RenderTexture* src, ::UnityEngine::RenderTexture* dest,
//     float_t radius, float_t intensity, float_t autoExposureLimit, float_t
//     downIntensityOffset, bool uniformPyramidWeights, bool
//     downsampleOnFirstPass, float_t pyramidWeightsParam, float_t alphaWeights,
//     float_t firstUpsampleBrightness, float_t finalUpsampleBrightness,
//     ::GlobalNamespace::__PyramidBloomRendererSO__Pass preFilterPass,
//     ::GlobalNamespace::__PyramidBloomRendererSO__Pass downsamplePass,
//     ::GlobalNamespace::__PyramidBloomRendererSO__Pass upsamplePass,
//     ::GlobalNamespace::__PyramidBloomRendererSO__Pass finalUpsamplePass, bool
//     legacyAutoExposure, bool isScreenspaceEffect
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
//     self->____material->SetFloat(self->getStaticF__combineSrcID(),
//     intensity); PyramidBloomRendererSO_RenderBloom(self, src, dest, radius,
//     intensity, autoExposureLimit, downIntensityOffset, uniformPyramidWeights,
//     downsampleOnFirstPass, pyramidWeightsParam, alphaWeights,
//     firstUpsampleBrightness, finalUpsampleBrightness, preFilterPass,
//     downsamplePass, upsamplePass, finalUpsamplePass, legacyAutoExposure,
//     isScreenspaceEffect);
// };

MAKE_HOOK_MATCH(GameplayCoreInstaller_InstallBindings,
                &GlobalNamespace::GameplayCoreInstaller::InstallBindings, void,
                GlobalNamespace::GameplayCoreInstaller *self) {
  DEBUG("GameplayCoreInstaller_InstallBindings hook called!");
  using namespace GlobalNamespace;
  using namespace UnityEngine;

  GameplayCoreInstaller_InstallBindings(self);

  bool isInRender = GraphicsTweaks::IsInRender();

  if (isInRender) {
    DEBUG("In render, skipping FPS counter installation");

    // Disable the Advanced FPS counter if it's enabled
    auto counter =  GraphicsTweaks::FPSCounter::counter;
    if (counter) {
      counter->SetActive(false);
    }
    return;
  }

  if (getGraphicsTweaksConfig().FpsCounter.GetValue()) {
    UnityW<FPSCounterUIController> fpsCounterUIController = Object::Instantiate(
        Resources::FindObjectsOfTypeAll<FPSCounterUIController *>()[0]);
    if (fpsCounterUIController->____fpsCounter) {
      fpsCounterUIController->____fpsCounter->set_enabled(true);
    }
  }
}

MAKE_HOOK_MATCH(SettingValidations_AdjustQuest1,
                &BeatSaber::Settings::SettingValidations::AdjustQuest1, void,
                ::ByRef<::BeatSaber::Settings::Settings> settings) {
  DEBUG("SettingValidations_AdjustQuest1 hook called!");
  SettingValidations_AdjustQuest1(settings);

  ApplySettingsToPreset(settings);
};

MAKE_HOOK_MATCH(SettingValidations_AdjustQuest2,
                &BeatSaber::Settings::SettingValidations::AdjustQuest2, void,
                ::ByRef<::BeatSaber::Settings::Settings> settings) {
  DEBUG("SettingValidations_AdjustQuest2 hook called!");
  SettingValidations_AdjustQuest2(settings);
  ApplySettingsToPreset(settings);
};
MAKE_HOOK_MATCH(SettingValidations_AdjustQuest3,
                &BeatSaber::Settings::SettingValidations::AdjustQuest3, void,
                ::ByRef<::BeatSaber::Settings::Settings> settings) {
  DEBUG("SettingValidations_AdjustQuest3 hook called!");
  SettingValidations_AdjustQuest3(settings);
  ApplySettingsToPreset(settings);
};

MAKE_HOOK_MATCH(SettingValidations_AdjustQuestPro,
                &BeatSaber::Settings::SettingValidations::AdjustQuestPro, void,
                ::ByRef<::BeatSaber::Settings::Settings> settings) {
  DEBUG("SettingValidations_AdjustQuestPro hook called!");
  SettingValidations_AdjustQuestPro(settings);
  ApplySettingsToPreset(settings);
};

MAKE_HOOK_MATCH(SettingValidations_AdjustStandalone,
                &BeatSaber::Settings::SettingValidations::AdjustStandalone, void,
                ::ByRef<::BeatSaber::Settings::Settings> settings,
                bool forceApplyQualityAll) {
  DEBUG("SettingValidations_AdjustStandalone hook called!");
  SettingValidations_AdjustStandalone(settings, forceApplyQualityAll);
  ApplySettingsToPreset(settings);
};


MAKE_HOOK_MATCH(SettingsManager_AdjustPlatformSettings,
                &GlobalNamespace::SettingsManager::AdjustPlatformSettings, void,
                ::ByRef< ::BeatSaber::Settings::Settings> settings, ::GlobalNamespace::HardwareCategory platform) {
  SettingsManager_AdjustPlatformSettings(settings, platform);

  DEBUG("SettingsManager_AdjustPlatformSettings hook called!");
  ApplySettingsToPreset(settings);
};

MAKE_HOOK_MATCH(MainFlowCoordinator_DidActivate,
                &GlobalNamespace::MainFlowCoordinator::DidActivate, void,
                GlobalNamespace::MainFlowCoordinator *self,
                bool firstActivation, bool addedToHierarchy,
                bool screenSystemEnabling) {
  MainFlowCoordinator_DidActivate(self, firstActivation, addedToHierarchy,
                                  screenSystemEnabling);
  DEBUG("MainFlowCoordinator_DidActivate");

  // Do not load the FPS counter if it's not needed
  if (getGraphicsTweaksConfig().FpsCounterAdvanced.GetValue() &&
      !GraphicsTweaks::FPSCounter::counter) {
    // Load the FPS counter
    self->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(
        GraphicsTweaks::FPSCounter::LoadBund()));
  }

  // If the FPS counter is loaded, set its active state
  if (GraphicsTweaks::FPSCounter::counter) {
    GraphicsTweaks::FPSCounter::counter->SetActive(
        getGraphicsTweaksConfig().FpsCounterAdvanced.GetValue());
  }
}

// TODO: Make more supportable
MAKE_HOOK(hCompileProgramImpl, nullptr, void,
    unsigned int& unknown,
    core::basic_string const& shaderName,
    core::basic_string const* vertex,
    core::basic_string& fragment,
    core::basic_string const& geometry,
    core::basic_string const& hull,
    core::basic_string const& domain,
    int& shader, int a, int * b, int *c, int* d) {

    auto shaderName_c = shaderName.c_str();
    // TODO: Figure out how to fix for mirror?
    if (strcmp(shaderName_c, "Custom/ParametricBoxFrameHD") == 0 && fragment.heap.size > 5400 && fragment.heap.size < 5500) {
        DEBUG("Patching ParametricBoxFrameHD shader");

        // DEBUG("Original Shader: {}", std::string((char*)fragment.heap.data, fragment.heap.size));
        memcpy((void*)fragment.heap.data, Assets::ParametricBoxFrameHD_shader.dataStart, Assets::ParametricBoxFrameHD_shader.data_length);
        fragment.heap.size = Assets::ParametricBoxFrameHD_shader.data_length;
    }

    // Vertex and Fragment check to make sure it only changes for the player and not mirror
    if (strcmp(shaderName_c, "Custom/SaberBlade") == 0 && fragment.heap.size == 2188 && vertex->heap.size == 5265) {
        DEBUG("Patching SaberBlade shader");
        // DEBUG("Original Shader: {}", std::string((char*)fragment.heap.data, fragment.heap.size));
        memcpy((void*)fragment.heap.data, Assets::SaberBlade_shader.dataStart, Assets::SaberBlade_shader.data_length);
        fragment.heap.size = Assets::SaberBlade_shader.data_length;
    }

    hCompileProgramImpl(unknown, shaderName, vertex, fragment, geometry, hull, domain, shader, a, b, c, d);
}

// Called later on in the game loading - a good time to install function hooks
GT_EXPORT_FUNC void load() {
  INFO("Loading GraphicsTweaks...");
  // TODO: Document how to get this address for the newer versions of unity  
  // Scary direct offset hook
  // void* jniOnUnload = dlsym(modloader_unity_handle, "JNI_OnUnload");
  // // GlslGpuProgramGLES::CompileProgramImpl
  // if (auto const CompileProgramImpl = static_cast<char*>(jniOnUnload) + 0x6E8EE0) {
  //     DEBUG("Found CompileProgramImpl at offset 0x6E8EE0");
  //     INSTALL_HOOK_DIRECT(Logger, hCompileProgramImpl, CompileProgramImpl);
  // }
      
  // DEBUG("Installed direct offset hooks!");
  il2cpp_functions::Init();
  custom_types::Register::AutoRegister();
  BSML::Init();

  DEBUG("Checking platform...");
  GraphicsTweaks::DetectPlatform();

  // Force Quest 1 for testing
  // GraphicsTweaks::isQuest1 = true;

  if (GraphicsTweaks::isQuest1) {
    INFO("Detected Quest 1! Hi!!");
  }

  DEBUG("Model: {}", GraphicsTweaks::model);
  DEBUG("Brand: {}", GraphicsTweaks::brand);

  INFO("Installing hooks...");
  // Phase Sync (latency reduction)
  // INSTALL_HOOK(Logger, OculusLoader_Initialize);
  INSTALL_HOOK(Logger, MainSystemInit_Init);
  INSTALL_HOOK(Logger, ConditionalActivation_Awake);
  // INSTALL_HOOK(Logger, DepthTextureController_OnPreRender);
  // INSTALL_HOOK(Logger,
  // GraphicsSettingsHandler_TryGetCurrentPerformancePreset);
  INSTALL_HOOK(Logger, SettingsApplicatorSO_ApplyGraphicSettings);
  INSTALL_HOOK(Logger, GameplayCoreInstaller_InstallBindings);
  INSTALL_HOOK(Logger, MainFlowCoordinator_DidActivate);

  INSTALL_HOOK(Logger, SettingValidations_AdjustQuest1);
  INSTALL_HOOK(Logger, SettingValidations_AdjustQuest2);
  INSTALL_HOOK(Logger, SettingValidations_AdjustQuest3);
  INSTALL_HOOK(Logger, SettingValidations_AdjustQuestPro);
  INSTALL_HOOK(Logger, SettingValidations_AdjustStandalone);


  INSTALL_HOOK(Logger, SettingsManager_AdjustPlatformSettings);

  // Debugging hooks
  // INSTALL_HOOK(Logger, PyramidBloom_PreRender);
  // INSTALL_HOOK(Logger, PyramidBloomRendererSO_RenderBloom);

  GraphicsTweaks::Hooks::VRRenderingParamsSetup();

  INFO("Installed all hooks!");
  BSML::Register::RegisterMainMenu<
      GraphicsTweaks::UI::GraphicsTweaksFlowCoordinator *>(
      "<color=#D1ACFF>Graphics Tweaks", "Tweak your graphics");
  INFO("GraphicsTweaks loaded!");
}
