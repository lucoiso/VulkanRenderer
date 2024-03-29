# Author: Lucas Vilas-Boas
# Year: 2023
# Repo: https://github.com/lucoiso/VulkanRenderer

# ----------- Global Definitions -----------
CMAKE_MINIMUM_REQUIRED(VERSION 3.28)
PROJECT(VulkanRenderer VERSION 0.0.1 LANGUAGES CXX)

IF (POLICY CMP0048)
    CMAKE_POLICY(SET CMP0048 NEW)
ENDIF ()

SET(CMAKE_CXX_STANDARD 23)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS OFF)
SET(CMAKE_CXX_SCAN_FOR_MODULES ON)

IF (UNIX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++23 -fmodules-ts")
ENDIF (UNIX)

SET(Boost_NO_WARN_NEW_VERSIONS ON)
SET(BUILD_TESTS $ENV{BUILD_TESTS})

# ---------------- OPTIONS -----------------
OPTION(VULKAN_RENDERER_ENABLE_IMGUI "Enable ImGui linkage, including support for custom viewport" OFF)

# -------------- Directories ---------------
ADD_SUBDIRECTORY(RenderCore)
ADD_SUBDIRECTORY(RadeonManager)
ADD_SUBDIRECTORY(cpp-submodules)
ADD_SUBDIRECTORY(ThirdParty)

IF (BUILD_TESTS)
    INCLUDE(CTest)
    ADD_SUBDIRECTORY(Tests)
ENDIF (BUILD_TESTS)
