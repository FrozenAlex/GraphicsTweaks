#include "main.hpp"

#include "GraphicsTweaksConfig.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "logging.hpp"

#include <cerrno>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <dlfcn.h>
#include <sys/mman.h>
#include <unistd.h>

namespace {

// Unity 2022.3.33f1 (b2c853adf198), GNU build ID d72b76a21d6560db.
// XRTextureManager::SetupRenderTextureFromXRRequest selects D16_UNorm here:
//     mov w1, #0x5a
constexpr std::uintptr_t kDepthFormatInstructionOffset = 0x11602F0;
constexpr std::uintptr_t kSetDepthStencilFormatOffset = 0x9C6E30;
constexpr std::uintptr_t kGetDepthStencilFormatOffset = 0x9C2B50;
constexpr std::uint32_t kExpectedD16Instruction = 0x52800B41;

// Replace GraphicsFormat::D16_UNorm (90 / 0x5a) with
// GraphicsFormat::D32_SFloat (93 / 0x5d):
//     mov w1, #0x5d
constexpr std::uint32_t kD32Instruction = 0x52800BA1;
constexpr std::uint32_t kExpectedFollowingInstruction = 0xAA1403E0;
constexpr std::uint32_t kD16UNorm = 90;
constexpr std::uint32_t kD24UNorm = 91;
constexpr std::uint32_t kD24UNormS8 = 92;
constexpr std::uint32_t kD32SFloat = 93;
constexpr std::uint32_t kD32SFloatS8 = 94;

std::uintptr_t unityBaseAddress = 0;

MAKE_HOOK(
    RenderTexture_SetDepthStencilFormat,
    nullptr,
    void,
    void* renderTexture,
    std::uint32_t requestedFormat
) {
    const auto caller = reinterpret_cast<std::uintptr_t>(
        __builtin_return_address(0)
    );

    // Keep stencil capability intact while upgrading the depth component.
    // This catches the D24 path actually used by this Quest build rather than
    // relying on one XRTextureManager call site.
    std::uint32_t upgradedFormat = requestedFormat;
    if (requestedFormat == kD16UNorm || requestedFormat == kD24UNorm) {
        upgradedFormat = kD32SFloat;
    } else if (requestedFormat == kD24UNormS8) {
        upgradedFormat = kD32SFloatS8;
    }

    RenderTexture_SetDepthStencilFormat(renderTexture, upgradedFormat);

    // This lower-level setter is shared by every RenderTexture creation path,
    // so it does not depend on XRTextureManager setup running after mod load.
    if (!renderTexture || !unityBaseAddress || requestedFormat < kD16UNorm ||
        requestedFormat > kD32SFloatS8) {
        return;
    }

    static std::atomic_uint32_t loggedCalls{0};
    const auto call = loggedCalls.fetch_add(1, std::memory_order_relaxed);
    if (call >= 32) {
        return;
    }

    const auto storedFormat = *reinterpret_cast<std::uint32_t*>(
        reinterpret_cast<std::uintptr_t>(renderTexture) + 0x15C
    );

    using GetDepthStencilFormat = std::uint32_t (*)(void const*, bool);
    auto getDepthStencilFormat = reinterpret_cast<GetDepthStencilFormat>(
        unityBaseAddress + kGetDepthStencilFormatOffset
    );
    const std::uint32_t resolvedFormat = getDepthStencilFormat(
        renderTexture,
        false
    );

    INFO(
        "RenderTexture depth format call {} from libunity + 0x{:x}: requested {}, upgraded {}, stored {}, resolved supported {}",
        call + 1,
        caller >= unityBaseAddress ? caller - unityBaseAddress : 0,
        requestedFormat,
        upgradedFormat,
        storedFormat,
        resolvedFormat
    );
}

bool PatchUnityDepthFormat() {
    auto* jniOnLoad = dlsym(modloader_unity_handle, "JNI_OnLoad");
    if (!jniOnLoad) {
        ERROR("Could not resolve JNI_OnLoad in libunity.so");
        return false;
    }

    Dl_info unityInfo{};
    if (!dladdr(jniOnLoad, &unityInfo) || !unityInfo.dli_fbase) {
        ERROR("Could not resolve the libunity.so base address");
        return false;
    }

    unityBaseAddress = reinterpret_cast<std::uintptr_t>(unityInfo.dli_fbase);

    auto* instruction = reinterpret_cast<std::uint32_t*>(
        unityBaseAddress +
        kDepthFormatInstructionOffset
    );

    if (instruction[0] != kExpectedD16Instruction ||
        instruction[1] != kExpectedFollowingInstruction) {
        ERROR(
            "Refusing Unity depth patch: unexpected instructions at +0x{:x}: 0x{:08x}, 0x{:08x}",
            kDepthFormatInstructionOffset,
            instruction[0],
            instruction[1]
        );
        return false;
    }

    const long pageSize = sysconf(_SC_PAGESIZE);
    if (pageSize <= 0) {
        ERROR("Could not determine the system page size");
        return false;
    }

    const auto pageAddress =
        reinterpret_cast<std::uintptr_t>(instruction) &
        ~(static_cast<std::uintptr_t>(pageSize) - 1);

    if (mprotect(
            reinterpret_cast<void*>(pageAddress),
            static_cast<std::size_t>(pageSize),
            PROT_READ | PROT_WRITE | PROT_EXEC
        ) != 0) {
        ERROR(
            "Could not make the Unity code page writable: {}",
            std::strerror(errno)
        );
        return false;
    }

    instruction[0] = kD32Instruction;
    __builtin___clear_cache(
        reinterpret_cast<char*>(instruction),
        reinterpret_cast<char*>(instruction + 1)
    );

    if (mprotect(
            reinterpret_cast<void*>(pageAddress),
            static_cast<std::size_t>(pageSize),
            PROT_READ | PROT_EXEC
        ) != 0) {
        WARNING(
            "Unity depth patch applied, but restoring page protection failed: {}",
            std::strerror(errno)
        );
    }

    INFO(
        "Patched Unity XR depth format from D16_UNorm to D32_SFloat at libunity + 0x{:x}",
        kDepthFormatInstructionOffset
    );
    return true;
}

} // namespace

void GraphicsTweaks::Hooks::DepthBufferPrecision() {
    if (!getGraphicsTweaksConfig().Force32BitDepthBuffer.GetValue()) {
        INFO("Unity 32-bit XR depth patch is disabled");
        return;
    }

    if (!PatchUnityDepthFormat()) {
        return;
    }

    auto* setDepthStencilFormat = reinterpret_cast<void*>(
        unityBaseAddress + kSetDepthStencilFormatOffset
    );
    INSTALL_HOOK_DIRECT(
        Logger,
        RenderTexture_SetDepthStencilFormat,
        setDepthStencilFormat
    );
}
