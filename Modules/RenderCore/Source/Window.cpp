// Copyright Notice: [...]

#include "Window.h"
#include "VulkanRender.h"
#include <stdexcept>
#include <boost/log/trivial.hpp>

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

using namespace RenderCore;

class Window::Impl
{
public:
    Impl(const Window&) = delete;
    Impl& operator=(const Window&) = delete;

    Impl()
        : m_Window(nullptr)
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Window Implementation";
    }

    ~Impl()
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destructing Window Implementation";

        Shutdown();
    }

    bool Initialize(const std::uint16_t Width, const std::uint16_t Height, const std::string_view Title)
    {
        if (IsInitialized())
        {
            throw std::runtime_error("Window is already initialized");
        }

        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Initializing Window";

        if (!glfwInit())
        {
            throw std::runtime_error("Failed to initialize GLFW Library");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        if (m_Window = glfwCreateWindow(Width, Height, Title.data(), nullptr, nullptr); !m_Window)
        {
            throw std::runtime_error("Failed to create GLFW Window");
        }

        if (m_Render = std::make_unique<RenderCore::VulkanRender>(); m_Render && m_Render->Initialize(m_Window))
        {
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Window initialized";
            return true;
        }

        Shutdown();

        return false;
    }

    void Shutdown()
    {
        if (IsInitialized())
        {
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down Window";

            glfwDestroyWindow(m_Window);
            glfwTerminate();

            m_Window = nullptr;
        }
    }

    bool IsInitialized() const
    {
        return m_Window != nullptr && m_Render != nullptr;
    }

    bool IsOpen() const
    {
        return !ShouldClose();
    }

    bool ShouldClose() const
    {
        return glfwWindowShouldClose(m_Window);
    }

    void PollEvents()
    {
        glfwPollEvents();
    }

private:
    GLFWwindow* m_Window;
    std::unique_ptr<VulkanRender> m_Render;
};

Window::Window()
    : m_Impl(std::make_unique<Window::Impl>())
{
}

Window::~Window()
{
    Shutdown();
}

bool Window::Initialize(const std::uint16_t Width, const std::uint16_t Height, const std::string_view Title)
{
    return m_Impl && m_Impl->Initialize(Width, Height, Title);
}

void Window::Shutdown()
{
    if (m_Impl)
    {
        m_Impl->Shutdown();
    }
}

bool Window::IsInitialized() const
{
    return m_Impl && m_Impl->IsInitialized();
}

bool Window::IsOpen() const
{
    return m_Impl && m_Impl->IsOpen();
}

bool Window::ShouldClose() const
{
    return m_Impl && m_Impl->ShouldClose();
}

void Window::PollEvents()
{
    if (m_Impl)
    {
        m_Impl->PollEvents();
    }
}