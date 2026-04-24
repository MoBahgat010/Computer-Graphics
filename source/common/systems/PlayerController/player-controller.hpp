#pragma once
#include "../../ecs/world.hpp"
#include "../../components/PlayerComponents/player-component.hpp"
#include "../../application.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/fast_trigonometry.hpp>
#include "../../components/camera.hpp"  


namespace our {
  class PlayerControllerSystem {
    private:
    Application* app = nullptr;
    void handleMovement(PlayerComponent* player, Entity* playerEntity, float deltaTime);
    void handleLook(PlayerComponent* player, Entity* playerEntity,Entity* cameraEntity);
    void handleCrouch(PlayerComponent* player,Entity* cameraEntity);
    void handleFire(PlayerComponent* player);
    void handleReload(PlayerComponent* player);
    void handleDeath();

    public:
    void enter(Application* app);
    void update(World* world, float deltaTime);
    void exit();
    

  };
}