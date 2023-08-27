// Copyright Notice: [...]

#include "VulkanRender.h"
#include "VulkanConfigurator.h"
#include "VulkanPipelineManager.h"
#include <boost/log/trivial.hpp>

using namespace RenderCore;

class VulkanRender::Impl
{
public:
    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;

    Impl()
        : m_Configurator(nullptr)
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan render implementation";
    }

    ~Impl()
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destructing vulkan render implementation";
        Shutdown();
    }

    bool Initialize(GLFWwindow* const Window)
    {
        if (IsInitialized())
        {
            return false;
        }

        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Initializing Vulkan render";

        const bool Result = InitializeConfigurator(Window)
                         && InitializePipelineManager(Window);

        if (Result)
        {
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Vulkan render initialized";
        }

        return Result;
    }

    void Shutdown()
    {
        if (IsInitialized())
        {
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down vulkan render";

            try
            {
                m_Configurator->Shutdown();
            }
            catch (const std::exception& Ex)
            {
                BOOST_LOG_TRIVIAL(error) << "[Exception]: " << Ex.what();
            }
        }
    }

    bool IsInitialized() const
    {
        return m_Configurator && m_Configurator->IsInitialized();
    }

private:

    bool InitializeConfigurator(GLFWwindow* const Window)
    {
        if (!Window)
        {
            throw std::runtime_error("GLFW Window is invalid.");
        }

        try
        {
            if (m_Configurator = std::make_unique<VulkanConfigurator>(); m_Configurator)
            {
                m_Configurator->CreateInstance();
                m_Configurator->CreateSurface(Window);
                m_Configurator->PickPhysicalDevice();
                m_Configurator->CreateLogicalDevice();
                m_Configurator->InitializeSwapChain(Window);
                return true;
            }
        }
        catch (const std::exception& Ex)
        {
            BOOST_LOG_TRIVIAL(error) << "[Exception]: " << Ex.what();
        }

        return false;
    }

    bool InitializePipelineManager(GLFWwindow* const Window)
    {
        try
        {
            if (m_PipelineManager = std::make_unique<VulkanPipelineManager>(); m_PipelineManager)
            {
                BOOST_LOG_TRIVIAL(warning) << "[" << __func__ << "]: Passing null Render Pass for testing. Location " << __FILE__ << ":" << __LINE__;
                m_PipelineManager->Initialize(m_Configurator->GetLogicalDevice(), VK_NULL_HANDLE, m_Configurator->GetExtent(Window));
                return true;
            }
        }
        catch (const std::exception& Ex)
        {
            BOOST_LOG_TRIVIAL(error) << "[Exception]: " << Ex.what();
        }

        return false;
    }

    std::unique_ptr<VulkanConfigurator> m_Configurator;
    std::unique_ptr<VulkanPipelineManager> m_PipelineManager;
};

VulkanRender::VulkanRender()
    : m_Impl(std::make_unique<VulkanRender::Impl>())
{
}

VulkanRender::~VulkanRender()
{
    Shutdown();
}

bool VulkanRender::Initialize(GLFWwindow* const Window)
{
    return m_Impl && m_Impl->Initialize(Window);
}

void VulkanRender::Shutdown()
{
    if (m_Impl)
    {
        m_Impl->Shutdown();
    }
}

bool VulkanRender::IsInitialized() const
{
    return m_Impl && m_Impl->IsInitialized();
}