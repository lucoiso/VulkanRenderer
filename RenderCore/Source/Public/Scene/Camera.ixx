// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <cstdint>
#include <memory>
#include <Volk/volk.h>
#include <glm/ext.hpp>
#include "RenderCoreModule.hpp"

export module RenderCore.Types.Camera;

import RenderCore.Types.Transform;
import RenderCore.Types.Object;

namespace RenderCore
{
    export enum class CameraMovementStateFlags : std::uint8_t {
        NONE     = 0,
        FORWARD  = 1 << 1,
        BACKWARD = 1 << 2,
        LEFT     = 1 << 3,
        RIGHT    = 1 << 4,
        UP       = 1 << 5,
        DOWN     = 1 << 6
    };

    export class RENDERCOREMODULE_API Camera
    {
        Vector m_CameraPosition {0.F, 0.F, 3.F};
        Rotator m_CameraRotation {0.F, -90.F, 0.F};

        float m_CameraSpeed {1.F};
        float m_CameraSensitivity {1.F};

        float m_FieldOfView {45.F};
        float m_NearPlane {0.001F};
        float m_FarPlane {1000.F};
        float m_CurrentAspectRatio {1.F};
        float m_DrawDistance {500.F};

        CameraMovementStateFlags m_CameraMovementStateFlags {CameraMovementStateFlags::NONE};

    public:
        Camera() = default;

        [[nodiscard]] Vector GetPosition() const;
        void SetPosition(Vector const&);

        [[nodiscard]] Rotator GetRotation() const;
        void SetRotation(Rotator const&);

        [[nodiscard]] float GetSpeed() const;
        void SetSpeed(float);

        [[nodiscard]] float GetSensitivity() const;
        void SetSensitivity(float);

        [[nodiscard]] float GetFieldOfView() const;
        void SetFieldOfView(float);

        [[nodiscard]] float GetNearPlane() const;
        void SetNearPlane(float);

        [[nodiscard]] float GetFarPlane() const;
        void SetFarPlane(float);

        [[nodiscard]] float GetDrawDistance() const;
        void SetDrawDistance(float);

        [[nodiscard]] glm::mat4 GetViewMatrix() const;
        [[nodiscard]] glm::mat4 GetProjectionMatrix(VkExtent2D const&) const;

        [[nodiscard]] CameraMovementStateFlags GetCameraMovementStateFlags() const;
        void SetCameraMovementStateFlags(CameraMovementStateFlags);
        void UpdateCameraMovement(float);

        [[nodiscard]] bool IsInsideCameraFrustum(Vector const&, VkExtent2D const&) const;
        [[nodiscard]] bool IsInAllowedDistance(Vector const&) const;
        [[nodiscard]] bool CanDrawObject(std::shared_ptr<Object> const&, VkExtent2D const&) const;
    };
}// namespace RenderCore