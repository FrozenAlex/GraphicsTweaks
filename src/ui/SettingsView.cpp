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

#include "GraphicsTweaksConfig.hpp"

DEFINE_TYPE(GraphicsTweaks::UI, SettingsView);


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
    INFO("Updating graphics settings");

    if(mirrorValue == "Off") {
        getGraphicsTweaksConfig().Mirror.SetValue(0);
    } else if(mirrorValue == "Low") {
        getGraphicsTweaksConfig().Mirror.SetValue(1);
    } else if(mirrorValue == "Medium") {
        getGraphicsTweaksConfig().Mirror.SetValue(2);
    } else if(mirrorValue == "High") {
        getGraphicsTweaksConfig().Mirror.SetValue(3);
    }

    if(antiAliasingValue == "Off") {
        getGraphicsTweaksConfig().AntiAliasing.SetValue(0);
    } else if(antiAliasingValue == "2x") {
        getGraphicsTweaksConfig().AntiAliasing.SetValue(1);
    } else if(antiAliasingValue == "4x") {
        getGraphicsTweaksConfig().AntiAliasing.SetValue(2);
    }

    if(bloomQualityValue == "Off") {
        getGraphicsTweaksConfig().Bloom.SetValue(false);
        getGraphicsTweaksConfig().BloomQuality.SetValue(0);
    } else if(bloomQualityValue == "Low") {
        getGraphicsTweaksConfig().Bloom.SetValue(true);
        getGraphicsTweaksConfig().BloomQuality.SetValue(1);
    } else if(bloomQualityValue == "High") {
        getGraphicsTweaksConfig().Bloom.SetValue(true);
        getGraphicsTweaksConfig().BloomQuality.SetValue(2);
    }

    if(smokeQualityValue == "Off") {
        getGraphicsTweaksConfig().SmokeQuality.SetValue(0);
    } else if(smokeQualityValue == "Low") {
        getGraphicsTweaksConfig().SmokeQuality.SetValue(1);
    } else if(smokeQualityValue == "High") {
        getGraphicsTweaksConfig().SmokeQuality.SetValue(2);
    }

    DEBUG("WallQualityValue: {}", wallQualityValue);
    if(wallQualityValue == "Transparent") {
        getGraphicsTweaksConfig().WallQuality.SetValue(0);
    } else if(wallQualityValue == "Textured") {
        getGraphicsTweaksConfig().WallQuality.SetValue(1);
    } else if(wallQualityValue == "Distorted") {
        getGraphicsTweaksConfig().WallQuality.SetValue(2);
    }
    DEBUG("config WallQuality: {}", getGraphicsTweaksConfig().WallQuality.GetValue());

    getGraphicsTweaksConfig().Shockwave.SetValue(shockwaveParticlesValue);

    if(foveationLevelValueMenu == "Off") {
        getGraphicsTweaksConfig().MenuFoveatedRenderingLevel.SetValue(0);
    } else if(foveationLevelValueMenu == "Low") {
        getGraphicsTweaksConfig().MenuFoveatedRenderingLevel.SetValue(1);
    } else if(foveationLevelValueMenu == "Medium") {
        getGraphicsTweaksConfig().MenuFoveatedRenderingLevel.SetValue(2);
    } else if(foveationLevelValueMenu == "High") {
        getGraphicsTweaksConfig().MenuFoveatedRenderingLevel.SetValue(3);
    } else if(foveationLevelValueMenu == "HighTop") {
        getGraphicsTweaksConfig().MenuFoveatedRenderingLevel.SetValue(4);
    }

    if(foveationLevelValueGame == "Off") {
        getGraphicsTweaksConfig().InGameFoveatedRenderingLevel.SetValue(0);
    } else if(foveationLevelValueGame == "Low") {
        getGraphicsTweaksConfig().InGameFoveatedRenderingLevel.SetValue(1);
    } else if(foveationLevelValueGame == "Medium") {
        getGraphicsTweaksConfig().InGameFoveatedRenderingLevel.SetValue(2);
    } else if(foveationLevelValueGame == "High") {
        getGraphicsTweaksConfig().InGameFoveatedRenderingLevel.SetValue(3);
    } else if(foveationLevelValueGame == "HighTop") {
        getGraphicsTweaksConfig().InGameFoveatedRenderingLevel.SetValue(4);
    }

    getGraphicsTweaksConfig().MenuResolution.SetValue(resolutionLevelValueMenu);
    getGraphicsTweaksConfig().GameResolution.SetValue(resolutionLevelValueGame);

    getGraphicsTweaksConfig().MenuRefreshRate.SetValue(targetFPSValueMenu);
    getGraphicsTweaksConfig().GameRefreshRate.SetValue(targetFPSValueGame);

    getGraphicsTweaksConfig().CpuLevel.SetValue(cpuLevelValue);
    getGraphicsTweaksConfig().GpuLevel.SetValue(gpuLevelValue);

    getGraphicsTweaksConfig().ScreenDistortion.SetValue(screenDistortionValue);

    getGraphicsTweaksConfig().Save();
}