#pragma once
#include <string>
#include "bsml/shared/BSML/Components/Settings/IncrementSetting.hpp"

namespace GraphicsTweaks::Formatters {
    std::string FormatTextureResolution(float value);
    std::string FormatIncrementAsIntegers(float value);

    void FormatSetting(UnityW<BSML::IncrementSetting> element);
}

