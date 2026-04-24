#pragma once

// ── Jolt Physics headers ────────────────────────────────────────────────────
// Always include Jolt.h first.
#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Body/AllowedDOFs.h>

// ── Engine headers ──────────────────────────────────────────────────────────
#include "ecs/entity.hpp"
#include "ecs/world.hpp"

// ── STL ─────────────────────────────────────────────────────────────────────
#include <unordered_map>
#include <cstdint>
#include <vector>
#include <glm/glm.hpp>

namespace our {

// ============================================================================
// Object Layers
// ============================================================================
// Each layer decides which other objects it can collide with.
// Evaluated in the broad phase — cheapest possible rejection.
// ============================================================================
namespace Layers {
    static constexpr JPH::ObjectLayer STATIC     = 0; ///< Walls, floors, ceilings (immovable)
    static constexpr JPH::ObjectLayer PLAYER     = 1; ///< The single player character
    static constexpr JPH::ObjectLayer ENEMY      = 2; ///< AI robot enemies
    static constexpr JPH::ObjectLayer NUM_LAYERS = 3;
}

// ============================================================================
// Broad-Phase Layers  (2 buckets: static vs. moving)
// ============================================================================
// Keeping static geometry in its own BVH tree avoids rebuilding it every frame.
// ============================================================================
namespace BPLayers {
    static constexpr JPH::BroadPhaseLayer NON_MOVING(0); ///< Mapped from Layers::STATIC
    static constexpr JPH::BroadPhaseLayer MOVING(1);     ///< Mapped from Layers::PLAYER and ENEMY
    static constexpr uint32_t             NUM_LAYERS = 2;
}

// ============================================================================
// BroadPhaseLayerInterface  — maps each ObjectLayer to a BroadPhaseLayer
// ============================================================================
class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface {
public:
    BPLayerInterfaceImpl() {
        mObjectToBroadPhase[Layers::STATIC] = BPLayers::NON_MOVING;
        mObjectToBroadPhase[Layers::PLAYER] = BPLayers::MOVING;
        mObjectToBroadPhase[Layers::ENEMY]  = BPLayers::MOVING;
    }

    JPH::uint GetNumBroadPhaseLayers() const override {
        return BPLayers::NUM_LAYERS;
    }

    JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override {
        JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
        return mObjectToBroadPhase[inLayer];
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override {
        switch (static_cast<JPH::BroadPhaseLayer::Type>(inLayer)) {
            case static_cast<JPH::BroadPhaseLayer::Type>(BPLayers::NON_MOVING): return "NON_MOVING";
            case static_cast<JPH::BroadPhaseLayer::Type>(BPLayers::MOVING):     return "MOVING";
            default: JPH_ASSERT(false); return "INVALID";
        }
    }
#endif

private:
    JPH::BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
};

// ============================================================================
// ObjectVsBroadPhaseLayerFilter
// Determines if an ObjectLayer should even be tested against a BroadPhaseLayer.
// ============================================================================
class ObjectVsBroadPhaseLayerFilterImpl final : public JPH::ObjectVsBroadPhaseLayerFilter {
public:
    bool ShouldCollide(JPH::ObjectLayer inLayer, JPH::BroadPhaseLayer inBPLayer) const override {
        switch (inLayer) {
            case Layers::STATIC:
                // Static geometry only needs to check moving objects
                return inBPLayer == BPLayers::MOVING;
            case Layers::PLAYER:
            case Layers::ENEMY:
                // Moving objects check against all broadphase layers
                return true;
            default:
                JPH_ASSERT(false);
                return false;
        }
    }
};

// ============================================================================
// ObjectLayerPairFilter — the final collision matrix
//
// Collision matrix:
//           STATIC   PLAYER   ENEMY
// STATIC  [  NO       YES      YES  ]
// PLAYER  [  YES      NO       YES  ]
// ENEMY   [  YES      YES      YES  ]
//
// Key decisions:
//   - STATIC vs STATIC = NO  (walls never check other walls)
//   - PLAYER vs PLAYER = NO  (only one player exists)
//   - ENEMY  vs ENEMY  = YES (robots should not overlap each other)
// ============================================================================
class ObjectLayerPairFilterImpl final : public JPH::ObjectLayerPairFilter {
public:
    bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::ObjectLayer inLayer2) const override {
        switch (inLayer1) {
            case Layers::STATIC:
                return inLayer2 != Layers::STATIC;  // Static vs Static = NO
            case Layers::PLAYER:
                return inLayer2 != Layers::PLAYER;  // Player vs Player = NO
            case Layers::ENEMY:
                return true;                         // Enemy collides with everything
            default:
                JPH_ASSERT(false);
                return false;
        }
    }
};

// ============================================================================
// JoltPhysicsSystem
// ============================================================================
// The central hub for all Jolt Physics. Owns the simulation world and provides
// a clean API for body management and raycasting to the rest of the engine.
//
// Lifecycle (call from play-state.hpp):
//   onInitialize() → joltPhysics.init()
//   onDraw()       → joltPhysics.update(deltaTime)
//   onDestroy()    → joltPhysics.shutdown()
// ============================================================================
class JoltPhysicsSystem {
public:
    // ── Lifecycle ─────────────────────────────────────────────────────────
    /// Initializes Jolt, registers all shape types, creates PhysicsSystem.
    void init();

    /// Steps the Jolt simulation forward by deltaTime seconds.
    void update(float deltaTime);

    /// Removes all bodies, destroys the PhysicsSystem, and frees all memory.
    void shutdown();

    /// Returns true after init() has been called successfully.
    bool isInitialized() const { return mPhysicsSystem != nullptr; }

    /// Scans ECS entities and creates matching Jolt objects (player + bodies).
    void buildFromWorld(World* world);

    // ── Body management ────────────────────────────────────────────────────
    JPH::BodyID addStaticBox(Entity* entity, glm::vec3 halfExtents,
                             glm::vec3 position, glm::vec3 eulerRotation = glm::vec3(0.0f));
    JPH::BodyID addStaticTriangleMesh(Entity* entity,
                                      const std::vector<glm::vec3>& localVertices,
                                      const std::vector<unsigned int>& indices,
                                      glm::vec3 position,
                                      glm::vec3 eulerRotation = glm::vec3(0.0f));
    JPH::BodyID addStaticConvexHull(Entity* entity, const std::vector<glm::vec3>& localVertices,
                                    glm::vec3 position, glm::vec3 eulerRotation = glm::vec3(0.0f));
    JPH::BodyID addDynamicBox(Entity* entity, glm::vec3 halfExtents,
                              glm::vec3 position, glm::vec3 eulerRotation = glm::vec3(0.0f));
    JPH::BodyID addDynamicConvexHull(Entity* entity, const std::vector<glm::vec3>& localVertices,
                                     glm::vec3 position, glm::vec3 eulerRotation = glm::vec3(0.0f));
    void        removeBody(Entity* entity);

    void createPlayerBody(glm::vec3 startPos,
                          float capsuleHalfHeight = 0.4f,
                          float capsuleRadius = 0.1f,
                          float capsuleCenterY = 0.5f);
    void setPlayerEntity(Entity* entity) { mPlayerEntity = entity; }
    void setPlayerVelocity(const glm::vec3& velocity) { mPendingPlayerVelocity = velocity; }

    // ── Raycasting ─────────────────────────────────────────────────────────
    struct RaycastResult {
        bool hit = false;
        Entity* entity = nullptr;
        glm::vec3 position = glm::vec3(0.0f);
        float distance = 0.0f;
    };
    RaycastResult raycast(glm::vec3 origin, glm::vec3 direction, float maxDist);

private:
    // ── Jolt core objects ─────────────────────────────────────────────────
    JPH::TempAllocatorImpl*   mTempAllocator = nullptr; ///< Scratch memory for physics steps
    JPH::JobSystemThreadPool* mJobSystem     = nullptr; ///< Multithreaded task scheduler
    JPH::PhysicsSystem*       mPhysicsSystem = nullptr; ///< The Jolt physics world

    // ── Layer interfaces (must outlive mPhysicsSystem) ────────────────────
    BPLayerInterfaceImpl              mBPLayerInterface; ///< ObjectLayer → BroadPhaseLayer mapping
    ObjectVsBroadPhaseLayerFilterImpl mObjVsBPFilter;    ///< ObjectLayer vs BroadPhaseLayer filter
    ObjectLayerPairFilterImpl         mObjPairFilter;    ///< Full collision matrix

    // ── Entity <-> Body bidirectional maps ────────────────────────────────
    // The BodyID is stored as its raw uint32 (via GetIndexAndSequenceNumber())
    // so that uint32 is hashable by std::unordered_map.
    std::unordered_map<Entity*, uint32_t> mEntityToBody; ///< ECS Entity → Jolt BodyID
    std::unordered_map<uint32_t, Entity*> mBodyToEntity; ///< Jolt BodyID → ECS Entity

    Entity*                mPlayerEntity    = nullptr; ///< ECS entity represented by mPlayerBody
    glm::vec3              mPendingPlayerVelocity = glm::vec3(0.0f); ///< Velocity to apply each update
};

} // namespace our