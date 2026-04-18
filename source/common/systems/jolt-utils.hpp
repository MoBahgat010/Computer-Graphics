#pragma once

// Always include Jolt.h before any other Jolt header.
#include <Jolt/Jolt.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

// ============================================================================
// GLM <-> Jolt Physics type conversion utilities
// ============================================================================
// Include this file wherever Jolt physics data needs to be exchanged with the
// engine's ECS (which uses GLM for all math).
// All functions are inline so there is zero link-time overhead.
// ============================================================================

namespace our {

    // ── glm::vec3  →  JPH::Vec3 ───────────────────────────────────────────
    inline JPH::Vec3 toJPH(const glm::vec3& v) {
        return JPH::Vec3(v.x, v.y, v.z);
    }

    // ── JPH::Vec3  →  glm::vec3 ───────────────────────────────────────────
    inline glm::vec3 toGLM(const JPH::Vec3& v) {
        return glm::vec3(v.GetX(), v.GetY(), v.GetZ());
    }

    // ── JPH::RVec3  →  glm::vec3 ──────────────────────────────────────────
    // RVec3 can alias Vec3 in single-precision builds, so this overload is
    // only emitted when Jolt is configured for double precision.
#if defined(JPH_DOUBLE_PRECISION)
    inline glm::vec3 toGLM(const JPH::RVec3& v) {
        return glm::vec3(static_cast<float>(v.GetX()),
                         static_cast<float>(v.GetY()),
                         static_cast<float>(v.GetZ()));
    }
#endif

    // ── GLM Euler angles (radians, XYZ order)  →  JPH::Quat ──────────────
    // The engine Transform stores rotation as Euler angles in radians (x=pitch, y=yaw, z=roll).
    inline JPH::Quat eulerToJPH(const glm::vec3& eulerRadians) {
        glm::quat q(eulerRadians); // GLM builds quaternion from Euler angles (XYZ order)
        return JPH::Quat(q.x, q.y, q.z, q.w);
    }

    // ── JPH::Quat  →  glm::quat ───────────────────────────────────────────
    // Note: glm::quat constructor is (w, x, y, z)
    inline glm::quat toGLMQuat(const JPH::Quat& q) {
        return glm::quat(q.GetW(), q.GetX(), q.GetY(), q.GetZ());
    }

} // namespace our