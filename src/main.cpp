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
#include "Unity/XR/Oculus/OculusLoader.hpp"
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

#include "GlobalNamespace/SettingsApplicatorSO.hpp"
#include "BeatSaber/Settings/QualitySettings.hpp"
#include "GraphicsTweaksConfig.hpp"

#include "UnityEngine/Graphics.hpp"

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
MAKE_HOOK_MATCH(OculusLoader_Initialize,
                &Unity::XR::Oculus::OculusLoader::Initialize, bool,
                Unity::XR::Oculus::OculusLoader *self) {
  // INFO("OculusLoader_Initialize hook called!");
  auto settings = self->GetSettings();
  settings->___PhaseSync = true;
  return OculusLoader_Initialize(self);
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

  // Burn marks are broken in Quest, so disable them
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

  // If the wall quality is ObstacleHW, enable screen displacement effects
  preset->quality.screenDisplacementEffects =
      wallQuality >= 2 || getGraphicsTweaksConfig().MenuShockwaves.GetValue() ||
      getGraphicsTweaksConfig().GameShockwaves.GetValue();

  bool distortionsUsed =
      getGraphicsTweaksConfig().WallQuality.GetValue() == 2 ||
      getGraphicsTweaksConfig().MenuShockwaves.GetValue() ||
      getGraphicsTweaksConfig().GameShockwaves.GetValue();

  preset->quality.antiAliasingLevel =
      distortionsUsed ? 0 : getGraphicsTweaksConfig().AntiAliasing.GetValue();

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

  bool distortionsUsed =
      getGraphicsTweaksConfig().WallQuality.GetValue() == 2 ||
      getGraphicsTweaksConfig().MenuShockwaves.GetValue() ||
      getGraphicsTweaksConfig().GameShockwaves.GetValue();
  UnityEngine::QualitySettings::set_antiAliasing(
      distortionsUsed ? 0 : getGraphicsTweaksConfig().AntiAliasing.GetValue());
}

// Enable or disable shockwaves
MAKE_HOOK_MATCH(ConditionalActivation_Awake,
                &GlobalNamespace::ConditionalActivation::Awake, void,
                GlobalNamespace::ConditionalActivation *self) {
  auto name = self->get_gameObject()->get_name();
  // DEBUG("ConditionalActivation_Awake hook called! {}",
  //       self->get_gameObject()->get_name());

  // Disable fake glow
  if ((name == "FakeGlow0" || name == "FakeGlow1" ||
       name == "ObstacleFakeGlow")) {
    self->get_gameObject()->SetActive(
        !getGraphicsTweaksConfig().Bloom.GetValue());
  }

  if (name == "BigSmokePS") {
    self->get_gameObject()->SetActive(
        getGraphicsTweaksConfig().SmokeQuality.GetValue() > 0);
  }

  if (name == "SaberBurnMarksArea") {
    self->get_gameObject()->SetActive(
        getGraphicsTweaksConfig().Burnmarks.GetValue());
  }
}

// BURNMARK STUFF ////

MAKE_HOOK_MATCH(SaberBurnmarkArea_Start,
                &GlobalNamespace::SaberBurnMarkArea::Start, void,
                GlobalNamespace::SaberBurnMarkArea *self) {
  DEBUG("SaberBurnmarkArea_Start");
  SaberBurnmarkArea_Start(self);

  // Prevent processing if burnmarks are disabled
  bool isEnabled = getGraphicsTweaksConfig().Burnmarks.GetValue();
  if (!isEnabled) {return;}

  // Print some debug info
  DEBUG("IsRendererEnabled: {}", self->_renderer->get_enabled());

  if (self->_lineRenderers && self->_camera) {
    DEBUG("Line renderers count: {}", self->_lineRenderers.size());
    DEBUG("Line renderer 1 enabled: {}",
          self->_lineRenderers[0]->get_enabled());
    DEBUG("Line renderer 2 enabled: {}",
          self->_lineRenderers[1]->get_enabled());
    DEBUG("Camera enabled: {}", self->_camera->get_enabled());
  } else {
    DEBUG("No line renderers");
  }
  DEBUG("SaberBurnmarkArea_Start_end");
};

MAKE_HOOK_MATCH(SaberBurnmarkArea_OnDestroy,
                &GlobalNamespace::SaberBurnMarkArea::OnDestroy, void,
                GlobalNamespace::SaberBurnMarkArea *self) {
  DEBUG("SaberBurnmarkArea_OnDestroy");
  SaberBurnmarkArea_OnDestroy(self);
};

MAKE_HOOK_MATCH(SaberBurnmarkArea_OnEnable,
                &GlobalNamespace::SaberBurnMarkArea::OnEnable, void,
                GlobalNamespace::SaberBurnMarkArea *self) {
  DEBUG("SaberBurnmarkArea_OnEnable");

  if (self->_lineRenderers) {
    DEBUG("Line renderers count: {}", self->_lineRenderers.size());
    DEBUG("Line renderer 1 enabled: {}",
          self->_lineRenderers[0]->get_enabled());
    DEBUG("Line renderer 2 enabled: {}",
          self->_lineRenderers[1]->get_enabled());
  } else {
    DEBUG("No line renderers");
  }

  SaberBurnmarkArea_OnEnable(self);

  if (self->_lineRenderers) {
    DEBUG("Line renderers count: {}", self->_lineRenderers.size());
    DEBUG("Line renderer 1 enabled: {}",
          self->_lineRenderers[0]->get_enabled());
    DEBUG("Line renderer 2 enabled: {}",
          self->_lineRenderers[1]->get_enabled());
  } else {
    DEBUG("No line renderers");
  }
};

MAKE_HOOK_MATCH(SaberBurnmarkArea_OnDisable,
                &GlobalNamespace::SaberBurnMarkArea::OnDisable, void,
                GlobalNamespace::SaberBurnMarkArea *self) {
  DEBUG("SaberBurnmarkArea_OnDisable");
  if (self->_lineRenderers) {
    DEBUG("Line renderers count: {}", self->_lineRenderers.size());
    DEBUG("Line renderer 1 enabled: {}",
          self->_lineRenderers[0]->get_enabled());
    DEBUG("Line renderer 2 enabled: {}",
          self->_lineRenderers[1]->get_enabled());
  } else {
    DEBUG("No line renderers");
  }
  SaberBurnmarkArea_OnDisable(self);
  if (self->_lineRenderers) {
    DEBUG("Line renderers count: {}", self->_lineRenderers.size());
    DEBUG("Line renderer 1 enabled: {}",
          self->_lineRenderers[0]->get_enabled());
    DEBUG("Line renderer 2 enabled: {}",
          self->_lineRenderers[1]->get_enabled());
  } else {
    DEBUG("No line renderers");
  }
  DEBUG("SaberBurnmarkArea_OnDisableEnd");
};

MAKE_HOOK_MATCH(SaberBurnmarkArea_LateUpdate,
                &GlobalNamespace::SaberBurnMarkArea::LateUpdate, void,
                GlobalNamespace::SaberBurnMarkArea *self) {
                  
  if (!self->_sabers || self->_sabers.size() == 0) {
    return;
  }

  // Debugging stuff
  // bool prevValue1 = false;
  // bool prevValue2 = false;
  // bool prevCameraEnabled = false;

  // if (self->_lineRenderers && self->_camera) {
  //   auto liner1 = self->_lineRenderers[0];
  //   auto liner2 = self->_lineRenderers[1];
  //   auto camera = self->_camera;

  //   prevValue1 = liner1->get_enabled();
  //   prevValue2 = liner2->get_enabled();
  //   prevCameraEnabled = camera->get_enabled();
  // }

  //   DEBUG("SaberBurnmarkArea_LateUpdate");
  SaberBurnmarkArea_LateUpdate(self);

  // Prevent processing if burnmarks are disabled
  bool isEnabled = getGraphicsTweaksConfig().Burnmarks.GetValue();
  if (!isEnabled) {return;}

  // This for some reason does not run on quest
  if (self->_camera) {
    // DEBUG("Disable blit timer: {}", self->_disableBlitTimer);

    float deltaTime = UnityEngine::Time::get_deltaTime();

    if (self->_disableBlitTimer  < 6.0f) {
      // self->_disableBlitTimer += deltaTime;
      self->____burnMarksFadeOutStrength = 0.3f;

      // Render the burnmarks
      self->_camera->set_targetTexture(self->_renderTextures[1]);
      self->_renderer->get_sharedMaterial()->set_mainTexture(
          self->_renderTextures[1]);
      self->_fadeOutMaterial->set_mainTexture(self->_renderTextures[0]);
      self->_fadeOutMaterial->SetFloat(
          self->____fadeOutStrengthShaderPropertyID,
          std::max(0.0f, 1.0f - (deltaTime *
                                    self->____burnMarksFadeOutStrength)));

      // 2f seems to work well, needs to be framerate independent though
      self->_fadeOutMaterial->SetFloat(
          self->____fadeOutStrengthShaderPropertyID,
          2.0f);

      // Blit the burnmarks
      UnityEngine::Graphics::Blit(self->_renderTextures[0],
                                 self->_renderTextures[1], self->_fadeOutMaterial);
      // Swap the render textures
      auto renderTexture = self->_renderTextures[0];
      self->_renderTextures[0] = self->_renderTextures[1];
      self->_renderTextures[1] = renderTexture;
    }

    // Enable the camera if the line renderers are enabled (sabers are touching the platform)
    if (self->_lineRenderers[0]->get_enabled() ||
        self->_lineRenderers[1]->get_enabled()) {
      self->____camera->set_enabled(true);
      self->_disableBlitTimer = 0.0f;
    } else {
      self->____camera->set_enabled(false);
    }
  }

  // Debugging stuff
  // if (self->_lineRenderers && self->_camera) {
  //   auto liner1 = self->_lineRenderers[0];
  //   auto liner2 = self->_lineRenderers[1];

  //   if (prevValue1 != liner1->get_enabled()) {
  //     DEBUG("Line1 changed state {} -> {}", prevValue1, liner1->get_enabled());
  //   }
  //   if (prevValue2 != liner2->get_enabled()) {
  //     DEBUG("Line2 changed state {} -> {}", prevValue2, liner2->get_enabled());
  //   }
  //   if (prevCameraEnabled != self->_camera->get_enabled()) {
  //     DEBUG("Camera changed state {} -> {}", prevCameraEnabled,
  //           self->_camera->get_enabled());
  //   }
  // }
};

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

  if (getGraphicsTweaksConfig().FpsCounter.GetValue()) {
    UnityW<FPSCounterUIController> fpsCounterUIController = Object::Instantiate(
        Resources::FindObjectsOfTypeAll<FPSCounterUIController *>()[0]);
    if (fpsCounterUIController->____fpsCounter) {
      fpsCounterUIController->____fpsCounter->set_enabled(true);
    }
  }
}

MAKE_HOOK_MATCH(MainFlowCoordinator_DidActivate,
                &GlobalNamespace::MainFlowCoordinator::DidActivate, void,
                GlobalNamespace::MainFlowCoordinator *self,
                bool firstActivation, bool addedToHierarchy,
                bool screenSystemEnabling) {
  MainFlowCoordinator_DidActivate(self, firstActivation, addedToHierarchy,
                                  screenSystemEnabling);
  DEBUG("MainFlowCoordinator_DidActivate");

  // Do not load the FPS counter if it's not needed
  // if (getGraphicsTweaksConfig().FpsCounterAdvanced.GetValue() &&
  //     !GraphicsTweaks::FPSCounter::counter) {
  //   // Load the FPS counter
  //   self->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(
  //       GraphicsTweaks::FPSCounter::LoadBund()));
  // }
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
  // INSTALL_HOOK(Logger, DepthTextureController_OnPreRender);
  // INSTALL_HOOK(Logger,
  // GraphicsSettingsHandler_TryGetCurrentPerformancePreset);
  INSTALL_HOOK(Logger, SettingsApplicatorSO_ApplyGraphicSettings);
  INSTALL_HOOK(Logger, GameplayCoreInstaller_InstallBindings);
  INSTALL_HOOK(Logger, MainFlowCoordinator_DidActivate);

  // Debugging hooks
  // INSTALL_HOOK(Logger, PyramidBloom_PreRender);
  // INSTALL_HOOK(Logger, PyramidBloomRendererSO_RenderBloom);

  // Burnmark hooks
  INSTALL_HOOK(Logger, SaberBurnmarkArea_Start);
  INSTALL_HOOK(Logger, SaberBurnmarkArea_OnDestroy);
  // INSTALL_HOOK(Logger, SaberBurnmarkArea_OnEnable);
  // INSTALL_HOOK(Logger, SaberBurnmarkArea_OnDisable);
  INSTALL_HOOK(Logger, SaberBurnmarkArea_LateUpdate);

  GraphicsTweaks::Hooks::VRRenderingParamsSetup();

  INFO("Installed all hooks!");
  BSML::Register::RegisterMainMenu<
      GraphicsTweaks::UI::GraphicsTweaksFlowCoordinator *>(
      "<color=#D1ACFF>Graphics Tweaks", "Tweak your graphics");
  INFO("GraphicsTweaks loaded!");
}
