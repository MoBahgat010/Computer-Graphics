#pragma once
#include "../../ecs/world.hpp"
#include "../../components/PlayerComponents/player-component.hpp"
#include "../../application.hpp"
namespace our {
  class PlayerControllerSystem {
    private:
    Application* app = nullptr;
    public:
    void enter(Application* app);
    void update(World* world, float deltaTime);
  };
}