#include "UI/PresetsView.hpp"
#include "assets.hpp"
#include "logging.hpp"
#include "GraphicsTweaksConfig.hpp"
#include "PlatformDetector.hpp"
#include "main.hpp"


DEFINE_TYPE(GraphicsTweaks::UI, PresetsView);

void GraphicsTweaks::UI::PresetsView::DidActivate(bool firstActivation, bool addedToHeirarchy, bool screenSystemDisabling)
{
    if (!firstActivation || !GraphicsTweaks::isQuest)
        return;

    INFO("PresetsView activated");

   
    BSML::parse_and_construct(Assets::PresetsView_bsml, this->get_transform(), this);

    #ifdef HotReload
        fileWatcher->filePath = "/sdcard/bsml/GraphicsTweaks/PresetsView.bsml";
    #endif
};

StringW GraphicsTweaks::UI::PresetsView::get_questSharpeningValue() {
    switch (getGraphicsTweaksConfig().QuestSharpening.GetValue()) {
        case 1: return "Normal";
        case 2: return "Quality";
        default: return "Off";
    }
}

void GraphicsTweaks::UI::PresetsView::set_questSharpeningValue(StringW value) {
    int level = value == "Quality" ? 2 : value == "Normal" ? 1 : 0;
    getGraphicsTweaksConfig().QuestSharpening.SetValue(level, true);
    GraphicsTweaks::VRRenderingParamsSetup::Reload();
}
