#include "player-controller.hpp"

namespace our {


  void PlayerControllerSystem::enter(Application* app){
      this->app = app;
  }


  void PlayerControllerSystem::update(World* world, float deltaTime) {
      if(!app || !world) return;

      PlayerComponent* player = nullptr;
      Entity* playerEntity = nullptr;
      for(auto entity : world->getEntities()){
        player = entity->getComponent<PlayerComponent>();
        if(player) {
          playerEntity = entity;
          break;
        }
      }
      if(!player || !playerEntity) return;
  }
}