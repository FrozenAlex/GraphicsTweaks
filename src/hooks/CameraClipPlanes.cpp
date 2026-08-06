#include "main.hpp"

#include "GlobalNamespace/MainCamera.hpp"
#include "GraphicsTweaksConfig.hpp"
#include "UnityEngine/Camera.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "logging.hpp"

namespace {
SafePtrUnity<GlobalNamespace::MainCamera> mainCamera;
}

MAKE_HOOK_MATCH(
    MainCamera_Awake,
    &GlobalNamespace::MainCamera::Awake,
    void,
    GlobalNamespace::MainCamera* self
) {
    MainCamera_Awake(self);

    mainCamera = self;
    GraphicsTweaks::CameraClipPlanes::Apply();
}

void GraphicsTweaks::CameraClipPlanes::Apply() {
    if (!mainCamera) {
        WARNING("Main camera is not available; clip planes were not changed");
        return;
    }

    auto camera = mainCamera->get_camera();
    if (!camera) {
        WARNING("Main camera is null; clip planes were not changed");
        return;
    }

    const float nearClipPlane =
        getGraphicsTweaksConfig().NearClipPlane.GetValue();
    const float farClipPlane =
        getGraphicsTweaksConfig().FarClipPlane.GetValue();

    if (nearClipPlane <= 0.0f || farClipPlane <= nearClipPlane) {
        WARNING(
            "Invalid camera clip planes (near: {}, far: {}); keeping near: {}, far: {}",
            nearClipPlane,
            farClipPlane,
            camera->get_nearClipPlane(),
            camera->get_farClipPlane()
        );
        return;
    }

    const float originalNearClipPlane = camera->get_nearClipPlane();
    const float originalFarClipPlane = camera->get_farClipPlane();

    camera->set_nearClipPlane(nearClipPlane);
    camera->set_farClipPlane(farClipPlane);

    INFO(
        "Changed main camera clip planes from near: {}, far: {} to near: {}, far: {}",
        originalNearClipPlane,
        originalFarClipPlane,
        nearClipPlane,
        farClipPlane
    );
}

void GraphicsTweaks::Hooks::CameraClipPlanes() {
    INSTALL_HOOK(Logger, MainCamera_Awake);
}
