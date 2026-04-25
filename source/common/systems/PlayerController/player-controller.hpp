#pragma once
#include "../../ecs/world.hpp"
#include "../../components/PlayerComponents/player-component.hpp"
#include "../../components/EnemyComponents/enemy-soldier-component.hpp"
#include "../../components/EnemyComponents/opus-boss-component.hpp"
#include "../../components/ServerComponents/server-component.hpp"
#include "../../application.hpp"
#include "../../audio/audio-player.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/fast_trigonometry.hpp>
#include "../../components/camera.hpp"  


namespace our {
  class JoltPhysicsSystem;

  class PlayerControllerSystem {
    private:
    Application* app = nullptr;
    JoltPhysicsSystem* joltPhysics = nullptr;
    AudioPlayer fireAudioPlayer;
    AudioPlayer reloadAudioPlayer;
    void handleMovement(PlayerComponent* player, Entity* playerEntity, float deltaTime);
    void handleLook(PlayerComponent* player, Entity* playerEntity,Entity* cameraEntity);
    void handleCrouch(PlayerComponent* player,Entity* cameraEntity);
    void handleFire(PlayerComponent* player,Entity* cameraEntity);
    void handleReload(PlayerComponent* player);
    void handleDeath();

    public:
    void enter(Application* app, JoltPhysicsSystem* physics = nullptr);
    void update(World* world, float deltaTime);
    void exit();
    

  };
}