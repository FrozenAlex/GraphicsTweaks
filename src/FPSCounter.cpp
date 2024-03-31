#include "FPSCounter.hpp"
#include "main.hpp"
#include "logging.hpp"
#include <string_view>

#include "UnityEngine/AssetBundleCreateRequest.hpp"
#include "UnityEngine/AssetBundleRequest.hpp"
#include "UnityEngine/WaitForSeconds.hpp"
#include "UnityEngine/Canvas.hpp"
#include "HMUI/CurvedCanvasSettings.hpp"

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
        //counterObj->get_transform()->set_localScale({0.1f,0.1f,0.1f});
        //counterObj->get_transform()->set_position({0,3,3});

        counter = counterObj;//BSML::FloatingScreen::CreateFloatingScreen({200,200}, true, {0,2,2}, UnityEngine::Quaternion::Euler(-10,0,0), 0.0f, false)->get_gameObject();

        //counterObj->get_transform()->SetParent(counter->get_transform(), false);
        //counterObj->get_transform()->set_localPosition({3,3,0});

        //UnityEngine::GameObject::DontDestroyOnLoad(counter.ptr());
        //counter->SetActive(true);

        co_return;
    }

    custom_types::Helpers::Coroutine FPSCounter::LoadBundleFromMemoryAsync(ArrayW<uint8_t> bytes, UnityEngine::AssetBundle*& out)
    {
        using AssetBundle_LoadFromMemoryAsync = function_ptr_t<UnityEngine::AssetBundleCreateRequest*, ArrayW<uint8_t>, int>;
        static AssetBundle_LoadFromMemoryAsync assetBundle_LoadFromMemoryAsync = reinterpret_cast<AssetBundle_LoadFromMemoryAsync>(il2cpp_functions::resolve_icall("UnityEngine.AssetBundle::LoadFromMemoryAsync_Internal"));

        auto req = assetBundle_LoadFromMemoryAsync(bytes, 0);
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