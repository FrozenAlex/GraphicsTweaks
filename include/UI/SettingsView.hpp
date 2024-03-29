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



#ifdef HotReload
DECLARE_CLASS_CUSTOM(GraphicsTweaks::UI, SettingsView, BSML::HotReloadViewController,
#else
DECLARE_CLASS_CODEGEN(GraphicsTweaks::UI, SettingsView, HMUI::ViewController,
#endif

    DECLARE_SIMPLE_DTOR();
    DECLARE_OVERRIDE_METHOD_MATCH(void, DidActivate, &HMUI::ViewController::DidActivate, bool firstActivation, bool addedToHeirarchy, bool screenSystemDisabling);

    DECLARE_INSTANCE_METHOD(void, PostParse);

    DECLARE_INSTANCE_METHOD(void, UpdateGraphicsSettings);

    DECLARE_INSTANCE_FIELD(ListW<float>, systemDisplayFrequenciesAvailableValues);
    DECLARE_INSTANCE_FIELD(ListW<StringW>, systemDisplayFrequenciesAvailableLabels);

    // Field values
    BSML_OPTIONS_LIST_OBJECT(foveationLevelOptions, "Off", "Low", "Medium", "High", "HighTop");
    BSML_OPTIONS_LIST_OBJECT(mirrorOptions, "Off", "Low", "Medium", "High");
    BSML_OPTIONS_LIST_OBJECT(antiAliasingOptions, "Off", "2x", "4x");
    BSML_OPTIONS_LIST_OBJECT(bloomQualityOptions, "Off", "Low", "High");
    BSML_OPTIONS_LIST_OBJECT(smokeQualityOptions, "Off", "Low", "High");
    BSML_OPTIONS_LIST_OBJECT(booleanOptions, "Off", "On");
    BSML_OPTIONS_LIST_OBJECT(wallQualityOptions, "Transparent", "Textured", "Distorted");

    // Fields
    DECLARE_INSTANCE_FIELD(StringW, mirrorValue);
    DECLARE_INSTANCE_FIELD(StringW, antiAliasingValue);
    DECLARE_INSTANCE_FIELD(StringW, bloomQualityValue);
    DECLARE_INSTANCE_FIELD(StringW, smokeQualityValue);
    DECLARE_INSTANCE_FIELD(StringW, wallQualityValue);

    DECLARE_INSTANCE_FIELD(int, shockwaveParticlesValue);

    // Instant settings
    DECLARE_INSTANCE_FIELD(StringW, foveationLevelValueMenu);
    DECLARE_INSTANCE_FIELD(StringW, foveationLevelValueGame);
    DECLARE_INSTANCE_FIELD(float, resolutionLevelValueMenu);
    DECLARE_INSTANCE_FIELD(float, resolutionLevelValueGame);
    DECLARE_INSTANCE_FIELD(float, targetFPSValueMenu);
    DECLARE_INSTANCE_FIELD(float, targetFPSValueGame);
    DECLARE_INSTANCE_FIELD(bool, menuScreenDistortionValue);
    DECLARE_INSTANCE_FIELD(bool, gameScreenDistortionValue);

    // Boolean settings
    DECLARE_INSTANCE_FIELD(bool, burnMarksValue);

    // Levels
    DECLARE_INSTANCE_FIELD(int, gpuLevelValue);
    DECLARE_INSTANCE_FIELD(int, cpuLevelValue);
)