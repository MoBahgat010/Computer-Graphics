#pragma once

#include <application.hpp>

#include <ecs/world.hpp>
#include <systems/forward-renderer.hpp>
#include <systems/free-camera-controller.hpp>
#include <systems/movement.hpp>
#include <asset-loader.hpp>
#include <components/mesh-renderer.hpp>
#include <deserialize-utils.hpp>

#include <algorithm>
#include <iostream>

// This state shows how to use the ECS framework and deserialization.
class Playstate: public our::State {

    our::World world;
    our::ForwardRenderer renderer;
    our::FreeCameraControllerSystem cameraController;
    our::MovementSystem movementSystem;

    void generateProceduralMap(const nlohmann::json& mapConfig) {
        if(!mapConfig.is_object()) return;

        int width = std::max(1, mapConfig.value("width", 50));
        int length = std::max(1, mapConfig.value("length", 100));
        std::string meshName = mapConfig.value("mesh", "floor_tile");
        std::string materialName = mapConfig.value("material", "floor_mat");

        auto* mesh = our::AssetLoader<our::Mesh>::get(meshName);
        auto* material = our::AssetLoader<our::Material>::get(materialName);
        if(!mesh || !material){
            std::cerr << "Map generation skipped: missing mesh/material ('"
                      << meshName << "', '" << materialName << "')." << std::endl;
            return;
        }

        glm::vec3 origin = mapConfig.value("origin", glm::vec3(0.0f));
        glm::vec2 spacing = mapConfig.value("spacing", glm::vec2(1.0f));
        glm::vec3 tileScale = mapConfig.value("tileScale", glm::vec3(1.0f));
        glm::vec3 tileRotation = glm::radians(mapConfig.value("tileRotationDegrees", glm::vec3(0.0f)));
        bool centered = mapConfig.value("centered", true);

        float xStart = centered ? -0.5f * (width - 1) * spacing.x : 0.0f;
        float zStart = centered ? -0.5f * (length - 1) * spacing.y : 0.0f;

        for(int z = 0; z < length; ++z){
            for(int x = 0; x < width; ++x){
                auto* tile = world.add();
                tile->localTransform.position = origin + glm::vec3(
                    xStart + x * spacing.x,
                    0.0f,
                    zStart + z * spacing.y
                );
                tile->localTransform.rotation = tileRotation;
                tile->localTransform.scale = tileScale;

                auto* rendererComponent = tile->addComponent<our::MeshRendererComponent>();
                rendererComponent->mesh = mesh;
                rendererComponent->material = material;
            }
        }

        std::cout << "Generated floor tiles: " << (width * length)
                  << " (" << width << "x" << length << ")" << std::endl;
    }

    void onInitialize() override {
        // First of all, we get the scene configuration from the app config
        auto& config = getApp()->getConfig()["scene"];
        // If we have assets in the scene config, we deserialize them
        if(config.contains("assets")){
            our::deserializeAllAssets(config["assets"]);
        }
        // If we have a world in the scene config, we use it to populate our world
        if(config.contains("world")){
            world.deserialize(config["world"]);
        }
        // Optional procedural map generation after deserializing the static world.
        if(config.contains("map")){
            generateProceduralMap(config["map"]);
        }
        // We initialize the camera controller system since it needs a pointer to the app
        cameraController.enter(getApp());
        // Then we initialize the renderer
        auto size = getApp()->getFrameBufferSize();
        renderer.initialize(size, config["renderer"]);
    }

    void onDraw(double deltaTime) override {
        // Here, we just run a bunch of systems to control the world logic
        movementSystem.update(&world, (float)deltaTime);
        cameraController.update(&world, (float)deltaTime);
        // And finally we use the renderer system to draw the scene
        renderer.render(&world);

        // Get a reference to the keyboard object
        auto& keyboard = getApp()->getKeyboard();

        if(keyboard.justPressed(GLFW_KEY_ESCAPE)){
            // If the escape  key is pressed in this frame, go to the play state
            getApp()->changeState("menu");
        }
    }

    void onDestroy() override {
        // Don't forget to destroy the renderer
        renderer.destroy();
        // On exit, we call exit for the camera controller system to make sure that the mouse is unlocked
        cameraController.exit();
        // Clear the world
        world.clear();
        // and we delete all the loaded assets to free memory on the RAM and the VRAM
        our::clearAllAssets();
    }
};