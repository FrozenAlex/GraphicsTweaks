#ifdef GRAPHICSTWEAKS_SHADER_GPU_CHECK
#include "RestoredBloom/UIShaderDiagnostics.hpp"
#include "logging.hpp"
#include "UnityEngine/Color.hpp"
#include "UnityEngine/GL.hpp"
#include "UnityEngine/Material.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/Shader.hpp"
#include "UnityEngine/Vector4.hpp"
#include "UnityEngine/Rect.hpp"
#include "UnityEngine/RenderTexture.hpp"
#include "UnityEngine/RenderTextureFormat.hpp"
#include "UnityEngine/RenderTextureReadWrite.hpp"
#include "UnityEngine/Texture2D.hpp"
#include "UnityEngine/TextureFormat.hpp"
#include <array>
#include <set>
#include <string>

namespace GraphicsTweaks::ShaderReplacement {
void CheckUIShaders() {
    using namespace UnityEngine;
    static std::set<std::string> checked;
    auto materials = Resources::FindObjectsOfTypeAll<Material*>();
    for (auto source : materials) {
        auto shader = source->get_shader();
        if (!shader) continue;
        std::string shaderName = shader->get_name(), name = source->get_name();
        if (shaderName != "Custom/CustomUI" || (name != "UINormal" && name != "UINoGlow" && name != "UIWhiteBoost")) continue;
        if (!checked.insert(name).second) continue;
        INFO("UI audit {}: blend RGB {}/{}, alpha {}/{}, boost {}, threshold {}, main {}", name,
            source->GetFloat("_BlendSrcFactor"), source->GetFloat("_BlendDstFactor"),
            source->GetFloat("_BlendSrcFactorA"), source->GetFloat("_BlendDstFactorA"),
            Shader::GetGlobalFloat("_BaseColorBoost"), Shader::GetGlobalFloat("_BaseColorBoostThreshold"), Shader::IsKeywordEnabled("MAIN_EFFECT_ENABLED"));
        std::array<std::string_view, 2> keys = {"STEREO_MULTIVIEW_ON", "INSTANCING_ON"};
        std::array<bool, 2> states;
        for (int i = 0; i < 2; ++i) { states[i] = Shader::IsKeywordEnabled(keys[i]); Shader::DisableKeyword(keys[i]); }
        auto previous = RenderTexture::get_active();
        auto target = RenderTexture::GetTemporary(8, 8, 0, RenderTextureFormat::ARGB32, RenderTextureReadWrite::Linear);
        auto pixels = Texture2D::New_ctor(8, 8, TextureFormat::RGBA32, false);
        auto material = Material::New_ctor(source);
        material->DisableKeyword("STEREO_MULTIVIEW_ON");
        material->SetTexture("_MainTex", Texture2D::get_whiteTexture().ptr());
        material->SetColor("_Color", Color(1, 1, 1, 1));
        material->SetVector("_ClipRect", Vector4(-32767, -32767, 32767, 32767));
        material->SetFloat("_BaseColorBoost", 1.0f);
        material->SetFloat("_BaseColorBoostThreshold", 0.0f);
        material->SetFloat("unity_GUIZTestMode", 8.0f);
        material->SetFloat("_StencilComp", 8.0f);
        for (float alpha : {0.5f, 1.0f}) {
            RenderTexture::set_active(target.ptr());
            GL::Clear(true, true, Color(0, 0, 0, 0));
            GL::PushMatrix(); GL::LoadOrtho();
            bool pass = material->SetPass(0);
            GL::Begin(7); GL::Color(Color(0.2f, 0.3f, 0.4f, alpha));
            GL::MultiTexCoord2(0, 0.5f, 0.5f);
            GL::MultiTexCoord2(1, 0.5f, 0.5f);
            GL::MultiTexCoord2(2, 0, 0);
            GL::Vertex3(0, 0, 0); GL::Vertex3(0, 1, 0); GL::Vertex3(1, 1, 0); GL::Vertex3(1, 0, 0);
            GL::End(); GL::PopMatrix();
            pixels->ReadPixels(Rect(0, 0, 8, 8), 0, 0, false);
            auto color = pixels->GetPixel(4, 4);
            INFO("UI GPU check {}: alpha {}, pass {}, RGBA {} {} {} {}", name, alpha, pass, color.r, color.g, color.b, color.a);
        }
        RenderTexture::set_active(previous ? previous.ptr() : nullptr);
        RenderTexture::ReleaseTemporary(target.ptr());
        Object::Destroy(material); Object::Destroy(pixels);
        for (int i = 0; i < 2; ++i) { if (states[i]) Shader::EnableKeyword(keys[i]); else Shader::DisableKeyword(keys[i]); }
    }
}
}
#endif
