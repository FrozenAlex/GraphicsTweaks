#include "PlatformDetector.hpp"
#include <sys/system_properties.h>
#include "logging.hpp"
#include <string>

static std::string Quest1Model = "Quest";

namespace GraphicsTweaks {
    bool isQuest1 = false;
    bool checkedPlatform = false;
    std::string model = "Unknown";
    std::string brand = "Unknown";
    
    void DetectPlatform() {
        if (checkedPlatform) {
            return;
        }

        {
            auto prop = __system_property_find("ro.product.model");
            if (prop) {
                __system_property_read_callback(prop, [](void* cookie, const char* name, const char* value, uint32_t serial) {
                    std::string parsedModel(value);
                    model = parsedModel;
                }, nullptr);

            } else {
                INFO("ro.product.model not found");
            }
        }

        {
            auto prop = __system_property_find("ro.product.brand");
            if (prop) {
                __system_property_read_callback(prop, [](void* cookie, const char* name, const char* value, uint32_t serial) {
                    std::string parsedBrand(value);
                    brand = parsedBrand;
                }, nullptr);
            } else {
                INFO("ro.product.brand not found");
            }
        } 

        // Detect if the device is a Quest 1 for special settings
        isQuest1 = Quest1Model == model;

        checkedPlatform = true;
    }
}
