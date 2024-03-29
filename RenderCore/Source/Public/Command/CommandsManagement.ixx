// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <Volk/volk.h>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

export module RenderCore.Management.CommandsManagement;

import RenderCore.Management.BufferManagement;
import RenderCore.Management.PipelineManagement;
import RenderCore.Types.Camera;
import RenderCore.Types.Object;
import RenderCore.Types.Transform;

namespace RenderCore
{
    export void ReleaseCommandsResources();

    export [[nodiscard]] VkCommandPool CreateCommandPool(std::uint8_t);

    export void CreateCommandsSynchronizationObjects();

    export void DestroyCommandsSynchronizationObjects(bool);

    export [[nodiscard]] std::optional<std::int32_t> RequestSwapChainImage(VkSwapchainKHR const&);

    export void RecordCommandBuffers(std::uint32_t,
                                     Camera const&,
                                     BufferManager const&,
                                     PipelineManager const&,
                                     std::vector<std::shared_ptr<Object>> const&,
                                     VkExtent2D const&);

    export void SubmitCommandBuffers();

    export void PresentFrame(std::uint32_t, VkSwapchainKHR const&);

    export void InitializeSingleCommandQueue(VkCommandPool&, std::vector<VkCommandBuffer>&, std::uint8_t);

    export void FinishSingleCommandQueue(VkQueue const&, VkCommandPool const&, std::vector<VkCommandBuffer>&);
}// namespace RenderCore
