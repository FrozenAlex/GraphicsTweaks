#include "UI/SettingsView.hpp"
#include "assets.hpp"
#include "logging.hpp"
#include "GlobalNamespace/OVRPlugin.hpp"
#include "bsml/shared/BSML.hpp"
#include "bsml/shared/BSML/Components/Backgroundable.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/Color.hpp"
#include "sombrero/shared/FastColor.hpp"
#include "GlobalNamespace/OVRPlugin.hpp"
#include "bsml/shared/BSML/MainThreadScheduler.hpp"
#include "GlobalNamespace/OVRPlugin.hpp"
#include "Unity/XR/Oculus/NativeMethods.hpp"
#include "GraphicsTweaksConfig.hpp"

DEFINE_TYPE(GraphicsTweaks::UI, SettingsView);

using namespace UnityEngine;
using namespace GlobalNamespace;

UnityEngine::Sprite* GetBGSprite(std::string str)
{
    return UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::Sprite*>()->First([str](UnityEngine::Sprite* x) {
        return x->get_name() == str;
    });
}

void GraphicsTweaks::UI::SettingsView::DidActivate(bool firstActivation, bool addedToHeirarchy, bool screenSystemDisabling)
{
    if (!firstActivation)
        return;

    INFO("SettingsView activated");

    // Get possible FPS values
    auto frequencies = GlobalNamespace::OVRPlugin::get_systemDisplayFrequenciesAvailable();
    // Create lists
    systemDisplayFrequenciesAvailableValues = ListW<float>::New();
    systemDisplayFrequenciesAvailableLabels = ListW<StringW>::New();
    for (const auto& item : frequencies) {
        systemDisplayFrequenciesAvailableValues->Add(item);
        systemDisplayFrequenciesAvailableLabels->Add(fmt::format("{}", item));
    }
  

    DEBUG("Got FPS values");

    BSML::parse_and_construct(Assets::SettingsView_bsml, this->get_transform(), this);

    #ifdef HotReload
        fileWatcher->checkInterval = 0.5f;
        fileWatcher->filePath = "/sdcard/bsml/GraphicsTweaks/SettingsView.bsml";
    #endif
};

void GraphicsTweaks::UI::SettingsView::PostParse() {
    DEBUG("SettingsView PostParse");

    auto getBgSprite = GetBGSprite("RoundRect10BorderFade");


    // Remove skew from all backgroundable elements
    auto backgroundables = GetComponentsInChildren<BSML::Backgroundable*>();
    for (auto & backgroundable : backgroundables) {
        auto imageView = backgroundable->GetComponent<HMUI::ImageView*>();
        if (!imageView || !imageView->get_color0().Equals(UnityEngine::Color::get_white()) || imageView->get_sprite()->get_name() != "RoundRect10") {
            continue;
        }
        imageView->____skew = 0.0f;
        imageView->set_overrideSprite(nullptr);
        imageView->set_sprite(getBgSprite);
        imageView->set_color(UnityEngine::Color(0.0f, 0.7f, 1.0f, 0.4f));
    }
}

void GraphicsTweaks::UI::SettingsView::UpdateGraphicsSettings() {
    BSML::MainThreadScheduler::ScheduleNextFrame([this]() {
        INFO("Updating graphics settings");

        
    });
    
}

// shockwaveParticlesValue
float GraphicsTweaks::UI::SettingsView::get_shockwaveParticlesValue() {
   return getGraphicsTweaksConfig().NumShockwaves.GetValue();
}
void GraphicsTweaks::UI::SettingsView::set_shockwaveParticlesValue(float value) {
    getGraphicsTweaksConfig().NumShockwaves.SetValue(value, false);
}

// foveationLevelValueMenu
StringW GraphicsTweaks::UI::SettingsView::get_foveationLevelValueMenu() {
    auto value = getGraphicsTweaksConfig().MenuFoveatedRenderingLevel.GetValue();
    switch(value) {
        case 0:
            return "Off";
        case 1:
            return "Low";
        case 2:
            return "Medium";
        case 3:
            return "High";
        case 4:
            return "HighTop";
    }
    return "Off";
}
void GraphicsTweaks::UI::SettingsView::set_foveationLevelValueMenu(StringW value) {
    if(value == "Off") {
        getGraphicsTweaksConfig().MenuFoveatedRenderingLevel.SetValue(0, false);
    } else if(value == "Low") {
        getGraphicsTweaksConfig().MenuFoveatedRenderingLevel.SetValue(1, false);
    } else if(value == "Medium") {
        getGraphicsTweaksConfig().MenuFoveatedRenderingLevel.SetValue(2, false);
    } else if(value == "High") {
        getGraphicsTweaksConfig().MenuFoveatedRenderingLevel.SetValue(3, false);
    } else if(value == "HighTop") {
        getGraphicsTweaksConfig().MenuFoveatedRenderingLevel.SetValue(4, false);
    }
}

// foveationLevelValueGame
StringW GraphicsTweaks::UI::SettingsView::get_foveationLevelValueGame() {
    auto value = getGraphicsTweaksConfig().InGameFoveatedRenderingLevel.GetValue();
    switch(value) {
        case 0:
            return "Off";
        case 1:
            return "Low";
        case 2:
            return "Medium";
        case 3:
            return "High";
        case 4:
            return "HighTop";
    }
    return "Off";
}
void GraphicsTweaks::UI::SettingsView::set_foveationLevelValueGame(StringW value) {
    if(value == "Off") {
        getGraphicsTweaksConfig().InGameFoveatedRenderingLevel.SetValue(0, false);
    } else if(value == "Low") {
        getGraphicsTweaksConfig().InGameFoveatedRenderingLevel.SetValue(1, false);
    } else if(value == "Medium") {
        getGraphicsTweaksConfig().InGameFoveatedRenderingLevel.SetValue(2, false);
    } else if(value == "High") {
        getGraphicsTweaksConfig().InGameFoveatedRenderingLevel.SetValue(3, false);
    } else if(value == "HighTop") {
        getGraphicsTweaksConfig().InGameFoveatedRenderingLevel.SetValue(4, false);
    }
}

// mirrorValue
StringW GraphicsTweaks::UI::SettingsView::get_mirrorValue() {
    if (getGraphicsTweaksConfig().Mirror.GetValue() == 0) {
        return "Off";
    } else if (getGraphicsTweaksConfig().Mirror.GetValue() == 1) {
        return "Low";
    } else if (getGraphicsTweaksConfig().Mirror.GetValue() == 2) {
        return "Medium";
    } else if (getGraphicsTweaksConfig().Mirror.GetValue() == 3) {
        return "High";
    }
    return "Off";
}
void GraphicsTweaks::UI::SettingsView::set_mirrorValue(StringW value) {
    if(value == "Off") {
        getGraphicsTweaksConfig().Mirror.SetValue(0, false);
    } else if(value == "Low") {
        getGraphicsTweaksConfig().Mirror.SetValue(1, false);
    } else if(value == "Medium") {
        getGraphicsTweaksConfig().Mirror.SetValue(2, false);
    } else if(value == "High") {
        getGraphicsTweaksConfig().Mirror.SetValue(3, false);
    }
}

// antiAliasingValue
StringW GraphicsTweaks::UI::SettingsView::get_antiAliasingValue() {
    if (getGraphicsTweaksConfig().AntiAliasing.GetValue() == 0) {
        return "Off";
    } else if (getGraphicsTweaksConfig().AntiAliasing.GetValue() == 1) {
        return "2x";
    } else if (getGraphicsTweaksConfig().AntiAliasing.GetValue() == 2) {
        return "4x";
    }
    return "Off";
}
void GraphicsTweaks::UI::SettingsView::set_antiAliasingValue(StringW value) {
    if(value == "Off") {
        getGraphicsTweaksConfig().AntiAliasing.SetValue(0, false);
    } else if(value == "2x") {
        getGraphicsTweaksConfig().AntiAliasing.SetValue(1, false);
    } else if(value == "4x") {
        getGraphicsTweaksConfig().AntiAliasing.SetValue(2, false);
    }
}

// bloomQualityValue
StringW GraphicsTweaks::UI::SettingsView::get_bloomQualityValue() {
    if (getGraphicsTweaksConfig().Bloom.GetValue() == false) {
        return "Off";
    } else if (getGraphicsTweaksConfig().BloomQuality.GetValue() == 1) {
        return "Low";
    } else if (getGraphicsTweaksConfig().BloomQuality.GetValue() == 2) {
        return "High";
    }
    return "Off";
}
void GraphicsTweaks::UI::SettingsView::set_bloomQualityValue(StringW value) {
    if(value == "Off") {
        getGraphicsTweaksConfig().Bloom.SetValue(false, false);
        getGraphicsTweaksConfig().BloomQuality.SetValue(0, false);
    } else if(value == "Low") {
        getGraphicsTweaksConfig().Bloom.SetValue(true, false);
        getGraphicsTweaksConfig().BloomQuality.SetValue(1, false);
    } else if(value == "High") {
        getGraphicsTweaksConfig().Bloom.SetValue(true, false);
        getGraphicsTweaksConfig().BloomQuality.SetValue(2, false);
    }
}

// smokeQualityValue
StringW GraphicsTweaks::UI::SettingsView::get_smokeQualityValue() {
    if (getGraphicsTweaksConfig().SmokeQuality.GetValue() == 0) {
        return "Off";
    } else if (getGraphicsTweaksConfig().SmokeQuality.GetValue() == 1) {
        return "Low";
    } else if (getGraphicsTweaksConfig().SmokeQuality.GetValue() == 2) {
        return "High";
    }
    return "Off";
}
void GraphicsTweaks::UI::SettingsView::set_smokeQualityValue(StringW value) {
    if(value == "Off") {
        getGraphicsTweaksConfig().SmokeQuality.SetValue(0, false);
    } else if(value == "Low") {
        getGraphicsTweaksConfig().SmokeQuality.SetValue(1, false);
    } else if(value == "High") {
        getGraphicsTweaksConfig().SmokeQuality.SetValue(2, false);
    }
}

// wallQualityValue
StringW GraphicsTweaks::UI::SettingsView::get_wallQualityValue() {
    if (getGraphicsTweaksConfig().WallQuality.GetValue() == 0) {
        return "Transparent";
    } else if (getGraphicsTweaksConfig().WallQuality.GetValue() == 1) {
        return "Textured";
    } else if (getGraphicsTweaksConfig().WallQuality.GetValue() == 2) {
        return "Distorted";
    }
    return "Transparent";
}
void GraphicsTweaks::UI::SettingsView::set_wallQualityValue(StringW value) {
    if(value == "Transparent") {
        getGraphicsTweaksConfig().WallQuality.SetValue(0, false);
    } else if(value == "Textured") {
        getGraphicsTweaksConfig().WallQuality.SetValue(1, false);
    } else if(value == "Distorted") {
        getGraphicsTweaksConfig().WallQuality.SetValue(2, false);
    }
}


// resolutionLevelValueMenu
float GraphicsTweaks::UI::SettingsView::get_resolutionLevelValueMenu() {
    
    return getGraphicsTweaksConfig().MenuResolution.GetValue();
}

void GraphicsTweaks::UI::SettingsView::set_resolutionLevelValueMenu(float value) {
    getGraphicsTweaksConfig().MenuResolution.SetValue(value, false);
}

// resolutionLevelValueGame
float GraphicsTweaks::UI::SettingsView::get_resolutionLevelValueGame() {
    return getGraphicsTweaksConfig().GameResolution.GetValue();
}
void GraphicsTweaks::UI::SettingsView::set_resolutionLevelValueGame(float value) {
    getGraphicsTweaksConfig().GameResolution.SetValue(value, false);
}

// targetFPSValueMenu
StringW GraphicsTweaks::UI::SettingsView::get_targetFPSValueMenu() {
    auto value = getGraphicsTweaksConfig().MenuRefreshRate.GetValue();
    
    auto index = this->systemDisplayFrequenciesAvailableValues->IndexOf(value);
    if (index >= 0) {
        return this->systemDisplayFrequenciesAvailableLabels->get_Item(index);
    } else {
        return "Unkown";
    }
}
void GraphicsTweaks::UI::SettingsView::set_targetFPSValueMenu(StringW value) {
    
    auto index = this->systemDisplayFrequenciesAvailableLabels->IndexOf(value);

    if (index >= 0) {
        DEBUG("Setting refresh rate to {}", value);
        auto item = this->systemDisplayFrequenciesAvailableValues->get_Item(index);
        OVRPlugin::set_systemDisplayFrequency(item);
        getGraphicsTweaksConfig().MenuRefreshRate.SetValue(item, false);
    }
}

// targetFPSValueGame
StringW GraphicsTweaks::UI::SettingsView::get_targetFPSValueGame() {
    auto value = getGraphicsTweaksConfig().GameRefreshRate.GetValue();
    
    auto index = this->systemDisplayFrequenciesAvailableValues->IndexOf(value);
    if (index >= 0) {
        return this->systemDisplayFrequenciesAvailableLabels->get_Item(index);
    } else {
        return "Unkown";
    }
}
void GraphicsTweaks::UI::SettingsView::set_targetFPSValueGame(StringW value) {
    auto index = this->systemDisplayFrequenciesAvailableLabels->IndexOf(value);

    if (index >= 0) {
        auto item = this->systemDisplayFrequenciesAvailableValues->get_Item(index);
        getGraphicsTweaksConfig().GameRefreshRate.SetValue(item, false);
    }
}

// menuScreenDistortionValue
bool GraphicsTweaks::UI::SettingsView::get_menuScreenDistortionValue() {
    return getGraphicsTweaksConfig().MenuShockwaves.GetValue();
}
void GraphicsTweaks::UI::SettingsView::set_menuScreenDistortionValue(bool value) {
    getGraphicsTweaksConfig().MenuShockwaves.SetValue(value, false);
}

// gameScreenDistortionValue
bool GraphicsTweaks::UI::SettingsView::get_gameScreenDistortionValue() {
    return getGraphicsTweaksConfig().GameShockwaves.GetValue();
}
void GraphicsTweaks::UI::SettingsView::set_gameScreenDistortionValue(bool value) {
    getGraphicsTweaksConfig().GameShockwaves.SetValue(value, false);
}

// burnMarksValue
bool GraphicsTweaks::UI::SettingsView::get_burnMarksValue() {
    return getGraphicsTweaksConfig().Burnmarks.GetValue();
}
void GraphicsTweaks::UI::SettingsView::set_burnMarksValue(bool value) {
    getGraphicsTweaksConfig().Burnmarks.SetValue(value, false);
}

// gpuLevelValue
float GraphicsTweaks::UI::SettingsView::get_gpuLevelValue() {
    DEBUG("Getting GPU Level {}", getGraphicsTweaksConfig().GpuLevel.GetValue());
    return getGraphicsTweaksConfig().GpuLevel.GetValue();
}
void GraphicsTweaks::UI::SettingsView::set_gpuLevelValue(float value) {
    DEBUG("Setting GPU Level {}", value);
    OVRPlugin::set_gpuLevel(value);
    getGraphicsTweaksConfig().GpuLevel.SetValue(static_cast<int>(value), false);
}

// cpuLevelValue
float GraphicsTweaks::UI::SettingsView::get_cpuLevelValue() {
    DEBUG("Getting CPU Level {}", getGraphicsTweaksConfig().CpuLevel.GetValue());
    
    return getGraphicsTweaksConfig().CpuLevel.GetValue();
}
void GraphicsTweaks::UI::SettingsView::set_cpuLevelValue(float value) {
    DEBUG("Setting CPU Level {}", value);
    OVRPlugin::set_cpuLevel(value);
    getGraphicsTweaksConfig().CpuLevel.SetValue(static_cast<int>(value), false);
}

// FPSCounterValue
bool GraphicsTweaks::UI::SettingsView::get_FPSCounterValue() {
    DEBUG("Getting FPS Counter {}", getGraphicsTweaksConfig().FpsCounter.GetValue());
    return getGraphicsTweaksConfig().FpsCounter.GetValue();
}
void GraphicsTweaks::UI::SettingsView::set_FPSCounterValue(bool value) {
    DEBUG("Setting FPS Counter {}", value);
    getGraphicsTweaksConfig().FpsCounter.SetValue(value, false);
}

// FPSCounterAdvancedValue
bool GraphicsTweaks::UI::SettingsView::get_FPSCounterAdvancedValue() {
    DEBUG("Getting Advanced FPS Counter {}", getGraphicsTweaksConfig().FpsCounterAdvanced.GetValue());
    return getGraphicsTweaksConfig().FpsCounterAdvanced.GetValue();
}

void GraphicsTweaks::UI::SettingsView::set_FPSCounterAdvancedValue(bool value) {
    DEBUG("Setting Advanced FPS Counter {}", value);
    getGraphicsTweaksConfig().FpsCounterAdvanced.SetValue(value, false);
}