#pragma once

#include "custom-types/shared/coroutine.hpp"

#include "UnityEngine/AssetBundle.hpp"
#include "UnityEngine/GameObject.hpp"

namespace GraphicsTweaks
{
    class FPSCounter
    {
    public:
        static SafePtrUnity<UnityEngine::GameObject> counter;

        static void UpdateCounter();

        static custom_types::Helpers::Coroutine LoadBund();

        static custom_types::Helpers::Coroutine LoadBundleFromMemoryAsync(ArrayW<uint8_t> bytes, UnityEngine::AssetBundle*& out);

        static custom_types::Helpers::Coroutine LoadAssetFromBundleAsync(UnityEngine::AssetBundle* bundle, std::string_view name, System::Type* type, UnityEngine::Object*& out);
    };
};