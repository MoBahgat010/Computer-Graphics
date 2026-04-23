#pragma once
#include "../../ecs/world.hpp"
#include "../../components/PlayerComponents/player-component.hpp"
#include "../../application.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
namespace our {
  class PlayerControllerSystem {
    private:
    Application* app = nullptr;
    void handleMovement(PlayerComponent* player, Entity* playerEntity, float deltaTime);
    void handleCrouch(PlayerComponent* player, Entity* playerEntity);
    public:
    void enter(Application* app);
    void update(World* world, float deltaTime);
    

  };
}