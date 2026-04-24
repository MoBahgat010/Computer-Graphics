#include "enemy-soldier-controller.hpp"
#include "../jolt-physics-system.hpp"
#include <vector>
#include <algorithm>
#include <iostream>
#include <array>
#include <random>



namespace our {
 
  void EnemySoldierControllerSystem::enter(Application* app, JoltPhysicsSystem* physics){
    this->app = app;
    this->physics = physics;
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
  
    if (enemy->getIsResting()) {
      // Resting logic: tick the timer until rested
      enemy->setChaseTimer(enemy->getChaseTimer() + deltaTime);
      if (enemy->getChaseTimer() >= enemy->getRestDuration()) {
        enemy->setIsResting(false);
        enemy->setChaseTimer(0.0f); // Reset for the next chase
      }
      handleIdle(enemy); // Force idle while resting
    } 
    else {
      // Normal behavior
      if(distanceToPlayer <= enemy->getAttackRange() ) {
        handleAttack(enemy,player,deltaTime);
        // Slowly recover stamina while attacking, or just keep it paused
        enemy->setChaseTimer(std::max(0.0f, enemy->getChaseTimer() - deltaTime));
      } 
      else if(distanceToPlayer <= enemy->getDetectionRange()) {
        // Chasing logic: tick the timer up
        enemy->setChaseTimer(enemy->getChaseTimer() + deltaTime);
        if (enemy->getChaseTimer() >= enemy->getMaxChaseDuration()) {
          // Exhausted! Start resting
          enemy->setIsResting(true);
          enemy->setChaseTimer(0.0f);
          handleIdle(enemy);
        } else {
          handleChase(enemy,player,deltaTime);
        }
      } 
      else {
        // Player is far away: recover stamina quickly and idle
        enemy->setChaseTimer(0.0f);
        handleIdle(enemy);
      }
    }
  }

  void EnemySoldierControllerSystem::handleAttack(EnemySoldierComponent* enemy,PlayerComponent* player,float deltaTime){

    enemy->setCurrentState(EnemyState::ATTACKING);
    
    // std::cout << "Enemy is attacking" << std::endl;   
    
    // handle cooldown
    if(enemy->getAttackTimer() <= enemy->getAttackCooldown()){
      // std::cout << "Enemy is on cooldown" << std::endl;   
      enemy->setAttackTimer(enemy->getAttackTimer() + deltaTime); 
      return;
    }
    // can attack
    player->decreaseHealth(enemy->getDamage());
    // reset timer
    enemy->setAttackTimer(0.0f);
    // std::cout << "Enemy attacked" << std::endl;   




    
      
  }

  void EnemySoldierControllerSystem::handleChase(EnemySoldierComponent* enemy,PlayerComponent* player,float deltaTime){

    // std::cout << "Enemy is chasing" << std::endl;   
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

    // std::cout << "Enemy is idle" << std::endl;   
    enemy->setCurrentState(EnemyState::IDLE);
  }

  void EnemySoldierControllerSystem::handleDead(EnemySoldierComponent* enemy){
    std::cout << "Enemy is dead" << std::endl;   
      static const std::array<const char*, 3> deathVoicelines = {
          "assets/audio/game/Got'em tango down..mp3",
          "assets/audio/game/He's down, Goodnight.mp3",
          "assets/audio/game/Target eliminated..mp3"
      };
      static std::mt19937 rng(std::random_device{}());
      std::uniform_int_distribution<int> pick(0, (int)deathVoicelines.size() - 1);
      enemyDeathAudioPlayer.play(deathVoicelines[pick(rng)], 0.7f);

      //  remove physics body first (before entity is deleted)
      Entity* enemyEntity = enemy->getOwner();
      if(enemyEntity) {
          if(physics) {
              physics->removeBody(enemyEntity);
          }
          if(enemyEntity->getWorld()) {
              enemyEntity->getWorld()->markForRemoval(enemyEntity);
          }
      }
  }


}