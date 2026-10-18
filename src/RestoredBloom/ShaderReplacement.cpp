#include "RestoredBloom/ShaderReplacement.hpp"
#include "RestoredBloom/UIShaderDiagnostics.hpp"
#include "RestoredBloom/GlowWallShaderCoverage.hpp"
#include "FPSCounter.hpp"
#include "bsml/shared/BSML/SharedCoroutineStarter.hpp"
#include "GraphicsTweaksConfig.hpp"
#include "assets.hpp"
#include "logging.hpp"
#include "beatsaber-hook/shared/hooking.hpp"
#include "UnityEngine/AssetBundle.hpp"
#include "UnityEngine/HideFlags.hpp"
#include "UnityEngine/Material.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/Rendering/LocalKeyword.hpp"
#include "UnityEngine/Shader.hpp"
#ifdef GRAPHICSTWEAKS_SHADER_GPU_CHECK
#include "UnityEngine/GL.hpp"
#include "UnityEngine/Color.hpp"
#include "UnityEngine/Rect.hpp"
#include "UnityEngine/RenderTexture.hpp"
#include "UnityEngine/RenderTextureFormat.hpp"
#include "UnityEngine/RenderTextureReadWrite.hpp"
#include "UnityEngine/Texture2D.hpp"
#include "UnityEngine/TextureFormat.hpp"
#endif
#include "UnityEngine/SceneManagement/Scene.hpp"
#include "UnityEngine/SceneManagement/SceneManager.hpp"
#include "UnityEngine/SceneManagement/LoadSceneMode.hpp"
#include <array>
#include <algorithm>
#include <span>
#include <string_view>
#include <vector>

namespace GraphicsTweaks::ShaderReplacement {
namespace {
    struct Entry {
        std::string_view sourceName, assetName;
        std::span<const uint8_t> bytes;
        UnityW<UnityEngine::AssetBundle> bundle;
        UnityW<UnityEngine::Shader> replacement, original;
        bool attempted = false;
    };
    auto& Entries() {
        static std::array<Entry, 14> entries = {{
            {"Custom/SimpleLit", "assets/shaders/simplelit.shader", Assets::simplelit_bloom_bund},
            {"Custom/SaberBlade", "assets/shaders/saberblade-bloom.shader", Assets::saberblade_bloom_bund},
            {"Custom/Glowing Pointer", "assets/shaders/pointer-bloom.shader", Assets::pointer_bloom_bund},
            {"Custom/Glowing", "assets/shaders/glowing-bloom.shader", Assets::glowing_bloom_bund},
            {"Custom/GlowingInstancedHD", "assets/shaders/glowing-instanced-hd-bloom.shader", Assets::glowing_instanced_hd_bloom_bund},
            {"Custom/GlowingInstancedLW", "assets/shaders/glowing-instanced-lw-bloom.shader", Assets::glowing_instanced_lw_bloom_bund},
            {"Custom/ParametricBoxFrameHD", "assets/shaders/wall-frame-hd-bloom.shader", Assets::wall_frame_hd_bloom_bund},
            {"Custom/ParametricBoxFrameLW", "assets/shaders/wall-frame-lw-bloom.shader", Assets::wall_frame_lw_bloom_bund},
            {"Custom/Parametric3SliceSprite", "assets/shaders/parametric-sprite-bloom.shader", Assets::parametric_sprite_bloom_bund},
            {"Custom/OpaqueNeonLight", "assets/shaders/opaque-neon-bloom.shader", Assets::opaque_neon_bloom_bund},
            {"Custom/TransparentNeonLight", "assets/shaders/transparent-neon-bloom.shader", Assets::transparent_neon_bloom_bund},
            {"Custom/ParametricBoxFakeGlow", "assets/shaders/box-fake-glow-bloom.shader", Assets::box_fake_glow_bloom_bund},
            {"Custom/NoteHD", "assets/shaders/note-hd-bloom.shader", Assets::note_hd_bloom_bund},
            {"Custom/CustomParticles", "assets/shaders/particles-bloom.shader", Assets::particles_bloom_bund}
        }};
        return entries;
    }
    constexpr std::array<std::string_view, 9> globals = {
        "STEREO_MULTIVIEW_ON", "INSTANCING_ON", "LIGHTMAP_ON",
        "UNITY_ASSUME_UNIFORM_SCALING", "ACES_TONE_MAPPING", "ENABLE_BLOOM_FOG",
        "NOTE_VERTEX_DISTORTION", "MAIN_EFFECT_ENABLED", "DEPTH_TEXTURE_ENABLED"
    };

    // Every rendered variant in each replacement has a MAIN_EFFECT_ENABLED twin,
    // so Unity selects the twin of whatever variant the original shader would
    // use. Coverage only records which keyword sets match a proven set exactly.
    bool Proven(UnityEngine::Material* material, std::string_view sourceName) {
        if (std::none_of(glowWallCoverage.begin(), glowWallCoverage.end(),
                         [sourceName](auto& coverage) { return coverage.shader == sourceName; })) return true;
        // shaderKeywords also contains obsolete serialized names. Only declared,
        // enabled local keywords can select a variant of the current shader.
        std::vector<std::string> enabled;
        for (auto keyword : material->get_enabledKeywords()) {
            std::string name = keyword.get_name();
            if (std::find(globals.begin(), globals.end(), name) == globals.end()) enabled.push_back(name);
        }
        std::sort(enabled.begin(), enabled.end());
        std::string signature;
        for (auto& name : enabled) { if (!signature.empty()) signature += '|'; signature += name; }
        return std::any_of(glowWallCoverage.begin(), glowWallCoverage.end(),
            [&](auto& coverage) { return coverage.shader == sourceName && coverage.features == signature; });
    }

#ifdef GRAPHICSTWEAKS_SHADER_GPU_CHECK
    void CheckGlowGPU(UnityEngine::Shader* shader) {
        using namespace UnityEngine;
        static std::vector<Shader*> checked;
        if (std::find(checked.begin(), checked.end(), shader) != checked.end()) return;
        checked.push_back(shader);
        auto shaderName = std::string(shader->get_name());
        std::array<std::string_view, 3> keys = {"MAIN_EFFECT_ENABLED", "STEREO_MULTIVIEW_ON", "INSTANCING_ON"};
        std::array<bool, 3> states;
        for (int i = 0; i < 3; i++) { states[i] = Shader::IsKeywordEnabled(keys[i]); Shader::DisableKeyword(keys[i]); }
        auto previous = RenderTexture::get_active();
        auto target = RenderTexture::GetTemporary(8, 8, 0, RenderTextureFormat::ARGB32, RenderTextureReadWrite::Linear);
        auto pixels = Texture2D::New_ctor(8, 8, TextureFormat::RGBA32, false);
        auto material = Material::New_ctor(shader);
        material->SetColor("_Color", Color(0.2f, 0.3f, 0.4f, 1.0f));
        material->SetFloat("_FadeStartNormalizedDistance", 0.5f);
        material->SetFloat("_BaseColorBoost", 1.0f);
        material->SetFloat("_BaseColorBoostThreshold", 0.0f);
        if (shaderName == "GraphicsTweaks/GlowingBloom") material->EnableKeyword("_CUTOUT_NONE");
        for (bool boost : {false, true}) for (bool main : {false, true}) {
            if (boost) material->EnableKeyword("_ENABLE_MAIN_EFFECT_WHITE_BOOST");
            else material->DisableKeyword("_ENABLE_MAIN_EFFECT_WHITE_BOOST");
            if (boost) material->EnableKeyword("_WHITEBOOSTTYPE_MAINEFFECT");
            else material->DisableKeyword("_WHITEBOOSTTYPE_MAINEFFECT");
            if (main) { material->EnableKeyword("MAIN_EFFECT_ENABLED"); Shader::EnableKeyword("MAIN_EFFECT_ENABLED"); }
            else { material->DisableKeyword("MAIN_EFFECT_ENABLED"); Shader::DisableKeyword("MAIN_EFFECT_ENABLED"); }
            RenderTexture::set_active(target.ptr());
            GL::Clear(true, true, Color(0, 0, 0, 0));
            GL::PushMatrix(); GL::LoadOrtho();
            bool pass = material->SetPass(0);
            GL::Begin(7); GL::Color(Color(0, 0, 0, 1));
            GL::Vertex3(0, 0, 0); GL::Vertex3(0, 1, 0); GL::Vertex3(1, 1, 0); GL::Vertex3(1, 0, 0);
            GL::End(); GL::PopMatrix();
            pixels->ReadPixels(Rect(0, 0, 8, 8), 0, 0, false);
            auto color = pixels->GetPixel(4, 4);
            INFO("{} GPU check: boost {}, main {}, pass {}, RGBA {} {} {} {}", shaderName, boost, main, pass, color.r, color.g, color.b, color.a);
        }
        RenderTexture::set_active(previous ? previous.ptr() : nullptr);
        RenderTexture::ReleaseTemporary(target.ptr());
        Object::Destroy(material); Object::Destroy(pixels);
        for (int i = 0; i < 3; i++) {
            if (states[i]) Shader::EnableKeyword(keys[i]); else Shader::DisableKeyword(keys[i]);
        }
    }

#endif

    custom_types::Helpers::Coroutine Load(Entry& entry) {
        UnityEngine::AssetBundle* bundle = nullptr;
        co_yield custom_types::Helpers::CoroutineHelper::New(
            FPSCounter::LoadBundleFromMemoryAsync(entry.bytes, bundle));
        entry.bundle = bundle;
        if (!entry.bundle) {
            ERROR("{} bundle could not be loaded; original shader retained", entry.sourceName);
            co_return;
        }
        entry.replacement = entry.bundle->LoadAsset<UnityEngine::Shader*>(entry.assetName);
        if (!entry.replacement || !entry.replacement->get_isSupported()) {
            ERROR("{} replacement unsupported; original shader retained", entry.sourceName);
            entry.replacement = nullptr;
            co_return;
        }
        entry.replacement->set_hideFlags(UnityEngine::HideFlags::DontUnloadUnusedAsset);
        INFO("Loaded experimental bloom shader for {}", entry.sourceName);
        co_return;
    }

    bool loading = false;

    custom_types::Helpers::Coroutine LoadBundles() {
        for (auto& entry : Entries()) {
            if (entry.attempted) continue;
            entry.attempted = true;
            co_yield custom_types::Helpers::CoroutineHelper::New(Load(entry));
        }
        loading = false;
        // Apply uses the current toggle, including changes made while loading.
        Apply();
    }
}

void Apply() {
#ifdef GRAPHICSTWEAKS_SHADER_GPU_CHECK
    CheckUIShaders();
#endif
    bool enabled = getGraphicsTweaksConfig().ExperimentalBloomShaders.GetValue();
    if (enabled && !loading && BSML::SharedCoroutineStarter::get_instance() &&
            std::any_of(Entries().begin(), Entries().end(),
            [](const Entry& entry) { return !entry.attempted; })) {
        loading = true;
        BSML::SharedCoroutineStarter::StartCoroutine(LoadBundles());
    }
    auto materials = UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::Material*>();
    for (auto& entry : Entries()) {
      if (!entry.replacement) continue;
      int replaced = 0, restored = 0, nearest = 0;
      for (auto material : materials) {
        if (!material) continue;
        auto shader = material->get_shader();
        if (!shader) continue;
        bool ours = shader.ptr() == entry.replacement.ptr();
        if (!ours && std::string(shader->get_name()) != entry.sourceName) continue;
        if (!enabled) {
            if (ours && entry.original) {
                auto keywords = material->get_shaderKeywords();
                int queue = material->get_renderQueue();
                material->set_shader(entry.original.ptr());
                material->set_shaderKeywords(keywords);
                material->set_renderQueue(queue);
                restored++;
            }
            continue;
        }
        if (ours) continue;
        bool proven = Proven(material, entry.sourceName);
        if (!proven) nearest++;
#ifdef GRAPHICSTWEAKS_SHADER_GPU_CHECK
        if (entry.sourceName.starts_with("Custom/Glowing")) CheckGlowGPU(entry.replacement.ptr());
#endif
        entry.original = shader;
        entry.original->set_hideFlags(UnityEngine::HideFlags::DontUnloadUnusedAsset);
        auto keywords = material->get_shaderKeywords();
        int queue = material->get_renderQueue();
        material->set_shader(entry.replacement.ptr());
        material->set_shaderKeywords(keywords);
        material->set_renderQueue(queue);
        INFO("Restored bloom shader on material {} ({}{})", std::string(material->get_name()), entry.sourceName,
             proven ? "" : ", nearest variant");
        replaced++;
      }
      INFO("{} materials: replaced {} ({} by nearest variant), restored {}; main effect {}",
           entry.sourceName, replaced, nearest, restored, UnityEngine::Shader::IsKeywordEnabled("MAIN_EFFECT_ENABLED"));
    }
}

MAKE_HOOK_MATCH(SceneLoaded, &UnityEngine::SceneManagement::SceneManager::Internal_SceneLoaded,
    void, UnityEngine::SceneManagement::Scene scene, UnityEngine::SceneManagement::LoadSceneMode mode) {
    SceneLoaded(scene, mode);
    Apply();
}

void InstallHooks() {
    INSTALL_HOOK(Logger, SceneLoaded);
}
}
