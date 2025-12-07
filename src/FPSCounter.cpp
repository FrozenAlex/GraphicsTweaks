#include "FPSCounter.hpp"
#include "logging.hpp"
#include <string_view>

#include "UnityEngine/AssetBundleCreateRequest.hpp"
#include "UnityEngine/AssetBundleRequest.hpp"
#include "GraphicsTweaksConfig.hpp"
#include "UnityEngine/Bindings/ManagedSpanWrapper.hpp"

#include "assets.hpp"

namespace GraphicsTweaks
{
    #define coro(...) custom_types::Helpers::CoroutineHelper::New(__VA_ARGS__)

    SafePtrUnity<UnityEngine::GameObject> FPSCounter::counter;

    custom_types::Helpers::Coroutine FPSCounter::LoadBund()
    {
        UnityEngine::AssetBundle* ass;
        co_yield coro(FPSCounter::LoadBundleFromMemoryAsync(Assets::fpscounter_bund, ass));
        if (!ass)
        {
            ERROR("Couldn't load bundle from file, dieing...");
            co_return;
        }

        UnityEngine::GameObject* data = nullptr;
        co_yield coro(FPSCounter::LoadAssetFromBundleAsync(ass, "Assets/[Graphy] VR.prefab", csTypeOf(UnityEngine::GameObject*), reinterpret_cast<UnityEngine::Object*&>(data)));
        if(data == nullptr)
        {
            ERROR("Couldn't load asset...");
            co_return;
        }
        ass->Unload(false);

        auto counterObj = UnityEngine::GameObject::Instantiate(data);
        UnityEngine::GameObject::DontDestroyOnLoad(counterObj);
        counterObj->SetActive(true);
        counter = counterObj;

        bool showAdvancedCounter = getGraphicsTweaksConfig().FpsCounterAdvanced.GetValue();
        if (counter) {
            counter->SetActive(showAdvancedCounter);
        }

        co_return;
    }

    ::UnityEngine::AssetBundleCreateRequest* LoadBundleFromMemoryAsync_Internal(ArrayW<uint8_t> bytes, uint32_t crc = 0) {
        using AssetBundle_LoadFromMemoryAsync = function_ptr_t<void *, ByRef<UnityEngine::Bindings::ManagedSpanWrapper>, uint32_t>;
        static AssetBundle_LoadFromMemoryAsync assetBundle_LoadFromMemoryAsync = reinterpret_cast<AssetBundle_LoadFromMemoryAsync>(il2cpp_functions::resolve_icall("UnityEngine.AssetBundle::LoadFromMemoryAsync_Internal_Injected"));
        CRASH_UNLESS(assetBundle_LoadFromMemoryAsync);
        
        auto *managedSpanWrapper = new UnityEngine::Bindings::ManagedSpanWrapper(bytes->begin(), bytes.size());

        void* request = assetBundle_LoadFromMemoryAsync(*managedSpanWrapper, crc);
        if (!request)
        {
            return nullptr;
        }
        return UnityEngine::AssetBundleCreateRequest::BindingsMarshaller::ConvertToManaged(request);
    }

    custom_types::Helpers::Coroutine FPSCounter::LoadBundleFromMemoryAsync(ArrayW<uint8_t> bytes, UnityEngine::AssetBundle*& out)
    {
        auto *req = LoadBundleFromMemoryAsync_Internal(bytes);
        if (!req)
        {
            ERROR("Failed to create AssetBundleCreateRequest!");
            out = nullptr;
            co_return;
        }

        req->set_allowSceneActivation(true);
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