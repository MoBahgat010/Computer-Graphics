
#include "./player-component.hpp"
#include <algorithm>
#include <random>
#include "../../audio/audio-player.hpp"
namespace our {
  void PlayerComponent::deserialize(const nlohmann::json& data) {
    if(!data.is_object()) return;
    health = data.value("health", health);
    bulletDamage = data.value("damage", bulletDamage);
    speed = data.value("speed", speed);
    mouseSensitivity = data.value("mouseSensitivity", mouseSensitivity);
    jumpSpeed = data.value("jumpSpeed", jumpSpeed);
  }


  // getters and setters for the player component


  // getters 

  int PlayerComponent::getHealth() const {
    return health;
  }
  int PlayerComponent::getMagazineAmmo() const {
    return currentMagazineAmmo;
  }
  int PlayerComponent::getTotalAmmo() const {
    return currentTotalAmmo;
  }
  int PlayerComponent::getBulletDamage() const {
    return bulletDamage;
  }
  bool PlayerComponent::getIsCrouch() const {
    return isCrouch;
  }

  bool PlayerComponent::getIsDead() const {
    return isDead;
  }
  float PlayerComponent::getSpeed() const {
    return speed;
  }

  float PlayerComponent::getMouseSensitivity() const {
    return mouseSensitivity;
  }

  float PlayerComponent::getJumpSpeed() const {
    return jumpSpeed;
  }


  // setters
  void PlayerComponent::setIsCrouch(bool isCrouch){
    this->isCrouch = isCrouch;
  }
  void PlayerComponent::setSpeed(float speed){
    this->speed = speed;
  }






  void PlayerComponent::increaseHealth(int amount) {
    health += amount;
  }

  void PlayerComponent::decreaseHealth(int amount) {
    health -= amount;
    if (health <= 0){
      health = 0;
      isDead=true;
    } 
  }

  void PlayerComponent::receiveDamage(int amount) {
    if (amount <= 0) return;
    
    decreaseHealth(amount);
    damageIndicatorTimer = 0.5f; // Set flash timer to 0.5 seconds
    
    // Grunt sound cooldown: Play pain sound only if the cooldown has elapsed to prevent spamming
    if (painSoundTimer <= 0.0f) {
        static AudioPlayer painAudioPlayer;
        painAudioPlayer.play("assets/audio/game/male_grunt.wav", 0.7f);
        painSoundTimer = 1.5f; // Add a 1.5 second cooldown interval between grunts
    }
  }

  void PlayerComponent::decreaseMagazineAmmo(int amount) {
  
      currentMagazineAmmo -= amount;

      currentMagazineAmmo = std::max(currentMagazineAmmo, 0);
  }

  void PlayerComponent::increaseTotalAmmo(int amount) {
    currentTotalAmmo += amount;
    currentTotalAmmo = std::min(currentTotalAmmo, MAX_TOTAL_AMMO);
  }

  void PlayerComponent::reloadWeapon() {
    if(currentMagazineAmmo >= MAX_MAGAZINE_AMMO || currentTotalAmmo <= 0) return;
    int ammoNeeded = MAX_MAGAZINE_AMMO - currentMagazineAmmo;
    int ammoToReload = std::min(ammoNeeded, currentTotalAmmo);
    currentMagazineAmmo += ammoToReload;
    currentTotalAmmo -= ammoToReload;
  }





}