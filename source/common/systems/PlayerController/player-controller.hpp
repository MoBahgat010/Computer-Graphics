#pragma once
#include "../../ecs/world.hpp"
#include "../../components/PlayerComponents/player-component.hpp"
#include "../../application.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/fast_trigonometry.hpp>
namespace our {
  class PlayerControllerSystem {
    private:
    Application* app = nullptr;
    void handleMovement(PlayerComponent* player, Entity* playerEntity, float deltaTime);
    void handleLook(PlayerComponent* player, Entity* playerEntity);
    void handleCrouch(PlayerComponent* player, Entity* playerEntity);
    void handleFire(PlayerComponent* player);
    void handleReload(PlayerComponent* player);

    public:
    void enter(Application* app);
    void update(World* world, float deltaTime);
    void exit();
    

  };
}