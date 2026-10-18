#include "main.hpp"
#include "logging.hpp"
#include "beatsaber-hook/shared/hooking.hpp"
#include "GlobalNamespace/OVRPlugin.hpp"
#include "Tayx/Graphy/Fps/G_FpsGraph.hpp"
#include "Tayx/Graphy/Fps/G_FpsMonitor.hpp"
#include "UnityEngine/Time.hpp"
#include <cmath>

namespace {
    // The game's Graphy stores each frame's FPS as a 16-bit integer from
    // 1 / unscaledDeltaTime. Around headset sleep/wake Unity reports frames
    // with zero or near-zero delta: their FPS wraps negative (corrupting the
    // 1% and 0.1% lows) or becomes a huge graph peak that decays by only one
    // FPS per frame, flattening the graph for minutes. A presented XR frame
    // cannot be shorter than half a display refresh, so such frames are skipped.
    bool IsPresentedFrame() {
        static int checkedFrame = -1;
        static bool presented = true;
        int frame = UnityEngine::Time::get_frameCount();
        if (frame == checkedFrame) return presented;
        checkedFrame = frame;
        float deltaTime = UnityEngine::Time::get_unscaledDeltaTime();
        float refreshRate = GlobalNamespace::OVRPlugin::get_systemDisplayFrequency();
        float maxFps = refreshRate > 0.0f ? refreshRate * 2.0f : 1000.0f;
        presented = std::isfinite(deltaTime) && deltaTime > 0.0f && 1.0f / deltaTime <= maxFps;
        return presented;
    }
}

MAKE_HOOK_MATCH(G_FpsMonitor_Update, &Tayx::Graphy::Fps::G_FpsMonitor::Update,
    void, Tayx::Graphy::Fps::G_FpsMonitor* self) {
    if (IsPresentedFrame()) G_FpsMonitor_Update(self);
}

MAKE_HOOK_MATCH(G_FpsGraph_UpdateGraph, &Tayx::Graphy::Fps::G_FpsGraph::UpdateGraph,
    void, Tayx::Graphy::Fps::G_FpsGraph* self) {
    if (IsPresentedFrame()) G_FpsGraph_UpdateGraph(self);
}

void GraphicsTweaks::Hooks::GraphyFps() {
    INSTALL_HOOK(Logger, G_FpsMonitor_Update);
    INSTALL_HOOK(Logger, G_FpsGraph_UpdateGraph);
}
