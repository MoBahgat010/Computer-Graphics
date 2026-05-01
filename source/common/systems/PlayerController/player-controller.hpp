#pragma once
#include "../../ecs/world.hpp"
#include "../../components/PlayerComponents/player-component.hpp"
#include "../../components/EnemyComponents/enemy-soldier-component.hpp"
#include "../../components/EnemyComponents/opus-boss-component.hpp"
#include "../../components/ServerComponents/server-component.hpp"
#include "../../components/animation-component.hpp"
#include "../../application.hpp"
#include "../../audio/audio-player.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/fast_trigonometry.hpp>
#include <GLFW/glfw3.h>
#include "../../components/camera.hpp"  


namespace our {
  class JoltPhysicsSystem;

  class PlayerControllerSystem {
    private:
    Application* app = nullptr;
    JoltPhysicsSystem* joltPhysics = nullptr;
    AudioPlayer fireAudioPlayer;
    AudioPlayer emptyAmmoAudioPlayer;
    AudioPlayer reloadAudioPlayer;
    GLFWgamepadstate currentGamepad{};
    GLFWgamepadstate previousGamepad{};
    bool hasPreviousGamepad = false;
    bool gamepadActive = false;
    void updateGamepadState();
    bool isGamepadButtonPressed(int button) const;
    bool isGamepadButtonJustPressed(int button) const;
    float getGamepadAxis(int axis, float deadzone = 0.12f) const;
    float getGamepadTrigger(int axis) const;
    bool isGamepadTriggerJustPressed(int axis, float threshold = 0.2f) const;
    void handleMovement(PlayerComponent* player, Entity* playerEntity, float deltaTime);
    void handleLook(PlayerComponent* player, Entity* playerEntity,Entity* cameraEntity, float deltaTime);
    void handleCrouch(PlayerComponent* player,Entity* cameraEntity);
    void handleJump(PlayerComponent* player, Entity* playerEntity);
    void handleFire(PlayerComponent* player,Entity* cameraEntity);
    void handleReload(PlayerComponent* player);
    void handleDeath();

    public:
    void enter(Application* app, JoltPhysicsSystem* physics = nullptr);
    void update(World* world, float deltaTime);
    void exit();
    

  };
}