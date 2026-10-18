#include <string>
#include <vector>

namespace GraphicsTweaks {
    extern bool isQuest;
    // Is it a quest 1? (needs to be set in setup)
    extern bool isQuest1;
    extern std::string model;
    extern std::string brand;
    // Empty for unrecognized headsets, which should use the runtime API.
    extern std::vector<float> predefinedRefreshRates;
    

    void DetectPlatform();
}
