    #pragma once

#include "bsml/shared/macros.hpp"
#include "bsml/shared/BSML.hpp"
#include "bsml/shared/BSML/Components/Settings/SliderSetting.hpp"
#include "bsml/shared/BSML/Components/Settings/DropdownListSetting.hpp"
#include "bsml/shared/BSML/Components/Settings/ListSetting.hpp"
#include "bsml/shared/BSML/Components/Settings/StringSetting.hpp"
#include "bsml/shared/BSML/ViewControllers/HotReloadViewController.hpp"
#include "bsml/shared/BSML/Components/ClickableText.hpp"
#include "UnityEngine/UI/HorizontalLayoutGroup.hpp"
#include "HMUI/ViewController.hpp"
#include "custom-types/shared/coroutine.hpp"
#include "custom-types/shared/macros.hpp"
#include "bsml/shared/BSML/Components/TabSelector.hpp"
#include "HMUI/CurvedTextMeshPro.hpp"



#ifdef HotReload
DECLARE_CLASS_CUSTOM(GraphicsTweaks::UI, SettingsView, BSML::HotReloadViewController) {
#else
DECLARE_CLASS_CODEGEN(GraphicsTweaks::UI, SettingsView, HMUI::ViewController) {
#endif

    DECLARE_SIMPLE_DTOR();
    DECLARE_OVERRIDE_METHOD_MATCH(void, DidActivate, &HMUI::ViewController::DidActivate, bool firstActivation, bool addedToHeirarchy, bool screenSystemDisabling);

    DECLARE_INSTANCE_METHOD(void, PostParse);
    DECLARE_INSTANCE_METHOD(void, UpdateGraphicsSettings);

    DECLARE_INSTANCE_FIELD(ListW<float>, systemDisplayFrequenciesAvailableValues);
    DECLARE_INSTANCE_FIELD(ListW<StringW>, systemDisplayFrequenciesAvailableLabels);

    // UI Elements
    DECLARE_INSTANCE_FIELD(UnityW<BSML::IncrementSetting>, shockwaveParticlesCountElement);
    DECLARE_INSTANCE_FIELD(UnityW<BSML::IncrementSetting>, resolutionLevelElementGame);
    DECLARE_INSTANCE_FIELD(UnityW<BSML::IncrementSetting>, resolutionLevelElementMenu);
    DECLARE_INSTANCE_FIELD(UnityW<BSML::TabSelector>, perfTabSelector);
    DECLARE_INSTANCE_FIELD(UnityW<HMUI::CurvedTextMeshPro>, perfTabSelectorTitle);

    // Field values
    BSML_OPTIONS_LIST_OBJECT(foveationLevelOptions, "Off", "Low", "Medium", "High", "HighTop");
    BSML_OPTIONS_LIST_OBJECT(perfLevelOptions, "PowerSavings", "SutainedLow", "SustainedHigh");
    BSML_OPTIONS_LIST_OBJECT(mirrorOptions, "Off", "Low", "Medium", "High");
    BSML_OPTIONS_LIST_OBJECT(antiAliasingOptions, "Off", "2x", "4x");
    BSML_OPTIONS_LIST_OBJECT(bloomQualityOptions, "Off", "On");
    BSML_OPTIONS_LIST_OBJECT(smokeQualityOptions, "Off", "Low", "High");
    BSML_OPTIONS_LIST_OBJECT(booleanOptions, "Off", "On");
    BSML_OPTIONS_LIST_OBJECT(wallQualityOptions, "Transparent", "Textured", "Distorted");
    BSML_OPTIONS_LIST_OBJECT(colorSpaceOptions, "Unknown", "Unmanaged", "Rec_2020", "Rec_709", "Rift_CV1", "Rift_S", "Quest", "P3", "Adobe_RGB" );

    // Fields
    DECLARE_BSML_PROPERTY(StringW, mirrorValue);
    DECLARE_BSML_PROPERTY(StringW, antiAliasingValue);
    DECLARE_BSML_PROPERTY(StringW, bloomQualityValue);
    DECLARE_BSML_PROPERTY(StringW, smokeQualityValue);
    DECLARE_BSML_PROPERTY(StringW, wallQualityValue);

    DECLARE_BSML_PROPERTY(float, shockwaveParticlesValue);
    DECLARE_BSML_PROPERTY(StringW, foveationLevelValueMenu);
    DECLARE_BSML_PROPERTY(StringW, foveationLevelValueGame);

    DECLARE_BSML_PROPERTY(float, resolutionLevelValueMenu);
    DECLARE_BSML_PROPERTY(float, resolutionLevelValueGame);

    DECLARE_BSML_PROPERTY(StringW, targetFPSValueMenu);
    DECLARE_BSML_PROPERTY(StringW, targetFPSValueGame);



    // Instant settings
    DECLARE_BSML_PROPERTY(bool, menuScreenDistortionValue);
    DECLARE_BSML_PROPERTY(bool, gameScreenDistortionValue);

    // Boolean settings
    DECLARE_BSML_PROPERTY(bool, burnMarksValue);
    DECLARE_BSML_PROPERTY(bool, FPSCounterValue);
    DECLARE_BSML_PROPERTY(bool, FPSCounterAdvancedValue);
    // DECLARE_BSML_PROPERTY(bool, dynamicResolutionValue);

    // Levels
    DECLARE_BSML_PROPERTY(StringW, gpuPerfLevelValue);
    DECLARE_BSML_PROPERTY(StringW, cpuPerfLevelValue);

    // Color space
    DECLARE_BSML_PROPERTY(StringW, colorSpaceValue);
};
