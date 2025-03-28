#pragma once

#include "config-utils/shared/config-utils.hpp"

DECLARE_CONFIG(GraphicsTweaksConfig) {
    CONFIG_VALUE(DynamicResolution, bool, "Dynamic Resolution", false);
    CONFIG_VALUE(MenuResolution, float, "Menu Resolution", 1);
    CONFIG_VALUE(GameResolution, float, "Game Resolution", 1);
    CONFIG_VALUE(AntiAliasing, int, "Anti Aliasing", 2);
    CONFIG_VALUE(MenuRefreshRate, float, "Menu Refresh Rate", -1); // We don't know the Refresh Rate, at this time.
    CONFIG_VALUE(GameRefreshRate, float, "Game Refresh Rate", -1); // We don't know the Refresh Rate, at this time.
    CONFIG_VALUE(Bloom, bool, "Bloom", false);
    CONFIG_VALUE(NumShockwaves, int, "NumShockwaves", 0);
    CONFIG_VALUE(BloomQuality, int, "BloomQuality", 0); // 0 = Low Quality, 1 = High Quality
    CONFIG_VALUE(Mirror, int, "Mirror", 1); // 0 = Off, 1 = Low, 2 = Medium, 3 = High
    CONFIG_VALUE(SmokeQuality, int, "SmokeQuality", 1); // 0 = Off, 1 = Low, 3 = High
    CONFIG_VALUE(WallQuality, int, "WallQuality", 1); // 0 = Transparent, 1 = Textured, 2 = Distorted
    CONFIG_VALUE(EnhancedLQMirror, bool, "EnhancedLQMirror", true);
    CONFIG_VALUE(FpsCounter, bool, "FpsCounter", true);
    CONFIG_VALUE(FpsCounterAdvanced, bool, "FpsCounterAdvanced", false);
    CONFIG_VALUE(LightBrightness, float, "LightBrightness", 1);
    CONFIG_VALUE(MenuFoveatedRenderingLevel, int, "MenuFoveatedRenderingLevel", 0);
    CONFIG_VALUE(InGameFoveatedRenderingLevel, int, "InGameFoveatedRenderingLevel", 4);
    CONFIG_VALUE(ColorSpace, int, "ColorSpace", 4);
    CONFIG_VALUE(CpuLevel, int, "CpuLevel", 2);
    CONFIG_VALUE(GpuLevel, int, "GpuLevel", 4);
    CONFIG_VALUE(Burnmarks, bool, "Burnmarks", true);
    CONFIG_VALUE(MenuShockwaves, bool, "MenuShockwaves", true);
    CONFIG_VALUE(GameShockwaves, bool, "GameShockwaves", true);
    CONFIG_VALUE(UsedGraphicsPresetBefore, bool, "UsedGraphicsPresetBefore", false);
};
