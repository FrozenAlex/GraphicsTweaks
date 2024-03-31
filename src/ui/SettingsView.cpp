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
#include "GlobalNamespace/ConditionalActivation.hpp"
#include "bsml/shared/BSML/MainThreadScheduler.hpp"
#include "GlobalNamespace/OVRPlugin.hpp"
#include "Unity/XR/Oculus/NativeMethods.hpp"
#include "GraphicsTweaksConfig.hpp"
#include "main.hpp"
#include "FPSCounter.hpp"
#include "bsml/shared/BSML/SharedCoroutineStarter.hpp"
DEFINE_TYPE(GraphicsTweaks::UI, SettingsView);

using namespace UnityEngine;
using namespace GlobalNamespace;

const bool instantlySave = true;

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

    
}

// shockwaveParticlesValue
float GraphicsTweaks::UI::SettingsView::get_shockwaveParticlesValue() {
   return getGraphicsTweaksConfig().NumShockwaves.GetValue();
}
void GraphicsTweaks::UI::SettingsView::set_shockwaveParticlesValue(float value) {
    getGraphicsTweaksConfig().NumShockwaves.SetValue(value, instantlySave);
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
        getGraphicsTweaksConfig().MenuFoveatedRenderingLevel.SetValue(0, instantlySave);
    } else if(value == "Low") {
        getGraphicsTweaksConfig().MenuFoveatedRenderingLevel.SetValue(1, instantlySave);
    } else if(value == "Medium") {
        getGraphicsTweaksConfig().MenuFoveatedRenderingLevel.SetValue(2, instantlySave);
    } else if(value == "High") {
        getGraphicsTweaksConfig().MenuFoveatedRenderingLevel.SetValue(3, instantlySave);
    } else if(value == "HighTop") {
        getGraphicsTweaksConfig().MenuFoveatedRenderingLevel.SetValue(4, instantlySave);
    }
    GraphicsTweaks::VRRenderingParamsSetup::Reload();
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
        getGraphicsTweaksConfig().InGameFoveatedRenderingLevel.SetValue(0, instantlySave);
    } else if(value == "Low") {
        getGraphicsTweaksConfig().InGameFoveatedRenderingLevel.SetValue(1, instantlySave);
    } else if(value == "Medium") {
        getGraphicsTweaksConfig().InGameFoveatedRenderingLevel.SetValue(2, instantlySave);
    } else if(value == "High") {
        getGraphicsTweaksConfig().InGameFoveatedRenderingLevel.SetValue(3, instantlySave);
    } else if(value == "HighTop") {
        getGraphicsTweaksConfig().InGameFoveatedRenderingLevel.SetValue(4, instantlySave);
    }
    GraphicsTweaks::VRRenderingParamsSetup::Reload();
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
    auto numberValue = 0;
    
    if (value == "Off") {
        numberValue = 0;
    } else if (value == "Low") {
        numberValue = 1;
    } else if (value == "Medium") {
        numberValue = 2;
    } else if (value == "High") {
        numberValue = 3;
    }
    getGraphicsTweaksConfig().Mirror.SetValue(numberValue, false);

    GraphicsTweaks::MirrorsData::ApplySettings();
}

// antiAliasingValue
StringW GraphicsTweaks::UI::SettingsView::get_antiAliasingValue() {
    if (getGraphicsTweaksConfig().AntiAliasing.GetValue() == 0) {
        return "Off";
    } else if (getGraphicsTweaksConfig().AntiAliasing.GetValue() == 1) {
        return "2x";
    } else if (getGraphicsTweaksConfig().AntiAliasing.GetValue() == 2) {
        return "4x";
    } else if (getGraphicsTweaksConfig().AntiAliasing.GetValue() == 4) {
        return "8x";
    }
    return "Off";
}
void GraphicsTweaks::UI::SettingsView::set_antiAliasingValue(StringW value) {
    int intValue = 0;
    if(value == "Off") {
        intValue = 0;
    } else if(value == "2x") {
        intValue = 1;
    } else if(value == "4x") {
        intValue = 2;
    }
    getGraphicsTweaksConfig().AntiAliasing.SetValue(intValue, instantlySave);
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
        getGraphicsTweaksConfig().Bloom.SetValue(false, instantlySave);
        getGraphicsTweaksConfig().BloomQuality.SetValue(0, instantlySave);
    } else if(value == "Low") {
        getGraphicsTweaksConfig().Bloom.SetValue(true, instantlySave);
        getGraphicsTweaksConfig().BloomQuality.SetValue(1, instantlySave);
    } else if(value == "High") {
        getGraphicsTweaksConfig().Bloom.SetValue(true, instantlySave);
        getGraphicsTweaksConfig().BloomQuality.SetValue(2, instantlySave);
    }

    GraphicsTweaks::BloomData::ApplySettings();

    auto conditionalActivations = UnityEngine::Resources::FindObjectsOfTypeAll<GlobalNamespace::ConditionalActivation*>();
    for (auto & conditionalActivation : conditionalActivations) {
        conditionalActivation->Awake();
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
        getGraphicsTweaksConfig().SmokeQuality.SetValue(0, instantlySave);
    } else if(value == "Low") {
        getGraphicsTweaksConfig().SmokeQuality.SetValue(1, instantlySave);
    } else if(value == "High") {
        getGraphicsTweaksConfig().SmokeQuality.SetValue(2, instantlySave);
    }

    auto conditionalActivations = UnityEngine::Resources::FindObjectsOfTypeAll<GlobalNamespace::ConditionalActivation*>();
    for (auto & conditionalActivation : conditionalActivations) {
        conditionalActivation->Awake();
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
        getGraphicsTweaksConfig().WallQuality.SetValue(0, instantlySave);
    } else if(value == "Textured") {
        getGraphicsTweaksConfig().WallQuality.SetValue(1, instantlySave);
    } else if(value == "Distorted") {
        getGraphicsTweaksConfig().WallQuality.SetValue(2, instantlySave);
    }
}


// resolutionLevelValueMenu
float GraphicsTweaks::UI::SettingsView::get_resolutionLevelValueMenu() {
    return getGraphicsTweaksConfig().MenuResolution.GetValue();
}

void GraphicsTweaks::UI::SettingsView::set_resolutionLevelValueMenu(float value) {
    getGraphicsTweaksConfig().MenuResolution.SetValue(value, instantlySave);
    GraphicsTweaks::VRRenderingParamsSetup::Reload();
}

// resolutionLevelValueGame
float GraphicsTweaks::UI::SettingsView::get_resolutionLevelValueGame() {
    return getGraphicsTweaksConfig().GameResolution.GetValue();
}
void GraphicsTweaks::UI::SettingsView::set_resolutionLevelValueGame(float value) {
    getGraphicsTweaksConfig().GameResolution.SetValue(value, instantlySave);
    GraphicsTweaks::VRRenderingParamsSetup::Reload();
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
        getGraphicsTweaksConfig().MenuRefreshRate.SetValue(item, instantlySave);
        GraphicsTweaks::VRRenderingParamsSetup::Reload();
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
        getGraphicsTweaksConfig().GameRefreshRate.SetValue(item, instantlySave);
        GraphicsTweaks::VRRenderingParamsSetup::Reload();
    }
}

// menuScreenDistortionValue
bool GraphicsTweaks::UI::SettingsView::get_menuScreenDistortionValue() {
    return getGraphicsTweaksConfig().MenuShockwaves.GetValue();
}
void GraphicsTweaks::UI::SettingsView::set_menuScreenDistortionValue(bool value) {
    getGraphicsTweaksConfig().MenuShockwaves.SetValue(value, instantlySave);

    auto conditionalActivations = UnityEngine::Resources::FindObjectsOfTypeAll<GlobalNamespace::ConditionalActivation*>();
    for (auto & conditionalActivation : conditionalActivations) {
        conditionalActivation->Awake();
    }
}

// gameScreenDistortionValue
bool GraphicsTweaks::UI::SettingsView::get_gameScreenDistortionValue() {
    return getGraphicsTweaksConfig().GameShockwaves.GetValue();
}
void GraphicsTweaks::UI::SettingsView::set_gameScreenDistortionValue(bool value) {
    getGraphicsTweaksConfig().GameShockwaves.SetValue(value, instantlySave);

    auto conditionalActivations = UnityEngine::Resources::FindObjectsOfTypeAll<GlobalNamespace::ConditionalActivation*>();
    for (auto & conditionalActivation : conditionalActivations) {
        conditionalActivation->Awake();
    }
}

// burnMarksValue
bool GraphicsTweaks::UI::SettingsView::get_burnMarksValue() {
    return getGraphicsTweaksConfig().Burnmarks.GetValue();
}
void GraphicsTweaks::UI::SettingsView::set_burnMarksValue(bool value) {
    getGraphicsTweaksConfig().Burnmarks.SetValue(value, instantlySave);
}

// gpuLevelValue
float GraphicsTweaks::UI::SettingsView::get_gpuLevelValue() {
    DEBUG("Getting GPU Level {}", getGraphicsTweaksConfig().GpuLevel.GetValue());
    return getGraphicsTweaksConfig().GpuLevel.GetValue();
}
void GraphicsTweaks::UI::SettingsView::set_gpuLevelValue(float value) {
    DEBUG("Setting GPU Level {}", value);
    getGraphicsTweaksConfig().GpuLevel.SetValue(static_cast<int>(value), instantlySave);
    GraphicsTweaks::VRRenderingParamsSetup::Reload();
}

// cpuLevelValue
float GraphicsTweaks::UI::SettingsView::get_cpuLevelValue() {
    DEBUG("Getting CPU Level {}", getGraphicsTweaksConfig().CpuLevel.GetValue());
    
    return getGraphicsTweaksConfig().CpuLevel.GetValue();
}
void GraphicsTweaks::UI::SettingsView::set_cpuLevelValue(float value) {
    DEBUG("Setting CPU Level {}", value);
    getGraphicsTweaksConfig().CpuLevel.SetValue(static_cast<int>(value), instantlySave);
    GraphicsTweaks::VRRenderingParamsSetup::Reload();
}

// FPSCounterValue
bool GraphicsTweaks::UI::SettingsView::get_FPSCounterValue() {
    DEBUG("Getting FPS Counter {}", getGraphicsTweaksConfig().FpsCounter.GetValue());
    return getGraphicsTweaksConfig().FpsCounter.GetValue();
}
void GraphicsTweaks::UI::SettingsView::set_FPSCounterValue(bool value) {
    DEBUG("Setting FPS Counter {}", value);
    getGraphicsTweaksConfig().FpsCounter.SetValue(value, instantlySave);
}

// FPSCounterAdvancedValue
bool GraphicsTweaks::UI::SettingsView::get_FPSCounterAdvancedValue() {
    DEBUG("Getting Advanced FPS Counter {}", getGraphicsTweaksConfig().FpsCounterAdvanced.GetValue());
    return getGraphicsTweaksConfig().FpsCounterAdvanced.GetValue();
}

void GraphicsTweaks::UI::SettingsView::set_FPSCounterAdvancedValue(bool value) {
    DEBUG("Setting Advanced FPS Counter {}", value);
    getGraphicsTweaksConfig().FpsCounterAdvanced.SetValue(value, instantlySave);

    if (FPSCounter::counter) {
        FPSCounter::counter->SetActive(value);
    }
}

// colorSpaceValue
StringW GraphicsTweaks::UI::SettingsView::get_colorSpaceValue() {
    auto value = getGraphicsTweaksConfig().ColorSpace.GetValue();
    switch(value) {
        case 0:
            return "Unknown";
        case 1:
            return "Unmanaged";
        case 2:
            return "Rec_2020";
        case 3:
            return "Rec_709";
        case 4:
            return "Rift_CV1";
        case 5:
            return "Rift_S";
        case 6:
            return "Quest";
        case 7:
            return "P3";
        case 8:
            return "Adobe_RGB";
    }
    return "Unknown";
}

void GraphicsTweaks::UI::SettingsView::set_colorSpaceValue(StringW value) {
    int intValue = 0;
    if(value == "Unknown") {
        intValue = 0;
    } else if(value == "Unmanaged") {
        intValue = 1;
    } else if(value == "Rec_2020") {
        intValue = 2;
    } else if(value == "Rec_709") {
        intValue = 3;
    } else if(value == "Rift_CV1") {
        intValue = 4;
    } else if(value == "Rift_S") {
        intValue = 5;
    } else if(value == "Quest") {
        intValue = 6;
    } else if(value == "P3") {
        intValue = 7;
    } else if(value == "Adobe_RGB") {
        intValue = 8;
    }
    
    getGraphicsTweaksConfig().ColorSpace.SetValue(intValue, instantlySave);
    GraphicsTweaks::VRRenderingParamsSetup::Reload();
}