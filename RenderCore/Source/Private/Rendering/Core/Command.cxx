// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <algorithm>
#include <thread>
#include <unordered_map>
#include <vector>
#include <Volk/volk.h>

module RenderCore.Runtime.Command;

import RenderCore.Runtime.Synchronization;
import RenderCore.Runtime.Scene;
import RenderCore.Runtime.SwapChain;
import RenderCore.Runtime.Memory;
import RenderCore.Runtime.Device;
import RenderCore.Runtime.Pipeline;
import RenderCore.Integrations.Viewport;
import RenderCore.Integrations.ImGuiOverlay;
import RenderCore.Types.Allocation;
import RenderCore.Utils.Helpers;
import RenderCore.Utils.Constants;
import ThreadPool;

using namespace RenderCore;

constexpr VkImageAspectFlags g_ImageAspect = VK_IMAGE_ASPECT_COLOR_BIT;
constexpr VkImageAspectFlags g_DepthAspect = VK_IMAGE_ASPECT_DEPTH_BIT;

constexpr VkImageLayout g_UndefinedLayout      = VK_IMAGE_LAYOUT_UNDEFINED;
constexpr VkImageLayout g_SwapChainMidLayout   = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
constexpr VkImageLayout g_SwapChainFinalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
constexpr VkImageLayout g_DepthLayout          = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;

#ifdef VULKAN_RENDERER_ENABLE_IMGUI
constexpr VkImageLayout g_ViewportMidLayout   = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
constexpr VkImageLayout g_ViewportFinalLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR;
#endif

constexpr VkCommandBufferBeginInfo g_CommandBufferBeginInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
};

struct ThreadResources
{
    VkCommandPool                CommandPool { VK_NULL_HANDLE };
    std::vector<VkCommandBuffer> CommandBuffers {};

    void Allocate(std::uint8_t const QueueFamilyIndex, std::uint8_t const NumberOfBuffers)
    {
        if (NumberOfBuffers == 0U)
        {
            return;
        }

        CommandPool = CreateCommandPool(QueueFamilyIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
        CommandBuffers.resize(NumberOfBuffers);

        VkCommandBufferAllocateInfo const CommandBufferAllocateInfo {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool = CommandPool,
                .level = VK_COMMAND_BUFFER_LEVEL_SECONDARY,
                .commandBufferCount = static_cast<std::uint32_t>(std::size(CommandBuffers))
        };

        CheckVulkanResult(vkAllocateCommandBuffers(volkGetLoadedDevice(), &CommandBufferAllocateInfo, std::data(CommandBuffers)));
    }

    void Free()
    {
        if (std::empty(CommandBuffers))
        {
            return;
        }

        vkFreeCommandBuffers(volkGetLoadedDevice(), CommandPool, static_cast<std::uint32_t>(std::size(CommandBuffers)), std::data(CommandBuffers));
    }

    void Destroy()
    {
        if (CommandPool == VK_NULL_HANDLE)
        {
            return;
        }

        Free();
        vkDestroyCommandPool(volkGetLoadedDevice(), CommandPool, nullptr);
    }
};

std::uint32_t                                     g_ObjectsPerThread { 0U };
std::uint32_t                                     g_NumThreads { 0U };
ThreadPool::Pool                                  g_ThreadPool {};
std::unordered_map<std::uint8_t, ThreadResources> g_CommandResources {};
std::pair<VkCommandPool, VkCommandBuffer>         g_PrimaryCommandBuffer {};

void RenderCore::AllocateCommandBuffers(std::uint32_t const QueueFamily, std::uint32_t const NumberOfBuffers)
{
    g_ObjectsPerThread = NumberOfBuffers == 0U ? 0U : std::clamp(NumberOfBuffers / g_NumThreads, 1U, NumberOfBuffers);

    for (std::uint8_t ThreadIndex = 0U; ThreadIndex < g_NumThreads; ++ThreadIndex)
    {
        if (NumberOfBuffers > 0U && NumberOfBuffers <= ThreadIndex)
        {
            break;
        }

        g_CommandResources.at(ThreadIndex).Allocate(QueueFamily, g_ObjectsPerThread);
    }

    g_PrimaryCommandBuffer = std::make_pair(CreateCommandPool(QueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT), VK_NULL_HANDLE);

    VkCommandBufferAllocateInfo const CommandBufferAllocateInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = g_PrimaryCommandBuffer.first,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1U
    };

    CheckVulkanResult(vkAllocateCommandBuffers(volkGetLoadedDevice(), &CommandBufferAllocateInfo, &g_PrimaryCommandBuffer.second));
}

void RenderCore::FreeCommandBuffers()
{
    for (auto &[_, CommandResources] : g_CommandResources)
    {
        CommandResources.Free();
    }

    vkFreeCommandBuffers(volkGetLoadedDevice(), g_PrimaryCommandBuffer.first, 1U, &g_PrimaryCommandBuffer.second);
}

void RenderCore::InitializeCommandsResources()
{
    g_NumThreads = static_cast<std::uint8_t>(std::thread::hardware_concurrency());
    g_ThreadPool.SetThreadCount(g_NumThreads);

    for (std::uint8_t ThreadIndex = 0U; ThreadIndex < g_NumThreads; ++ThreadIndex)
    {
        g_CommandResources.emplace(ThreadIndex, ThreadResources {});
    }
}

void RenderCore::ReleaseCommandsResources()
{
    for (auto &[_, CommandResources] : g_CommandResources)
    {
        CommandResources.Destroy();
    }
    g_CommandResources.clear();

    vkDestroyCommandPool(volkGetLoadedDevice(), g_PrimaryCommandBuffer.first, nullptr);
}

VkCommandPool RenderCore::CreateCommandPool(std::uint8_t const FamilyQueueIndex, VkCommandPoolCreateFlags const Flags)
{
    VkCommandPoolCreateInfo const CommandPoolCreateInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = Flags,
            .queueFamilyIndex = static_cast<std::uint32_t>(FamilyQueueIndex)
    };

    VkCommandPool Output = VK_NULL_HANDLE;
    CheckVulkanResult(vkCreateCommandPool(volkGetLoadedDevice(), &CommandPoolCreateInfo, nullptr, &Output));

    return Output;
}

void SetViewport(VkCommandBuffer const &CommandBuffer, VkExtent2D const &SwapChainExtent)
{
    VkViewport const Viewport {
            .x = 0.F,
            .y = 0.F,
            .width = static_cast<float>(SwapChainExtent.width),
            .height = static_cast<float>(SwapChainExtent.height),
            .minDepth = 0.F,
            .maxDepth = 1.F
    };

    VkRect2D const Scissor { .offset = { 0, 0 }, .extent = SwapChainExtent };

    vkCmdSetViewport(CommandBuffer, 0U, 1U, &Viewport);
    vkCmdSetScissor(CommandBuffer, 0U, 1U, &Scissor);
}

void BeginRendering(std::uint32_t const ImageIndex, VkExtent2D const &SwapChainExtent)
{
    ImageAllocation const &SwapchainAllocation = GetSwapChainImages().at(ImageIndex);
    RenderCore::MoveImageLayout<g_UndefinedLayout, g_SwapChainMidLayout, g_ImageAspect>(g_PrimaryCommandBuffer.second,
                                                                                        SwapchainAllocation.Image,
                                                                                        SwapchainAllocation.Format);

    #ifdef VULKAN_RENDERER_ENABLE_IMGUI
    ImageAllocation const &ViewportAllocation = GetViewportImages().at(ImageIndex);
    RenderCore::MoveImageLayout<g_UndefinedLayout, g_ViewportMidLayout, g_ImageAspect>(g_PrimaryCommandBuffer.second,
                                                                                       ViewportAllocation.Image,
                                                                                       SwapchainAllocation.Format);

    VkRenderingAttachmentInfoKHR ColorAttachment {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
            .imageView = ViewportAllocation.View,
            .imageLayout = g_ViewportMidLayout,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = g_ClearValues.at(0U)
    };
    #else
    VkRenderingAttachmentInfoKHR ColorAttachment {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
        .imageView = SwapchainAllocation.View,
        .imageLayout = g_SwapChainMidLayout,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = g_ClearValues.at(0U)
    };
    #endif

    ImageAllocation const &DepthAllocation = GetDepthImage();
    RenderCore::MoveImageLayout<g_UndefinedLayout, g_DepthLayout, g_DepthAspect>(g_PrimaryCommandBuffer.second,
                                                                                 DepthAllocation.Image,
                                                                                 DepthAllocation.Format);

    VkRenderingAttachmentInfoKHR const DepthAttachmentInfo {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
            .imageView = DepthAllocation.View,
            .imageLayout = g_DepthLayout,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = g_ClearValues.at(2U)
    };

    VkRenderingInfo const RenderingInfo {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
            .pNext = nullptr,
            .flags = VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT_KHR,
            .renderArea = { .offset = { 0, 0 }, .extent = SwapChainExtent },
            .layerCount = 1U,
            .colorAttachmentCount = 1U,
            .pColorAttachments = &ColorAttachment,
            .pDepthAttachment = &DepthAttachmentInfo,
            .pStencilAttachment = &DepthAttachmentInfo
    };

    vkCmdBeginRendering(g_PrimaryCommandBuffer.second, &RenderingInfo);
}

void EndRendering(std::uint32_t const ImageIndex)
{
    vkCmdEndRendering(g_PrimaryCommandBuffer.second);

    ImageAllocation const &SwapchainAllocation = GetSwapChainImages().at(ImageIndex);

    #ifdef VULKAN_RENDERER_ENABLE_IMGUI
    ImageAllocation const &ViewportAllocation = GetViewportImages().at(ImageIndex);
    RenderCore::MoveImageLayout<g_ViewportMidLayout, g_ViewportFinalLayout, g_ImageAspect>(g_PrimaryCommandBuffer.second,
                                                                                           ViewportAllocation.Image,
                                                                                           SwapchainAllocation.Format);
    #endif
}

std::vector<VkCommandBuffer> RecordSceneCommands()
{
    std::vector<VkCommandBuffer> Output {};
    Output.reserve(g_NumThreads);

    std::vector const ColorAttachmentFormat { GetSwapChainImageFormat() };
    VkFormat const    DepthFormat = GetDepthImage().Format;

    VkCommandBufferInheritanceRenderingInfo const InheritanceRenderingInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDERING_INFO,
            .flags = VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT,
            .viewMask = 0,
            .colorAttachmentCount = static_cast<std::uint32_t>(std::size(ColorAttachmentFormat)),
            .pColorAttachmentFormats = std::data(ColorAttachmentFormat),
            .depthAttachmentFormat = DepthFormat,
            .stencilAttachmentFormat = DepthFormat,
            .rasterizationSamples = g_MSAASamples,
    };

    VkCommandBufferInheritanceInfo const InheritanceInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
            .pNext = &InheritanceRenderingInfo,
            .renderPass = VK_NULL_HANDLE
    };

    VkCommandBufferBeginInfo SecondaryBeginInfo = g_CommandBufferBeginInfo;
    SecondaryBeginInfo.flags                    = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    SecondaryBeginInfo.pInheritanceInfo         = &InheritanceInfo;

    VkPipeline const &Pipeline = GetMainPipeline();
    auto &            Objects  = GetObjects();

    for (std::uint32_t ThreadIndex = 0U; ThreadIndex < g_NumThreads; ++ThreadIndex)
    {
        auto &CommandResources = g_CommandResources.at(ThreadIndex);

        if (std::empty(CommandResources.CommandBuffers))
        {
            continue;
        }

        for (std::uint32_t ObjectIndex = 0U; ObjectIndex < g_ObjectsPerThread; ++ObjectIndex)
        {
            VkCommandBuffer const &CommandBuffer = CommandResources.CommandBuffers.at(ObjectIndex);

            CheckVulkanResult(vkBeginCommandBuffer(CommandBuffer, &SecondaryBeginInfo));
            {
                vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);
                SetViewport(CommandBuffer, GetSwapChainExtent());

                Object const &Object = Objects.at(ThreadIndex * g_ObjectsPerThread + ObjectIndex);
                Object.UpdateUniformBuffers();
                Object.DrawObject(CommandBuffer);
            }
            CheckVulkanResult(vkEndCommandBuffer(CommandBuffer));

            Output.push_back(CommandBuffer);
        }
    }

    g_ThreadPool.Wait();

    return Output;
}

void RenderCore::RecordCommandBuffers(std::uint32_t const ImageIndex)
{
    CheckVulkanResult(vkBeginCommandBuffer(g_PrimaryCommandBuffer.second, &g_CommandBufferBeginInfo));
    {
        std::vector<VkCommandBuffer> CommandBuffers {};
        CommandBuffers.reserve(g_NumThreads);

        BeginRendering(ImageIndex, GetSwapChainExtent());
        CommandBuffers.append_range(RecordSceneCommands());

        if (!std::empty(CommandBuffers))
        {
            vkCmdExecuteCommands(g_PrimaryCommandBuffer.second, static_cast<std::uint32_t>(std::size(CommandBuffers)), std::data(CommandBuffers));
        }

        EndRendering(ImageIndex);

        #ifdef VULKAN_RENDERER_ENABLE_IMGUI
        RecordImGuiCommandBuffer(g_PrimaryCommandBuffer.second, ImageIndex, g_SwapChainMidLayout);
        #endif

        ImageAllocation const &SwapchainAllocation = GetSwapChainImages().at(ImageIndex);
        RenderCore::MoveImageLayout<g_SwapChainMidLayout, g_SwapChainFinalLayout, g_ImageAspect>(g_PrimaryCommandBuffer.second,
                                                                                                 SwapchainAllocation.Image,
                                                                                                 SwapchainAllocation.Format);
    }
    CheckVulkanResult(vkEndCommandBuffer(g_PrimaryCommandBuffer.second));
}

void RenderCore::SubmitCommandBuffers()
{
    VkSemaphoreSubmitInfoKHR WaitSemaphoreInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR,
            .semaphore = GetImageAvailableSemaphore(),
            .value = 1U,
            .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
            .deviceIndex = 0U
    };

    VkSemaphoreSubmitInfoKHR SignalSemaphoreInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR,
            .semaphore = GetRenderFinishedSemaphore(),
            .value = 1U
    };

    VkCommandBufferSubmitInfo PrimarySubmission = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO_KHR,
            .commandBuffer = g_PrimaryCommandBuffer.second,
            .deviceMask = 0U
    };

    VkSubmitInfo2KHR const SubmitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2_KHR,
            .waitSemaphoreInfoCount = 1U,
            .pWaitSemaphoreInfos = &WaitSemaphoreInfo,
            .commandBufferInfoCount = 1U,
            .pCommandBufferInfos = &PrimarySubmission,
            .signalSemaphoreInfoCount = 1U,
            .pSignalSemaphoreInfos = &SignalSemaphoreInfo
    };

    auto const &GraphicsQueue = GetGraphicsQueue().second;

    CheckVulkanResult(vkQueueSubmit2(GraphicsQueue, 1U, &SubmitInfo, GetFence()));
    WaitAndResetFences();
}

void RenderCore::InitializeSingleCommandQueue(VkCommandPool &               CommandPool,
                                              std::vector<VkCommandBuffer> &CommandBuffers,
                                              std::uint8_t const            QueueFamilyIndex)
{
    if (std::empty(CommandBuffers))
    {
        return;
    }

    CommandPool = CreateCommandPool(QueueFamilyIndex, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);

    constexpr VkCommandBufferBeginInfo CommandBufferBeginInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    VkCommandBufferAllocateInfo const CommandBufferAllocateInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = CommandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = static_cast<std::uint32_t>(std::size(CommandBuffers)),
    };

    CheckVulkanResult(vkAllocateCommandBuffers(volkGetLoadedDevice(), &CommandBufferAllocateInfo, std::data(CommandBuffers)));
    for (VkCommandBuffer const &CommandBufferIter : CommandBuffers)
    {
        CheckVulkanResult(vkBeginCommandBuffer(CommandBufferIter, &CommandBufferBeginInfo));
    }
}

void RenderCore::FinishSingleCommandQueue(VkQueue const &Queue, VkCommandPool const &CommandPool, std::vector<VkCommandBuffer> const &CommandBuffers)
{
    if (std::empty(CommandBuffers))
    {
        return;
    }

    std::vector<VkCommandBufferSubmitInfoKHR> CommandBufferInfos;
    CommandBufferInfos.reserve(std::size(CommandBuffers));

    for (VkCommandBuffer const &CommandBufferIter : CommandBuffers)
    {
        CheckVulkanResult(vkEndCommandBuffer(CommandBufferIter));

        CommandBufferInfos.push_back(VkCommandBufferSubmitInfo {
                                             .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO_KHR,
                                             .commandBuffer = CommandBufferIter,
                                             .deviceMask = 0U
                                     });
    }

    VkSubmitInfo2KHR const SubmitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2_KHR,
            .commandBufferInfoCount = static_cast<std::uint32_t>(std::size(CommandBufferInfos)),
            .pCommandBufferInfos = std::data(CommandBufferInfos)
    };

    CheckVulkanResult(vkQueueSubmit2(Queue, 1U, &SubmitInfo, VK_NULL_HANDLE));
    CheckVulkanResult(vkQueueWaitIdle(Queue));

    vkFreeCommandBuffers(volkGetLoadedDevice(), CommandPool, static_cast<std::uint32_t>(std::size(CommandBuffers)), std::data(CommandBuffers));
    vkDestroyCommandPool(volkGetLoadedDevice(), CommandPool, nullptr);
}