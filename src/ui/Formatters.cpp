#include "UI/Formatters.hpp"
#include "logging.hpp"
#include "UnityEngine/XR/XRSettings.hpp"

namespace GraphicsTweaks::Formatters {
    std::string FormatTextureResolution(float value) {
        // Get game texture base resolution (imprecise, but good enough for this purpose)
        auto resolutionScale = UnityEngine::XR::XRSettings::get_eyeTextureResolutionScale();
        auto eyeTextureWidth = UnityEngine::XR::XRSettings::get_eyeTextureWidth();
        auto eyeTextureHeight = UnityEngine::XR::XRSettings::get_eyeTextureHeight();
        float aspectRatio = (float)eyeTextureWidth / (float)eyeTextureHeight;

        // Calculate the actual base resolution
        auto actualResolution = eyeTextureHeight/resolutionScale;
        auto scaledTextureResolution = actualResolution * value;
        
        auto scaledTextureWidth = scaledTextureResolution * aspectRatio;

        return fmt::format("{:.1f} ({:.0f})", value, scaledTextureWidth);
    }

    std::string FormatIncrementAsIntegers(float value) {
        return fmt::format("{:.0f}", value);
    }
    
    void FormatSetting(UnityW<BSML::IncrementSetting> element){
        if (!element) {
            WARNING("Element is null, skipping formatting");
            return;
        }

        auto value = element->get_Value();
        auto formatter = element->formatter;
        if (!formatter) {
            WARNING("No formatter found for setting, skipping formatting");
            return;
        }

        element->text->set_text(formatter ? formatter(value) : StringW(fmt::format("{}", value)));
    }
}

