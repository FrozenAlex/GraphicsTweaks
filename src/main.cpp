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
#include "UnityEngine/Material.hpp"
#include "UnityEngine/Mesh.hpp"
#include "UnityEngine/Object.hpp"
#include "UnityEngine/ParticleSystem.hpp"
#include "UnityEngine/ParticleSystemRenderer.hpp"
#include "UnityEngine/QualitySettings.hpp"
#include "UnityEngine/Renderer.hpp"
#include "UnityEngine/Rendering/CommandBuffer.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/Time.hpp"
#include "UnityEngine/Transform.hpp"
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
#include "FPSCounter.hpp"
#include "basic_string.hpp"
#include "UnityEngine/Graphics.hpp"
#include "PlatformDetector.hpp"
#include "Utils.hpp"
#include "assets.hpp"

inline modloader::ModInfo modInfo = {
    MOD_ID, VERSION, GIT_COMMIT}; // Stores the ID and version of our mod, and
                                  // is sent to the modloader upon startup

using namespace GraphicsTweaks;

static bool PatchStringInPlace(
    core::basic_string const& source,
    void const* replacement,
    std::size_t replacementSize
) {
    auto& mutableSource =
        const_cast<core::basic_string&>(source);

    if (mutableSource.mode ==
        core::basic_string::StorageMode::External) {
        WARNING("Refusing to patch external Unity string storage");
        return false;
    }

    const std::size_t oldSize = mutableSource.size();

    if (replacementSize > oldSize) {
        WARNING(
            "Replacement shader is too large: {} > {}",
            replacementSize,
            oldSize
        );
        return false;
    }

    char* destination = mutableSource.mutable_data_unsafe();

    if (!destination) {
        ERROR("Unity string has a null data pointer");
        return false;
    }

    std::memcpy(destination, replacement, replacementSize);
    destination[replacementSize] = '\0';
    mutableSource.set_size_unsafe(replacementSize);

    return true;
}

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

  // Keep turning on saber burn marks area even if the game is in render state
  if (name == "SaberBurnMarksArea") {
    gameObject->SetActive(
        getGraphicsTweaksConfig().Burnmarks.GetValue());
  }

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

// BURNMARK STUFF ////
MAKE_HOOK_MATCH(SaberBurnmarkArea_LateUpdate,
                &GlobalNamespace::SaberBurnMarkArea::LateUpdate, void,
                GlobalNamespace::SaberBurnMarkArea *self) {
                  
  if (!self->_sabers || self->_sabers.size() == 0) {
    return;
  }

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

static bool UsesPanicWindow(UnityEngine::Material* material) {
  if (!material) {
    return false;
  }

  auto texture = material->get_mainTexture();
  return texture && texture->get_name() == "PanicWindow";
}

static custom_types::Helpers::Coroutine InspectPanicWindowParticleRenderer() {
  using namespace UnityEngine;

  // Custom level assets may not exist when installers run. Scan twice per
  // second for up to five minutes, and stop once matching renderers exist.
  for (int scan = 0; scan < 600; ++scan) {
    auto renderers =
        Resources::FindObjectsOfTypeAll<ParticleSystemRenderer*>();
    bool found = false;

    for (auto renderer : renderers) {
      if (!renderer) {
        continue;
      }

      auto material = renderer->get_sharedMaterial();
      auto trailMaterial = renderer->get_trailMaterial();
      const bool mainMatch = UsesPanicWindow(material);
      const bool trailMatch = UsesPanicWindow(trailMaterial);
      if (!mainMatch && !trailMatch) {
        continue;
      }

      found = true;
      auto transform = renderer->get_transform();
      auto gameObject = renderer->get_gameObject();
      auto particleSystem = renderer->GetComponent<ParticleSystem*>();
      const auto bounds = renderer->get_bounds();
      const auto boundsCenter = bounds.get_center();
      const auto boundsSize = bounds.get_size();
      const auto position = transform->get_position();
      const auto localScale = transform->get_localScale();
      const auto lossyScale = transform->get_lossyScale();

      float startSizeMultiplier = 0.0f;
      int scalingMode = -1;
      int simulationSpace = -1;
      int particleCount = -1;
      if (particleSystem) {
        auto main = particleSystem->get_main();
        startSizeMultiplier = main.get_startSizeMultiplier();
        scalingMode = static_cast<int>(main.get_scalingMode());
        simulationSpace = static_cast<int>(main.get_simulationSpace());
        particleCount = particleSystem->get_particleCount();
      }

      auto mesh = renderer->get_mesh();
      INFO(
          "PanicWindow particles: object '{}', instance {}, texture use {}, "
          "renderMode {}, alignment {}, sortMode {}, maxParticleSize {}, "
          "mesh '{}' ({} meshes), particles {}, startSizeMultiplier {}, "
          "scalingMode {}, simulationSpace {}, position ({}, {}, {}), "
          "localScale ({}, {}, {}), lossyScale ({}, {}, {}), bounds center "
          "({}, {}, {}), bounds size ({}, {}, {})",
          gameObject ? std::string(gameObject->get_name()) : "<null>",
          renderer->GetInstanceID(),
          mainMatch && trailMatch ? "main+trail" :
              (mainMatch ? "main" : "trail"),
          static_cast<int>(renderer->get_renderMode()),
          static_cast<int>(renderer->get_alignment()),
          static_cast<int>(renderer->get_sortMode()),
          renderer->get_maxParticleSize(),
          mesh ? std::string(mesh->get_name()) : "<none>",
          renderer->get_meshCount(),
          particleCount,
          startSizeMultiplier,
          scalingMode,
          simulationSpace,
          position.x, position.y, position.z,
          localScale.x, localScale.y, localScale.z,
          lossyScale.x, lossyScale.y, lossyScale.z,
          boundsCenter.x, boundsCenter.y, boundsCenter.z,
          boundsSize.x, boundsSize.y, boundsSize.z
      );
    }

    if (found) {
      co_return;
    }

    for (int frame = 0; frame < 60; ++frame) {
      co_yield nullptr;
    }
  }

  WARNING("No ParticleSystemRenderer using PanicWindow was found");
}

MAKE_HOOK_MATCH(GameplayCoreInstaller_InstallBindings,
                &GlobalNamespace::GameplayCoreInstaller::InstallBindings, void,
                GlobalNamespace::GameplayCoreInstaller *self) {
  DEBUG("GameplayCoreInstaller_InstallBindings hook called!");
  using namespace GlobalNamespace;
  using namespace UnityEngine;

  GameplayCoreInstaller_InstallBindings(self);
  self->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(
      InspectPanicWindowParticleRenderer()));

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

static std::size_t ReplaceAll(
    std::string& source,
    std::string_view from,
    std::string_view to
) {
    std::size_t count = 0;
    std::size_t position = 0;

    while ((position = source.find(from, position)) != std::string::npos) {
        source.replace(position, from.size(), to);
        position += to.size();
        ++count;
    }

    return count;
}

struct PatchedShader {
    std::string source;
    std::size_t replacements;
};

static core::basic_string MakeExternalString(std::string& source) {
    core::basic_string result{};

    result.allocated.data = source.data();
    result.allocated.unknown08 = source.size();
    result.allocated.size = source.size();
    result.allocated.unknown18 = 0;
    result.mode = core::basic_string::StorageMode::External;

    return result;
}

static PatchedShader MaxPrecision(core::basic_string const& source) {
    PatchedShader result{
        std::string(source.c_str(), source.size()),
        0
    };

    // GLSL ES precision qualifiers
    result.replacements += ReplaceAll(result.source, "lowp", "highp");
    result.replacements += ReplaceAll(result.source, "mediump", "highp");

    // HLSL/Unity matrix types — longest first
    result.replacements += ReplaceAll(result.source, "fixed4x4", "float4x4");
    result.replacements += ReplaceAll(result.source, "fixed3x3", "float3x3");
    result.replacements += ReplaceAll(result.source, "fixed2x2", "float2x2");
    result.replacements += ReplaceAll(result.source, "half4x4",  "float4x4");
    result.replacements += ReplaceAll(result.source, "half3x3",  "float3x3");
    result.replacements += ReplaceAll(result.source, "half2x2",  "float2x2");

    // HLSL/Unity vector types
    result.replacements += ReplaceAll(result.source, "fixed4", "float4");
    result.replacements += ReplaceAll(result.source, "fixed3", "float3");
    result.replacements += ReplaceAll(result.source, "fixed2", "float2");
    result.replacements += ReplaceAll(result.source, "half4",  "float4");
    result.replacements += ReplaceAll(result.source, "half3",  "float3");
    result.replacements += ReplaceAll(result.source, "half2",  "float2");

    // Scalar types
    result.replacements += ReplaceAll(result.source, "min16float", "float");
    result.replacements += ReplaceAll(result.source, "min10float", "float");
    result.replacements += ReplaceAll(result.source, "fixed",      "float");
    result.replacements += ReplaceAll(result.source, "half",       "float");

    // Integer precision variants
    result.replacements += ReplaceAll(result.source, "min16uint", "uint");
    result.replacements += ReplaceAll(result.source, "min16int",  "int");
    result.replacements += ReplaceAll(result.source, "min12int",  "int");

    return result;
}

// TODO: Make more supportable
MAKE_HOOK(
    hCompileProgramImpl,
    nullptr,
    std::uint32_t,
    unsigned int& program,
    core::basic_string const& shaderName,
    core::basic_string const& vertex,
    core::basic_string const& fragment,
    core::basic_string const& geometry,
    core::basic_string const& hull,
    core::basic_string const& domain,
    void* channelMask,
    int a,
    unsigned int b,
    int* c,
    int* d
) {
    auto patchedVertex = MaxPrecision(vertex);
    auto patchedFragment = MaxPrecision(fragment);
    auto patchedGeometry = MaxPrecision(geometry);
    auto patchedHull = MaxPrecision(hull);
    auto patchedDomain = MaxPrecision(domain);

    auto vertexString = MakeExternalString(patchedVertex.source);
    auto fragmentString = MakeExternalString(patchedFragment.source);
    auto geometryString = MakeExternalString(patchedGeometry.source);
    auto hullString = MakeExternalString(patchedHull.source);
    auto domainString = MakeExternalString(patchedDomain.source);

    const std::size_t totalReplacements =
        patchedVertex.replacements +
        patchedFragment.replacements +
        patchedGeometry.replacements +
        patchedHull.replacements +
        patchedDomain.replacements;

    DEBUG(
        "Shader precision {}: {} replacements (vertex: {}, fragment: {}, geometry: {}, hull: {}, domain: {})",
        std::string(shaderName.c_str(), shaderName.size()),
        totalReplacements,
        patchedVertex.replacements,
        patchedFragment.replacements,
        patchedGeometry.replacements,
        patchedHull.replacements,
        patchedDomain.replacements
    );

    return hCompileProgramImpl(
        program,
        shaderName,
        vertexString,
        fragmentString,
        geometryString,
        hullString,
        domainString,
        channelMask,
        a,
        b,
        c,
        d
    );
}

// Called later on in the game loading - a good time to install function hooks
GT_EXPORT_FUNC void load() {
  INFO("Loading GraphicsTweaks...");
  // TODO: Document how to get this address for the newer versions of unity  
  // Scary direct offset hook
  void* jniOnUnload = dlsym(modloader_unity_handle, "JNI_OnUnload");
  // GlslGpuProgramGLES::CompileProgramImpl
  if (auto const CompileProgramImpl = static_cast<char*>(jniOnUnload) + 0x6D9B4C) {
      DEBUG("Found CompileProgramImpl at offset 0x6D9B4C");

      INSTALL_HOOK_DIRECT(Logger, hCompileProgramImpl, CompileProgramImpl);
  }
      
  DEBUG("Installed direct offset hooks!");
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
  GraphicsTweaks::Hooks::DepthBufferPrecision();
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
  INSTALL_HOOK(Logger, SaberBurnmarkArea_LateUpdate);

  GraphicsTweaks::Hooks::CameraClipPlanes();
  GraphicsTweaks::Hooks::VRRenderingParamsSetup();

  INFO("Installed all hooks!");
  BSML::Register::RegisterMainMenu<
      GraphicsTweaks::UI::GraphicsTweaksFlowCoordinator *>(
      "<color=#D1ACFF>Graphics Tweaks", "Tweak your graphics");
  INFO("GraphicsTweaks loaded!");
}
