#include "UI/PresetsView.hpp"
#include "assets.hpp"
#include "logging.hpp"


DEFINE_TYPE(GraphicsTweaks::UI, PresetsView);

void GraphicsTweaks::UI::PresetsView::DidActivate(bool firstActivation, bool addedToHeirarchy, bool screenSystemDisabling)
{
    if (!firstActivation)
        return;

    INFO("PresetsView activated");

   
    BSML::parse_and_construct(Assets::PresetsView_bsml, this->get_transform(), this);

    #ifdef HotReload
        fileWatcher->filePath = "/sdcard/bsml/GraphicsTweaks/PresetsView.bsml";
    #endif
};