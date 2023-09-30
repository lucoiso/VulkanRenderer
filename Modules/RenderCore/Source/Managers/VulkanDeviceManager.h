// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#pragma once

#include "Types/DeviceProperties.h"
#include <vector>
#include <string>
#include <optional>
#include <volk.h>

struct GLFWwindow;

namespace RenderCore
{
    class VulkanDeviceManager
    {
    public:
        VulkanDeviceManager(const VulkanDeviceManager&)            = delete;
        VulkanDeviceManager& operator=(const VulkanDeviceManager&) = delete;

        VulkanDeviceManager();
        ~VulkanDeviceManager();

        static VulkanDeviceManager& Get();

        void PickPhysicalDevice();
        void CreateLogicalDevice();

        void Shutdown();

        bool IsInitialized() const;

        [[nodiscard]] static std::vector<VkPhysicalDevice> GetAvailablePhysicalDevices();
        [[nodiscard]] std::vector<VkExtensionProperties>   GetAvailablePhysicalDeviceExtensions() const;
        [[nodiscard]] std::vector<VkLayerProperties>       GetAvailablePhysicalDeviceLayers() const;
        [[nodiscard]] std::vector<VkExtensionProperties>   GetAvailablePhysicalDeviceLayerExtensions(std::string_view LayerName) const;
        [[nodiscard]] std::vector<std::string>             GetAvailablePhysicalDeviceExtensionsNames() const;
        [[nodiscard]] std::vector<std::string>             GetAvailablePhysicalDeviceLayerExtensionsNames(std::string_view LayerName) const;
        [[nodiscard]] std::vector<std::string>             GetAvailablePhysicalDeviceLayersNames() const;
        [[nodiscard]] VkSurfaceCapabilitiesKHR             GetAvailablePhysicalDeviceSurfaceCapabilities() const;
        [[nodiscard]] std::vector<VkSurfaceFormatKHR>      GetAvailablePhysicalDeviceSurfaceFormats() const;
        [[nodiscard]] std::vector<VkPresentModeKHR>        GetAvailablePhysicalDeviceSurfacePresentationModes() const;
        [[nodiscard]] VkDeviceSize                         GetMinUniformBufferOffsetAlignment() const;

        [[nodiscard]] static bool IsPhysicalDeviceSuitable(const VkPhysicalDevice& Device);

        bool                                  UpdateDeviceProperties(GLFWwindow* Window);
        [[nodiscard]] VulkanDeviceProperties& GetDeviceProperties();

        [[nodiscard]] VkDevice&         GetLogicalDevice();
        [[nodiscard]] VkPhysicalDevice& GetPhysicalDevice();

        [[nodiscard]] std::pair<std::uint8_t, VkQueue>& GetGraphicsQueue();
        [[nodiscard]] std::pair<std::uint8_t, VkQueue>& GetPresentationQueue();
        [[nodiscard]] std::pair<std::uint8_t, VkQueue>& GetTransferQueue();

        [[nodiscard]] std::vector<std::uint8_t>& GetUniqueQueueFamilyIndices();
        [[nodiscard]] std::vector<std::uint32_t> GetUniqueQueueFamilyIndices_u32();

        [[nodiscard]] std::uint32_t GetMinImageCount() const;

    private:
        bool GetQueueFamilyIndices(std::optional<std::uint8_t>& GraphicsQueueFamilyIndex,
                                   std::optional<std::uint8_t>& PresentationQueueFamilyIndex,
                                   std::optional<std::uint8_t>& TransferQueueFamilyIndex) const;

        #ifdef _DEBUG
        static void ListAvailablePhysicalDevices();
        void        ListAvailablePhysicalDeviceExtensions() const;
        void        ListAvailablePhysicalDeviceLayers() const;
        void        ListAvailablePhysicalDeviceLayerExtensions(std::string_view LayerName) const;
        void        ListAvailablePhysicalDeviceSurfaceCapabilities() const;
        void        ListAvailablePhysicalDeviceSurfaceFormats() const;
        void        ListAvailablePhysicalDeviceSurfacePresentationModes() const;
        #endif

        static VulkanDeviceManager g_Instance;

        VkPhysicalDevice m_PhysicalDevice;
        VkDevice         m_Device;

        std::pair<std::uint8_t, VkQueue> m_GraphicsQueue;
        std::pair<std::uint8_t, VkQueue> m_PresentationQueue;
        std::pair<std::uint8_t, VkQueue> m_TransferQueue;
        std::vector<std::uint8_t>        m_UniqueQueueFamilyIndices;
        VulkanDeviceProperties           m_DeviceProperties;
    };
}
