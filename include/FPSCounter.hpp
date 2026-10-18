#pragma once
#include <span>
#include "beatsaber-hook/shared/safeptr.hpp"

#include "custom-types/shared/coroutine.hpp"

#include "UnityEngine/AssetBundle.hpp"
#include "UnityEngine/GameObject.hpp"

namespace GraphicsTweaks
{
    class FPSCounter
    {
    public:
        static safe_ptr<UnityEngine::GameObject*> counter;

        static void UpdateCounter();

        static custom_types::Helpers::Coroutine LoadBund();

        static custom_types::Helpers::Coroutine LoadBundleFromMemoryAsync(std::span<const uint8_t> bytes, UnityEngine::AssetBundle*& out);

        static custom_types::Helpers::Coroutine LoadAssetFromBundleAsync(UnityEngine::AssetBundle* bundle, std::string_view name, System::Type* type, UnityEngine::Object*& out);
    };
};
