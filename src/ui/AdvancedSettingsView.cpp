#include "UI/AdvancedSettingsView.hpp"
#include "assets.hpp"
#include "logging.hpp"

DEFINE_TYPE(GraphicsTweaks::UI, AdvancedSettingsView);

void GraphicsTweaks::UI::AdvancedSettingsView::DidActivate(bool firstActivation, bool addedToHeirarchy, bool screenSystemDisabling)
{
    if (!firstActivation)
        return;

    INFO("AdvancedSettingsView activated");

    BSML::parse_and_construct(Assets::AdvancedSettingsView_bsml, this->get_transform(), this);

};