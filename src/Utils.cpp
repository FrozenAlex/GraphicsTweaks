#include "Utils.hpp"
#include <optional>
#include "conditional-dependencies/shared/main.hpp"


namespace GraphicsTweaks {
    bool IsInReplay() {
        static std::optional<bool (*)()> isInReplayFunc  = CondDeps::Find<bool>("replay", "IsInReplay");

        if (isInReplayFunc.has_value())
        {
            return isInReplayFunc.value()();
        }

        return false;
    }

    bool IsInRender() {
        static std::optional<bool (*)()> isInRenderFunc  = CondDeps::Find<bool>("replay", "IsInRender");

        if (isInRenderFunc.has_value())
        {
            return isInRenderFunc.value()();
        }
        
        return false;
    }

}