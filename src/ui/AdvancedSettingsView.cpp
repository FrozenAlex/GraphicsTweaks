#include "UI/AdvancedSettingsView.hpp"
#include "GraphicsTweaksConfig.hpp"
#include "assets.hpp"
#include "logging.hpp"
#include "main.hpp"

DEFINE_TYPE(GraphicsTweaks::UI, AdvancedSettingsView);

void GraphicsTweaks::UI::AdvancedSettingsView::DidActivate(bool firstActivation, bool addedToHeirarchy, bool screenSystemDisabling)
{
    if (!firstActivation)
        return;

    INFO("AdvancedSettingsView activated");

    BSML::parse_and_construct(Assets::AdvancedSettingsView_bsml, this->get_transform(), this);
    #ifdef HotReload
        fileWatcher->filePath = "/sdcard/bsml/GraphicsTweaks/AdvancedSettingsView.bsml";
    #endif
};

float GraphicsTweaks::UI::AdvancedSettingsView::get_nearClipPlaneValue() {
    return getGraphicsTweaksConfig().NearClipPlane.GetValue();
}

void GraphicsTweaks::UI::AdvancedSettingsView::set_nearClipPlaneValue(float value) {
    getGraphicsTweaksConfig().NearClipPlane.SetValue(value, true);
    GraphicsTweaks::CameraClipPlanes::Apply();
}

float GraphicsTweaks::UI::AdvancedSettingsView::get_farClipPlaneValue() {
    return getGraphicsTweaksConfig().FarClipPlane.GetValue();
}

void GraphicsTweaks::UI::AdvancedSettingsView::set_farClipPlaneValue(float value) {
    getGraphicsTweaksConfig().FarClipPlane.SetValue(value, true);
    GraphicsTweaks::CameraClipPlanes::Apply();
}

bool GraphicsTweaks::UI::AdvancedSettingsView::get_force32BitDepthBufferValue() {
    return getGraphicsTweaksConfig().Force32BitDepthBuffer.GetValue();
}

void GraphicsTweaks::UI::AdvancedSettingsView::set_force32BitDepthBufferValue(bool value) {
    getGraphicsTweaksConfig().Force32BitDepthBuffer.SetValue(value, true);
}
