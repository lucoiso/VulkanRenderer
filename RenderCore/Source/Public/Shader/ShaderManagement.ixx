// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <Volk/volk.h>
#include <glslang/Public/ShaderLang.h>

export module RenderCore.Management.ShaderManagement;

#include <cstdint>
#include <string>
#include <vector>

namespace RenderCore
{
    export [[nodiscard]] bool Compile(std::string_view, std::vector<uint32_t>&);
    export [[nodiscard]] bool Load(std::string_view, std::vector<uint32_t>&);
    export [[nodiscard]] bool CompileOrLoadIfExists(std::string_view, std::vector<uint32_t>&);
    export [[nodiscard]] VkShaderModule CreateModule(std::vector<uint32_t> const&, EShLanguage);

    export [[nodiscard]] VkPipelineShaderStageCreateInfo GetStageInfo(VkShaderModule const& Module);
    export [[nodiscard]] std::vector<VkShaderModule> GetShaderModules();
    export [[nodiscard]] std::vector<VkPipelineShaderStageCreateInfo> GetStageInfos();

    export void ReleaseShaderResources();
    export void FreeStagedModules(std::vector<VkPipelineShaderStageCreateInfo> const&);

    export [[nodiscard]] std::vector<VkPipelineShaderStageCreateInfo> CompileDefaultShaders();
}// namespace RenderCore