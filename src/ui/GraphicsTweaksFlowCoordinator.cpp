#include "UI/GraphicsTweaksFlowCoordinator.hpp"

#include "bsml/shared/Helpers/creation.hpp"
#include "bsml/shared/Helpers/getters.hpp"
#include "logging.hpp"

#include "GlobalNamespace/MenuTransitionsHelper.hpp"
#include "UnityEngine/Resources.hpp"
#include "bsml/shared/BSML/ViewControllers/HotReloadViewController.hpp"
#include "GraphicsTweaksConfig.hpp"
#include "bsml/shared/BSML/MainThreadScheduler.hpp"
DEFINE_TYPE(GraphicsTweaks::UI, GraphicsTweaksFlowCoordinator);

void GraphicsTweaks::UI::GraphicsTweaksFlowCoordinator::Awake() {
    fcInstance = this;
    if (!settingsView) {
        settingsView = BSML::Helpers::CreateViewController<GraphicsTweaks::UI::SettingsView*>();
    }
    if (!advancedSettingsView) {
        advancedSettingsView = BSML::Helpers::CreateViewController<GraphicsTweaks::UI::AdvancedSettingsView*>();
    }
    if (!presetsView) {
        presetsView = BSML::Helpers::CreateViewController<GraphicsTweaks::UI::PresetsView*>();
    }
}

void GraphicsTweaks::UI::GraphicsTweaksFlowCoordinator::DidActivate(bool firstActivation, bool addedToHeirarchy, bool screenSystemEnabling) {
    if (!firstActivation) return;

    SetTitle("Graphics Tweaks", HMUI::ViewController::AnimationType::In);
    this->____showBackButton = true;
    ProvideInitialViewControllers(settingsView, presetsView,  advancedSettingsView, nullptr, nullptr);
}

void GraphicsTweaks::UI::GraphicsTweaksFlowCoordinator::BackButtonWasPressed(HMUI::ViewController* topViewController) {
    this->Close();
}

void GraphicsTweaks::UI::GraphicsTweaksFlowCoordinator::Close(bool immediately){
    // Do nothing if there's no parent flow coordinator (in multiplayer if you never called it it crashed)
    
    BSML::MainThreadScheduler::ScheduleAfterTime(1.0f,[this]() {
        INFO("Saving GraphicsTweaks config...");
        getGraphicsTweaksConfig().Save();
    });

    if (fcInstance && fcInstance->get_isActiveAndEnabled() && fcInstance->get_isActivated()) {
        this->____parentFlowCoordinator->DismissFlowCoordinator(this, HMUI::ViewController::AnimationDirection::Horizontal, nullptr, immediately);
    }
};