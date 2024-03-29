// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

#pragma once

#include <Utils.hpp>
#include <catch2/catch_test_macros.hpp>

import RenderCore.Window;
import RenderCore.Utils.Helpers;

TEST_CASE("Helper Functions", "[RenderCore]")
{
    ScopedWindow Window;

    SECTION("Get Available GLFW Extensions")
    {
        auto const Extensions = RenderCore::GetGLFWExtensions();
        REQUIRE_FALSE(std::empty(Extensions));
    }

    SECTION("Get Available Instance Layers")
    {
        auto const Layers = RenderCore::GetAvailableInstanceLayers();
        REQUIRE_FALSE(std::empty(Layers));
    }

    SECTION("Get Available Instance Layers Names")
    {
        auto const Layers = RenderCore::GetAvailableInstanceLayersNames();
        REQUIRE_FALSE(std::empty(Layers));
    }

    SECTION("Get Available Instance Extensions")
    {
        auto const Extensions = RenderCore::GetAvailableInstanceExtensions();
        REQUIRE_FALSE(std::empty(Extensions));
    }

    SECTION("Get Available Instance Extensions Names")
    {
        auto const Extensions = RenderCore::GetAvailableInstanceExtensionsNames();
        REQUIRE_FALSE(std::empty(Extensions));
    }
}