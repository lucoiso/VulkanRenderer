// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <GLFW/glfw3.h>
#include <Volk/volk.h>
#include <atomic>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <vma/vk_mem_alloc.h>

export module RenderCore.Management.BufferManagement;

import RenderCore.Types.Object;
import RenderCore.Types.Camera;
import RenderCore.Types.Transform;
import RenderCore.Types.AllocationTypes;
import RenderCore.Types.SurfaceProperties;
import RenderCore.Utils.Helpers;
import RenderCore.Utils.EnumHelpers;

namespace RenderCore
{
    export class BufferManager
    {
        VkSurfaceKHR   m_Surface {VK_NULL_HANDLE};
        VmaAllocator   m_Allocator {VK_NULL_HANDLE};
        VkSwapchainKHR m_SwapChain {VK_NULL_HANDLE};
        VkSwapchainKHR m_OldSwapChain {VK_NULL_HANDLE};
        VkFormat       m_SwapChainImageFormat {VK_FORMAT_UNDEFINED};
        VkExtent2D     m_SwapChainExtent {0U, 0U};

        std::vector<ImageAllocation> m_SwapChainImages {};

#ifdef VULKAN_RENDERER_ENABLE_IMGUI
        std::vector<ImageAllocation> m_ViewportImages {};
#endif

        VkSampler                                           m_Sampler {VK_NULL_HANDLE};
        ImageAllocation                                     m_DepthImage {};
        VkFormat                                            m_DepthFormat {VK_FORMAT_UNDEFINED};
        std::unordered_map<std::uint32_t, ObjectAllocation> m_Objects {};
        std::atomic<std::uint32_t>                          m_ObjectIDCounter {0U};

    public:
        void CreateVulkanSurface(GLFWwindow *);

        void CreateMemoryAllocator(VkPhysicalDevice const &);

        void CreateImageSampler();

        void CreateSwapChain(SurfaceProperties const &, VkSurfaceCapabilitiesKHR const &);

#ifdef VULKAN_RENDERER_ENABLE_IMGUI
        void CreateViewportResources(SurfaceProperties const &);
#endif

        void CreateDepthResources(SurfaceProperties const &);

        std::vector<Object> AllocateScene(std::string_view);

        void ReleaseScene(std::vector<std::uint32_t> const &);

        void DestroyBufferResources(bool);

        void ReleaseBufferResources();

        [[nodiscard]] VkSurfaceKHR const &GetSurface() const;

        [[nodiscard]] VkSwapchainKHR const &GetSwapChain() const;

        [[nodiscard]] VkExtent2D const &GetSwapChainExtent() const;

        [[nodiscard]] VkFormat const &GetSwapChainImageFormat() const;

        [[nodiscard]] std::vector<ImageAllocation> const &GetSwapChainImages() const;

#ifdef VULKAN_RENDERER_ENABLE_IMGUI
        [[nodiscard]] std::vector<ImageAllocation> const &GetViewportImages() const;
#endif

        [[nodiscard]] ImageAllocation const &GetDepthImage() const;

        [[nodiscard]] VkFormat const &GetDepthFormat() const;

        [[nodiscard]] VkSampler const &GetSampler() const;

        [[nodiscard]] VkBuffer GetVertexBuffer(std::uint32_t) const;

        [[nodiscard]] VkBuffer GetIndexBuffer(std::uint32_t) const;

        [[nodiscard]] std::uint32_t GetIndicesCount(std::uint32_t) const;

        [[nodiscard]] void *GetUniformData(std::uint32_t) const;

        [[nodiscard]] bool ContainsObject(std::uint32_t) const;

        [[nodiscard]] std::unordered_map<std::uint32_t, ObjectAllocation> &GetMutableAllocatedObjects();

        [[nodiscard]] const std::unordered_map<std::uint32_t, ObjectAllocation> &GetAllocatedObjects() const;

        [[nodiscard]] std::uint32_t GetNumAllocations() const;

        [[nodiscard]] std::uint32_t GetClampedNumAllocations() const;

        [[nodiscard]] VmaAllocator const &GetAllocator() const;

        void UpdateUniformBuffers(std::shared_ptr<Object> const &, Camera const &, VkExtent2D const &) const;

        void SaveImageToFile(VkImage const &, std::string_view) const;

        template <VkImageLayout OldLayout, VkImageLayout NewLayout, VkImageAspectFlags Aspect>
        static constexpr void MoveImageLayout(VkCommandBuffer &CommandBuffer, VkImage const &Image, VkFormat const &Format)
        {
            VkImageMemoryBarrier2KHR ImageBarrier {.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                                                   .srcAccessMask       = 0U,
                                                   .dstAccessMask       = 0U,
                                                   .oldLayout           = OldLayout,
                                                   .newLayout           = NewLayout,
                                                   .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                                   .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                                   .image               = Image,
                                                   .subresourceRange
                                                   = {.aspectMask = Aspect, .baseMipLevel = 0U, .levelCount = 1U, .baseArrayLayer = 0U, .layerCount = 1U}};

            if constexpr (HasFlag<VkImageAspectFlags>(Aspect, VK_IMAGE_ASPECT_DEPTH_BIT))
            {
                if (DepthHasStencil(Format))
                {
                    ImageBarrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
                }
            }

            if constexpr (OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && NewLayout == VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL)
            {
                if constexpr (HasFlag<VkImageAspectFlags>(Aspect, VK_IMAGE_ASPECT_DEPTH_BIT))
                {
                    ImageBarrier.srcAccessMask = VK_ACCESS_2_NONE;
                    ImageBarrier.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                    ImageBarrier.srcStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
                    ImageBarrier.dstStageMask  = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
                }
                else
                {
                    ImageBarrier.srcAccessMask = VK_ACCESS_2_NONE;
                    ImageBarrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
                    ImageBarrier.srcStageMask  = VK_PIPELINE_STAGE_2_NONE;
                    ImageBarrier.dstStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
                }
            }
            else if constexpr (OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && NewLayout == VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL)
            {
                ImageBarrier.srcAccessMask = VK_ACCESS_2_NONE;
                ImageBarrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
                ImageBarrier.srcStageMask  = VK_PIPELINE_STAGE_2_NONE;
                ImageBarrier.dstStageMask  = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
            }
            else if constexpr (OldLayout == VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL)
            {
                ImageBarrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
                ImageBarrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
                ImageBarrier.srcStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
                ImageBarrier.dstStageMask  = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
            }
            else if constexpr (OldLayout == VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
            {
                ImageBarrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
                ImageBarrier.dstAccessMask = VK_ACCESS_2_NONE;
                ImageBarrier.srcStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
                ImageBarrier.dstStageMask  = VK_PIPELINE_STAGE_2_NONE;
            }
            else if constexpr (OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && NewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
            {
                ImageBarrier.srcAccessMask = VK_ACCESS_2_NONE;
                ImageBarrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                ImageBarrier.srcStageMask  = VK_PIPELINE_STAGE_2_NONE;
                ImageBarrier.dstStageMask  = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            }
            else if constexpr (OldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL)
            {
                ImageBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                ImageBarrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
                ImageBarrier.srcStageMask  = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
                ImageBarrier.dstStageMask  = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
            }
            else
            {
                throw std::runtime_error("Unsupported layout transition!");
            }

            VkDependencyInfoKHR const DependencyInfo {.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                                                      .imageMemoryBarrierCount = 1U,
                                                      .pImageMemoryBarriers    = &ImageBarrier};

            vkCmdPipelineBarrier2(CommandBuffer, &DependencyInfo);
        }
    };
} // namespace RenderCore
