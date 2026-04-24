#pragma once

#include <asset-loader.hpp>
#include <components/EnemyComponents/enemy-soldier-component.hpp>
#include <components/mesh-renderer.hpp>
#include <ecs/world.hpp>
#include <systems/jolt-physics-system.hpp>

#include <glm/glm.hpp>

#include <algorithm>
#include <string>
#include <vector>

namespace our {

class EnemySpawnerSystem {
public:
    void spawnEnemies(World* world, const nlohmann::json& config, JoltPhysicsSystem* physics) {
        if (!world || !config.is_object()) return;

        const int enemiesPerPoint = std::max(1, config.value("enemiesPerPoint", 2));
        const float spawnOffset = config.value("spawnOffset", 0.5f);

        nlohmann::json enemyConfig = nlohmann::json::object();
        if (config.contains("enemy") && config["enemy"].is_object()) {
            enemyConfig = config["enemy"];
        }

        const std::string meshName = enemyConfig.value("mesh", std::string("robot"));
        const std::string materialName = enemyConfig.value("material", std::string("robot"));

        glm::vec3 enemyScale = glm::vec3(1.0f);
        if (enemyConfig.contains("scale") && enemyConfig["scale"].is_array() && enemyConfig["scale"].size() == 3) {
            enemyScale = glm::vec3(
                enemyConfig["scale"][0].get<float>(),
                enemyConfig["scale"][1].get<float>(),
                enemyConfig["scale"][2].get<float>()
            );
        }

        std::vector<Entity*> spawnPoints;
        spawnPoints.reserve(world->getEntities().size());
        for (Entity* entity : world->getEntities()) {
            if (entity && entity->name == "EnemySpawnPoint") {
                spawnPoints.push_back(entity);
            }
        }

        if (spawnPoints.empty()) return;

        for (Entity* spawnPoint : spawnPoints) {
            if (!spawnPoint) continue;

            const glm::vec3 basePosition = spawnPoint->localTransform.position;
            const glm::vec3 spawnRotation = spawnPoint->localTransform.rotation;

            for (int i = 0; i < enemiesPerPoint; ++i) {
                Entity* enemy = world->add();
                enemy->name = "Robot Enemy";
                enemy->parent = nullptr;
                enemy->localTransform.position = basePosition + glm::vec3(computeXOffset(i, enemiesPerPoint, spawnOffset), 0.0f, 0.0f);
                enemy->localTransform.rotation = spawnRotation;
                enemy->localTransform.scale = enemyScale;

                auto* meshRenderer = enemy->addComponent<MeshRendererComponent>();
                meshRenderer->mesh = AssetLoader<Mesh>::get(meshName);
                meshRenderer->material = AssetLoader<Material>::get(materialName);

                auto* enemySoldier = enemy->addComponent<EnemySoldierComponent>();
                enemySoldier->deserialize(enemyConfig);

                if (physics && physics->isInitialized()) {
                    physics->createEnemySoldierBody(enemy, meshRenderer);
                }
            }

            world->markForRemoval(spawnPoint);
        }

        world->deleteMarkedEntities();
    }

private:
    static float computeXOffset(int index, int count, float spawnOffset) {
        if (count <= 1) return 0.0f;
        if (count == 2) {
            return index == 0 ? -spawnOffset : spawnOffset;
        }

        const float centerIndex = 0.5f * static_cast<float>(count - 1);
        return (static_cast<float>(index) - centerIndex) * spawnOffset;
    }
};

} 
