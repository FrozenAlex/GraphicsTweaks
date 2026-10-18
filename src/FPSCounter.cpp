#include "FPSCounter.hpp"
#include "logging.hpp"
#include <string_view>
#include <limits>
#include "beatsaber-hook/shared/api.hpp"
#include "System/IntPtr.hpp"
#include "UnityEngine/Bindings/ManagedSpanWrapper.hpp"

#include "UnityEngine/AssetBundleCreateRequest.hpp"
#include "UnityEngine/AssetBundleRequest.hpp"
#include "GraphicsTweaksConfig.hpp"

#include "assets.hpp"

namespace GraphicsTweaks
{
    #define coro(...) custom_types::Helpers::CoroutineHelper::New(__VA_ARGS__)

    safe_ptr<UnityEngine::GameObject*> FPSCounter::counter;

    custom_types::Helpers::Coroutine FPSCounter::LoadBund()
    {
        UnityEngine::AssetBundle* ass = nullptr;
        co_yield coro(FPSCounter::LoadBundleFromMemoryAsync(Assets::fpscounter_bund, ass));
        if (!ass)
        {
            ERROR("Could not load the embedded FPS bundle.");
            co_return;
        }

        UnityEngine::GameObject* data = nullptr;
        co_yield coro(FPSCounter::LoadAssetFromBundleAsync(ass, "Assets/[Graphy] VR.prefab", i2c::cs_type_of<UnityEngine::GameObject*>(), reinterpret_cast<UnityEngine::Object*&>(data)));
        if(data == nullptr)
        {
            ERROR("Couldn't load asset...");
            co_return;
        }
        ass->Unload(false);

        // Keep Graphy inactive until the instance is owned and its requested
        // visibility is known. The rebuilt bundle contains current font assets.
        data->SetActive(false);
        auto counterObj = UnityEngine::GameObject::Instantiate(data);
        UnityEngine::GameObject::DontDestroyOnLoad(counterObj);
        counter = counterObj;

        bool showAdvancedCounter = getGraphicsTweaksConfig().FpsCounterAdvanced.GetValue();
        if (counter) {
            counter->SetActive(showAdvancedCounter);
        }

        co_return;
    }

    custom_types::Helpers::Coroutine FPSCounter::LoadBundleFromMemoryAsync(std::span<const uint8_t> bytes, UnityEngine::AssetBundle*& out)
    {
        out = nullptr;
        if (bytes.empty() || bytes.size() > std::numeric_limits<int32_t>::max()) {
            ERROR("Invalid embedded FPS bundle size.");
            co_return;
        }

        using UnityEngine::Bindings::ManagedSpanWrapper;
        static auto loadFromMemory = i2c::resolve_icall<System::IntPtr, ManagedSpanWrapper*, uint32_t>(
            "UnityEngine.AssetBundle::LoadFromMemoryAsync_Internal_Injected");
        if (!loadFromMemory) {
            ERROR("FPS bundle loading requires Unity's LoadFromMemoryAsync icall.");
            co_return;
        }

        // Unity 6's injected binding takes a span and returns a native request pointer.
        ManagedSpanWrapper span{const_cast<uint8_t*>(bytes.data()), static_cast<int32_t>(bytes.size())};
        auto nativeRequest = (*loadFromMemory)(&span, 0);
        if (!nativeRequest.m_value) {
            ERROR("Could not create the FPS bundle load request.");
            co_return;
        }
        safe_ptr<UnityEngine::AssetBundleCreateRequest*> req(
            UnityEngine::AssetBundleCreateRequest::BindingsMarshaller::ConvertToManaged(nativeRequest));
        while (!req->get_isDone())
            co_yield nullptr;

        out = req->get_assetBundle();
        co_return;
    }

    custom_types::Helpers::Coroutine FPSCounter::LoadAssetFromBundleAsync(UnityEngine::AssetBundle* bundle, std::string_view name, System::Type* type, UnityEngine::Object*& out)
    {
        auto req = bundle->LoadAssetAsync(name, type);
        req->set_allowSceneActivation(true);
        while (!req->get_isDone())
            co_yield nullptr;

        out = req->get_asset();

        co_return;
    }
}
