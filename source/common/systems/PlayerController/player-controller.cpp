#include "player-controller.hpp"
#include "../jolt-physics-system.hpp"
#include <iostream>

namespace our {


  void PlayerControllerSystem::enter(Application* app, JoltPhysicsSystem* physics){
      this->app = app;
      this->joltPhysics = physics;
      app->getMouse().lockMouse(app->getWindow());
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
      Entity* cameraEntity = nullptr;
      for(auto entity : world->getEntities()) {
        if(entity->parent == playerEntity && entity->getComponent<CameraComponent>()) {
            cameraEntity = entity;
            break;
        }
      }

      if(!cameraEntity) return;

      if ( player->getIsDead()){
        handleDeath();
      } 

      handleMovement(player,playerEntity,deltaTime);
      handleLook(player,playerEntity, cameraEntity);
      handleCrouch(player, cameraEntity);
      handleFire(player, cameraEntity);
      handleReload(player);
  }

  void PlayerControllerSystem::handleMovement(PlayerComponent* player, Entity* playerEntity, float deltaTime){
    auto& keyboard = app->getKeyboard();

    // Build the matrix from the entity's current local transform
    glm::mat4 matrix = playerEntity->localTransform.toMat4();

    // Extract direction vectors (w=0 for vectors)
    glm::vec3 front = glm::vec3(matrix * glm::vec4(0, 0, -1, 0));
    front.y = 0;
    front = glm::normalize(front);
    glm::vec3 right = glm::vec3(matrix * glm::vec4(1, 0,  0, 0));
    right.y = 0;
    right = glm::normalize(right);

    glm::vec3 desiredVelocity(0.0f);

    //  move forward
    if (keyboard.isPressed(GLFW_KEY_W) ){
      desiredVelocity += front * player->getSpeed();
    }

    // move backward
    if (keyboard.isPressed(GLFW_KEY_S) ){
      desiredVelocity -= front * player->getSpeed();
    }
    //  move left
    if (keyboard.isPressed(GLFW_KEY_A) ){
      desiredVelocity -= right * player->getSpeed();
    }
    //  move right
    if (keyboard.isPressed(GLFW_KEY_D) ){
      desiredVelocity += right * player->getSpeed();
    }

    float len = glm::length(desiredVelocity);
    if(len > player->getSpeed() && len > 1e-6f) {
      desiredVelocity = (desiredVelocity / len) * player->getSpeed();
    }

    if(joltPhysics && joltPhysics->isInitialized()) {
      joltPhysics->setPlayerEntity(playerEntity);
      joltPhysics->setPlayerVelocity(desiredVelocity);
    } else {
      // Fallback when physics is disabled.
      glm::vec3& position = playerEntity->localTransform.position;
      position += desiredVelocity * deltaTime;
    }
  }

  void PlayerControllerSystem::handleLook(PlayerComponent* player, Entity* playerEntity, Entity* cameraEntity){
    auto& mouse=app->getMouse();

    glm::vec2 delta = mouse.getMouseDelta();

    glm::vec3& rotation = playerEntity->localTransform.rotation;
    glm::vec3& cameraRotation = cameraEntity->localTransform.rotation;
    
    // Yaw: Rotate the player horizontally
    rotation.y -= delta.x * player->getMouseSensitivity();
    
    // Pitch: Rotate the camera vertically
    cameraRotation.x -= delta.y * player->getMouseSensitivity();

    // Clamp pitch: don't go beyond straight up or down
    if(cameraRotation.x < -glm::half_pi<float>() * 0.99f) cameraRotation.x = -glm::half_pi<float>() * 0.99f;
    if(cameraRotation.x >  glm::half_pi<float>() * 0.99f) cameraRotation.x =  glm::half_pi<float>() * 0.99f;
    
    rotation.y = glm::wrapAngle(rotation.y);
  }


  void PlayerControllerSystem::handleCrouch(PlayerComponent* player, Entity* cameraEntity){
    auto& keyboard = app->getKeyboard();
    
    if (keyboard.justPressed(GLFW_KEY_C) ){
      bool playerIsCrouch = player->getIsCrouch();
      glm::vec3& cameraPosition = cameraEntity->localTransform.position;
      if(playerIsCrouch){
        // stand up → restore head height
        cameraPosition.y *=2;
        float speed = player->getSpeed();
        player->setSpeed(speed*2);
      } else {
        // crouch → lower head height
        cameraPosition.y /=2;
        float speed = player->getSpeed();
        player->setSpeed(speed/2);
      }
      player->setIsCrouch(!playerIsCrouch);
    }
  }

  void PlayerControllerSystem::handleFire(PlayerComponent* player,Entity* cameraEntity){
    auto& mouse=app->getMouse();
    if(mouse.justPressed(GLFW_MOUSE_BUTTON_LEFT))  {
      std::cout << "fire" << std::endl;  
      if (player->getMagazineAmmo()<=0) return;

      glm::mat4 cameraToWorld = cameraEntity->getLocalToWorldMatrix();
      
      //  (camera position in world space)
      glm::vec3 origin = glm::vec3(cameraToWorld * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
      
      //  calc direction neg z by default 
      glm::vec3 direction = glm::normalize(glm::vec3(cameraToWorld * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));

      if (joltPhysics && joltPhysics->isInitialized()) {
        JoltPhysicsSystem::RaycastResult result = joltPhysics->raycast(origin, direction, 50.0f);

        std::cout << "raycast hit: " << result.hit << std::endl;
        std::cout << "raycast hit entity: " << result.entity << std::endl;
        
        if (result.hit && result.entity) {
          std::cout << "hit entity!" << std::endl;
          auto* enemy = result.entity->getComponent<EnemySoldierComponent>();
          std::cout << "enemy: " << enemy << std::endl;
          if (enemy) {
            enemy->decreaseHealth(player->getBulletDamage());
            std::cout << "Dealt " << player->getBulletDamage() << " damage to enemy!" << std::endl;
          }
        }
      }
      player->decreaseMagazineAmmo();
    }  
  }

  void PlayerControllerSystem::handleReload(PlayerComponent*player){
    auto& keyboard = app->getKeyboard();
    if(keyboard.justPressed(GLFW_KEY_R))  {
      std::cout << "reload" << std::endl;  
      player->reloadWeapon();
    }
  }

  void PlayerControllerSystem::handleDeath(){
    app->getMouse().unlockMouse(app->getWindow());
    app->changeState("game-over");
  }

  void PlayerControllerSystem::exit(){
      app->getMouse().unlockMouse(app->getWindow());
  }

}