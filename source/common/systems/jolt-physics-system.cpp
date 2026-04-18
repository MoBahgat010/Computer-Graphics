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
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/EActivation.h>

#include "systems/jolt-physics-system.hpp"
#include "systems/jolt-utils.hpp"
#include "components/camera.hpp"
#include "components/collider.hpp"
#include "components/free-camera-controller.hpp"
#include "components/mesh-renderer.hpp"
#include "components/movement.hpp"
#include "components/player.hpp"

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

    delete mPlayerCharacter;
    mPlayerCharacter = nullptr;
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
        createPlayerCharacter(worldTranslation);
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

        auto* collider = entity->getComponent<ColliderComponent>();
        auto* movement = entity->getComponent<MovementComponent>();
        const bool isMoving = movement &&
            (glm::length(movement->linearVelocity) > 0.0f || glm::length(movement->angularVelocity) > 0.0f);
        if (!collider) continue;

        auto* meshRenderer = entity->getComponent<MeshRendererComponent>();

        glm::mat4 m = entity->getLocalToWorldMatrix();
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::vec3 worldScale;
        glm::quat worldOrientation;
        glm::vec3 worldTranslation;
        glm::decompose(m, worldScale, worldOrientation, worldTranslation, skew, perspective);

        const glm::vec3 eulerRotation = glm::eulerAngles(worldOrientation);

        // Try convex hull for any mesh-backed collider entity.
        if (meshRenderer && meshRenderer->mesh) {
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

    if (mPlayerCharacter) {
        mPlayerCharacter->SetLinearVelocity(toJPH(mPendingPlayerVelocity));
    }

    // Step 2: Physics step
    mPhysicsSystem->Update(dt, 1, mTempAllocator, mJobSystem);

    if (mPlayerCharacter) {
        JPH::CharacterVirtual::ExtendedUpdateSettings updateSettings;
        mPlayerCharacter->ExtendedUpdate(
            dt,
            mPhysicsSystem->GetGravity(),
            updateSettings,
            mPhysicsSystem->GetDefaultBroadPhaseLayerFilter(Layers::PLAYER),
            mPhysicsSystem->GetDefaultLayerFilter(Layers::PLAYER),
            {},
            {},
            *mTempAllocator
        );
    }

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

    if (mPlayerCharacter && mPlayerEntity) {
        mPlayerEntity->localTransform.position = toGLM(mPlayerCharacter->GetPosition());
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

    delete mPlayerCharacter;
    mPlayerCharacter = nullptr;
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
        std::cerr << "[JoltPhysicsSystem] Failed to create convex hull shape: "
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

// Creates a kinematic CharacterVirtual capsule used to represent the player.
void JoltPhysicsSystem::createPlayerCharacter(glm::vec3 startPos) {
    if (!mPhysicsSystem) return;

    delete mPlayerCharacter;
    mPlayerCharacter = nullptr;

    // Capsule: halfHeight=0.7, radius=0.3, shifted up so feet rest at entity origin.
    JPH::RefConst<JPH::Shape> characterShape = JPH::RotatedTranslatedShapeSettings(
        JPH::Vec3(0.0f, 1.0f, 0.0f),
        JPH::Quat::sIdentity(),
        new JPH::CapsuleShape(0.7f, 0.3f)
    ).Create().Get();

    JPH::Ref<JPH::CharacterVirtualSettings> settings = new JPH::CharacterVirtualSettings();
    settings->mShape = characterShape;
    settings->mMaxSlopeAngle = JPH::DegreesToRadians(45.0f);
    settings->mMaxStrength = 100.0f;
    settings->mCharacterPadding = 0.02f;
    settings->mPenetrationRecoverySpeed = 1.0f;
    settings->mPredictiveContactDistance = 0.1f;
    settings->mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -0.3f);

    mPlayerCharacter = new JPH::CharacterVirtual(
        settings,
        JPH::RVec3(toJPH(startPos)),
        JPH::Quat::sIdentity(),
        0,
        mPhysicsSystem
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
    if (!mPhysicsSystem->GetNarrowPhaseQuery().CastRay(ray, hit)) {
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

}
