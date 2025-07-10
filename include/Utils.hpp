#pragma once

namespace GraphicsTweaks {
    /**
    * @brief Checks if the application is currently in a rendering state.
    *
    * This function uses conditional dependencies to check if the game is in a
    * rendering state according to the Replay mod. It retrieves a function pointer
    * to the "IsInRender" function from the Replay mod and invokes it if
    * available.
    *
    * @return true if the application is in a replay state, false otherwise.
    */
    bool IsInRender();
    /**
    * @brief Checks if the application is currently in a replay state.
    *
    * This function uses conditional dependencies to check if the game is in a
    * replay state according to the Replay mod. It retrieves a function pointer
    * to the "IsInReplay" function from the Replay mod and invokes it if
    * available.
    *
    * @return true if the application is in a replay state, false otherwise.
    */
    bool IsInReplay();
}
