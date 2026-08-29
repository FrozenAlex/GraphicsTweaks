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
    std::vector<float> predefinedRefreshRates;
    
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

        // Use Meta's documented app refresh rates for known Quest models.
        // 60 Hz is intentionally omitted because it is reserved for media apps.
        if (model == "Quest") {
            predefinedRefreshRates = {72.0f};
        } else if (model == "Quest 2") {
            predefinedRefreshRates = {72.0f, 80.0f, 90.0f, 96.0f, 100.0f, 120.0f};
        } else if (model == "Quest Pro") {
            predefinedRefreshRates = {72.0f, 80.0f, 90.0f};
        } else if (model == "Quest 3") {
            predefinedRefreshRates = {72.0f, 80.0f, 90.0f, 96.0f, 100.0f, 120.0f, 207.0f};
        } else if (model == "Quest 3S") {
            predefinedRefreshRates = {72.0f, 80.0f, 90.0f, 96.0f, 100.0f, 120.0f};
        } else {
            predefinedRefreshRates.clear();
        }

        checkedPlatform = true;
    }
}
