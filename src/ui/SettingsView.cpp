#include "UI/SettingsView.hpp"
#include "assets.hpp"
#include "logging.hpp"
#include "GlobalNamespace/OVRPlugin.hpp"

DEFINE_TYPE(GraphicsTweaks::UI, SettingsView);

void GraphicsTweaks::UI::SettingsView::DidActivate(bool firstActivation, bool addedToHeirarchy, bool screenSystemDisabling)
{
    if (!firstActivation)
        return;

    INFO("SettingsView activated");

    BSML::parse_and_construct(Assets::SettingsView_bsml, this->get_transform(), this);

};