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

    // while (frequenciesEnumerator->MoveNext()) {
    //     auto frequency = frequenciesEnumerator->get_Current();
    //     systemDisplayFrequenciesAvailableValues->Add(frequency);
    //     systemDisplayFrequenciesAvailableNames->Add(frequency.ToString());
    // }
  

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