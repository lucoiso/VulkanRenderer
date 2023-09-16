// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#pragma once

#include "Types/TextureData.h"
#include <vulkan/vulkan_core.h>

namespace RenderCore
{
    struct VulkanTextureData
    {
        VkImageView ImageView;
        VkSampler Sampler;
    };
}