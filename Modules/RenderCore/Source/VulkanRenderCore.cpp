// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

#include "VulkanRenderCore.h"
#include "Managers/VulkanDeviceManager.h"
#include "Managers/VulkanPipelineManager.h"
#include "Managers/VulkanBufferManager.h"
#include "Managers/VulkanCommandsManager.h"
#include "Managers/VulkanShaderManager.h"
#include "Utils/RenderCoreHelpers.h"
#include "Utils/VulkanDebugHelpers.h"
#include "Utils/VulkanConstants.h"
#include <filesystem>
#include <boost/log/trivial.hpp>

#ifndef VOLK_IMPLEMENTATION
#define VOLK_IMPLEMENTATION
#endif
// ReSharper disable once CppUnusedIncludeDirective
#include <volk.h>

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

#ifdef VK_USE_PLATFORM_WIN32_KHR
#include <windows.h>
#endif

using namespace RenderCore;

VulkanRenderCore VulkanRenderCore::g_Instance;

VulkanRenderCore::VulkanRenderCore()
    : m_Instance(VK_NULL_HANDLE)
  , m_Surface(VK_NULL_HANDLE)
  , StateFlags(VulkanRenderCoreStateFlags::NONE)
    #ifdef _DEBUG
  , m_DebugMessenger(VK_NULL_HANDLE)
    #endif
  , m_ObjectID(0u)
{
}

VulkanRenderCore::~VulkanRenderCore()
{
    Shutdown();
}

VulkanRenderCore& VulkanRenderCore::Get()
{
    return g_Instance;
}

void VulkanRenderCore::Initialize(GLFWwindow* const Window)
{
    if (IsInitialized())
    {
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Initializing vulkan render core";

    RENDERCORE_CHECK_VULKAN_RESULT(volkInitialize());

    #ifdef _DEBUG
    RenderCoreHelpers::ListAvailableInstanceLayers();

    for (const char* const & RequiredLayerIter : g_RequiredInstanceLayers)
    {
        RenderCoreHelpers::ListAvailableInstanceLayerExtensions(RequiredLayerIter);
    }

    for (const char* const & DebugLayerIter : g_DebugInstanceLayers)
    {
        RenderCoreHelpers::ListAvailableInstanceLayerExtensions(DebugLayerIter);
    }
    #endif

    CreateVulkanInstance();
    CreateVulkanSurface(Window);
    InitializeRenderCore();
}

void VulkanRenderCore::Shutdown()
{
    if (!IsInitialized())
    {
        return;
    }

    RenderCoreHelpers::RemoveFlags(StateFlags, VulkanRenderCoreStateFlags::INITIALIZED);

    VulkanShaderManager::Get().Shutdown();
    VulkanCommandsManager::Get().Shutdown();
    VulkanBufferManager::Get().Shutdown();
    VulkanPipelineManager::Get().Shutdown();
    VulkanDeviceManager::Get().Shutdown();

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down vulkan render core";

    #ifdef _DEBUG
    if (m_DebugMessenger != VK_NULL_HANDLE)
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down vulkan debug messenger";

        DebugHelpers::DestroyDebugUtilsMessenger(m_Instance, m_DebugMessenger, nullptr);
        m_DebugMessenger = VK_NULL_HANDLE;
    }
    #endif

    vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
    m_Surface = VK_NULL_HANDLE;

    vkDestroyInstance(m_Instance, nullptr);
    m_Instance = VK_NULL_HANDLE;
}

void VulkanRenderCore::DrawFrame(GLFWwindow* const Window) const
{
    if (!IsInitialized())
    {
        return;
    }

    if (Window == nullptr)
    {
        throw std::runtime_error("Window is invalid");
    }

    constexpr VulkanRenderCoreStateFlags InvalidStatesToRender = VulkanRenderCoreStateFlags::PENDING_DEVICE_PROPERTIES_UPDATE | VulkanRenderCoreStateFlags::PENDING_RESOURCES_DESTRUCTION |
        VulkanRenderCoreStateFlags::PENDING_RESOURCES_CREATION | VulkanRenderCoreStateFlags::PENDING_PIPELINE_REFRESH;

    if (RenderCoreHelpers::HasAnyFlag(StateFlags, InvalidStatesToRender))
    {
        if (RenderCoreHelpers::HasFlag(StateFlags, VulkanRenderCoreStateFlags::PENDING_RESOURCES_DESTRUCTION))
        {
            VulkanCommandsManager::Get().DestroySynchronizationObjects();
            VulkanBufferManager::Get().DestroyResources(false);
            VulkanPipelineManager::Get().DestroyResources();

            RenderCoreHelpers::RemoveFlags(StateFlags, VulkanRenderCoreStateFlags::PENDING_RESOURCES_DESTRUCTION);
            RenderCoreHelpers::AddFlags(StateFlags, VulkanRenderCoreStateFlags::PENDING_RESOURCES_CREATION);
        }

        if (RenderCoreHelpers::HasFlag(StateFlags, VulkanRenderCoreStateFlags::PENDING_DEVICE_PROPERTIES_UPDATE))
        {
            if (VulkanDeviceManager::Get().UpdateDeviceProperties(Window))
            {
                BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Device properties updated, starting to draw frames with new properties";
                RenderCoreHelpers::RemoveFlags(StateFlags, VulkanRenderCoreStateFlags::PENDING_DEVICE_PROPERTIES_UPDATE);
            }
        }

        if (RenderCoreHelpers::HasFlag(StateFlags, VulkanRenderCoreStateFlags::PENDING_RESOURCES_CREATION))
        {
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Refreshing resources...";
            VulkanBufferManager::Get().CreateSwapChain();
            VulkanBufferManager::Get().CreateDepthResources();
            VulkanCommandsManager::Get().CreateSynchronizationObjects();

            RenderCoreHelpers::RemoveFlags(StateFlags, VulkanRenderCoreStateFlags::PENDING_RESOURCES_CREATION);
            RenderCoreHelpers::AddFlags(StateFlags, VulkanRenderCoreStateFlags::PENDING_PIPELINE_REFRESH);
        }

        if (RenderCoreHelpers::HasFlag(StateFlags, VulkanRenderCoreStateFlags::PENDING_PIPELINE_REFRESH))
        {
            VulkanPipelineManager::Get().CreateRenderPass();
            VulkanPipelineManager::Get().CreateDescriptorSetLayout();
            VulkanPipelineManager::Get().CreateGraphicsPipeline();

            VulkanBufferManager::Get().CreateFrameBuffers();

            VulkanPipelineManager::Get().CreateDescriptorPool();
            VulkanPipelineManager::Get().CreateDescriptorSets();

            RenderCoreHelpers::RemoveFlags(StateFlags, VulkanRenderCoreStateFlags::PENDING_PIPELINE_REFRESH);
        }
    }

    if (!RenderCoreHelpers::HasAnyFlag(StateFlags, InvalidStatesToRender))
    {
        if (const std::optional<std::int32_t> ImageIndice = TryRequestDrawImage();
            ImageIndice.has_value())
        {
            VulkanCommandsManager::Get().RecordCommandBuffers(ImageIndice.value());
            VulkanCommandsManager::Get().SubmitCommandBuffers();
            VulkanCommandsManager::Get().PresentFrame(ImageIndice.value());
        }
    }
}

bool VulkanRenderCore::IsInitialized() const
{
    return RenderCoreHelpers::HasFlag(StateFlags, VulkanRenderCoreStateFlags::INITIALIZED);
}

void VulkanRenderCore::LoadScene(const std::string_view ModelPath, const std::string_view TexturePath)
{
    if (!IsInitialized())
    {
        return;
    }

    if (!std::filesystem::exists(ModelPath))
    {
        throw std::runtime_error("Model path is invalid");
    }

    if (!std::filesystem::exists(TexturePath))
    {
        throw std::runtime_error("Texture path is invalid");
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Loading scene...";

    m_ObjectID = VulkanBufferManager::Get().LoadObject(ModelPath, TexturePath);

    RenderCoreHelpers::AddFlags(StateFlags, VulkanRenderCoreStateFlags::PENDING_RESOURCES_DESTRUCTION);
    RenderCoreHelpers::AddFlags(StateFlags, VulkanRenderCoreStateFlags::PENDING_RESOURCES_CREATION);
}

void VulkanRenderCore::UnloadScene() const
{
    if (!IsInitialized())
    {
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Unloading scene...";

    VulkanBufferManager::Get().UnLoadObject(m_ObjectID);

    RenderCoreHelpers::AddFlags(StateFlags, VulkanRenderCoreStateFlags::PENDING_RESOURCES_DESTRUCTION);
}

VkInstance& VulkanRenderCore::GetInstance()
{
    return m_Instance;
}

VkSurfaceKHR& VulkanRenderCore::GetSurface()
{
    return m_Surface;
}

VulkanRenderCoreStateFlags VulkanRenderCore::GetStateFlags() const
{
    return StateFlags;
}

std::optional<std::int32_t> VulkanRenderCore::TryRequestDrawImage() const
{
    if (!VulkanDeviceManager::Get().GetDeviceProperties().IsValid())
    {
        RenderCoreHelpers::AddFlags(StateFlags, VulkanRenderCoreStateFlags::PENDING_DEVICE_PROPERTIES_UPDATE);
        RenderCoreHelpers::AddFlags(StateFlags, VulkanRenderCoreStateFlags::PENDING_RESOURCES_DESTRUCTION);

        return std::nullopt;
    }
    RenderCoreHelpers::RemoveFlags(StateFlags, VulkanRenderCoreStateFlags::PENDING_DEVICE_PROPERTIES_UPDATE);

    const std::optional<std::int32_t> Output = VulkanCommandsManager::Get().DrawFrame();

    if (!Output.has_value())
    {
        RenderCoreHelpers::AddFlags(StateFlags, VulkanRenderCoreStateFlags::PENDING_RESOURCES_DESTRUCTION);
    }
    else
    {
        RenderCoreHelpers::RemoveFlags(StateFlags, VulkanRenderCoreStateFlags::PENDING_RESOURCES_DESTRUCTION);
    }

    return Output;
}

void VulkanRenderCore::CreateVulkanInstance()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan instance";

    constexpr VkApplicationInfo AppInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "VulkanApp",
        .applicationVersion = VK_MAKE_VERSION(1u, 0u, 0u),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1u, 0u, 0u),
        .apiVersion = VK_API_VERSION_1_0
    };

    VkInstanceCreateInfo CreateInfo{.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, .pNext = nullptr, .pApplicationInfo = &AppInfo, .enabledLayerCount = 0u};

    std::vector              Layers(g_RequiredInstanceLayers.begin(), g_RequiredInstanceLayers.end());
    std::vector<const char*> Extensions = RenderCoreHelpers::GetGLFWExtensions();
    Extensions.insert(Extensions.end(), g_RequiredInstanceExtensions.begin(), g_RequiredInstanceExtensions.end());

    #ifdef _DEBUG
    const VkValidationFeaturesEXT ValidationFeatures = DebugHelpers::GetInstanceValidationFeatures();
    CreateInfo.pNext                                 = &ValidationFeatures;

    Layers.insert(Layers.end(), g_DebugInstanceLayers.begin(), g_DebugInstanceLayers.end());
    Extensions.insert(Extensions.end(), g_DebugInstanceExtensions.begin(), g_DebugInstanceExtensions.end());

    VkDebugUtilsMessengerCreateInfoEXT CreateDebugInfo{};
    DebugHelpers::PopulateDebugInfo(CreateDebugInfo);
    #endif

    CreateInfo.enabledLayerCount   = static_cast<std::uint32_t>(Layers.size());
    CreateInfo.ppEnabledLayerNames = Layers.data();

    CreateInfo.enabledExtensionCount   = static_cast<std::uint32_t>(Extensions.size());
    CreateInfo.ppEnabledExtensionNames = Extensions.data();

    RENDERCORE_CHECK_VULKAN_RESULT(vkCreateInstance(&CreateInfo, nullptr, &m_Instance));
    volkLoadInstance(m_Instance);

    #ifdef _DEBUG
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Setting up debug messages";
    RENDERCORE_CHECK_VULKAN_RESULT(DebugHelpers::CreateDebugUtilsMessenger(m_Instance, &CreateDebugInfo, nullptr, &m_DebugMessenger));
    #endif
}

void VulkanRenderCore::CreateVulkanSurface(GLFWwindow* const Window)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan surface";

    if (m_Instance == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan instance is invalid.");
    }

    RENDERCORE_CHECK_VULKAN_RESULT(glfwCreateWindowSurface(m_Instance, Window, nullptr, &m_Surface));
}

void VulkanRenderCore::InitializeRenderCore() const
{
    VulkanDeviceManager::Get().PickPhysicalDevice();
    VulkanDeviceManager::Get().CreateLogicalDevice();
    volkLoadDevice(VulkanDeviceManager::Get().GetLogicalDevice());

    VulkanBufferManager::Get().CreateMemoryAllocator();
    CompileDefaultShaders();

    RenderCoreHelpers::AddFlags(StateFlags, VulkanRenderCoreStateFlags::INITIALIZED);
    RenderCoreHelpers::AddFlags(StateFlags, VulkanRenderCoreStateFlags::PENDING_RESOURCES_CREATION);
}

std::vector<VkPipelineShaderStageCreateInfo> VulkanRenderCore::CompileDefaultShaders()
{
    constexpr std::array FragmentShaders{DEBUG_SHADER_FRAG};
    constexpr std::array VertexShaders{DEBUG_SHADER_VERT};

    std::vector<VkPipelineShaderStageCreateInfo> ShaderStages;

    const VkDevice& VulkanLogicalDevice = VulkanDeviceManager::Get().GetLogicalDevice();

    for (const char* const & FragmentShaderIter : FragmentShaders)
    {
        if (std::vector<std::uint32_t> FragmentShaderCode;
            VulkanShaderManager::Get().CompileOrLoadIfExists(FragmentShaderIter, FragmentShaderCode))
        {
            const VkShaderModule FragmentModule = VulkanShaderManager::Get().CreateModule(VulkanLogicalDevice, FragmentShaderCode, EShLangFragment);
            ShaderStages.push_back(VulkanShaderManager::Get().GetStageInfo(FragmentModule));
        }
    }

    for (const char* const & VertexShaderIter : VertexShaders)
    {
        if (std::vector<std::uint32_t> VertexShaderCode;
            VulkanShaderManager::Get().CompileOrLoadIfExists(VertexShaderIter, VertexShaderCode))
        {
            const VkShaderModule VertexModule = VulkanShaderManager::Get().CreateModule(VulkanLogicalDevice, VertexShaderCode, EShLangVertex);
            ShaderStages.push_back(VulkanShaderManager::Get().GetStageInfo(VertexModule));
        }
    }

    return ShaderStages;
}