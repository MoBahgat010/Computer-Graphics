#pragma once

#include <application.hpp>

#include <ecs/world.hpp>
#include <systems/forward-renderer.hpp>
#include <systems/free-camera-controller.hpp>
#include <systems/movement.hpp>
#include <systems/PlayerController/player-controller.hpp>
#include <asset-loader.hpp>

#include <iostream>

// This state shows how to use the ECS framework and deserialization.
class Playstate: public our::State {

    our::World world;
    our::ForwardRenderer renderer;
    our::FreeCameraControllerSystem cameraController;
    our::MovementSystem movementSystem;
    our::PlayerControllerSystem playerController;
    our::Entity* getCameraEntity() {
        for(auto entity : world.getEntities()) {
            if(entity->getComponent<our::CameraComponent>()) {
                return entity;
            }
        }
        return nullptr;
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
        // We initialize the camera controller system since it needs a pointer to the app
        cameraController.enter(getApp());
        // Then we initialize the renderer
        auto size = getApp()->getFrameBufferSize();
        renderer.initialize(size, config["renderer"]);

        // Initialize the player controller system
        playerController.enter(getApp());

    }

    void onDraw(double deltaTime) override {
        // Here, we just run a bunch of systems to control the world logic
        movementSystem.update(&world, (float)deltaTime);
        cameraController.update(&world, (float)deltaTime);
        playerController.update(&world, (float)deltaTime);
        // And finally we use the renderer system to draw the scene
        renderer.render(&world);

        // Get a reference to the keyboard object
        auto& keyboard = getApp()->getKeyboard();

        if(keyboard.justPressed(GLFW_KEY_ESCAPE)){
            // If the escape  key is pressed in this frame, go to the play state
            getApp()->changeState("menu");
        }
    }

    void onImmediateGui() override {
        ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(340.0f, 120.0f), ImGuiCond_FirstUseEver);

        if(ImGui::Begin("Camera Debug")) {
            if(auto* cameraEntity = getCameraEntity()) {
                const glm::vec3 position = cameraEntity->localTransform.position;
                ImGui::Text("Camera Position");
                ImGui::Text("x: %.3f   y: %.3f   z: %.3f", position.x, position.y, position.z);

                if(ImGui::Button("Print x,y,z")) {
                    std::cout << "Camera Position (x,y,z): "
                              << position.x << ", "
                              << position.y << ", "
                              << position.z << std::endl;
                }
            } else {
                ImGui::TextUnformatted("No camera entity found.");
            }
        }
        ImGui::End();

        // ── Player Status Panel ──────────────────────────────────────────────
        ImGui::SetNextWindowPos(ImVec2(20.0f, 160.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(300.0f, 220.0f), ImGuiCond_FirstUseEver);

        if(ImGui::Begin("Player Status")) {

            // Find the player component in the world
            our::PlayerComponent* player = nullptr;
            for(auto entity : world.getEntities()) {
                player = entity->getComponent<our::PlayerComponent>();
                if(player) break;
            }

            if(player) {
                // ── HEALTH BAR ───────────────────────────────────────────────
                int   health     = player->getHealth();
                float healthFrac = health / 100.0f;

                // Color: green > 50% | yellow > 25% | red otherwise
                ImVec4 healthColor;
                if     (healthFrac > 0.5f)  healthColor = ImVec4(0.15f, 0.80f, 0.15f, 1.0f);
                else if(healthFrac > 0.25f) healthColor = ImVec4(0.90f, 0.70f, 0.10f, 1.0f);
                else                        healthColor = ImVec4(0.90f, 0.10f, 0.10f, 1.0f);

                ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 1.0f), "HEALTH");
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, healthColor);
                char healthLabel[32];
                snprintf(healthLabel, sizeof(healthLabel), "%d / 100", health);
                ImGui::ProgressBar(healthFrac, ImVec2(-1.0f, 22.0f), healthLabel);
                ImGui::PopStyleColor();

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // ── AMMO ─────────────────────────────────────────────────────
                int mag   = player->getMagazineAmmo();
                int total = player->getTotalAmmo();

                ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 1.0f), "AMMO");

                // Magazine bar (orange)
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.95f, 0.55f, 0.05f, 1.0f));
                char magLabel[32];
                snprintf(magLabel, sizeof(magLabel), "Magazine  %d / 30", mag);
                ImGui::ProgressBar(mag / 30.0f, ImVec2(-1.0f, 18.0f), magLabel);
                ImGui::PopStyleColor();

                // Reserve bar (grey-blue)
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.30f, 0.55f, 0.85f, 1.0f));
                char totalLabel[32];
                snprintf(totalLabel, sizeof(totalLabel), "Reserve   %d / 180", total);
                ImGui::ProgressBar(total / 180.0f, ImVec2(-1.0f, 18.0f), totalLabel);
                ImGui::PopStyleColor();

                // RELOAD prompt
                if(mag == 0) {
                    ImGui::Spacing();
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.20f, 0.20f, 1.0f));
                    ImGui::Text("  !! RELOAD !!  Press  R");
                    ImGui::PopStyleColor();
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // ── STANCE & DEAD ─────────────────────────────────────────────
                bool crouching = player->getIsCrouch();
                ImGui::Text("Stance :  %s", crouching ? "CROUCHING" : "STANDING");

                if(player->getIsDead()) {
                    ImGui::Spacing();
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                    ImGui::Text("        -- YOU ARE DEAD --");
                    ImGui::PopStyleColor();
                }

            } else {
                ImGui::TextUnformatted("No player entity found.");
            }
        }
        ImGui::End();
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