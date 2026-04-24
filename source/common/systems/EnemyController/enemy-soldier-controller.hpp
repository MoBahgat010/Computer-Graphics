#pragma once
#include "../../ecs/world.hpp"
#include "../../components/EnemyComponents/enemy-soldier-component.hpp"
#include "../../components/PlayerComponents/player-component.hpp"
#include "../../application.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/fast_trigonometry.hpp>
#include "../../components/camera.hpp"

namespace our { class JoltPhysicsSystem; }


namespace our{
  class EnemySoldierControllerSystem{
    private:
    Application* app = nullptr;
    JoltPhysicsSystem* physics = nullptr;

    void handleEnemySoldierBehavior(EnemySoldierComponent* enemy,PlayerComponent* player,float deltaTime);
    void handleAttack(EnemySoldierComponent* enemy,PlayerComponent* player,float deltaTime);
    void handleChase(EnemySoldierComponent* enemy,PlayerComponent* player,float deltaTime);
    void handleIdle(EnemySoldierComponent* enemy);
    void handleDead(EnemySoldierComponent* enemy);
    
    public:
    void enter(Application* app, JoltPhysicsSystem* physics = nullptr);
    void update(World* world, float deltaTime);
  };
}