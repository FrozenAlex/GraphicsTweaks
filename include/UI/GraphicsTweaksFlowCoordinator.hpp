#pragma once

#include "custom-types/shared/coroutine.hpp"
#include "custom-types/shared/macros.hpp"

#include "HMUI/ViewController.hpp"
#include "HMUI/FlowCoordinator.hpp"
#include "UI/AdvancedSettingsView.hpp"
#include "UI/PresetsView.hpp"
#include "UI/SettingsView.hpp"

#define GET_FIND_METHOD(mPtr) il2cpp_utils::il2cpp_type_check::MetadataGetter<mPtr>::get()

DECLARE_CLASS_CODEGEN(GraphicsTweaks::UI, GraphicsTweaksFlowCoordinator, HMUI::FlowCoordinator) {
    DECLARE_INSTANCE_FIELD(GraphicsTweaks::UI::AdvancedSettingsView*, advancedSettingsView);
    DECLARE_INSTANCE_FIELD(GraphicsTweaks::UI::PresetsView*, presetsView);
    DECLARE_INSTANCE_FIELD(GraphicsTweaks::UI::SettingsView*, settingsView);

    DECLARE_INSTANCE_METHOD(void, Awake);

    DECLARE_OVERRIDE_METHOD_MATCH(void, DidActivate, &HMUI::FlowCoordinator::DidActivate, bool firstActivation, bool addedToHeirarchy, bool screenSystemEnabling);
    DECLARE_OVERRIDE_METHOD_MATCH(void, BackButtonWasPressed, &HMUI::FlowCoordinator::BackButtonWasPressed, HMUI::ViewController* topViewController);

    void Close(bool immediately = false);


};


inline SafePtrUnity<GraphicsTweaks::UI::GraphicsTweaksFlowCoordinator> fcInstance; // Stores the ID and version of our mod, and is sent to the modloader upon startup
