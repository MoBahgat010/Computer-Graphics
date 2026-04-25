#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Body/AllowedDOFs.h>
#include <Jolt/Physics/Body/BodyFilter.h>
#include <Jolt/Physics/EActivation.h>

#include "systems/jolt-physics-system.hpp"
#include "systems/jolt-utils.hpp"
#include "components/camera.hpp"
#include "components/collider.hpp"
#include "components/free-camera-controller.hpp"
#include "components/mesh-renderer.hpp"
#include "components/movement.hpp"
#include "components/PlayerComponents/player-component.hpp"
#include "components/EnemyComponents/enemy-soldier-component.hpp"

#include <iostream>
#include <cstdarg>
#include <thread>
#include <algorithm>
#include <cmath>
#include <vector>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

// Routes Jolt's internal trace logging to stdout.
static void JoltTraceImpl(const char* inFMT, ...) {
    va_list list;
    va_start(list, inFMT);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), inFMT, list);
    va_end(list);
    std::cout << "[Jolt] " << buffer << std::endl;
}

#ifdef JPH_ENABLE_ASSERTS
// Routes Jolt assertion failures to stderr and returns true to break in debug.
static bool JoltAssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, JPH::uint inLine) {
    std::cerr << "[Jolt Assert] " << inFile << ":" << inLine << " (" << inExpression << ") "
              << (inMessage ? inMessage : "") << std::endl;
    return true;
}
#endif

namespace our {

// Scans ECS entities and rebuilds the physics world bindings (player + rigid bodies).
void JoltPhysicsSystem::buildFromWorld(World* world) {
    if (!mPhysicsSystem || !world) return;

    // Reset previous physics world objects managed by this system.
    std::vector<Entity*> existing;
    existing.reserve(mEntityToBody.size());
    for (const auto& pair : mEntityToBody) existing.push_back(pair.first);
    for (Entity* entity : existing) removeBody(entity);

    mPlayerEntity = nullptr;
    mPendingPlayerVelocity = glm::vec3(0.0f);

    // Pass 1: detect player entity.
    // Prefer explicit PlayerComponent, fallback to first camera.
    Entity* fallbackCameraEntity = nullptr;
    for (Entity* entity : world->getEntities()) {
        auto* player = entity->getComponent<PlayerComponent>();
        if (player) {
            mPlayerEntity = entity;
            break;
        }

        auto* camera = entity->getComponent<CameraComponent>();
        if (camera && !fallbackCameraEntity) {
            fallbackCameraEntity = entity;
        }
    }

    if (!mPlayerEntity) {
        mPlayerEntity = fallbackCameraEntity;
    }

    if (mPlayerEntity) {
        glm::mat4 m = mPlayerEntity->getLocalToWorldMatrix();
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::vec3 worldScale;
        glm::quat worldOrientation;
        glm::vec3 worldTranslation;
        glm::decompose(m, worldScale, worldOrientation, worldTranslation, skew, perspective);

        // Default capsule matches previous behavior.
        float capsuleHalfHeight = 0.4f;
        float capsuleRadius = 0.1f;
        float capsuleCenterY = 0.5f;

        // If the player has an explicit ColliderComponent, use it to size the capsule.
        // This makes the gameplay collider match the visual/player scale in the scene.
        if (auto* playerCollider = mPlayerEntity->getComponent<ColliderComponent>()) {
            const glm::vec3 scaledHalfExtents = glm::max(
                glm::abs(playerCollider->halfExtents * worldScale),
                glm::vec3(0.01f)
            );

            capsuleRadius = glm::max(0.01f, glm::min(scaledHalfExtents.x, scaledHalfExtents.z));
            capsuleHalfHeight = glm::max(0.01f, scaledHalfExtents.y - capsuleRadius);
            capsuleCenterY = playerCollider->centerOffset.y + scaledHalfExtents.y;
        }

        createPlayerBody(worldTranslation, capsuleHalfHeight, capsuleRadius, capsuleCenterY);
    }

    auto isDescendantOfPlayer = [&](Entity* entity) {
        Entity* cur = entity;
        while (cur) {
            if (cur == mPlayerEntity) return true;
            cur = cur->parent;
        }
        return false;
    };

    size_t staticBodies = 0;
    size_t dynamicBodies = 0;

    // Pass 2: create rigid bodies for explicit colliders only.
    // - Mesh + Collider entities try ConvexHull first (static or dynamic).
    // - On hull build failure, fallback to box shapes from ColliderComponent.
    for (Entity* entity : world->getEntities()) {
        if (!entity || isDescendantOfPlayer(entity)) continue;
        if (mEntityToBody.find(entity) != mEntityToBody.end()) continue;

        auto* collider = entity->getComponent<ColliderComponent>();
        auto* enemySoldier = entity->getComponent<EnemySoldierComponent>();
        auto* meshRenderer = entity->getComponent<MeshRendererComponent>();

        // Enemy soldiers get a Kinematic Convex Hull (or Capsule) handled by a dedicated function.
        if (enemySoldier) {
            JPH::BodyID id = createEnemySoldierBody(entity, meshRenderer);
            if (!id.IsInvalid()) {
                ++dynamicBodies; // count as dynamic for logging
            }
            continue; // Skip the rest of the mesh/collider logic for enemies
        }

        if (!collider) continue;

        auto* movement = entity->getComponent<MovementComponent>();
        const bool hasMovementVelocity = movement &&
            (glm::length(movement->linearVelocity) > 0.0f || glm::length(movement->angularVelocity) > 0.0f);
        const bool isMoving = hasMovementVelocity;

        glm::mat4 m = entity->getLocalToWorldMatrix();
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::vec3 worldScale;
        glm::quat worldOrientation;
        glm::vec3 worldTranslation;
        glm::decompose(m, worldScale, worldOrientation, worldTranslation, skew, perspective);

        const glm::vec3 eulerRotation = glm::eulerAngles(worldOrientation);

        // Try convex hull only for moving mesh-backed entities.
        // For large static concave levels (city/map), a convex hull is usually wrong
        // and can enclose empty space, causing bad spawn/collision behavior.
        if (isMoving && meshRenderer && meshRenderer->mesh) {
            const auto& meshVertices = meshRenderer->mesh->getVertices();
            std::vector<glm::vec3> hullVertices;
            hullVertices.reserve(meshVertices.size());
            for (const auto& v : meshVertices) {
                hullVertices.push_back(v.position * worldScale);
            }

            JPH::BodyID id = isMoving
                ? addDynamicConvexHull(entity, hullVertices, worldTranslation, eulerRotation)
                : addStaticConvexHull(entity, hullVertices, worldTranslation, eulerRotation);
            if (!id.IsInvalid()) {
                if (isMoving) ++dynamicBodies;
                else ++staticBodies;
                continue;
            }
        }

        // For static mesh-backed colliders, use a triangle mesh shape for accurate map collision.
        if (!isMoving && meshRenderer && meshRenderer->mesh) {
            const auto& meshVertices = meshRenderer->mesh->getVertices();
            const auto& meshIndices = meshRenderer->mesh->getIndices();

            std::vector<glm::vec3> triangleVertices;
            triangleVertices.reserve(meshVertices.size());
            for (const auto& v : meshVertices) {
                triangleVertices.push_back(v.position * worldScale);
            }

            JPH::BodyID id = addStaticTriangleMesh(entity, triangleVertices, meshIndices, worldTranslation, eulerRotation);
            if (!id.IsInvalid()) {
                ++staticBodies;
                continue;
            }
        }

        // If this entity was skipped for convex hull creation and has no collider, skip.
        if (!collider) {
            continue;
        }

        const glm::vec3 halfExtents = glm::max(glm::abs(collider->halfExtents * worldScale), glm::vec3(0.01f));
        const glm::vec3 centerOffsetWorld = worldOrientation * collider->centerOffset;
        const glm::vec3 position = worldTranslation + centerOffsetWorld;


        JPH::BodyID id = isMoving
            ? addDynamicBox(entity, halfExtents, position, eulerRotation)
            : addStaticBox(entity, halfExtents, position, eulerRotation);

        if (!id.IsInvalid()) {
            if (isMoving) ++dynamicBodies;
            else ++staticBodies;
        }
    }

    std::cout << "[JoltPhysicsSystem] Jolt initialized with "
              << (staticBodies + dynamicBodies) << " bodies"
              << " (" << staticBodies << " static, " << dynamicBodies << " dynamic)."
              << std::endl;
}

// Initializes the full Jolt runtime (allocators, factory, job system, physics system).
void JoltPhysicsSystem::init() {
    if (mPhysicsSystem) return;

    JPH::RegisterDefaultAllocator();
    JPH::Trace = JoltTraceImpl;
    JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = JoltAssertFailedImpl;)

    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();

    mTempAllocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024);

    int workerThreads = std::max(1, (int)std::thread::hardware_concurrency() - 1);
    mJobSystem = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, workerThreads);

    const JPH::uint cMaxBodies = 1024;
    const JPH::uint cNumBodyMutexes = 0;
    const JPH::uint cMaxBodyPairs = 1024;
    const JPH::uint cMaxContactConstraints = 1024;

    mPhysicsSystem = new JPH::PhysicsSystem();
    mPhysicsSystem->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints,
                         mBPLayerInterface, mObjVsBPFilter, mObjPairFilter);

    mPhysicsSystem->SetGravity(JPH::Vec3(0.0f, -9.81f, 0.0f));

    std::cout << "[JoltPhysicsSystem] Initialized successfully (" << workerThreads << " worker threads)." << std::endl;
}

// Advances physics one frame and performs ECS <-> Jolt synchronization.
void JoltPhysicsSystem::update(float deltaTime) {
    if (!mPhysicsSystem) return;

    float dt = std::min(deltaTime, 0.1f);

    // Step 1: Sync ECS -> Jolt (non-static rigid bodies)
    JPH::BodyInterface& bodyInterface = mPhysicsSystem->GetBodyInterface();
    for (auto& pair : mEntityToBody) {
        Entity* entity = pair.first;
        if (!entity || entity == mPlayerEntity) continue;

        // Enemies are moved by physics velocity, skip syncing ECS -> Jolt
        if (entity->getComponent<EnemySoldierComponent>()) continue;

        JPH::BodyID id(pair.second);
        if (!bodyInterface.IsAdded(id)) continue;

        if (bodyInterface.GetMotionType(id) != JPH::EMotionType::Static) {
            bodyInterface.SetPositionAndRotationWhenChanged(
                id,
                JPH::RVec3(toJPH(entity->localTransform.position)),
                eulerToJPH(entity->localTransform.rotation),
                JPH::EActivation::Activate
            );
        }
    }

    if (mPlayerEntity) {
        auto it = mEntityToBody.find(mPlayerEntity);
        if (it != mEntityToBody.end()) {
            JPH::BodyID id(it->second);
            JPH::Vec3 currentVel = bodyInterface.GetLinearVelocity(id);
            bodyInterface.SetLinearVelocity(id, JPH::Vec3(
                mPendingPlayerVelocity.x,
                currentVel.GetY(),
                mPendingPlayerVelocity.z
            ));
        }
    }

    // Step 2: Physics step
    mPhysicsSystem->Update(dt, 1, mTempAllocator, mJobSystem);

    // Step 3: Sync Jolt -> ECS (non-static rigid bodies + player character)
    for (auto& pair : mEntityToBody) {
        Entity* entity = pair.first;
        if (!entity || entity == mPlayerEntity) continue;

        JPH::BodyID id(pair.second);
        if (!bodyInterface.IsAdded(id)) continue;

        if (bodyInterface.GetMotionType(id) != JPH::EMotionType::Static) {
            entity->localTransform.position = toGLM(bodyInterface.GetPosition(id));
        }
    }

    // Sync player entity position specifically
    if (mPlayerEntity) {
        auto it = mEntityToBody.find(mPlayerEntity);
        if (it != mEntityToBody.end()) {
            JPH::BodyID id(it->second);
            JPH::RVec3 pos = bodyInterface.GetPosition(id);
            mPlayerEntity->localTransform.position = toGLM(pos);
        }
    }
}

// Destroys all physics objects and shuts down Jolt-owned resources.
void JoltPhysicsSystem::shutdown() {
    if (!mPhysicsSystem) return;

    JPH::BodyInterface& bodyInterface = mPhysicsSystem->GetBodyInterface();
    for (auto& p : mEntityToBody) {
        JPH::BodyID id(p.second);
        bodyInterface.RemoveBody(id);
        bodyInterface.DestroyBody(id);
    }

    mEntityToBody.clear();
    mBodyToEntity.clear();

    mPlayerEntity = nullptr;
    mPendingPlayerVelocity = glm::vec3(0.0f);

    delete mPhysicsSystem; mPhysicsSystem = nullptr;
    delete mJobSystem;     mJobSystem = nullptr;
    delete mTempAllocator; mTempAllocator = nullptr;

    JPH::UnregisterTypes();
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;

    std::cout << "[JoltPhysicsSystem] Shutdown complete." << std::endl;
}

// Creates and registers a static triangle-mesh body and maps it to an ECS entity.
JPH::BodyID JoltPhysicsSystem::addStaticTriangleMesh(Entity* entity,
                                                     const std::vector<glm::vec3>& localVertices,
                                                     const std::vector<unsigned int>& indices,
                                                     glm::vec3 position,
                                                     glm::vec3 eulerRotation) {
    if (!mPhysicsSystem || localVertices.size() < 3 || indices.size() < 3) return JPH::BodyID();

    JPH::MeshShapeSettings shapeSettings;
    shapeSettings.mTriangleVertices.reserve(localVertices.size());
    for (const auto& v : localVertices) {
        shapeSettings.mTriangleVertices.push_back(JPH::Float3(v.x, v.y, v.z));
    }

    shapeSettings.mIndexedTriangles.reserve(indices.size() / 3);
    size_t preFilterSkipped = 0;

    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        const JPH::uint32 i0 = static_cast<JPH::uint32>(indices[i]);
        const JPH::uint32 i1 = static_cast<JPH::uint32>(indices[i + 1]);
        const JPH::uint32 i2 = static_cast<JPH::uint32>(indices[i + 2]);
        
        if (i0 >= localVertices.size() || i1 >= localVertices.size() || i2 >= localVertices.size() ||
            i0 == i1 || i0 == i2 || i1 == i2) {
            ++preFilterSkipped;
            continue;
        }

        const glm::vec3& v0 = localVertices[i0];
        const glm::vec3& v1 = localVertices[i1];
        const glm::vec3& v2 = localVertices[i2];
        if (!std::isfinite(v0.x) || !std::isfinite(v1.x) || !std::isfinite(v2.x)) {
            ++preFilterSkipped;
            continue;
        }

        // Restoring the sliver-triangle filter we agreed upon (threshold 1e-8f) to fix the chairs
        const glm::vec3 edge1 = v1 - v0;
        const glm::vec3 edge2 = v2 - v0;
        const glm::vec3 cross = glm::cross(edge1, edge2);
        const float crossLenSq = glm::dot(cross, cross);
        if (crossLenSq < 1e-8f) {
            ++preFilterSkipped;
            continue;
        }

        shapeSettings.mIndexedTriangles.push_back(JPH::IndexedTriangle(i0, i1, i2));
    }

    if (preFilterSkipped > 0) {
        std::cout << "[JoltPhysicsSystem] Static mesh collider: pre-filter skipped "
                  << preFilterSkipped << " invalid triangles." << std::endl;
    }

    if (shapeSettings.mIndexedTriangles.empty()) {
        std::cerr << "[JoltPhysicsSystem] Failed to create static mesh shape: no valid triangles after pre-filter." << std::endl;
        return JPH::BodyID();
    }

    const size_t triCountBefore = shapeSettings.mIndexedTriangles.size();

    // Let Jolt remove duplicate and degenerate triangles in one efficient pass.
    shapeSettings.Sanitize();

    const size_t triCountAfter = shapeSettings.mIndexedTriangles.size();
    if (triCountAfter < triCountBefore) {
        std::cout << "[JoltPhysicsSystem] Static mesh collider: Jolt Sanitize() removed "
                  << (triCountBefore - triCountAfter) << " degenerate/duplicate triangles ("
                  << triCountAfter << " remaining)." << std::endl;
    }

    if (shapeSettings.mIndexedTriangles.empty()) {
        std::cerr << "[JoltPhysicsSystem] Failed to create static mesh shape: no triangles survived sanitization." << std::endl;
        return JPH::BodyID();
    }

    std::cout << "[JoltPhysicsSystem] Building static mesh shape with "
              << triCountAfter << " triangles..." << std::endl;

    JPH::ShapeSettings::ShapeResult shapeResult = shapeSettings.Create();
    if (shapeResult.HasError()) {
        std::cerr << "[JoltPhysicsSystem] Failed to create static mesh shape: "
                  << shapeResult.GetError().c_str() << std::endl;
        return JPH::BodyID();
    }

    JPH::BodyCreationSettings bodySettings(
        shapeResult.Get(),
        JPH::RVec3(toJPH(position)),
        eulerToJPH(eulerRotation),
        JPH::EMotionType::Static,
        Layers::STATIC
    );

    JPH::BodyInterface& bodyInterface = mPhysicsSystem->GetBodyInterface();
    JPH::BodyID id = bodyInterface.CreateAndAddBody(bodySettings, JPH::EActivation::DontActivate);

    if (!id.IsInvalid() && entity) {
        uint32_t raw = id.GetIndexAndSequenceNumber();
        mEntityToBody[entity] = raw;
        mBodyToEntity[raw] = entity;
    }

    return id;
}

// Creates and registers a static box body and maps it to an ECS entity.
JPH::BodyID JoltPhysicsSystem::addStaticBox(Entity* entity, glm::vec3 halfExtents,
                                            glm::vec3 position, glm::vec3 eulerRotation) {
    if (!mPhysicsSystem) return JPH::BodyID();

    glm::vec3 safeHalfExtents = glm::max(glm::abs(halfExtents), glm::vec3(0.001f));

    JPH::BoxShapeSettings shapeSettings(toJPH(safeHalfExtents));
    JPH::ShapeSettings::ShapeResult shapeResult = shapeSettings.Create();
    if (shapeResult.HasError()) {
        std::cerr << "[JoltPhysicsSystem] Failed to create static box shape: "
                  << shapeResult.GetError().c_str() << std::endl;
        return JPH::BodyID();
    }

    JPH::BodyCreationSettings bodySettings(
        shapeResult.Get(),
        JPH::RVec3(toJPH(position)),
        eulerToJPH(eulerRotation),
        JPH::EMotionType::Static,
        Layers::STATIC
    );

    JPH::BodyInterface& bodyInterface = mPhysicsSystem->GetBodyInterface();
    JPH::BodyID id = bodyInterface.CreateAndAddBody(bodySettings, JPH::EActivation::DontActivate);

    if (!id.IsInvalid() && entity) {
        uint32_t raw = id.GetIndexAndSequenceNumber();
        mEntityToBody[entity] = raw;
        mBodyToEntity[raw] = entity;
    }

    return id;
}

// Creates and registers a static convex hull body and maps it to an ECS entity.
JPH::BodyID JoltPhysicsSystem::addStaticConvexHull(Entity* entity, const std::vector<glm::vec3>& localVertices,
                                                   glm::vec3 position, glm::vec3 eulerRotation) {
    if (!mPhysicsSystem || localVertices.size() < 4) return JPH::BodyID();

    JPH::Array<JPH::Vec3> points;
    points.reserve(localVertices.size());
    for (const auto& v : localVertices) {
        points.push_back(toJPH(v));
    }

    JPH::ConvexHullShapeSettings shapeSettings(points);
    JPH::ShapeSettings::ShapeResult shapeResult = shapeSettings.Create();
    if (shapeResult.HasError()) {
        std::cerr << "[JoltPhysicsSystem] Failed to create static convex hull shape: "
                  << shapeResult.GetError().c_str() << std::endl;
        return JPH::BodyID();
    }

    JPH::BodyCreationSettings bodySettings(
        shapeResult.Get(),
        JPH::RVec3(toJPH(position)),
        eulerToJPH(eulerRotation),
        JPH::EMotionType::Static,
        Layers::STATIC
    );

    JPH::BodyInterface& bodyInterface = mPhysicsSystem->GetBodyInterface();
    JPH::BodyID id = bodyInterface.CreateAndAddBody(bodySettings, JPH::EActivation::DontActivate);

    if (!id.IsInvalid() && entity) {
        uint32_t raw = id.GetIndexAndSequenceNumber();
        mEntityToBody[entity] = raw;
        mBodyToEntity[raw] = entity;
    }

    return id;
}

// Creates and registers a dynamic convex hull body and maps it to an ECS entity.
JPH::BodyID JoltPhysicsSystem::addDynamicConvexHull(Entity* entity, const std::vector<glm::vec3>& localVertices,
                                                    glm::vec3 position, glm::vec3 eulerRotation, bool lockRotation) {
    if (!mPhysicsSystem || localVertices.size() < 4) return JPH::BodyID();

    JPH::Array<JPH::Vec3> points;
    points.reserve(localVertices.size());
    for (const auto& v : localVertices) {
        points.push_back(toJPH(v));
    }

    JPH::ConvexHullShapeSettings shapeSettings(points);
    JPH::ShapeSettings::ShapeResult shapeResult = shapeSettings.Create();
    if (shapeResult.HasError()) {
        std::cerr << "[JoltPhysicsSystem] Failed to create convex hull shape: "
                  << shapeResult.GetError().c_str() << std::endl;
        return JPH::BodyID();
    }

    JPH::BodyCreationSettings bodySettings(
        shapeResult.Get(),
        JPH::RVec3(toJPH(position)),
        eulerToJPH(eulerRotation),
        JPH::EMotionType::Dynamic,
        Layers::ENEMY // Or maybe dynamic
    );
    
    if (lockRotation) {
        bodySettings.mAllowedDOFs = JPH::EAllowedDOFs::TranslationX | 
                                    JPH::EAllowedDOFs::TranslationY | 
                                    JPH::EAllowedDOFs::TranslationZ;
        bodySettings.mAllowSleeping = false;
    }

    JPH::BodyInterface& bodyInterface = mPhysicsSystem->GetBodyInterface();
    JPH::BodyID id = bodyInterface.CreateAndAddBody(bodySettings, JPH::EActivation::Activate);

    if (!id.IsInvalid() && entity) {
        uint32_t raw = id.GetIndexAndSequenceNumber();
        mEntityToBody[entity] = raw;
        mBodyToEntity[raw] = entity;
    }

    return id;
}

// Creates and registers a kinematic convex hull body and maps it to an ECS entity.
JPH::BodyID JoltPhysicsSystem::addKinematicConvexHull(Entity* entity, const std::vector<glm::vec3>& localVertices,
                                                      glm::vec3 position, glm::vec3 eulerRotation) {
    if (!mPhysicsSystem || localVertices.size() < 4) return JPH::BodyID();

    JPH::Array<JPH::Vec3> points;
    points.reserve(localVertices.size());
    for (const auto& v : localVertices) {
        points.push_back(toJPH(v));
    }

    JPH::ConvexHullShapeSettings shapeSettings(points);
    shapeSettings.mMaxConvexRadius = 0.01f; // Prevent failure on small models
    JPH::ShapeSettings::ShapeResult shapeResult = shapeSettings.Create();
    if (shapeResult.HasError()) {
        std::cerr << "[JoltPhysicsSystem] Failed to create kinematic convex hull shape: "
                  << shapeResult.GetError().c_str() << std::endl;
        return JPH::BodyID();
    }

    JPH::BodyCreationSettings bodySettings(
        shapeResult.Get(),
        JPH::RVec3(toJPH(position)),
        eulerToJPH(eulerRotation),
        JPH::EMotionType::Kinematic,
        Layers::ENEMY
    );

    // Lock rotation to keep upright (optional, but good for kinematic actors)
    bodySettings.mAllowedDOFs = JPH::EAllowedDOFs::TranslationX | 
                                JPH::EAllowedDOFs::TranslationY | 
                                JPH::EAllowedDOFs::TranslationZ;
    bodySettings.mAllowSleeping = false;

    JPH::BodyInterface& bodyInterface = mPhysicsSystem->GetBodyInterface();
    JPH::BodyID id = bodyInterface.CreateAndAddBody(bodySettings, JPH::EActivation::Activate);

    if (!id.IsInvalid() && entity) {
        uint32_t raw = id.GetIndexAndSequenceNumber();
        mEntityToBody[entity] = raw;
        mBodyToEntity[raw] = entity;
    }

    return id;
}

// Creates and registers a dynamic box body and maps it to an ECS entity.
JPH::BodyID JoltPhysicsSystem::addDynamicBox(Entity* entity, glm::vec3 halfExtents,
                                             glm::vec3 position, glm::vec3 eulerRotation) {
    if (!mPhysicsSystem) return JPH::BodyID();

    glm::vec3 safeHalfExtents = glm::max(glm::abs(halfExtents), glm::vec3(0.001f));

    JPH::BoxShapeSettings shapeSettings(toJPH(safeHalfExtents));
    JPH::ShapeSettings::ShapeResult shapeResult = shapeSettings.Create();
    if (shapeResult.HasError()) {
        std::cerr << "[JoltPhysicsSystem] Failed to create dynamic box shape: "
                  << shapeResult.GetError().c_str() << std::endl;
        return JPH::BodyID();
    }

    JPH::BodyCreationSettings bodySettings(
        shapeResult.Get(),
        JPH::RVec3(toJPH(position)),
        eulerToJPH(eulerRotation),
        JPH::EMotionType::Dynamic,
        Layers::ENEMY
    );

    JPH::BodyInterface& bodyInterface = mPhysicsSystem->GetBodyInterface();
    JPH::BodyID id = bodyInterface.CreateAndAddBody(bodySettings, JPH::EActivation::Activate);

    if (!id.IsInvalid() && entity) {
        uint32_t raw = id.GetIndexAndSequenceNumber();
        mEntityToBody[entity] = raw;
        mBodyToEntity[raw] = entity;
    }

    return id;
}

// Creates and registers a kinematic capsule body and maps it to an ECS entity.
JPH::BodyID JoltPhysicsSystem::addKinematicCapsule(Entity* entity, float halfHeight, float radius,
                                                   glm::vec3 position, glm::vec3 eulerRotation) {
    if (!mPhysicsSystem) return JPH::BodyID();

    // Shift up so feet rest at entity origin (like player).
    JPH::RefConst<JPH::Shape> capsuleShape = JPH::RotatedTranslatedShapeSettings(
        JPH::Vec3(0.0f, halfHeight + radius, 0.0f),
        JPH::Quat::sIdentity(),
        new JPH::CapsuleShape(halfHeight, radius)
    ).Create().Get();

    JPH::BodyCreationSettings bodySettings(
        capsuleShape,
        JPH::RVec3(toJPH(position)),
        eulerToJPH(eulerRotation),
        JPH::EMotionType::Kinematic,
        Layers::ENEMY
    );

    // Lock rotation to keep upright
    bodySettings.mAllowedDOFs = JPH::EAllowedDOFs::TranslationX | 
                                JPH::EAllowedDOFs::TranslationY | 
                                JPH::EAllowedDOFs::TranslationZ;
    bodySettings.mAllowSleeping = false;

    JPH::BodyInterface& bodyInterface = mPhysicsSystem->GetBodyInterface();
    JPH::BodyID id = bodyInterface.CreateAndAddBody(bodySettings, JPH::EActivation::Activate);

    if (!id.IsInvalid() && entity) {
        uint32_t raw = id.GetIndexAndSequenceNumber();
        mEntityToBody[entity] = raw;
        mBodyToEntity[raw] = entity;
    }

    return id;
}

// Removes and destroys the physics body associated with a given ECS entity.
void JoltPhysicsSystem::removeBody(Entity* entity) {
    if (!mPhysicsSystem || !entity) return;

    auto it = mEntityToBody.find(entity);
    if (it == mEntityToBody.end()) return;

    JPH::BodyID id(it->second);
    JPH::BodyInterface& bodyInterface = mPhysicsSystem->GetBodyInterface();

    if (bodyInterface.IsAdded(id)) {
        bodyInterface.RemoveBody(id);
    }
    bodyInterface.DestroyBody(id);

    mBodyToEntity.erase(it->second);
    mEntityToBody.erase(it);
}

// Creates and registers a dynamic capsule body and maps it to an ECS entity.
JPH::BodyID JoltPhysicsSystem::addDynamicCapsule(Entity* entity, float halfHeight, float radius, float centerY,
                                                 glm::vec3 position, glm::vec3 eulerRotation, JPH::ObjectLayer layer) {
    if (!mPhysicsSystem) return JPH::BodyID();

    JPH::RefConst<JPH::Shape> capsuleShape = JPH::RotatedTranslatedShapeSettings(
        JPH::Vec3(0.0f, centerY, 0.0f),
        JPH::Quat::sIdentity(),
        new JPH::CapsuleShape(halfHeight, radius)
    ).Create().Get();

    JPH::BodyCreationSettings bodySettings(
        capsuleShape,
        JPH::RVec3(toJPH(position)),
        eulerToJPH(eulerRotation),
        JPH::EMotionType::Dynamic,
        layer // <--- Now uses the passed-in layer
    );

    bodySettings.mAllowedDOFs = JPH::EAllowedDOFs::TranslationX | 
                                JPH::EAllowedDOFs::TranslationY | 
                                JPH::EAllowedDOFs::TranslationZ;
    bodySettings.mAllowSleeping = false;
    bodySettings.mMotionQuality = JPH::EMotionQuality::LinearCast;
    bodySettings.mMassPropertiesOverride.mMass = 70.0f;
    bodySettings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
    bodySettings.mFriction = 0.0f;
    bodySettings.mRestitution = 0.0f;

    JPH::BodyInterface& bodyInterface = mPhysicsSystem->GetBodyInterface();
    JPH::BodyID id = bodyInterface.CreateAndAddBody(bodySettings, JPH::EActivation::Activate);

    if (!id.IsInvalid() && entity) {
        uint32_t raw = id.GetIndexAndSequenceNumber();
        mEntityToBody[entity] = raw;
        mBodyToEntity[raw] = entity;
    }

    return id;
}

void JoltPhysicsSystem::createPlayerBody(glm::vec3 startPos,
                                         float capsuleHalfHeight,
                                         float capsuleRadius,
                                         float capsuleCenterY) {
    if (!mPhysicsSystem) return;

    capsuleRadius = glm::max(0.01f, capsuleRadius);
    capsuleHalfHeight = glm::max(0.01f, capsuleHalfHeight);

    // Use the universal helper!
    JPH::BodyID id = addDynamicCapsule(mPlayerEntity, capsuleHalfHeight, capsuleRadius, capsuleCenterY, startPos, glm::vec3(0.0f), Layers::PLAYER);

    if (!id.IsInvalid() && mPlayerEntity) {
        const float totalHeight = (2.0f * capsuleHalfHeight) + (2.0f * capsuleRadius);
        std::cout << "[JoltPhysicsSystem] Player capsule: radius=" << capsuleRadius
                  << ", halfHeight=" << capsuleHalfHeight
                  << ", totalHeight=" << totalHeight
                  << ", centerY=" << capsuleCenterY
                  << std::endl;
    }
}

// Dedicated function to create the physics body for an EnemySoldier
JPH::BodyID JoltPhysicsSystem::createEnemySoldierBody(Entity* entity, MeshRendererComponent* meshRenderer) {
    glm::mat4 m = entity->getLocalToWorldMatrix();
    
    // Safe extraction without glm::decompose which can fail with tiny scales
    glm::vec3 worldTranslation = glm::vec3(m[3]);
    glm::vec3 worldScale(
        glm::length(glm::vec3(m[0])),
        glm::length(glm::vec3(m[1])),
        glm::length(glm::vec3(m[2]))
    );
    
    // For rotation, safely extract from matrix if scale is not zero
    glm::mat3 rotMat(1.0f);
    if (worldScale.x > 1e-6f) rotMat[0] = glm::vec3(m[0]) / worldScale.x;
    if (worldScale.y > 1e-6f) rotMat[1] = glm::vec3(m[1]) / worldScale.y;
    if (worldScale.z > 1e-6f) rotMat[2] = glm::vec3(m[2]) / worldScale.z;
    glm::quat worldOrientation = glm::quat_cast(rotMat);
    
    const glm::vec3 eulerRotation = glm::eulerAngles(worldOrientation);
    // Fallback defaults just in case a robot is spawned without a collider in the JSON
    float capsuleHalfHeight = 0.2f;
    float capsuleRadius = 0.15f;
    float centerY = 0.0f; // Still defaults to the chest

    // Read directly from the JSON ColliderComponent!
    if (auto* collider = entity->getComponent<ColliderComponent>()) {
        
        // Multiply the JSON halfExtents by the visual scale of the robot
        glm::vec3 scaledHalfExtents = glm::max(
            glm::abs(collider->halfExtents * worldScale),
            glm::vec3(0.01f)
        );

        capsuleRadius = glm::max(0.01f, glm::min(scaledHalfExtents.x, scaledHalfExtents.z));
        capsuleHalfHeight = glm::max(0.01f, scaledHalfExtents.y - capsuleRadius);
        
        // Use the centerOffset from JSON so you can nudge the hitbox up or down without code
        centerY = collider->centerOffset.y; 
    }

    // Use the universal helper, explicitly assigning the robot to the ENEMY collision layer
    return addDynamicCapsule(
        entity, 
        capsuleHalfHeight, 
        capsuleRadius, 
        centerY, 
        worldTranslation, 
        eulerRotation, 
        Layers::ENEMY
    );
}

// Casts a ray in the physics world and returns closest hit info (if any).
JoltPhysicsSystem::RaycastResult JoltPhysicsSystem::raycast(glm::vec3 origin,
                                                            glm::vec3 direction,
                                                            float maxDist) {
    RaycastResult result;
    if (!mPhysicsSystem || maxDist <= 0.0f) return result;

    float dirLen = glm::length(direction);
    if (dirLen <= 1e-6f) return result;

    glm::vec3 dirNorm = direction / dirLen;
    JPH::RRayCast ray(JPH::RVec3(toJPH(origin)), toJPH(dirNorm * maxDist));

    JPH::RayCastResult hit;
    bool hasHit = false;
    
    if (mPlayerEntity) {
        auto it = mEntityToBody.find(mPlayerEntity);
        if (it != mEntityToBody.end()) {
            JPH::IgnoreSingleBodyFilter bodyFilter(JPH::BodyID(it->second));
            hasHit = mPhysicsSystem->GetNarrowPhaseQuery().CastRay(ray, hit, { }, { }, bodyFilter);
        } else {
            hasHit = mPhysicsSystem->GetNarrowPhaseQuery().CastRay(ray, hit);
        }
    } else {
        hasHit = mPhysicsSystem->GetNarrowPhaseQuery().CastRay(ray, hit);
    }

    if (!hasHit) {
        return result;
    }

    result.hit = true;
    result.distance = hit.mFraction * maxDist;
    result.position = toGLM(ray.GetPointOnRay(hit.mFraction));

    uint32_t raw = hit.mBodyID.GetIndexAndSequenceNumber();
    auto it = mBodyToEntity.find(raw);
    if (it != mBodyToEntity.end()) {
        result.entity = it->second;
    }

    return result;
}

void JoltPhysicsSystem::setLinearVelocity(Entity* entity, const glm::vec3& velocity) {
    if (!mPhysicsSystem || !entity) return;
    auto it = mEntityToBody.find(entity);
    if (it != mEntityToBody.end()) {
        JPH::BodyID id(it->second);
        JPH::BodyInterface& bodyInterface = mPhysicsSystem->GetBodyInterface();
        if (bodyInterface.IsAdded(id)) {
            JPH::Vec3 currentVel = bodyInterface.GetLinearVelocity(id);
            bodyInterface.SetLinearVelocity(id, JPH::Vec3(
                velocity.x,
                currentVel.GetY(),
                velocity.z
            ));
        }
    }
}

}
