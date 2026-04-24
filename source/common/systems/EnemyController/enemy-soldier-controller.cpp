#include "enemy-soldier-controller.hpp"
#include <vector>
#include <algorithm>



namespace our {
 
  void EnemySoldierControllerSystem::enter(Application* app){
    this->app = app;
  }

  void EnemySoldierControllerSystem::update(World* world, float deltaTime){
      if(!app || !world) return;
      
    std::vector<EnemySoldierComponent*>enemySoldiers;
    PlayerComponent* player = nullptr;
    
    // gather all enemy soldiers and the player
    for(auto entity : world->getEntities()){
      auto enemySoldierComponent = entity->getComponent<EnemySoldierComponent>();
      if(enemySoldierComponent) enemySoldiers.push_back(enemySoldierComponent);
      auto playerComponent = entity->getComponent<PlayerComponent>();
      if(playerComponent) player = playerComponent;
    }

    if (!player) return;

    for (auto* enemy:enemySoldiers){
      handleEnemySoldierBehavior(enemy,player,deltaTime);
    }

    
      
  }




  void EnemySoldierControllerSystem::handleEnemySoldierBehavior(EnemySoldierComponent* enemy,PlayerComponent* player,float deltaTime){
    Entity* enemyEntity = enemy->getOwner();
    Entity* playerEntity = player->getOwner();
    float distanceToPlayer = glm::distance(enemyEntity->localTransform.position, playerEntity->localTransform.position); 
    if(enemy->getIsDead()) {
      handleDead(enemy);
      return;
    } 
  
    if(distanceToPlayer <= enemy->getAttackRange() ){
      handleAttack(enemy,player,deltaTime);
    }else if(distanceToPlayer <= enemy->getDetectionRange()){
      handleChase(enemy,player,deltaTime);
    }else{
      handleIdle(enemy);
    }
  }

  void EnemySoldierControllerSystem::handleAttack(EnemySoldierComponent* enemy,PlayerComponent* player,float deltaTime){

    enemy->setCurrentState(EnemyState::ATTACKING);
    // handle cooldown
    if(enemy->getAttackTimer() <= enemy->getAttackCooldown()){
    
      enemy->setAttackTimer(enemy->getAttackTimer() + deltaTime); 
      return;
    }
    // can attack
    player->decreaseHealth(enemy->getDamage());
    // reset timer
    enemy->setAttackTimer(0.0f);




    
      
  }

  void EnemySoldierControllerSystem::handleChase(EnemySoldierComponent* enemy,PlayerComponent* player,float deltaTime){


    enemy->setCurrentState(EnemyState::CHASING);
    Entity* enemyEntity = enemy->getOwner();
    Entity* playerEntity = player->getOwner();

    // 1. get direction 
    glm::vec3 directionToPlayer = (playerEntity->localTransform.position - enemyEntity->localTransform.position);
    // 2. remove y axis
    directionToPlayer.y = 0.0f;


    if(glm::length(directionToPlayer) <=0.01f ) return ; // this to avoid be in top of each others

    // 3. normalize
    directionToPlayer = glm::normalize(directionToPlayer);
    // 4. move
    enemyEntity->localTransform.position += directionToPlayer * enemy->getSpeed() * deltaTime;
    
    // 5. look at the player
    enemyEntity->localTransform.rotation.y = glm::atan(directionToPlayer.x, directionToPlayer.z);
    


      
  }

  void EnemySoldierControllerSystem::handleIdle(EnemySoldierComponent* enemy){

    enemy->setCurrentState(EnemyState::IDLE);
  }

  void EnemySoldierControllerSystem::handleDead(EnemySoldierComponent* enemy){
      //  remove from world
        world->removeEntity(enemy->getOwner());
  }


}