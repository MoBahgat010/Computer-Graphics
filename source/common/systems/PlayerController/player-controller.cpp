#include "player-controller.hpp"
#include <iostream>
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
      handleMovement(player,playerEntity,deltaTime);
      handleCrouch(player,playerEntity);
      handleFire(player);
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

    // Get a reference to the position
    glm::vec3& position = playerEntity->localTransform.position;

    //  move forward
    if (keyboard.isPressed(GLFW_KEY_W) ){
      position += front * player->getSpeed() * deltaTime;
    }

    // move backward
    if (keyboard.isPressed(GLFW_KEY_S) ){
      position -= front * player->getSpeed() * deltaTime;
    }
    //  move left
    if (keyboard.isPressed(GLFW_KEY_A) ){
      position -= right * player->getSpeed() * deltaTime;
    }
    //  move right
    if (keyboard.isPressed(GLFW_KEY_D) ){
      position += right * player->getSpeed() * deltaTime;
    }
  }


  void PlayerControllerSystem::handleCrouch(PlayerComponent* player, Entity* playerEntity){
    auto& keyboard = app->getKeyboard();
    
    if (keyboard.justPressed(GLFW_KEY_C) ){
      bool playerIsCrouch = player->getIsCrouch();
      glm::vec3& position = playerEntity->localTransform.position;
      if(playerIsCrouch){
        // stand up
        position.y += 0.5f;
        player->setSpeed(5.0f);
      } else {
        // crouch 
        position.y -= 0.5f;
        player->setSpeed(2.5f);
      }
      player->setIsCrouch(!playerIsCrouch);
    }
  }

  void PlayerControllerSystem::handleFire(PlayerComponent* player){
    auto& mouse=app->getMouse();
    if(mouse.justPressed(GLFW_MOUSE_BUTTON_LEFT))  {
      std::cout << "fire" << std::endl;  
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

}