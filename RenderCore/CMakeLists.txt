# Author: Lucas Vilas-Boas
# Year: 2023
# Repo: https://github.com/lucoiso/VulkanRenderer

# ----------- Global Definitions -----------
SET(LIBRARY_NAME RenderCore)
SET(RENDERER_RESOURCES_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Resources)

# ------------- Library Setup --------------
SET(PRIVATE_MODULES_BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private)
SET(PUBLIC_MODULES_BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Modules/Public)

SET(PRIVATE_MODULES
        ${PRIVATE_MODULES_BASE_DIRECTORY}/Windowing/Window.cppm
        ${PRIVATE_MODULES_BASE_DIRECTORY}/Windowing/Control.cppm
        ${PRIVATE_MODULES_BASE_DIRECTORY}/Windowing/GLFWHandler.cppm
        ${PRIVATE_MODULES_BASE_DIRECTORY}/Rendering/Renderer.cppm
        ${PRIVATE_MODULES_BASE_DIRECTORY}/Shader/ShaderManagement.cppm
        ${PRIVATE_MODULES_BASE_DIRECTORY}/Command/CommandsManagement.cppm
        ${PRIVATE_MODULES_BASE_DIRECTORY}/Pipeline/PipelineManager.cppm
        ${PRIVATE_MODULES_BASE_DIRECTORY}/Buffer/BufferManager.cppm
        ${PRIVATE_MODULES_BASE_DIRECTORY}/Buffer/AllocationTypes.cppm
        ${PRIVATE_MODULES_BASE_DIRECTORY}/Buffer/AllocationRegister.cppm
        ${PRIVATE_MODULES_BASE_DIRECTORY}/Device/DeviceManagement.cppm
        ${PRIVATE_MODULES_BASE_DIRECTORY}/Overlay/ImGuiManagement.cppm
        ${PRIVATE_MODULES_BASE_DIRECTORY}/Input/GLFWCallbacks.cppm
        ${PRIVATE_MODULES_BASE_DIRECTORY}/Scene/Camera.cppm
        ${PRIVATE_MODULES_BASE_DIRECTORY}/Scene/Object.cppm
        ${PRIVATE_MODULES_BASE_DIRECTORY}/Utils/Debug/DebugHelpers.cppm
        ${PRIVATE_MODULES_BASE_DIRECTORY}/Utils/Library/Helpers.cppm
        ${PRIVATE_MODULES_BASE_DIRECTORY}/Subsystem/RenderingSubsystem.cppm
)

SET(PUBLIC_MODULES
        ${PUBLIC_MODULES_BASE_DIRECTORY}/Windowing/Window.ixx
        ${PUBLIC_MODULES_BASE_DIRECTORY}/Windowing/Control.ixx
        ${PUBLIC_MODULES_BASE_DIRECTORY}/Windowing/WindowFlags.ixx
        ${PUBLIC_MODULES_BASE_DIRECTORY}/Windowing/GLFWHandler.ixx
        ${PUBLIC_MODULES_BASE_DIRECTORY}/Rendering/EngineStateFlags.ixx
        ${PUBLIC_MODULES_BASE_DIRECTORY}/Rendering/Renderer.ixx
        ${PUBLIC_MODULES_BASE_DIRECTORY}/Shader/ShaderManagement.ixx
        ${PUBLIC_MODULES_BASE_DIRECTORY}/Command/CommandsManagement.ixx
        ${PUBLIC_MODULES_BASE_DIRECTORY}/Pipeline/PipelineManager.ixx
        ${PUBLIC_MODULES_BASE_DIRECTORY}/Scene/Camera.ixx
        ${PUBLIC_MODULES_BASE_DIRECTORY}/Scene/Object.ixx
        ${PUBLIC_MODULES_BASE_DIRECTORY}/Scene/Transform.ixx
        ${PUBLIC_MODULES_BASE_DIRECTORY}/Scene/Vertex.ixx
        ${PUBLIC_MODULES_BASE_DIRECTORY}/Subsystem/RenderingSubsystem.ixx
        ${PUBLIC_MODULES_BASE_DIRECTORY}/Utils/Debug/DebugHelpers.ixx
        ${PUBLIC_MODULES_BASE_DIRECTORY}/Utils/Library/Helpers.ixx
        ${PUBLIC_MODULES_BASE_DIRECTORY}/Utils/Library/Constants.ixx
        ${PUBLIC_MODULES_BASE_DIRECTORY}/Utils/Enum/EnumConverter.ixx
        ${PUBLIC_MODULES_BASE_DIRECTORY}/Utils/Enum/EnumHelpers.ixx
        ${PUBLIC_MODULES_BASE_DIRECTORY}/Overlay/ImGuiManagement.ixx
        ${PUBLIC_MODULES_BASE_DIRECTORY}/Input/GLFWCallbacks.ixx
        ${PUBLIC_MODULES_BASE_DIRECTORY}/Device/DeviceManagement.ixx
        ${PUBLIC_MODULES_BASE_DIRECTORY}/Device/SurfaceProperties.ixx
        ${PUBLIC_MODULES_BASE_DIRECTORY}/Buffer/MeshBufferData.ixx
        ${PUBLIC_MODULES_BASE_DIRECTORY}/Buffer/TextureType.ixx
        ${PUBLIC_MODULES_BASE_DIRECTORY}/Buffer/UniformBufferObject.ixx
        ${PUBLIC_MODULES_BASE_DIRECTORY}/Buffer/AllocationTypes.ixx
        ${PUBLIC_MODULES_BASE_DIRECTORY}/Buffer/BufferManager.ixx
        ${PUBLIC_MODULES_BASE_DIRECTORY}/Buffer/AllocationRegister.ixx
)

ADD_LIBRARY(${LIBRARY_NAME} ${PRIVATE_MODULES})

SET_TARGET_PROPERTIES(${LIBRARY_NAME} PROPERTIES LINKER_LANGUAGE CXX)

TARGET_SOURCES(${LIBRARY_NAME}
        PUBLIC
        FILE_SET cxx_public_modules
        TYPE CXX_MODULES
        BASE_DIRS ${PUBLIC_MODULES_BASE_DIRECTORY}
        FILES ${PUBLIC_MODULES}
)

TARGET_INCLUDE_DIRECTORIES(${LIBRARY_NAME} PRIVATE ${PRIVATE_MODULES_BASE_DIRECTORY})
TARGET_COMPILE_DEFINITIONS(${LIBRARY_NAME} PRIVATE BUILD_DLL)

FIND_PACKAGE(Boost REQUIRED COMPONENTS log)
FIND_PACKAGE(glm REQUIRED)
FIND_PATH(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")

FIND_PACKAGE(volk CONFIG REQUIRED)
FIND_PACKAGE(VulkanMemoryAllocator CONFIG REQUIRED)
FIND_PACKAGE(SPIRV-Tools CONFIG REQUIRED)

FIND_PACKAGE(glslang CONFIG REQUIRED)

FIND_PACKAGE(glfw3 CONFIG REQUIRED)
FIND_PACKAGE(imgui CONFIG REQUIRED)

TARGET_LINK_LIBRARIES(${LIBRARY_NAME} PRIVATE
        Boost::log

        volk::volk_headers
        GPUOpen::VulkanMemoryAllocator
        SPIRV-Tools-static

        glslang::glslang
        glslang::glslang-default-resource-limits
        glslang::SPIRV

        glfw
        glm::glm
        imgui::imgui

        Timer
        RuntimeInfo
        Configuration
        Coroutine

        ImGui-Backend
)

TARGET_INCLUDE_DIRECTORIES(${LIBRARY_NAME} PRIVATE
        ${TINYGLTF_INCLUDE_DIRS}
)

TARGET_COMPILE_DEFINITIONS(${LIBRARY_NAME} PRIVATE
        # Assets directory (relative to binaries)
        DEFAULT_SHADER_VERT="Resources/Shaders/TRIANGLE_VERT_SHADER.vert"
        DEFAULT_SHADER_FRAG="Resources/Shaders/TRIANGLE_FRAG_SHADER.frag"
)

TARGET_COMPILE_DEFINITIONS(${LIBRARY_NAME} PUBLIC
        GPU_API_DUMP=0
        VK_NO_PROTOTYPES
)

IF (WIN32)
    TARGET_COMPILE_DEFINITIONS(${LIBRARY_NAME} PUBLIC
            VK_USE_PLATFORM_WIN32_KHR
            WIN32_LEAN_AND_MEAN
            NOMINMAX
            _WIN32_WINNT=0x0601
    )
ELSEIF (MACOS)
    TARGET_COMPILE_DEFINITIONS(${LIBRARY_NAME} PUBLIC
            VK_USE_PLATFORM_MACOS_MVK
    )
ELSEIF (UNIX)
    TARGET_COMPILE_DEFINITIONS(${LIBRARY_NAME} PUBLIC
            VK_USE_PLATFORM_XCB_KHR
    )
ENDIF (WIN32)

ADD_CUSTOM_COMMAND(TARGET ${LIBRARY_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${RENDERER_RESOURCES_DIRECTORY}/Shaders" "$<TARGET_FILE_DIR:${LIBRARY_NAME}>/Resources/Shaders"
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${RENDERER_RESOURCES_DIRECTORY}/Assets/glTF-Sample-Assets/Models"
        "$<TARGET_FILE_DIR:${LIBRARY_NAME}>/Resources/Assets"
)