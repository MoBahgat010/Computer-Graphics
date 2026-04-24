#pragma once

#include <application.hpp>

#include <ecs/world.hpp>
#include <systems/forward-renderer.hpp>
#include <systems/free-camera-controller.hpp>
#include <systems/movement.hpp>
#include <systems/jolt-physics-system.hpp>
#include <systems/PlayerController/player-controller.hpp>
#include <systems/EnemyController/enemy-soldier-controller.hpp>
#include <systems/EnemySpawner/enemy-spawner-system.hpp>
#include <asset-loader.hpp>

#include <iostream>

// This state shows how to use the ECS framework and deserialization.
class Playstate: public our::State {

    our::World world;
    our::ForwardRenderer renderer;
    our::FreeCameraControllerSystem cameraController;
    our::MovementSystem movementSystem;
    our::JoltPhysicsSystem joltPhysics;
    our::PlayerControllerSystem playerController;
    our::EnemySoldierControllerSystem enemySoldierController;
    our::EnemySpawnerSystem enemySpawner;
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
        cameraController.enter(getApp(), &joltPhysics);
        // Initialize Jolt physics (Part 1 integration milestone)
        joltPhysics.init();

        if(config.contains("enemySpawner")) {
            enemySpawner.spawnEnemies(&world, config["enemySpawner"], &joltPhysics);
        }

        // Build physics bodies/character from the current ECS world
        joltPhysics.buildFromWorld(&world);
        // Then we initialize the renderer
        auto size = getApp()->getFrameBufferSize();
        renderer.initialize(size, config["renderer"]);

        // Initialize the player controller system
        playerController.enter(getApp(), &joltPhysics);

        // Initialize the enemy soldier controller system
        enemySoldierController.enter(getApp(), &joltPhysics);

    }

    void onDraw(double deltaTime) override {
        // Here, we just run a bunch of systems to control the world logic
        movementSystem.update(&world, (float)deltaTime);
        playerController.update(&world, (float)deltaTime);
        enemySoldierController.update(&world, (float)deltaTime);
        joltPhysics.update((float)deltaTime);
        // And finally we use the renderer system to draw the scene
        renderer.render(&world);
        // Delete any entities that were marked for removal this frame (e.g. dead enemies)
        world.deleteMarkedEntities();

        // Get a reference to the keyboard object
        auto& keyboard = getApp()->getKeyboard();

        if(keyboard.justPressed(GLFW_KEY_ESCAPE)){
            // If the escape  key is pressed in this frame, go to the play state
            getApp()->changeState("menu");
        }
    }

    void drawMinimap(our::Entity* playerEntity, ImVec2 ammoPos) {
        if (!playerEntity) return;

        ImGuiIO& io = ImGui::GetIO();
        ImVec2 minimapSize(180.0f, 180.0f);
        ImVec2 minimapPos(io.DisplaySize.x - 200.0f, ammoPos.y - minimapSize.y - 30.0f);
        
        // Map Background
        ImGui::GetBackgroundDrawList()->AddRectFilled(
            minimapPos, 
            ImVec2(minimapPos.x + minimapSize.x, minimapPos.y + minimapSize.y), 
            IM_COL32(10, 15, 20, 220), 5.0f);

        // Map Border
        ImGui::GetBackgroundDrawList()->AddRect(
            minimapPos, 
            ImVec2(minimapPos.x + minimapSize.x, minimapPos.y + minimapSize.y), 
            IM_COL32(0, 255, 200, 100), 5.0f, 0, 2.0f);

        ImVec2 minimapCenter(minimapPos.x + minimapSize.x * 0.5f, minimapPos.y + minimapSize.y * 0.5f);
        
        // Player Dot (Blue)
        ImGui::GetBackgroundDrawList()->AddCircleFilled(minimapCenter, 5.0f, IM_COL32(0, 150, 255, 255));
        // Inner glow for player
        ImGui::GetBackgroundDrawList()->AddCircleFilled(minimapCenter, 2.0f, IM_COL32(200, 255, 255, 255));

        // View direction cone (FOV Arc)
        float yaw = playerEntity->localTransform.rotation.y;
        float screen_angle = -yaw - glm::half_pi<float>(); // Map 3D rotation to 2D screen angle
        float fov = glm::radians(70.0f); // 70 degree view cone
        float arcRadius = 35.0f; // Length of the view cone
        
        ImGui::GetBackgroundDrawList()->PathLineTo(minimapCenter);
        ImGui::GetBackgroundDrawList()->PathArcTo(minimapCenter, arcRadius, screen_angle - fov*0.5f, screen_angle + fov*0.5f, 15);
        ImGui::GetBackgroundDrawList()->PathFillConvex(IM_COL32(255, 255, 255, 50)); // Semi-transparent white cone

        float mapScale = 3.0f; // 1 meter in 3D = 3 pixels on map
        glm::vec3 pPos = playerEntity->localTransform.position;

        // Enemy Dots (Red)
        for (auto entity : world.getEntities()) {
            if (entity->getComponent<our::EnemySoldierComponent>()) {
                glm::vec3 ePos = entity->localTransform.position;
                float dx = ePos.x - pPos.x;
                float dz = ePos.z - pPos.z; // Z is forward/backward in the world

                float mapDx = dx * mapScale;
                float mapDz = dz * mapScale;

                // Check if within minimap bounds to avoid drawing outside the box
                if (mapDx > -minimapSize.x*0.45f && mapDx < minimapSize.x*0.45f &&
                    mapDz > -minimapSize.y*0.45f && mapDz < minimapSize.y*0.45f) {
                    ImVec2 enemyMapPos(minimapCenter.x + mapDx, minimapCenter.y + mapDz);
                    ImGui::GetBackgroundDrawList()->AddCircleFilled(enemyMapPos, 4.0f, IM_COL32(255, 50, 50, 255));
                }
            }
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

        // ── Player HUD ───────────────────────────────────────────────────────
        our::PlayerComponent* player = nullptr;
        our::Entity* playerEntity = nullptr;
        for(auto entity : world.getEntities()) {
            player = entity->getComponent<our::PlayerComponent>();
            if(player) {
                playerEntity = entity;
                break;
            }
        }

        if(player && !player->getIsDead()) {
            ImGuiIO& io = ImGui::GetIO();
            ImGuiWindowFlags hudFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | 
                                        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | 
                                        ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground;

            // ── HEALTH BAR (Bottom Left) ─────────────────────────────────────
            ImVec2 healthPos(30.0f, io.DisplaySize.y - 70.0f);
            ImGui::SetNextWindowPos(healthPos);
            
            // Draw a subtle dark background behind the health manually for a sleek look
            ImGui::GetBackgroundDrawList()->AddRectFilled(
                ImVec2(healthPos.x - 10.0f, healthPos.y - 10.0f), 
                ImVec2(healthPos.x + 320.0f, healthPos.y + 45.0f), 
                IM_COL32(10, 15, 20, 200), 5.0f);

            if(ImGui::Begin("HUD_Health", nullptr, hudFlags)) {
                int health = player->getHealth();
                float healthFrac = health / 100.0f;

                ImVec4 healthColor = (healthFrac > 0.3f) ? ImVec4(0.0f, 1.0f, 0.8f, 1.0f) : ImVec4(1.0f, 0.1f, 0.1f, 1.0f);
                
                ImGui::SetWindowFontScale(1.5f);
                ImGui::TextColored(healthColor, "HP");
                ImGui::SameLine();
                ImGui::SetWindowFontScale(1.0f);
                
                // Nudge the progress bar down a bit to align with the larger text
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.0f);
                
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, healthColor);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 0.8f));
                char healthLabel[32];
                snprintf(healthLabel, sizeof(healthLabel), "%d", health);
                ImGui::ProgressBar(healthFrac, ImVec2(250.0f, 20.0f), healthLabel);
                ImGui::PopStyleColor(2);
            }
            ImGui::End();

            // ── AMMO COUNTER (Bottom Right) ──────────────────────────────────
            ImVec2 ammoPos(io.DisplaySize.x - 220.0f, io.DisplaySize.y - 85.0f);
            ImGui::SetNextWindowPos(ammoPos);

            ImGui::GetBackgroundDrawList()->AddRectFilled(
                ImVec2(ammoPos.x - 10.0f, ammoPos.y - 10.0f), 
                ImVec2(ammoPos.x + 200.0f, ammoPos.y + 60.0f), 
                IM_COL32(10, 15, 20, 200), 5.0f);

            if(ImGui::Begin("HUD_Ammo", nullptr, hudFlags)) {
                int mag = player->getMagazineAmmo();
                int total = player->getTotalAmmo();
                
                // Print CROUCHING or RELOAD warnings at the top of this box
                ImGui::SetWindowFontScale(1.0f);
                if(player->getIsCrouch()) {
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.8f, 1.0f), "CROUCHING");
                } else if(mag == 0) {
                    // We can flash RELOAD up top instead, or just keep it blank if standing
                    ImGui::TextColored(ImVec4(1.0f, 0.1f, 0.1f, 1.0f), "RELOAD [R]");
                } else {
                    // Empty space to keep the layout from jumping around when standing/crouching
                    ImGui::Text(" "); 
                }

                ImVec4 ammoColor = (mag > 0) ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(1.0f, 0.1f, 0.1f, 1.0f);
                
                ImGui::SetWindowFontScale(2.5f);
                ImGui::TextColored(ammoColor, "%02d", mag);
                ImGui::SameLine();
                
                ImGui::SetWindowFontScale(1.2f);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15.0f);
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "/ %03d", total);
                
                ImGui::SetWindowFontScale(1.0f);
                if(mag == 0 && player->getIsCrouch()) {
                    // If they are both crouching AND empty, put reload at the bottom
                    ImGui::TextColored(ImVec4(1.0f, 0.1f, 0.1f, 1.0f), "RELOAD [R]");
                }
            }
            ImGui::End();

            // ── MINIMAP (Above Ammo Counter) ─────────────────────────────────
            drawMinimap(playerEntity, ammoPos);
        }
        
        // ── Crosshair Overlay ────────────────────────────────────────────────
        auto* drawList = ImGui::GetForegroundDrawList();
        glm::ivec2 frameSize = getApp()->getFrameBufferSize();
        ImVec2 center((float)frameSize.x / 2.0f, (float)frameSize.y / 2.0f);
        ImU32 color = IM_COL32(255, 255, 255, 220); // Semi-transparent white
        
        float gap = 4.0f;
        float size = 10.0f;
        float thickness = 1.5f;

        // Draw the 4 arms of the crosshair
        drawList->AddLine(ImVec2(center.x - gap - size, center.y), ImVec2(center.x - gap, center.y), color, thickness); // Left
        drawList->AddLine(ImVec2(center.x + gap, center.y), ImVec2(center.x + gap + size, center.y), color, thickness); // Right
        drawList->AddLine(ImVec2(center.x, center.y - gap - size), ImVec2(center.x, center.y - gap), color, thickness); // Top
        drawList->AddLine(ImVec2(center.x, center.y + gap), ImVec2(center.x, center.y + gap + size), color, thickness); // Bottom

        // Center dot
        drawList->AddCircleFilled(center, 2.0f, color);
    
    }

    void onDestroy() override {
        // Don't forget to destroy the renderer
        renderer.destroy();
        // Shutdown Jolt physics and release all related resources
        joltPhysics.shutdown();
        // On exit, we call exit for the camera controller system to make sure that the mouse is unlocked
        cameraController.exit();
        playerController.exit();
        // Clear the world
        world.clear();
        // and we delete all the loaded assets to free memory on the RAM and the VRAM
        our::clearAllAssets();
    }
};