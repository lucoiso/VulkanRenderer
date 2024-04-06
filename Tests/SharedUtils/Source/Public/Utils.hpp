// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

#pragma once

#include <string>

import RenderCore.UserInterface.Window;
import RenderCore.UserInterface.Window.Flags;

class ScopedWindow final
{
    RenderCore::Window m_Window {};

    std::string const m_WindowTitle {"Vulkan Renderer: Tests"};
    std::uint16_t const m_WindowWidth {600U};
    std::uint16_t const m_WindowHeight {600U};
    RenderCore::InitializationFlags const m_WindowFlags {RenderCore::InitializationFlags::HEADLESS};

public:
    ScopedWindow();
    ~ScopedWindow();

    RenderCore::Window& GetWindow();
};